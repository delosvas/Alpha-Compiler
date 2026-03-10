#ifndef QUAD_H
#define QUAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SymTable.h"

#define EXPAND_SIZE 1024

struct SymbolTableEntry;
typedef struct SymbolTableEntry SymbolTableEntry;

typedef enum expr_t {
    var_e,
    tableitem_e,
    programfunc_e,
    libraryfunc_e,
    arithexpr_e,
    boolexpr_e,
    assignexpr_e,
    newtable_e,
    constnum_e,
    constbool_e,
    conststring_e,
    nil_e,
    call_e	     
} expr_t;

typedef struct expr {
    enum expr_t type;
    SymbolTableEntry* sym;
    double numConst;
    char* strConst;
    unsigned char boolConst;

    unsigned quadstart;
    unsigned* truelist;
    unsigned* falselist;

    struct expr* index;
    struct expr* next;
} expr;

typedef enum iopcode {
    assign,
    add,
    sub,
    mul,
    div_,
    mod,
    uminus,
    and,
    or_,
    not_,
    if_eq,
    if_noteq,
    if_lesseq,
    if_greatereq,
    if_less,
    if_greater,
    jump_,
    call,
    param,
    getretval,
    return_,
    funcstart,
    funcend,
    tablecreate,
    tablegetelem,
    tablesetelem
} iopcode;

typedef struct quad {
    iopcode op;
    expr* result;
    expr* arg1;
    expr* arg2;
    unsigned label;
    unsigned line;
        unsigned taddress;
} quad;

struct forprefix {
    unsigned test;
    unsigned enter;
};

extern quad* quads;
extern unsigned total;
extern unsigned currQuad;

void init_quads(void);
void expand(void);
void emit(iopcode op, expr* arg1, expr* arg2, expr* result, unsigned label, unsigned line);
void print_quads(void);
void patchlabel(unsigned quadNo, unsigned label);
void patchresult(unsigned quadNo, unsigned label);
unsigned nextquadlabel(void);
SymEntry* symtable_insert_checked(unsigned int scope, unsigned int line, Type type, char* name);
void load_quads_from_file(const char* filename);

expr* newexpr(expr_t type);
expr* newexpr_constnum(double i);
expr* newexpr_constbool(unsigned char i);
unsigned int* newlist(unsigned int quad);
unsigned int* mergelist(unsigned int* list1, unsigned int* list2);
void backpatch(unsigned int* list, unsigned int label);
void backpatch_result(unsigned int* list, unsigned int label);
expr* emit_iftableitem(expr* e);
expr* newexpr_constvar(const char* name);

SymbolTableEntry* newtemp(void);

#endif




