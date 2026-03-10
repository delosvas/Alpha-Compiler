#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "instruction.h"
#include "expr.h"
#include "consts.h"
#include "quad.h"
#include "generate.h"

extern quad* quads;
extern unsigned currQuad;

void expand_instructions(void) {
    if (instructions == NULL)
        instructions = (instruction*)malloc(instr_total * sizeof(instruction));

    if (instr_curr >= instr_total) {
        instr_total *= 2;
        instructions = (instruction*)realloc(instructions, instr_total * sizeof(instruction));
    }
}
void make_operand(expr* e, vmarg* arg) {
    assert(arg);

    if (!e) {
        arg->type = nil_a;
        arg->val = 0;
        return;
    }

    const char* name = (e->sym && e->sym->name) ? e->sym->name : "<anon>";

    switch (e->type) {
        case constnum_e:
            arg->type = number_a;
            arg->val = consts_newnumber(e->numConst);
            break;

        case conststring_e:
            arg->type = string_a;
            arg->val = consts_newstring(e->strConst);
            break;

        case constbool_e:
            arg->type = bool_a;
            arg->val = e->boolConst;
            break;

        case nil_e:
            arg->type = nil_a;
            arg->val = 0;
            break;

        case programfunc_e:
            arg->type = userfunc_a;
            arg->val = 0;  // Function address will be set during linking
            break;

        case libraryfunc_e:
            arg->type = libfunc_a;
            arg->val = consts_newlibfunc(e->sym->name);
            break;

        case var_e:
        case tableitem_e:
        case arithexpr_e:
        case boolexpr_e:
        case newtable_e:
        case assignexpr_e: {
            if (!e->sym) {
                arg->type = nil_a;
                arg->val = 0;
                return;
            }

            // Set vmarg type based on scope space
            switch (e->sym->space) {
                case programvar:   arg->type = global_a; break;
                case functionlocal: arg->type = local_a; break;
                case formalarg:    arg->type = formal_a; break;
                default:           arg->type = nil_a; break;
            }
            arg->val = e->sym->offset;
            break;
        }

        default:
            fprintf(stderr, "make_operand: Unsupported expr type %d\n", e->type);
            arg->type = nil_a;
            arg->val = 0;
            break;
    }
}

void write_instructions_to_file(FILE* f) {
    //  Γράφουμε πρώτα πόσες εντολές υπάρχουν
    fwrite(&instr_curr, sizeof(unsigned), 1, f);

    for (unsigned i = 0; i < instr_curr; ++i) {
        instruction* instr = &instructions[i];

        fwrite(&instr->opcode, sizeof(vmopcode), 1, f);
        fwrite(&instr->result, sizeof(vmarg), 1, f);
        fwrite(&instr->arg1, sizeof(vmarg), 1, f);
        fwrite(&instr->arg2, sizeof(vmarg), 1, f);
    }
}

unsigned nextinstructionlabel(void) {
    return instr_curr;
}
void emit_instr(vmopcode op, vmarg* result, vmarg* arg1, vmarg* arg2) {
    // Προσθήκη απλής εντολής στον πίνακα instructions
    instruction instr;
    instr.opcode = op;
    if (result) instr.result = *result; else instr.result.type = undef_a;
    if (arg1) instr.arg1 = *arg1; else instr.arg1.type = undef_a;
    if (arg2) instr.arg2 = *arg2; else instr.arg2.type = undef_a;

    instructions[instr_total++] = instr;
}
