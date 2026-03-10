#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "SymTable.h"

// Global flag for debug output - defined weak so it can be overridden
__attribute__((weak)) int suppress_debug_output = 0;

SymEntry* ScopeList = NULL;
SymTable* SymHash = NULL;

// Offset counters for different memory spaces
static unsigned programvar_offset = 0;    // Global variables
static unsigned functionlocal_offset = 0; // Local variables  
static unsigned formalarg_offset = 0;     // Function parameters

int hashCode(int key) {
    return key % HASH_SIZE;
}

SymTable* symtable_init() {
    SymHash = malloc(sizeof(SymTable));
    SymHash->hash_count = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        SymHash->hashTable[i] = NULL;
    }
    return SymHash;
}

SymEntry* symtable_create_scope(int scope, int isActive) {
    SymEntry *node = malloc(sizeof(SymEntry));
    node->isActive = isActive;
    node->next_link = NULL;
    node->next_scope = NULL;
    node->scope = scope;
    return node;
}

SymEntry* symtable_insert_direct(int scope, int line, Type type, char* name) {
    // Enable debug output to diagnose offset issue
    // fprintf(stderr, ">> Inserting: name=%s, type=%d, scope=%u\n", name, type, scope);
    int key = hashCode(SymHash->hash_count++);
    SymEntry *entry = malloc(sizeof(SymEntry));
    entry->isActive = 1;
    entry->type = type;
    entry->scope = scope;
    entry->line = line;
    entry->name = name;
    entry->next_link = SymHash->hashTable[key];
    SymHash->hashTable[key] = entry;
    
    // ASSIGN PROPER OFFSET AND SPACE BASED ON TYPE AND SCOPE
    switch (type) {
        case GLOBAL:
    entry->space = programvar;
            // fprintf(stderr, ">> GLOBAL: before_offset=%u ", programvar_offset);
    entry->offset = programvar_offset++;
            // fprintf(stderr, "assigned_offset=%u after_counter=%u\n", entry->offset, programvar_offset);
    break;


        case LOCALVAR:
            if (scope == 0) {
                // Local variables in global scope are treated as global
                entry->space = programvar;
                // fprintf(stderr, ">> LOCALVAR scope=0: before_offset=%u ", programvar_offset);
                entry->offset = programvar_offset++;
                // fprintf(stderr, "assigned_offset=%u after_counter=%u\n", entry->offset, programvar_offset);
            } else {
                // Local variables in function scope
                entry->space = functionlocal;
                entry->offset = functionlocal_offset++;
            }
            break;
        case FORMAL:
            entry->space = formalarg;
            entry->offset = formalarg_offset++;
            break;
        case USERFUNC:
        case LIBFUNC:
            // Functions don't need memory allocation
            entry->space = programvar; // Default value
            entry->offset = 0;
            break;
        default:
            entry->space = programvar;
            entry->offset = 0;
            break;
    }
    
    // Enable debug output to diagnose offset issue
    // fprintf(stderr, ">> Assigned: name=%s, space=%d, offset=%u\n", name, entry->space, entry->offset);

    SymEntry *currentScope = ScopeList, *prev = NULL;
    while (currentScope && currentScope->scope < scope) {
        prev = currentScope;
        currentScope = currentScope->next_link;
    }

    if (!currentScope || currentScope->scope != scope) {
        SymEntry *newScope = symtable_create_scope(scope, -1);
        if (!prev) {
            newScope->next_link = ScopeList;
            ScopeList = newScope;
        } else {
            newScope->next_link = prev->next_link;
            prev->next_link = newScope;
        }
        currentScope = newScope;
    }

    SymEntry *last = currentScope;
    while (last->next_scope) last = last->next_scope;
    last->next_scope = entry;
    entry->next_scope = NULL;
}

SymEntry* symtable_lookup(char* name, int scope) {
    SymEntry* scopeNode = ScopeList;
    while (scopeNode && scopeNode->scope != scope) {
        scopeNode = scopeNode->next_link;
    }
    if (!scopeNode) return NULL;

    SymEntry* entry = scopeNode->next_scope;
    while (entry && (strcmp(entry->name, name) || !entry->isActive)) {
        entry = entry->next_scope;
    }
    return entry;
}

