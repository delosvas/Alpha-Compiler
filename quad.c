#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"
#include "SymTable.h"
unsigned totalQuads = 0;
unsigned total = 0;
unsigned currQuad = 0;
quad* quads = NULL;
extern int yylineno;
void init_quads(void) {
    if (quads == NULL) {
        expand();
    }
    currQuad = 0;
}

extern int scope;

expr* newexpr_constvar(const char* name) {
    expr* e = newexpr(var_e);
    e->sym = symtable_lookup((char*)name, scope);

    if (!e->sym) {
        // Κρίσιμη αλλαγή εδώ 👇
        Type declared_type = (scope == 0) ? GLOBAL : LOCALVAR;
        symtable_insert_checked(scope, yylineno, declared_type, (char*)name);
        e->sym = symtable_lookup((char*)name, scope);
    }

    return e;
}



void expand(void) {
    if (quads == NULL) {
        quads = (quad*)malloc(EXPAND_SIZE * sizeof(quad));
        if (!quads) {
            fprintf(stderr, "[expand] Allocation failed\n");
            exit(1);
        }
        total = EXPAND_SIZE;
    } else {
        quad* newQuads = (quad*)realloc(quads, (total + EXPAND_SIZE) * sizeof(quad));
        if (!newQuads) {
            fprintf(stderr, "[expand] Reallocation failed\n");
            exit(1);
        }
        quads = newQuads;
        total += EXPAND_SIZE;
    }
}

expr* newexpr(expr_t type) {
    expr* e = (expr*)malloc(sizeof(expr));
    if (!e) {
        fprintf(stderr, "[newexpr] Allocation failed\n");
        exit(1);
    }
    memset(e, 0, sizeof(expr));
    e->type = type;
    return e;
}

SymEntry* newtemp(void) {
    static unsigned tempCount = 0;
    char name[20];
    sprintf(name, "_t%u", tempCount++);
    return insertTemp(strdup(name));
}

const char* opcodeToStr(iopcode op);
void print_expr(expr* e);

void emit(iopcode op, expr* arg1, expr* arg2, expr* result, unsigned label, unsigned line) {
    if (currQuad == total) expand();

    quads[currQuad].op = op;
    quads[currQuad].arg1 = arg1;
    quads[currQuad].arg2 = arg2;
    quads[currQuad].result = result;
    quads[currQuad].label = label;
    quads[currQuad].line = line;

    currQuad++;
    if ((op >= if_eq && op <= if_greater) && (!arg1 || !arg2)) {
    fprintf(stderr, "waring: emit(%s) with NULL operand at line %u\n", opcodeToStr(op), line);
}

}

const char* opcodeToStr(iopcode op) {
    static const char* names[] = {
        "assign", "add", "sub", "mul", "div", "mod", "uminus", "and", "or", "not",
        "if_eq", "if_noteq", "if_lesseq", "if_greatereq", "if_less", "if_greater",
        "jump", "call", "param", "ret", "getretval", "funcstart", "funcend", "tablecreate",
        "tablegetelem", "tablesetelem"
    };
    return names[op];
}

void print_expr(expr* e) {
    if (!e) {
        printf("_");
        return;
    }

    switch (e->type) {
        case constnum_e:
            printf("%.2f", e->numConst);
            break;
        case constbool_e:
            printf(e->boolConst ? "TRUE" : "FALSE");
            break;
        case conststring_e:
            printf("\"%s\"", e->strConst);
            break;
        case var_e:
        case tableitem_e:
        case arithexpr_e:
        case boolexpr_e:
        case assignexpr_e:
        case newtable_e:
        case programfunc_e:
        case libraryfunc_e:
            if (e->sym && e->sym->name)
                printf("%s", e->sym->name);
            else
                printf("_");
            break;
        default:
            printf("_");
    }
}

int strlen_expr(expr* e) {
    if (!e) return 1;
    switch (e->type) {
        case constnum_e:
            return snprintf(NULL, 0, "%.2f", e->numConst);
        case constbool_e:
            return e->boolConst ? 4 : 5;
        case conststring_e:
            return strlen(e->strConst) + 2;
        default:
            return (e->sym && e->sym->name) ? strlen(e->sym->name) : 1;
    }
}

