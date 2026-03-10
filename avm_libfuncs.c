#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "avm.h"

extern unsigned totalActuals;
extern unsigned avm_totalactuals(void);
extern avm_memcell* avm_getactual(unsigned i);
extern char* avm_tostring(avm_memcell* m);
extern unsigned int topsp;  // Add topsp declaration

// Library function registry hash table
#define AVM_LIBFUNC_HASHSIZE 23

typedef struct libfunc_entry {
    char* id;
    library_func_t func;
    struct libfunc_entry* next;
} libfunc_entry;

static libfunc_entry* libfuncs_table[AVM_LIBFUNC_HASHSIZE];

static unsigned hash(const char* s) {
    unsigned hash = 0;
    while (*s)
        hash = hash * 31 + *s++;
    return hash % AVM_LIBFUNC_HASHSIZE;
}

void avm_registerlibfunc(const char* id, library_func_t func) {
    unsigned index = hash(id);
    libfunc_entry* new_entry = malloc(sizeof(libfunc_entry));
    new_entry->id = strdup(id);
    new_entry->func = func;
    new_entry->next = libfuncs_table[index];
    libfuncs_table[index] = new_entry;
}

library_func_t avm_getlibfunc(const char* id) {
    unsigned index = hash(id);
    libfunc_entry* entry = libfuncs_table[index];
    while (entry) {
        if (strcmp(entry->id, id) == 0)
            return entry->func;
        entry = entry->next;
    }
    return NULL;
}

// Μήνυμα λάθους και έξοδος
void avm_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Runtime Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    executionFinished = 1;
    exit(EXIT_FAILURE);
}
void avm_warning(const char* format, ...) {
    va_list args;
    fprintf(stderr, "Runtime Warning: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
// Αποθήκευση περιβάλλοντος κλήσης (πλήρης υλοποίηση)
void avm_callsaveenvironment(void) {
    // Parameters are already on the stack from top+1 to top+totalActuals
    // We need to save environment and set topsp correctly
    
    // Save environment data
    avm_memcell* arg_count = &stack[top];
    arg_count->type = number_m;
    arg_count->data.numVal = totalActuals;
    --top;

    avm_memcell* retaddr = &stack[top];
    retaddr->type = number_m;
    retaddr->data.numVal = pc + 1;
    --top;

    avm_memcell* old_top = &stack[top];
    old_top->type = number_m;
    old_top->data.numVal = top + totalActuals + 3;
    --top;

    avm_memcell* old_topsp = &stack[top];
    old_topsp->type = number_m;
    old_topsp->data.numVal = topsp;
    --top;
    
    // Set topsp to point to the last parameter pushed
    // Parameters are at: top+5, top+6, ..., top+4+totalActuals
    // The last parameter (first to be printed) is at top+4+totalActuals
    topsp = top + 4 + totalActuals;
}

void avm_calllibfunc(const char* id) {
    library_func_t f = avm_getlibfunc(id);
    if (f)
        (*f)();
    else
        avm_error("Unsupported library function called: %s", id);
}

unsigned avm_totalactuals(void) {
    return totalActuals;
}

avm_memcell* avm_getactual(unsigned i) {
    // Parameters are pushed in reverse order: last parameter is at topsp
    // To get them in the original order, we need to reverse the indexing
    unsigned n = avm_totalactuals();
    return &stack[topsp - (n - 1) + i];
}

char* avm_tostring(avm_memcell* m) {
    static char buffer[1024];
    switch (m->type) {
        case number_m: 
            sprintf(buffer, "%.3f", m->data.numVal); 
            break;
        case string_m: 
            sprintf(buffer, "%s", m->data.strVal);
            break;
        case bool_m: 
            sprintf(buffer, "%s", m->data.boolVal ? "true" : "false");
            break;
        case nil_m: 
            sprintf(buffer, "nil");
            break;
        case undef_m: 
            sprintf(buffer, "undefined");
            break;
        default: 
            sprintf(buffer, "[unsupported]");
            break;
    }
    return buffer;
}
void libfunc_print(void) {
    unsigned n = avm_totalactuals();
    
    // Use avm_getactual to access parameters in correct order
    // avm_getactual(0) gives the first parameter to be printed
    for (unsigned i = 0; i < n; ++i) {
        avm_memcell* arg = avm_getactual(i);
        const char* s = avm_tostring(arg);

        if (!s) s = "nil";

        printf("%s", s);
        if (i < n - 1) printf(" ");
    }

    printf("\n");
    retval.type = nil_m;
    totalActuals = 0;
}

// VM initialization function
void avm_initialize(void) {
    // Initialize the hash table
    for (int i = 0; i < AVM_LIBFUNC_HASHSIZE; i++) {
        libfuncs_table[i] = NULL;
    }
    
    // Register implemented library functions
    avm_registerlibfunc("print", libfunc_print);
    
    // TODO: Implement and register other library functions:
    // input, objectmemberkeys, objecttotalmembers, objectcopy,
    // totalarguments, argument, typeof, strtonum, sqrt, cos, sin
    
}

void execute_halt(instruction* instr) {
    printf("HALT instruction reached. Stopping VM.\n");
    executionFinished = 1;
}

