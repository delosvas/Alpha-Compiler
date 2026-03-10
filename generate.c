#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "quad.h"
#include "instruction.h"
#include "consts.h"
#include "target_codegen.h"
#include "generate.h"

extern quad* quads;
extern unsigned currQuad;
extern void add_incomplete_jump(unsigned instrNo, unsigned iaddress);
extern unsigned instr_total;

void generate_ASSIGN(quad* q) {
    instruction t;
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    t.arg2.type = nil_a;
    emit_instruction(t);
}

void generate_ADD(quad* q) {
    instruction t;
    t.opcode = add_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}

void generate_SUB(quad* q) {
    instruction t;
    t.opcode = sub_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}

void generate_MUL(quad* q) {
    instruction t;
    t.opcode = mul_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}

void generate_DIV(quad* q) {
    instruction t;
    t.opcode = div_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}

void generate_MOD(quad* q) {
    instruction t;
    t.opcode = mod_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}


void generate_JUMP(quad* q) {
    instruction t;
    t.opcode = jump_v;
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    t.result.type = label_a;

    q->taddress = instr_total;

    // Προστασία: Αν το label είναι ίδιο με τη θέση του instruction → skip
    if (q->label == instr_total) {
        printf("Self-jump skipped at instruction %u (quad %u)\n", instr_total, q - quads);
        return;
    }

    if (q->label < instr_total) {
        t.result.val = q->label;
    } else {
        t.result.val = 0; // temporary, patch later
        add_incomplete_jump(instr_total, q->label);
    }

    emit_instruction(t);
}



void generate_RELATIONAL(vmopcode op, quad* q) {
    instruction t;
    t.opcode = op;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    t.result.val = q->label;
    emit_instruction(t);
}

void generate_IF_EQ(quad* q) {
    instruction t;
    t.opcode = jeq_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    t.result.val = q->label;
    emit_instruction(t);
}
void generate_UMINUS(quad* q) {
    instruction t;
    t.opcode = mul_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);

    t.arg2.type = number_a;
    t.arg2.val = consts_newnumber(-1); // needs consts.c helper
    emit_instruction(t);
}

void generate_NEWTABLE(quad* q) {
    instruction t;
    t.opcode = newtable_v;
    make_operand(q->result, &t.result);
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}
void generate_TABLEGETELEM(quad* q) {
    instruction t;
    t.opcode = tablegetelem_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1); // table
    make_operand(q->arg2, &t.arg2); // index
    emit_instruction(t);
}

void generate_TABLESETELEM(quad* q) {
    instruction t;
    t.opcode = tablesetelem_v;
    make_operand(q->result, &t.result); // table
    make_operand(q->arg1, &t.arg1);     // index
    make_operand(q->arg2, &t.arg2);     // value
    emit_instruction(t);
}

void generate_IF_NOTEQ(quad* q)     { generate_RELATIONAL(jne_v, q); }
void generate_IF_GREATER(quad* q)   { generate_RELATIONAL(jgt_v, q); }
void generate_IF_GREATEREQ(quad* q) { generate_RELATIONAL(jge_v, q); }
void generate_IF_LESS(quad* q)      { generate_RELATIONAL(jlt_v, q); }
void generate_IF_LESSEQ(quad* q)    { generate_RELATIONAL(jle_v, q); }

void generate_NOT(quad* q) {
    instruction t;
    t.opcode = not_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    t.arg2.type = nil_a;
    emit_instruction(t);
}

void generate_OR(quad* q) {
    instruction t;
    t.opcode = or_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}

void generate_AND(quad* q) {
    instruction t;
    t.opcode = and_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    emit_instruction(t);
}

void generate_PARAM(quad* q) {
    instruction t;
    t.opcode = param_v;
    make_operand(q->arg1, &t.arg1);
    t.result.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}

void generate_CALL(quad* q) {
    instruction t;
    t.opcode = call_v;
    make_operand(q->arg1, &t.result);  // Function goes in result field
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}
void generate_GETRETVAL(quad* q) {
    // Αναζήτηση του προηγούμενου CALL quad προς τα πίσω
    for (int i = (int)(q - quads) - 1; i >= 0; --i) {
        if (quads[i].op == call) {
            expr* f = quads[i].arg1;
            if (f && f->type == libraryfunc_e && f->sym && f->sym->name &&
                strcmp(f->sym->name, "print") == 0) {
                return; //skip getretval για print
            }
            break;  // βρέθηκε άλλο call → κανονική getretval
        }
    }

    instruction t;
    t.opcode = getretval_v;
    make_operand(q->result, &t.result);
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}
void generate_RETURN(quad* q) {
    instruction t;
    t.opcode = return_v;
    make_operand(q->arg1, &t.arg1);
    t.result.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}

void generate_FUNCSTART(quad* q) {
    instruction t;
    t.opcode = funcenter_v;
    make_operand(q->result, &t.result);
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}

