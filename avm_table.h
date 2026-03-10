#ifndef AVM_TABLE_H
#define AVM_TABLE_H

#include "avm_memcell.h"

#define AVM_TABLE_HASHSIZE 211

typedef struct avm_table_bucket {
    avm_memcell key;
    avm_memcell value;
    struct avm_table_bucket* next;
} avm_table_bucket;

typedef struct avm_table {
    unsigned refCounter;
    avm_table_bucket* strIndexed[AVM_TABLE_HASHSIZE];
    avm_table_bucket* numIndexed[AVM_TABLE_HASHSIZE];
} avm_table;

// Δημιουργία πίνακα
avm_table* avm_tablenew(void);

// Καταστροφή πίνακα (αν refCounter φτάσει 0)
void avm_tabledestroy(avm_table* t);

// Χρήσιμες συναρτήσεις για set/get
void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* val);
avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key);

#endif

