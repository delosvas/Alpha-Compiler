#ifndef GENERATE_H
#define GENERATE_H

#include "quad.h"
#include "instruction.h"

// Γεννήτριες εντολών
void generate_ASSIGN(quad* q);
void generate_ADD(quad* q);
void generate_SUB(quad* q);
void generate_MUL(quad* q);
void generate_DIV(quad* q);
void generate_MOD(quad* q);
void generate_UMINUS(quad* q);

void generate_AND(quad* q);
void generate_OR(quad* q);
void generate_NOT(quad* q);

// Συνθήκες
void generate_IF_EQ(quad* q);
void generate_IF_NOTEQ(quad* q);
void generate_IF_GREATER(quad* q);
void generate_IF_GREATEREQ(quad* q);
void generate_IF_LESS(quad* q);
void generate_IF_LESSEQ(quad* q);

// Έλεγχος ροής
void generate_JUMP(quad* q);
void patch_incomplete_jumps(void);

// Συναρτήσεις / Κλήσεις
void generate_PARAM(quad* q);
void generate_CALL(quad* q);
void generate_GETRETVAL(quad* q);
void generate_RETURN(quad* q);
void generate_FUNCSTART(quad* q);
void generate_FUNCEND(quad* q);
void generate_BOOL_EXPR(vmopcode op, quad* q);
// Πίνακες
void generate_NEWTABLE(quad* q);
void generate_TABLEGETELEM(quad* q);
void generate_TABLESETELEM(quad* q);
unsigned nextinstructionlabel(void);
// Μαζική παραγωγή εντολών
void generate(void);

#endif