void print_quads_to_file(FILE* fp) {
    if (!fp) {
        fprintf(stderr, "Error: Invalid file pointer\n");
        return;
    }

    if (quads == NULL) {
        fprintf(fp, "No quads generated\n");
        return;
    }

    fprintf(fp, "\n#  %-13s%-13s%-13s%-13s%-5s\n", "opcode", "result", "arg1", "arg2", "label");
    for (unsigned i = 0; i < currQuad; ++i) {
        fprintf(fp, "%-3u%-13s", i + 1, opcodeToStr(quads[i].op));

        if (!quads[i].result) {
            fprintf(fp, "_%*s", 13, "");
        } else {
            switch (quads[i].result->type) {
                case constnum_e:
                    fprintf(fp, "%.2f%*s", quads[i].result->numConst, 15 - strlen_expr(quads[i].result), "");
                    break;
                case constbool_e:
                    fprintf(fp, "%s%*s", quads[i].result->boolConst ? "TRUE" : "FALSE", 15 - strlen_expr(quads[i].result), "");
                    break;
                case conststring_e:
                    fprintf(fp, "\"%s\"%*s", quads[i].result->strConst, 15 - strlen_expr(quads[i].result), "");
                    break;
                default:
                    if (quads[i].result->sym && quads[i].result->sym->name)
                        fprintf(fp, "%s%*s", quads[i].result->sym->name, 15 - strlen_expr(quads[i].result), "");
                    else
                        fprintf(fp, "_%*s", 14, "");
            }
        }

        if (!quads[i].arg1) {
            fprintf(fp, "_%*s", 13, "");
        } else {
            switch (quads[i].arg1->type) {
                case constnum_e:
                    fprintf(fp, "%.2f%*s", quads[i].arg1->numConst, 15 - strlen_expr(quads[i].arg1), "");
                    break;
                case constbool_e:
                    fprintf(fp, "%s%*s", quads[i].arg1->boolConst ? "TRUE" : "FALSE", 15 - strlen_expr(quads[i].arg1), "");
                    break;
                case conststring_e:
                    fprintf(fp, "\"%s\"%*s", quads[i].arg1->strConst, 15 - strlen_expr(quads[i].arg1), "");
                    break;
                default:
                    if (quads[i].arg1->sym && quads[i].arg1->sym->name)
                        fprintf(fp, "%s%*s", quads[i].arg1->sym->name, 15 - strlen_expr(quads[i].arg1), "");
                    else
                        fprintf(fp, "_%*s", 14, "");
            }
        }

        if (!quads[i].arg2) {
            fprintf(fp, "_%*s", 13, "");
        } else {
            switch (quads[i].arg2->type) {
                case constnum_e:
                    fprintf(fp, "%.2f%*s", quads[i].arg2->numConst, 15 - strlen_expr(quads[i].arg2), "");
                    break;
                case constbool_e:
                    fprintf(fp, "%s%*s", quads[i].arg2->boolConst ? "TRUE" : "FALSE", 15 - strlen_expr(quads[i].arg2), "");
                    break;
                case conststring_e:
                    fprintf(fp, "\"%s\"%*s", quads[i].arg2->strConst, 15 - strlen_expr(quads[i].arg2), "");
                    break;
                default:
                    if (quads[i].arg2->sym && quads[i].arg2->sym->name)
                        fprintf(fp, "%s%*s", quads[i].arg2->sym->name, 15 - strlen_expr(quads[i].arg2), "");
                    else
                        fprintf(fp, "_%*s", 14, "");
            }
        }

        fprintf(fp, "%-5u\n", quads[i].label);
    }
}

void print_quads(void) {
    print_quads_to_file(stdout);
    
    FILE* fp = fopen("quads.txt", "w");
    if (!fp) {
        fprintf(stderr, "Error opening quads.txt for writing\n");
        return;
    }
    print_quads_to_file(fp);
    fclose(fp);
}

expr* newexpr_constnum(double i) {
    expr* e = newexpr(constnum_e);
    e->numConst = i;
    return e;
}

expr* newexpr_constbool(unsigned char i) {
    expr* e = newexpr(constbool_e);
    e->boolConst = i;
    return e;
}

expr* newexpr_conststring(char* s) {
    expr* e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}

unsigned nextquadlabel(void) {
    return currQuad;
}

void patchlabel(unsigned quadNo, unsigned label) {
    quads[quadNo].label = label;
}

void patchresult(unsigned quadNo, unsigned label) {
    // Create a constant number expression for the jump target
    expr* target = newexpr_constnum(label);
    quads[quadNo].result = target;
}

unsigned int* newlist(unsigned int quad) {
    unsigned int* list = malloc(2 * sizeof(unsigned int));
    if (!list) {
        fprintf(stderr, "[newlist] Allocation failed\n");
        exit(1);
    }
    list[0] = quad;
    list[1] = 0xFFFFFFFF;  // Use -1 as terminator instead of 0
    return list;
}

unsigned int* mergelist(unsigned int* list1, unsigned int* list2) {
    if (!list1) return list2;
    if (!list2) return list1;

    // Count elements in both lists
    int count1 = 0, count2 = 0;
    while (list1[count1] != 0xFFFFFFFF) count1++;
    while (list2[count2] != 0xFFFFFFFF) count2++;

    // Reallocate list1 to hold both lists
    list1 = realloc(list1, (count1 + count2 + 1) * sizeof(unsigned int));
    if (!list1) {
        fprintf(stderr, "[mergelist] Reallocation failed\n");
        exit(1);
    }

    // Copy list2 into list1
    for (int i = 0; i < count2; i++) {
        list1[count1 + i] = list2[i];
    }
    list1[count1 + count2] = 0xFFFFFFFF;  // terminator

    free(list2);
    return list1;
}

void backpatch(unsigned int* list, unsigned int label) {
    while (list && *list != 0xFFFFFFFF) {
        patchlabel(*list, label);
        list++;
    }
}

void backpatch_result(unsigned int* list, unsigned int label) {
    while (list && *list != 0xFFFFFFFF) {
        patchresult(*list, label);
        list++;
    }
}

expr* emit_iftableitem(expr* e) {
    if (e->type != tableitem_e) return e;

    expr* result = newexpr(var_e);
    result->sym = newtemp();
    emit(tablegetelem, e, e->index, result, 0, yylineno);
    return result;
}




