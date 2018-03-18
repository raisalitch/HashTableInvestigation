# Hash Table Investigation (Design of Algorithms Assignment 2)
Received 24/20 marks. (Full marks + 4 bonus marks)
An exploration of cuckoo and extendable hash tables.

## Files created by Raisa:
### Documentation:
- [report.pdf](https://github.com/raisalitch/HashTableInvestigation/blob/master/report.pdf)
- [specification.pdf](https://github.com/raisalitch/HashTableInvestigation/blob/master/specification.pdf) (written by the University)

### C files:
#### Cuckoo Hash Table: [tables/cuckoo.c](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/cuckoo.c)
- Dynamic hash table using cuckoo hashing, resolving collisions by switching keys between two tables with two separate hash functions
#### Extendable Hash Table with Multiple Buckets: [tables/xtndbln.c](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/xtndbln.c)
- Dynamic hash table using extendible hashing with multiple keys per bucket, resolving collisions by incrementally growing the hash table
#### Extendable and Cuckoo Hash Table: [tables/xuckoo.c](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/xuckoo.c)
- Dynamic hash table using a combination of extendible hashing and cuckoo hashing with a single keys per bucket, resolving collisions by switching keys  between two tables with two separate hash functions and growing the tables incrementally in response to cycles
#### Extendable and Cuckoo Hash Table with Multiple Buckets: [tables/xuckoon.c](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/xuckoon.c)
- Dynamic hash table using a combination of extendible hashing and cuckoo hashing with n keys per bucket, resolving collisions by switching keys between two tables with two separate hash functions and growing the tables incrementally in response to cycles

### Header files:
- [tables/cuckoo.h](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/cuckoo.h)
- [tables/xtndbln.h](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/xtndbln.h)
- [tables/xuckoo.h](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/xuckoo.h)
- [tables/xuckoon.h](https://github.com/raisalitch/HashTableInvestigation/blob/master/tables/xuckoon.h)
