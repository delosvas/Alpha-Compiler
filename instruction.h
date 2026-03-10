#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

typedef enum {
    assign_v, add_v, sub_v, mul_v, div_v, mod_v,uminus_v,
    jeq_v, jne_v, jle_v, jge_v, jlt_v, jgt_v,
    jump_v,
    call_v, pusharg_v, funcenter_v, funcexit_v,
    newtable_v, tablegetelem_v, tablesetelem_v,
    nop_v,
    not_v,
    or_v,
    and_v,
    param_v,
    getretval_v,
    return_v,
    halt_v
} vmopcode;

typedef enum {
    label_a,
    global_a,
    formal_a,
    local_a,
    number_a,
    string_a,
    bool_a,
    nil_a,
    userfunc_a,
    libfunc_a,
    retval_a,
    undef_a
} vmarg_t;

typedef struct {
    vmarg_t type;
    unsigned val;
    const char* strVal;   // Πρόσθετο πεδίο για strings (libfuncs κλπ)
} vmarg;

typedef struct {
    vmopcode opcode;
    vmarg result;
    vmarg arg1;
    vmarg arg2;
    unsigned srcLine;
} instruction;

#define EXPAND_SIZE_INSTR 1024

extern instruction* instructions;
extern unsigned instr_total;
extern unsigned instr_curr;
extern instruction* instructions;
extern unsigned instr_curr;
extern unsigned instr_total;

void init_instructions(void);
void emit_instruction(instruction instr);

#endif // INSTRUCTION_H



