#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "instruction.h"

instruction* instructions = NULL;
unsigned instr_total = 0;
unsigned instr_curr = 0;

static void expand_instruction_array(void) {
    assert(instr_curr == instr_total);
    instruction* newspace = malloc((instr_total + EXPAND_SIZE_INSTR) * sizeof(instruction));
    if (instructions) {
        memcpy(newspace, instructions, instr_total * sizeof(instruction));
        free(instructions);
    }
    instructions = newspace;
    instr_total += EXPAND_SIZE_INSTR;
}

void emit_instruction(instruction instr) {
    if (instr_curr == instr_total)
        expand_instruction_array();
    instructions[instr_curr++] = instr;
}

void init_instructions(void) {
    instructions = NULL;
    instr_total = 0;
    instr_curr = 0;
    expand_instruction_array();
}