SymEntry* symtable_lookup_formal(char* name, int scope) {
    SymEntry* scopeNode = ScopeList;
    while (scopeNode && scopeNode->scope != scope) {
        scopeNode = scopeNode->next_link;
    }
    if (!scopeNode) return NULL;

    SymEntry* entry = scopeNode->next_scope;
    while (entry && (strcmp(entry->name, name) || entry->type != FORMAL)) {
        entry = entry->next_scope;
    }
    return entry;
}

SymEntry* symtable_lookup_above(char* name, int scope) {
    while (--scope > 0) {
        SymEntry* result = symtable_lookup(name, scope);
        if (result) return result;
    }
    return NULL;
}

SymEntry* symtable_lookup_lib(char* name) {
    SymEntry* lib = ScopeList ? ScopeList->next_scope : NULL;
    while (lib && lib->type == LIBFUNC && strcmp(lib->name, name)) {
        lib = lib->next_scope;
    }
    return (lib && !strcmp(lib->name, name) && lib->type == LIBFUNC) ? lib : NULL;
}
SymEntry* symtable_insert_checked(unsigned int scope, unsigned int line, Type type, char* name) {
    SymEntry* existing = symtable_lookup(name, scope);

    if (existing) {
        // Αν υπάρχει ήδη η ίδια κατηγορία (π.χ. function vs function)
        if (existing->type == type) {
            printf("ERROR: duplicate definition token: %s, at line %d\n", name, line);
            return NULL;
        }

        // Αν έχεις function/lib και προσπαθείς να κάνεις insert μεταβλητή
        if ((existing->type == USERFUNC || existing->type == LIBFUNC) &&
            (type != USERFUNC && type != LIBFUNC)) {
            printf("ERROR: variable already defined as function token: %s, at line %d\n", name, line);
            return NULL;
        }

        // Αν έχεις variable και προσπαθείς να το κάνεις function
        if ((existing->type != USERFUNC && existing->type != LIBFUNC) && type == USERFUNC) {
            printf("ERROR: function redefined as variable token: %s, at line %d\n", name, line);
            return NULL;
        }

        return existing;
    }

    // Αν υπάρχει conflict με βιβλιοθήκη (LIBFUNC)
    if (type != LIBFUNC && symtable_lookup_lib(name)) {
        printf("ERROR: shadowing library function: %s, at line %d\n", name, line);
        return NULL;
    }

    // Αλλιώς κάνε insert στο scope
    symtable_insert_direct(scope, line, type, name);
    return symtable_lookup(name, scope);
}

SymEntry* insertTemp(char* name) {
    extern int scope;
    extern int yylineno;
    return symtable_insert_checked(scope, yylineno, LOCALVAR, name);
}

void hide_scope(int scope) {
    if (scope == 0) {
        fprintf(stderr, "ERROR: Cannot hide global scope (0)\n");
        return;
    }
    SymEntry* node = ScopeList;
    while (node && node->scope != scope) {
        node = node->next_link;
    }
    if (!node) return;

    SymEntry* entry = node->next_scope;
    while (entry) {
        entry->isActive = 0;
        entry = entry->next_scope;
    }
}

int isHidden(SymEntry* record) {
    return !record->isActive;
}

void insert_LIBFUNCTs() {
    const char* libFuncs[] = {
        "print", "input", "objectmemberkeys", "objecttotalmembers",
        "objectcopy", "totalarguments", "argument", "typeof",
        "strtonum", "sqrt", "cos", "sin", "f"
    };
    for (int i = 0; i < sizeof(libFuncs)/sizeof(libFuncs[0]); ++i) {
        symtable_insert_direct(0, 0, LIBFUNC, (char*)libFuncs[i]);
    }
}

void symtable_print() {
    SymEntry *scopeNode = ScopeList;
    while (scopeNode) {
        printf("\n----------    Scope: #%d    ----------\n", scopeNode->scope);
        SymEntry *entry = scopeNode->next_scope;
        while (entry) {
            const char* typeStr = "";
            switch (entry->type) {
                case FORMAL: typeStr = "[formal argument]"; break;
                case USERFUNC: typeStr = "[user function]"; break;
                case GLOBAL: typeStr = "[global variable]"; break;
                case LOCALVAR: typeStr = "[local variable]"; break;
                case LIBFUNC: typeStr = "[library function]"; break;
                default: break;
            }
            printf("\"%s\" %s (line: %d) (scope: %d)\n", entry->name, typeStr, entry->line, entry->scope);
            entry = entry->next_scope;
        }
        scopeNode = scopeNode->next_link;
    }
}


