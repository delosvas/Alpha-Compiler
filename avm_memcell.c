#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "avm_memcell.h"
#include "error.h"
#include "avm.h"
#include "instruction.h"
#include "avm_table.h"
#include "vm.h"



extern avm_memcell stack[AVM_STACKSIZE];
extern unsigned int pc;
extern unsigned int top;
extern unsigned int topsp;
extern int executionFinished;
extern avm_memcell retval;

// Forward declaration
void avm_memcellclear(avm_memcell* m);
void avm_assign(avm_memcell* lv, avm_memcell* rv) {
    if (lv == rv) {
        return;
    }

    if (rv->type == undef_m) {
        avm_memcellclear(lv);
        lv->type = bool_m;
        lv->data.boolVal = 0;
        return;
    }

    // Clear previous table if needed
    if (lv->type == table_m && lv->data.tableVal) {
        if (--(lv->data.tableVal->refCounter) == 0)
            avm_tabledestroy(lv->data.tableVal);
    }

    avm_memcellclear(lv);
    lv->type = rv->type;

    switch (rv->type) {
        case number_m:
            lv->data.numVal = rv->data.numVal;
            break;
        case string_m:
            lv->data.strVal = strdup(rv->data.strVal);
            break;
        case bool_m:
            lv->data.boolVal = rv->data.boolVal;
            break;
        case table_m:
            lv->data.tableVal = rv->data.tableVal;
            ++(lv->data.tableVal->refCounter);
            break;
        case userfunc_m:
            lv->data.funcVal = rv->data.funcVal;
            break;
        case libfunc_m:
            lv->data.libfuncVal = rv->data.libfuncVal;
            break;
        case nil_m:
            break;
        default:
            fprintf(stderr, "[assign][ERROR] Unknown type %d\n", rv->type);
            exit(EXIT_FAILURE);
    }
}

// Clear memcell content before overwrite
void avm_memcellclear(avm_memcell* m) {
    if (!m) return;

    switch (m->type) {
        case string_m:
            free(m->data.strVal);
            m->data.strVal = NULL;
            break;
        case table_m:
            if (m->data.tableVal && --(m->data.tableVal->refCounter) == 0) {
                avm_tabledestroy(m->data.tableVal);
            }
            m->data.tableVal = NULL;
            break;
        default:
            // No action needed for other types
            break;
    }
    m->type = undef_m;
}
avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg) {
    if (!arg) {
        fprintf(stderr, "[translate_operand] NULL argument\n");
        executionFinished = 1;
        return NULL;
    }

    switch (arg->type) {
        case global_a: {
            avm_memcell* addr = &stack[arg->val];
            return addr;
        }
        case local_a: {
            avm_memcell* addr = &stack[topsp - arg->val];
            return addr;
        }
        case formal_a: {
            avm_memcell* addr = &stack[topsp + 4 + arg->val];
            return addr;
        }
        case retval_a:
            return &retval;

        case number_a:
            reg->type = number_m;
            reg->data.numVal = (numConsts && arg->val < totalNumConsts) ? numConsts[arg->val] : 0.0;
            return reg;

        case string_a:
            reg->type = string_m;
            reg->data.strVal = (stringConsts && arg->val < totalStringConsts) ? strdup(stringConsts[arg->val]) : strdup("");
            return reg;

        case bool_a:
            reg->type = bool_m;
            reg->data.boolVal = arg->val != 0;
            return reg;

        case nil_a:
            reg->type = nil_m;
            return reg;

        case userfunc_a:
            reg->type = userfunc_m;
            reg->data.funcVal = arg->val;
            return reg;

        case libfunc_a:
            reg->type = libfunc_m;
            reg->data.libfuncVal = (libFuncs && arg->val < totalLibFuncs) ? strdup(libFuncs[arg->val]) : strdup("unknown_libfunc");
            return reg;

        case label_a:
            reg->type = number_m;
            reg->data.numVal = (double)arg->val;
            return reg;

        default:
            fprintf(stderr, "Error: Unknown vmarg type %d in avm_translate_operand\n", arg->type);
            executionFinished = 1;
            return NULL;
    }
}

void avm_initstack(void) {
    for (int i = 0; i < AVM_STACKSIZE; ++i) {
        stack[i].type = undef_m;
    }
}

unsigned char avm_tobool(avm_memcell* m) {
    assert(m);
    switch(m->type) {
        case nil_m:    return 0;
        case undef_m:  return 0;
        case bool_m:   return m->data.boolVal;
        case number_m: return m->data.numVal != 0;
        case string_m: return m->data.strVal && m->data.strVal[0] != '\0';
        default:       return 1;
    }
}

void avm_dump_mem(void) {
    printf("\n========= AVM MEMORY DUMP =========\n");
    for (unsigned i = 0; i < AVM_STACKSIZE; ++i) {
        avm_memcell* m = &stack[i];
        if (m->type != undef_m) {
            printf("stack[%u]: ", i);
            switch (m->type) {
                case number_m:
                    printf("NUMBER %.2f\n", m->data.numVal);
                    break;
                case string_m:
                    printf("STRING \"%s\"\n", m->data.strVal);
                    break;
                case bool_m:
                    printf("BOOL %s\n", m->data.boolVal ? "true" : "false");
                    break;
                case table_m:
                    printf("TABLE @%p\n", (void*)m->data.tableVal);
                    break;
                case userfunc_m:
                    printf("USERFUNC @%u\n", m->data.funcVal);
                    break;
                case libfunc_m:
                    printf("LIBFUNC %s\n", m->data.libfuncVal);
                    break;
                case nil_m:
                    printf("NIL\n");
                    break;
                case undef_m:
                    // won't be printed
                    break;
                default:
                    printf("UNKNOWN TYPE %d\n", m->type);
            }
        }
    }
    printf("===================================\n");
}