void generate_FUNCEND(quad* q) {
    instruction t;
    t.opcode = funcexit_v;
    make_operand(q->result, &t.result);
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    emit_instruction(t);
}
void generate_BOOL_EXPR(vmopcode op, quad* q) {
    unsigned label_false = nextinstructionlabel() + 1; // Points to "assign false"
    unsigned label_true = label_false + 2;             // Points to "assign true"
    unsigned label_end = label_true + 1;               // Points to end

    // 1. if op → jump to TRUE
    instruction t1;
    t1.opcode = op;
    make_operand(q->arg1, &t1.arg1);  
    make_operand(q->arg2, &t1.arg2);
    t1.result.type = label_a;
    t1.result.val = label_true;  // Jump to TRUE if condition is met
    emit_instruction(t1);

    // 2. result := false (fallthrough when condition is false)
    instruction t2;
    t2.opcode = assign_v;
    make_operand(q->result, &t2.result);
    t2.arg1.type = bool_a;
    t2.arg1.val = 0;  // FALSE
    t2.arg2.type = nil_a;
    emit_instruction(t2);

    // 3. jump to END
    instruction t3;
    t3.opcode = jump_v;
    t3.result.type = label_a;
    t3.result.val = label_end;
    t3.arg1.type = nil_a;
    t3.arg2.type = nil_a;
    emit_instruction(t3);

    // 4. TRUE: result := true
    instruction t4;
    t4.opcode = assign_v;
    make_operand(q->result, &t4.result);
    t4.arg1.type = bool_a;
    t4.arg1.val = 1;  // TRUE
    t4.arg2.type = nil_a;
    emit_instruction(t4);

    // Update the quad's target address
    q->taddress = nextinstructionlabel();
}

// === Ενιαία γεννήτρια bytecode από quads ===
void generate(void) {
    for (unsigned i = 0; i < currQuad; i++) {
        switch (quads[i].op) {
            case assign:       generate_ASSIGN(&quads[i]); break;
            case add:          generate_ADD(&quads[i]); break;
            case sub:          generate_SUB(&quads[i]); break;
            case mul:          generate_MUL(&quads[i]); break;
            case div_:         generate_DIV(&quads[i]); break;
            case mod:          generate_MOD(&quads[i]); break;
            case uminus: generate_UMINUS(&quads[i]); break;
            case jump_:        generate_JUMP(&quads[i]); break;
            case if_eq:
    if (quads[i].result)
        generate_BOOL_EXPR(jeq_v, &quads[i]);
    else
        generate_IF_EQ(&quads[i]);
    break;

case if_noteq:
    if (quads[i].result)
        generate_BOOL_EXPR(jne_v, &quads[i]);
    else
        generate_IF_NOTEQ(&quads[i]);
    break;

case if_greater:
    if (quads[i].result)
        generate_BOOL_EXPR(jgt_v, &quads[i]);
    else
        generate_IF_GREATER(&quads[i]);
    break;

case if_greatereq:
    if (quads[i].result)
        generate_BOOL_EXPR(jge_v, &quads[i]);
    else
        generate_IF_GREATEREQ(&quads[i]);
    break;

case if_less:
    if (quads[i].result)
        generate_BOOL_EXPR(jlt_v, &quads[i]);
    else
        generate_IF_LESS(&quads[i]);
    break;

case if_lesseq:
    if (quads[i].result)
        generate_BOOL_EXPR(jle_v, &quads[i]);
    else
        generate_IF_LESSEQ(&quads[i]);
    break;

            case not_:         generate_NOT(&quads[i]); break;
            case or_:          generate_OR(&quads[i]); break;
            case and:          generate_AND(&quads[i]); break;
            case param:        generate_PARAM(&quads[i]); break;
            case call:         generate_CALL(&quads[i]); break;
            case getretval:    generate_GETRETVAL(&quads[i]); break;
            case return_:      generate_RETURN(&quads[i]); break;
            case funcstart:    generate_FUNCSTART(&quads[i]); break;
            case funcend:      generate_FUNCEND(&quads[i]); break;
            default:
                fprintf(stderr, "Unknown quad opcode %d at quad %u\n", quads[i].op, i);
                exit(1);
        }
    }
}

typedef struct incomplete_jump {
    unsigned instrNo;    // πού στο instruction array υπάρχει το jump
    unsigned iaddress;   // σε ποιο quad πήγαινε
    struct incomplete_jump* next;
} incomplete_jump;

incomplete_jump* ij_head = NULL;
unsigned ij_total = 0;

// Καλείται όταν έχεις jump σε label που δεν ξέρεις ακόμα (π.χ. σε if ή while)
void add_incomplete_jump(unsigned instrNo, unsigned iaddress) {
    incomplete_jump* new_node = malloc(sizeof(incomplete_jump));
    assert(new_node);

    new_node->instrNo = instrNo;
    new_node->iaddress = iaddress;
    new_node->next = ij_head;
    ij_head = new_node;
    ++ij_total;
}
extern quad* quads;
extern instruction* instructions;

void patch_incomplete_jumps(void) {
    incomplete_jump* curr = ij_head;
    while (curr) {
        assert(curr->iaddress < currQuad);
        instructions[curr->instrNo].result.val = quads[curr->iaddress].taddress;  // ή .label αν αυτό κρατάει τον στόχο
        curr = curr->next;
    }
}



