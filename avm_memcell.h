
#ifndef AVM_MEMCELL_H
#define AVM_MEMCELL_H

typedef struct avm_table avm_table;

typedef enum {
    number_m = 0,
    string_m,
    bool_m,
    table_m,
    userfunc_m,
    libfunc_m,
    nil_m,
    undef_m
} avm_memcell_t;

typedef struct avm_memcell {
    avm_memcell_t type;
    union {
        double numVal;
        char* strVal;
        unsigned char boolVal;
        avm_table* tableVal;
        unsigned int funcVal;
        char* libfuncVal;
    } data;
} avm_memcell;

// Δήλωση συναρτήσεων που χρησιμοποιούν τον typedef avm_memcell
void avm_tableincrefcounter(avm_table* t);
unsigned char avm_tobool(avm_memcell* m);

int avm_is_true(avm_memcell* m);
void avm_dump_mem(void);
#endif // AVM_MEMCELL_H


