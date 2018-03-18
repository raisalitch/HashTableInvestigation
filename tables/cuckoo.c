/* * * * * * * * *
 * Dynamic hash table using cuckoo hashing, resolving collisions by switching
 * keys between two tables with two separate hash functions
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by Raisa Litchfield
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "cuckoo.h"

// an inner table represents one of the two internal tables for a cuckoo
// hash table. it stores two parallel arrays: 'slots' for storing keys and
// 'inuse' for marking which entries are occupied
typedef struct inner_table {
	int64 *slots;	// array of slots holding keys
	bool  *inuse;	// is this slot in use or not?
    int load;       // number of keys in the inner table right now
} InnerTable;

// a cuckoo hash table stores its keys in two inner tables
struct cuckoo_table {
	InnerTable *table1; // first table
	InnerTable *table2; // second table
	int size;			// size of each table
    int time;           // how much CPU time has been used to insert/lookup keys
};

/* * * *
 * helper functions
 */

// set up the internals of a linear hash table struct with new
// arrays of size 'size'
// function modified from provided function in linear.c
static void initialise_table(InnerTable *table, int size) {
    assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");
    
    table->slots = malloc((sizeof *table->slots) * size);
    assert(table->slots);
    table->inuse = malloc((sizeof *table->inuse) * size);
    assert(table->inuse);
    int i;
    for (i = 0; i < size; i++) {
        table->inuse[i] = false;
    }
    
    table->load = 0;
}

// double the size of the internal table arrays and re-hash all
// keys in the old tables
// function modified from provided function in linear.c
static void double_table(CuckooHashTable *table) {
    int64 *oldslots1 = table->table1->slots, *oldslots2 = table->table2->slots;
    bool  *oldinuse1 = table->table1->inuse, *oldinuse2 = table->table2->inuse;
    int oldsize = table->size;
    
    table->size *= 2;
    
    initialise_table(table->table1, table->size);
    initialise_table(table->table2, table->size);
    
    int i;
    for (i = 0; i < oldsize; i++) {
        if (oldinuse1[i] == true) {
            cuckoo_hash_table_insert(table, oldslots1[i]);
        }
        if (oldinuse2[i] == true) {
            cuckoo_hash_table_insert(table, oldslots2[i]);
        }
    }
    
    free(oldslots1);
    free(oldinuse1);
    free(oldslots2);
    free(oldinuse2);
    
}

// insert a key into the cuckoo hash table
static bool insert_key(int64 key, CuckooHashTable *table, InnerTable *tableA,
                       InnerTable *tableB, int (*hA)(int64),
                       int (*hB)(int64), int *replacements) {
    int h = hA(key) % table->size;
    int64 prekey;
    
    if (!tableA->inuse[h]) {
        // address not in use, so insert key
        tableA->slots[h] = key;
        tableA->inuse[h] = true;
        tableA->load++;
        return true;
    }
    
    // address in use, so pop the preexisting key
    prekey = tableA->slots[h];
    // and insert the new key
    tableA->slots[h] = key;
    
    // double table and rehash all values if number of replacements is too high
    // (there is a 'cycle')
    if ((*replacements >= 1000)) {
        double_table(table);
    }
    
    (*replacements)++;
    // insert preexisting key using recursive call
    return insert_key(prekey, table, tableB, tableA, hB, hA, replacements);
}

/* * * *
 * all functions
 */

// initialise a cuckoo hash table with 'size' slots in each table
CuckooHashTable *new_cuckoo_hash_table(int size) {
    CuckooHashTable *table = malloc(sizeof *table);
    assert(table);
    
    table->table1 = malloc((sizeof *table->table1) * size);
    table->table2 = malloc((sizeof *table->table2) * size);
    
    // set up the internals of the table struct with arrays of size 'size'
    initialise_table(table->table1, size);
    initialise_table(table->table2, size);
    
    table->size = size;
    table->time = 0;
    
	return table;
}


