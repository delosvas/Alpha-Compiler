#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdio.h>

#define HASH_SIZE 50

typedef enum SymbolTableType {
    GLOBAL,
    LOCALVAR,
    FORMAL,
    USERFUNC,
    LIBFUNC
} Type;

typedef enum scopespace_t {
    programvar,
    functionlocal,
    formalarg
} scopespace_t;

typedef struct SymbolTableEntry {
    int isActive;
    unsigned int scope;
    unsigned int line;
    char* name;
    Type type;

    // Πεδία VM
    scopespace_t space;   // π.χ. για make_operand
    unsigned offset;      // offset ανά χώρο μνήμης

    struct SymbolTableEntry* next_scope;
    struct SymbolTableEntry* next_link;
} SymEntry;

typedef struct HashTable {
    int hash_count;
    SymEntry* hashTable[HASH_SIZE];
} SymTable;

// global pointers
extern SymEntry* ScopeList;
extern SymTable* SymHash;

// Functions
int hashCode(int key);
SymTable* symtable_init(void);
SymEntry* symtable_create_scope(int scope, int isActive);
SymEntry* symtable_insert_direct(int scope, int line, Type type, char* name);
SymEntry* symtable_insert_checked(unsigned scope, unsigned line, Type type, char* name);
SymEntry* insertTemp(char* name);

SymEntry* symtable_lookup(char* sym_name, int scope);
SymEntry* symtable_lookup_formal(char* sym_name, int scope);
SymEntry* symtable_lookup_above(char* sym_name, int scope);
SymEntry* symtable_lookup_lib(char* name);

void hide_scope(int scope);
int isHidden(SymEntry* record);
void insert_LIBFUNCTS(void);
void symtable_print(void);

#endif

