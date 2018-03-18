/* * * * * * * * *
* Dynamic hash table using a combination of extendible hashing and cuckoo
* hashing with n keys per bucket, resolving collisions by switching keys
* between two tables with two separate hash functions and growing the tables 
* incrementally in response to cycles
*
* created for COMP20007 Design of Algorithms - Assignment 2, 2017
* by Raisa Litchfield
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "xuckoon.h"

// macro to calculate the rightmost n bits of a number x
#define rightmostnbits(n, x) (x) & ((1 << (n)) - 1)

// a bucket stores a single key (full=true) or is empty (full=false)
// it also knows how many bits are shared between possible keys, and the first 
// table address that references it
typedef struct xtndbln_bucket {
	int id;         // a unique id for this bucket, equal to the first address
                    // in the table which points to it
	int depth;      // how many hash value bits are being used by this bucket
    int nkeys;      // number of keys currently contained in this bucket
    int64 *keys;	// the keys stored in this bucket
} Bucket;

// an inner table is an extendible hash table with an array of slots pointing 
// to buckets holding up to 1 key, along with some information about the number 
// of hash value bits to use for addressing
typedef struct inner_table {
	Bucket **buckets;	// array of pointers to buckets
	int size;			// how many entries in the table of pointers (2^depth)
	int depth;			// how many bits of the hash value to use (log2(size))
    int nbuckets;       // how many distinct buckets does the table point to
	int nkeys;			// how many keys are being stored in the table
} InnerTable;

// a xuckoo hash table is just two inner tables for storing inserted keys
struct xuckoon_table {
	InnerTable *table1;
	InnerTable *table2;
    int bucketsize;		// maximum number of keys per bucket
    int time;           // how much CPU time has been used to insert/lookup keys
};

/* * * *
 * helper functions
 */

// create a new bucket first referenced from 'first_address', based on 'depth'
// bits of its keys' hash values
// function modified from provided function in xtndbl1.c
static Bucket *new_bucket(int first_address, int depth, int bucketsize) {
    Bucket *bucket = malloc(sizeof *bucket);
    assert(bucket);
    
    bucket->id = first_address;
    bucket->depth = depth;
    bucket->nkeys = 0;
    
    bucket->keys = malloc((sizeof bucket->keys) * bucketsize);
    
    return bucket;
}

// set up the internals of a linear hash table struct with new
// arrays of size 'size'
// function modified from provided function in linear.c
static void initialise_table(InnerTable *table, int bucketsize) {
    
    table->size = 1;
    table->buckets = malloc(sizeof *table->buckets);
    assert(table->buckets);
    table->buckets[0] = new_bucket(0, 0, bucketsize);
    table->depth = 0;
    
    table->nbuckets = 1;
    table->nkeys = 0;
}

// frees memory from the inner table
// function modified from provided functions in linear.c/xtndbln1
static void free_inner_table(InnerTable *table) {
    assert(table);
    
    // loop backwards through the array of pointers, freeing buckets only as we
    // reach their first reference
    // (if we loop through forwards, we wouldn't know which reference was last)
    int i;
    for (i = table->size-1; i >= 0; i--) {
        if (table->buckets[i]->id == i) {
            // free the array of keys in the bucket
            free(table->buckets[i]->keys);
            // free the bucket itself
            free(table->buckets[i]);
        }
    }
    
    // free the array of bucket pointers
    free(table->buckets);
    
    // free the inner table struct itself
    free(table);
}

// checks if key is in an inner table, returns true if it is in the table,
// false if it is not
static bool find_key(InnerTable *table, int address, int64 key) {
    if (table->buckets[address]->nkeys) {
        int i;
        for (i = 0; i < table->buckets[address]->nkeys; i++) {
            if (table->buckets[address]->keys[i] == key) {
                // key is already in table
                return true;
            }
        }
    }
    // key is not in table
    return false;
}

// double the table of bucket pointers, duplicating the bucket pointers in the
// first half into the new second half of the table
// function modified from provided function in xtndbl1.c
static void double_table(InnerTable *table) {
    int size = table->size * 2;
    assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");
    
    // get a new array of twice as many bucket pointers, and copy pointers down
    table->buckets = realloc(table->buckets, (sizeof *table->buckets) * size);
    assert(table->buckets);
    int i;
    for (i = 0; i < table->size; i++) {
        table->buckets[table->size + i] = table->buckets[i];
    }
    
    // finally, increase the table size and the depth we are using to hash keys
    table->size = size;
    table->depth++;
}

