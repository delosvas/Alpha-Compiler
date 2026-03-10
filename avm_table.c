#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "avm_table.h"
#include "avm.h"

static unsigned hash_str(const char* str) {
    unsigned hash = 0;
    while (*str)
        hash = hash * 31 + *str++;
    return hash % AVM_TABLE_HASHSIZE;
}

static unsigned hash_num(double num) {
    return ((unsigned)num) % AVM_TABLE_HASHSIZE;
}

avm_table* avm_tablenew(void) {
    avm_table* t = malloc(sizeof(avm_table));
    if (!t) {
        fprintf(stderr, "Memory allocation failed for avm_table\n");
        exit(1);
    }
    t->refCounter = 0;
    memset(t->strIndexed, 0, sizeof(t->strIndexed));
    memset(t->numIndexed, 0, sizeof(t->numIndexed));
    return t;
}

void avm_tableincrefcounter(avm_table* t) {
    assert(t);
    ++t->refCounter;
}

void avm_tabledestroy(avm_table* t) {
    assert(t);
    for (int i = 0; i < AVM_TABLE_HASHSIZE; ++i) {
        avm_table_bucket* b = t->strIndexed[i];
        while (b) {
            avm_table_bucket* next = b->next;
            avm_memcellclear(&b->key);
            avm_memcellclear(&b->value);
            free(b);
            b = next;
        }
        b = t->numIndexed[i];
        while (b) {
            avm_table_bucket* next = b->next;
            avm_memcellclear(&b->key);
            avm_memcellclear(&b->value);
            free(b);
            b = next;
        }
    }
    free(t);
}

void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* val) {
    assert(t && key && val);

    avm_table_bucket** bucket_list = NULL;
    unsigned index = 0;

    if (key->type == string_m) {
        index = hash_str(key->data.strVal);
        bucket_list = &t->strIndexed[index];
    } else if (key->type == number_m) {
        index = hash_num(key->data.numVal);
        bucket_list = &t->numIndexed[index];
    } else {
        avm_error("Unsupported key type in avm_tablesetelem");
        return;
    }

    for (avm_table_bucket* b = *bucket_list; b != NULL; b = b->next) {
        if ((key->type == string_m && strcmp(b->key.data.strVal, key->data.strVal) == 0) ||
            (key->type == number_m && b->key.data.numVal == key->data.numVal)) {
            avm_memcellclear(&b->value);
            avm_assign(&b->value, val);
            return;
        }
    }

    avm_table_bucket* new_bucket = malloc(sizeof(avm_table_bucket));
    if (!new_bucket) {
        fprintf(stderr, "Memory allocation failed for avm_table_bucket\n");
        exit(1);
    }
    avm_assign(&new_bucket->key, key);
    avm_assign(&new_bucket->value, val);
    new_bucket->next = *bucket_list;
    *bucket_list = new_bucket;
}

avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key) {
    assert(t && key);

    avm_table_bucket* b = NULL;
    unsigned index = 0;

    if (key->type == string_m) {
        index = hash_str(key->data.strVal);
        b = t->strIndexed[index];
        while (b) {
            if (strcmp(b->key.data.strVal, key->data.strVal) == 0)
                return &b->value;
            b = b->next;
        }
    } else if (key->type == number_m) {
        index = hash_num(key->data.numVal);
        b = t->numIndexed[index];
        while (b) {
            if (b->key.data.numVal == key->data.numVal)
                return &b->value;
            b = b->next;
        }
    } else {
        avm_error("Unsupported key type in avm_tablegetelem");
    }

    return NULL;
}