// free all memory associated with 'table'
void free_cuckoo_hash_table(CuckooHashTable *table) {
    assert(table != NULL);
    
    // free the inner tables' arrays
    free(table->table1->slots);
    free(table->table1->inuse);
    free(table->table2->slots);
    free(table->table2->inuse);
    
    // free the inner tables
    free(table->table1);
    free(table->table2);
    
    // free the table struct itself
    free(table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool cuckoo_hash_table_insert(CuckooHashTable *table, int64 key) {
    assert(table != NULL);
    int start_time = clock(); // start timing
    
    // calculate the addresses for the key
    int hA = h1(key) % table->size;
    int hB = h2(key) % table->size;
    
    // check if the key is already in the table
    if ((table->table1->inuse[hA] && table->table1->slots[hA] == key) ||
        (table->table2->inuse[hB] && table->table2->slots[hB] == key)) {
        
        // key is in table - no need to insert
        table->time += clock() - start_time; // add time elapsed
        return false;
    }
    
    int replacements = 0;
    
    // key is not in table - so insert it
    bool inserted = insert_key(key, table, table->table1, table->table2, h1,
                               h2, &replacements);
    
    table->time += clock() - start_time; // add time elapsed
    return inserted;
}

// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool cuckoo_hash_table_lookup(CuckooHashTable *table, int64 key) {
    assert(table != NULL);
    int start_time = clock(); // start timing
    
    // calculate the addresses for the key
    // the key will be in one of these addresses if it's in the hash table
    int hA = h1(key) % table->size;
    int hB = h2(key) % table->size;
    
    if ((table->table1->inuse[hA] && table->table1->slots[hA] == key) ||
        (table->table2->inuse[hB] && table->table2->slots[hB] == key)) {
        // key is in table
        table->time += clock() - start_time; // add time elapsed
        return true;
        
    } else {
        // key is not in table
        table->time += clock() - start_time; // add time elapsed
        return false;
    }
}


// print the contents of 'table' to stdout
void cuckoo_hash_table_print(CuckooHashTable *table) {
	assert(table);
	printf("--- table size: %d\n", table->size);

	// print header
	printf("                    table one         table two\n");
	printf("                  key | address     address | key\n");
	
	// print rows of each table
	int i;
	for (i = 0; i < table->size; i++) {

		// table 1 key
		if (table->table1->inuse[i]) {
			printf(" %20llu ", table->table1->slots[i]);
		} else {
			printf(" %20s ", "-");
		}

		// addresses
		printf("| %-9d %9d |", i, i);

		// table 2 key
		if (table->table2->inuse[i]) {
			printf(" %llu\n", table->table2->slots[i]);
		} else {
			printf(" %s\n",  "-");
		}
	}

	// done!
	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void cuckoo_hash_table_stats(CuckooHashTable *table) {
    assert(table != NULL);
    
    int total_load = table->table1->load + table->table2->load;
    
    printf("--- table stats ---\n");
    
    // print some information about the entire table
    printf("    current size: %d x 2 = %d slots\n", table->size,
           table->size * 2);
    printf("    current load: %d items\n", total_load);
    printf("    load  factor: %.3f%%\n",
           total_load * 100.0 / (table->size * 2));
    
    // information about table 1
    printf("Inner Table 1\n");
    printf("    current load: %d items\n", table->table1->load);
    printf("    load  factor: %.3f%%\n",
           table->table1->load * 100.0 / table->size);
    
    // information about table 2
    printf("Inner Table 2\n");
    printf("    current load: %d items\n", table->table2->load);
    printf("    load  factor: %.3f%%\n",
           table->table2->load * 100.0 / table->size);
    
    // also calculate CPU usage in seconds and print this
    float seconds = table->time * 1.0 / CLOCKS_PER_SEC;
    printf("         CPU time spent: %.6f sec\n", seconds);
    
    printf("--- end stats ---\n");
}