// reinsert a key into the hash table after splitting a bucket --- we can assume
// that there will definitely be space for this key because it was already
// inside the hash table previously
// use 'xtndbl1_hash_table_insert()' instead for inserting new keys
// function modified from provided function in xtndbl1.c
static void reinsert_key(InnerTable *table, int64 key, int (*h)(int64)) {
    int address = rightmostnbits(table->depth, h(key));
    table->buckets[address]->keys[table->buckets[address]->nkeys] = key;
    table->buckets[address]->nkeys++;
}

// split the bucket in 'table' at address 'address', growing table if necessary
// function modified from provided function in xtndbl1.c
static void split_bucket(InnerTable *table, int address, int bucketsize,
                         int (*h)(int64)) {
    
    // FIRST,
    // do we need to grow the table?
    if (table->buckets[address]->depth == table->depth) {
        // yep, this bucket is down to its last pointer
        double_table(table);
    }
    // either way, now it's time to split this bucket
    
    
    // SECOND,
    // create a new bucket and update both buckets' depth
    Bucket *bucket = table->buckets[address];
    int depth = bucket->depth;
    int first_address = bucket->id;
    
    int new_depth = depth + 1;
    bucket->depth = new_depth;
    
    // new bucket's first address will be a 1 bit plus the old first address
    int new_first_address = 1 << depth | first_address;
    Bucket *newbucket = new_bucket(new_first_address, new_depth, bucketsize);
    table->nbuckets++;
    
    // THIRD,
    // redirect every second address pointing to this bucket to the new bucket
    // construct addresses by joining a bit 'prefix' and a bit 'suffix'
    // (defined below)
    
    // suffix: a 1 bit followed by the previous bucket bit address
    int bit_address = rightmostnbits(depth, first_address);
    int suffix = (1 << depth) | bit_address;
    
    // prefix: all bitstrings of length equal to the difference between the new
    // bucket depth and the table depth
    // use a for loop to enumerate all possible prefixes less than maxprefix:
    int maxprefix = 1 << (table->depth - new_depth);
    
    int prefix;
    for (prefix = 0; prefix < maxprefix; prefix++) {
        
        // construct address by joining this prefix and the suffix
        int a = (prefix << new_depth) | suffix;
        
        // redirect this table entry to point at the new bucket
        table->buckets[a] = newbucket;
    }
    
    // FINALLY,
    // filter the keys from the old bucket into their rightful places in the
    // new table (which may be the old bucket, or may be the new bucket)
    
    // remove and reinsert the keys
    int i;
    int64 key;
    int nkeys = bucket->nkeys;
    bucket->nkeys = 0;
    for (i = 0; i < nkeys; i++) {
        key = bucket->keys[i];
        reinsert_key(table, key, h);
    }
}

// insert a key into the xuckoo hash table
// function modified from provided function in xtndbl1.c
static bool insert_key(int64 key, XuckooNHashTable *table, InnerTable *tableA,
                       InnerTable *tableB, int (*hA)(int64),
                       int (*hB)(int64), int *replacements) {
    int address = rightmostnbits(tableA->depth, hA(key));
    int64 prekey;
    
    if (tableA->buckets[address]->nkeys != table->bucketsize) {
        // bucket not full, so insert key
        tableA->buckets[address]->keys[tableA->buckets[address]->nkeys] = key;
        tableA->buckets[address]->nkeys++;
        tableA->nkeys++;
        return true;
    }
    
    // bucket full, so pop a random preexisting key
    int random_index = rand() % tableA->buckets[address]->nkeys;
    prekey = tableA->buckets[address]->keys[random_index];
    // and insert the new key
    tableA->buckets[address]->keys[random_index] = key;
    
    // split bucket if number of replacements is too high (there is a 'cycle')
    if ((*replacements >= 1000)) {
        split_bucket(tableA, address, table->bucketsize, hA);
    }
    
    (*replacements)++;
    // insert preexisting key using recursive call
    return insert_key(prekey, table, tableB, tableA, hB, hA, replacements);
}


/* * * *
 * all functions
 */

// initialise an n-key extendible cuckoo hash table
// function modified from provided function in xtndbl1.c
XuckooNHashTable *new_xuckoon_hash_table(int bucketsize) {
    XuckooNHashTable *table = malloc(sizeof *table);
    assert(table);
    
    table->table1 = malloc(sizeof *table->table1);
    table->table2 = malloc(sizeof *table->table2);
    
    // set up the internals of the table struct with arrays of size 'size'
    initialise_table(table->table1, bucketsize);
    initialise_table(table->table2, bucketsize);
    
    table->bucketsize = bucketsize;
    table->time = 0;
    
    return table;
}


