/* * * * * * * * *
* Dynamic hash table using a combination of extendible hashing and cuckoo
* hashing with n keys per bucket, resolving collisions by switching keys
* between two tables with two separate hash functions and growing the tables 
* incrementally in response to cycles
*
* created for COMP20007 Design of Algorithms - Assignment 2, 2017
* by Raisa Litchfield
*/

#ifndef XUCKOON_H
#define XUCKOON_H

#include <stdbool.h>
#include "../inthash.h"

typedef struct xuckoon_table XuckooNHashTable;

// initialise an extendible cuckoo hash table
XuckooNHashTable *new_xuckoon_hash_table();

// free all memory associated with 'table'
void free_xuckoon_hash_table(XuckooNHashTable *table);

// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xuckoon_hash_table_insert(XuckooNHashTable *table, int64 key);

// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xuckoon_hash_table_lookup(XuckooNHashTable *table, int64 key);

// print the contents of 'table' to stdout
void xuckoon_hash_table_print(XuckooNHashTable *table);

// print some statistics about 'table' to stdout
void xuckoon_hash_table_stats(XuckooNHashTable *table);

#endif
