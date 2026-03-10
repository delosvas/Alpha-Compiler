#ifndef TARGET_CODEGEN_H
#define TARGET_CODEGEN_H

#include "instruction.h"
#include "expr.h"
#include "quad.h"

void make_operand(expr* e, vmarg* arg);

void generate_ASSIGN(quad* q);
void generate_ADD(quad* q);
void generate_SUB(quad* q);
void generate_MUL(quad* q);
void generate_DIV(quad* q);
void generate_MOD(quad* q);

void generate_JUMP(quad* q);
void generate_IF_EQ(quad* q);
void generate_IF_NOTEQ(quad* q);

void generate_FUNCSTART(quad* q);
void generate_FUNCEND(quad* q);
void generate_BOOL_EXPR(vmopcode op, quad* q);
void generate_RETURN(quad* q);

void generate_PARAM(quad* q);
void generate_CALL(quad* q);
void generate_GETRETVAL(quad* q);

void generate_NEWTABLE(quad* q);
void generate_TABLEGETELEM(quad* q);
void generate_TABLESETELEM(quad* q);

#endif