// free all memory associated with 'table'
void free_xuckoon_hash_table(XuckooNHashTable *table) {
    assert(table);
    
    free_inner_table(table->table1);
    free_inner_table(table->table2);
    
    // free the table struct itself
    free(table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
// function modified from provided function in xtndbl1.c
bool xuckoon_hash_table_insert(XuckooNHashTable *table, int64 key) {
    assert(table);
    
    int start_time = clock(); // start timing
    
    InnerTable *table1 = table->table1;
    InnerTable *table2 = table->table2;
    
    // calculate the addresses for the key
    int addressA = rightmostnbits(table1->depth, h1(key));
    int addressB = rightmostnbits(table2->depth, h2(key));
    
    // is this key already there?
    if ((find_key(table1, addressA, key) ||
        find_key(table2, addressB, key))) {
        table->time += clock() - start_time; // add time elapsed
        return false;
    }
    
    InnerTable *tableA, *tableB;
    int (*hA)(int64), (*hB)(int64);
    
    // set tableA as the table with fewer keys (or table 1 if nkeys is same)
    if (table1->nkeys <= table2->nkeys) {
        tableA = table1;
        hA = h1;
        tableB = table2;
        hB = h2;

    } else {
        tableA = table2;
        hA = h2;
        tableB = table1;
        hB = h1;
    }
    
    int replacements = 0;
    srand(time(NULL));
    
    // key is not in table - so insert it
    // (attempt to insert into tableA, the table with fewer keys, first)
    bool inserted = insert_key(key, table, tableA, tableB, hA, hB, &replacements);
    
    table->time += clock() - start_time; // add time elapsed
    return inserted;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
// function modified from provided function in xtndbl1.c
bool xuckoon_hash_table_lookup(XuckooNHashTable *table, int64 key) {
    assert(table);
    int start_time = clock(); // start timing
    
    InnerTable *table1 = table->table1;
    InnerTable *table2 = table->table2;
    
    // calculate the addresses for the key
    int addressA = rightmostnbits(table1->depth, h1(key));
    int addressB = rightmostnbits(table2->depth, h2(key));
    
    // look for the key in that bucket (unless it's empty)
    bool found = (find_key(table1, addressA, key) ||
                  find_key(table2, addressB, key));
    
    // add time elapsed to total CPU time before returning result
    table->time += clock() - start_time;
    return found;
}


// print the contents of 'table' to stdout
void xuckoon_hash_table_print(XuckooNHashTable *table) {
	assert(table != NULL);

	printf("--- table ---\n");

	// loop through the two tables, printing them
	InnerTable *innertables[2] = {table->table1, table->table2};
	int t;
	for (t = 0; t < 2; t++) {
		// print header
		printf("table %d\n", t+1);

		printf("  table:               buckets:\n");
		printf("  address | bucketid   bucketid [key]\n");
		
		// print table and buckets
		int i;
		for (i = 0; i < innertables[t]->size; i++) {
			// table entry
			printf("%9d | %-9d ", i, innertables[t]->buckets[i]->id);

			// if this is the first address at which a bucket occurs, print it
			if (innertables[t]->buckets[i]->id == i) {
				printf("%9d ", innertables[t]->buckets[i]->id);
                
                // print the bucket's contents
                printf("[");
                for(int j = 0; j < table->bucketsize; j++) {
                    if (j < innertables[t]->buckets[i]->nkeys) {
                        printf(" %llu", innertables[t]->buckets[i]->keys[j]);
                    } else {
                        printf(" -");
                    }
                }
                printf(" ]");
			}

			// end the line
			printf("\n");
		}
	}
	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
// function modified from provided function in xtndbl1.c
void xuckoon_hash_table_stats(XuckooNHashTable *table) {
    assert(table);
    
    InnerTable *table1 = table->table1;
    InnerTable *table2 = table->table2;
    
    printf("--- table stats ---\n");
    
    // print some stats about state of the entire table
    printf("       total table size: %d\n", table1->size + table2->size);
    printf("   total number of keys: %d\n", table1->nkeys + table2->nkeys);
    printf("total number of buckets: %d\n", table1->nbuckets + table2->nbuckets);
    printf("            bucket size: %d\n", table->bucketsize);
    
    // information about table 1
    printf("Inner Table 1\n");
    printf("             table size: %d\n", table1->size);
    printf("         number of keys: %d\n", table1->nkeys);
    printf("      number of buckets: %d\n", table1->nbuckets);
    
    // information about table 2
    printf("Inner Table 2\n");
    printf("             table size: %d\n", table2->size);
    printf("         number of keys: %d\n", table2->nkeys);
    printf("      number of buckets: %d\n", table2->nbuckets);
    
    // also calculate CPU usage in seconds and print this
    float seconds = table->time * 1.0 / CLOCKS_PER_SEC;
    printf("         CPU time spent: %.6f sec\n", seconds);
    
    printf("--- end stats ---\n");

}
