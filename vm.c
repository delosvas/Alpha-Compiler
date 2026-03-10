#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>          // Για assert
#include "avm.h"
#include "avm_memcell.h"
#include "instruction.h"
#include "error.h"
#include "avm_table.h"

// ΜΟΝΟ ΕΔΩ γίνεται η οριστική δήλωση:
#define SAVED_PC_OFFSET 1
int executionFinished = 0;
avm_memcell stack[AVM_STACKSIZE];
unsigned int pc = 0;
unsigned int top = AVM_STACKSIZE - 1;
unsigned int topsp = 0;
extern unsigned instr_curr;
extern unsigned instr_total;
avm_memcell retval;
unsigned totalActuals = 0;

// Προσωρινές μεταβλητές (registers) για χρήση στις εντολές
static avm_memcell ax, bx;

// Τύπος δείκτη συνάρτησης για εκτέλεση εντολών
typedef void (*execute_func_t)(instruction*);

// Δηλώσεις των συναρτήσεων εκτέλεσης
void execute_assign(instruction* instr);
void execute_add(instruction* instr);
void execute_sub(instruction* instr);
void execute_mul(instruction* instr);
void execute_div(instruction* instr);
void execute_mod(instruction* instr);
void execute_uminus(instruction* instr);
void execute_jeq(instruction* instr);
void execute_jne(instruction* instr);
void execute_jle(instruction* instr);
void execute_jge(instruction* instr);
void execute_jlt(instruction* instr);
void execute_jgt(instruction* instr);
void execute_jump(instruction* instr);
void execute_call(instruction* instr);
void execute_pusharg(instruction* instr);
void execute_funcenter(instruction* instr);
void execute_funcexit(instruction* instr);
void execute_newtable(instruction* instr);
void execute_tablegetelem(instruction* instr);
void execute_tablesetelem(instruction* instr);
void execute_nop(instruction* instr);
void execute_not(instruction* instr);
void execute_or(instruction* instr);
void execute_and(instruction* instr);
void execute_param(instruction* instr);
void execute_getretval(instruction* instr);
void execute_return(instruction* instr);
// Πίνακας δεικτών στις συναρτήσεις εκτέλεσης
static execute_func_t executeFuncs[] = {
    [assign_v] = execute_assign,
    [add_v] = execute_add,
    [sub_v] = execute_sub,
    [mul_v] = execute_mul,
    [div_v] = execute_div,
    [mod_v] = execute_mod,
    [uminus_v] = execute_uminus,
    [jeq_v] = execute_jeq,
    [jne_v] = execute_jne,
    [jle_v] = execute_jle,
    [jge_v] = execute_jge,
    [jlt_v] = execute_jlt,
    [jgt_v] = execute_jgt,
    [jump_v] = execute_jump,
    [call_v] = execute_call,
    [pusharg_v] = execute_pusharg,
    [funcenter_v] = execute_funcenter,
    [funcexit_v] = execute_funcexit,
    [newtable_v] = execute_newtable,
    [tablegetelem_v] = execute_tablegetelem,
    [tablesetelem_v] = execute_tablesetelem,
    [nop_v] = execute_nop,
    [not_v] = execute_not,
    [or_v] = execute_or,
    [and_v] = execute_and,
    [param_v] = execute_param,
    [getretval_v] = execute_getretval,
    [return_v] = execute_return,
    [halt_v]   = execute_halt
};


// Κύκλος εκτέλεσης VM
void execute_cycle(void) {
    if (executionFinished) return;
    if (instr_curr >= instr_total) {
        executionFinished = 1;
        return;
    }

    instruction* instr = &instructions[instr_curr];
    
    unsigned old_instr_curr = instr_curr;  // Save current instruction

    // Execute the instruction
    if (instr->opcode >= 0 && instr->opcode < sizeof(executeFuncs)/sizeof(executeFuncs[0])) {
        executeFuncs[instr->opcode](instr);
    } else {
        avm_error("Invalid instruction opcode");
        executionFinished = 1;
        return;
    }

    // Only increment if the instruction didn't change instr_curr (i.e., no jump)
    if (!executionFinished && instr_curr == old_instr_curr) {
        instr_curr++;
    }
    
    if (instr_curr >= instr_total) {
        executionFinished = 1;
    }
}

// Υλοποίηση εκχώρησης
void execute_assign(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);

    assert(lv && rv);

    avm_assign(lv, rv);
}

void execute_add(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* lv  = avm_translate_operand(&instr->result, NULL);

    assert(lv && rv1 && rv2);

    if (rv1->type != number_m || rv2->type != number_m) {
        avm_error("Error: add expects number operands\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(lv);  
    lv->type = number_m;
    lv->data.numVal = rv1->data.numVal + rv2->data.numVal;
}

void execute_call(instruction* instr) {
    avm_memcell* func = avm_translate_operand(&instr->result, &ax);
    assert(func);

    avm_memcellclear(&retval);  // Clear retval before new call
    avm_callsaveenvironment();

    if (func->type == userfunc_m) {
        pc = func->data.funcVal;
        assert(pc < instr_total);
    }
    else if (func->type == string_m) {
        avm_calllibfunc(func->data.strVal);
    }
    else if (func->type == libfunc_m) {
        avm_calllibfunc(func->data.libfuncVal);
    }
    else {
        avm_error("Error: call expects user function or library function\n");
        executionFinished = 1;
    }
}

void execute_sub(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* lv  = avm_translate_operand(&instr->result, NULL);

    assert(lv && rv1 && rv2);

    if (rv1->type != number_m || rv2->type != number_m) {
        avm_error("Error: sub expects number operands\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = rv1->data.numVal - rv2->data.numVal;
}

void execute_mul(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* lv  = avm_translate_operand(&instr->result, NULL);

    assert(lv && rv1 && rv2);

    if (rv1->type != number_m || rv2->type != number_m) {
        avm_error("Error: mul expects number operands\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = rv1->data.numVal * rv2->data.numVal;
}
void execute_div(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* lv  = avm_translate_operand(&instr->result, NULL);

    assert(lv && rv1 && rv2);

    if (rv1->type != number_m || rv2->type != number_m) {
        avm_error("Error: div expects number operands\n");
        executionFinished = 1;
        return;
    }

    if (fabs(rv2->data.numVal) < 1e-9) {  // Επιτρέπει αριθμούς κοντά στο 0
        avm_error("Runtime Warning: division by near-zero — result set to 0\n");
        avm_memcellclear(lv);
        lv->type = number_m;
        lv->data.numVal = 0.0;
        return;
    }

    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = rv1->data.numVal / rv2->data.numVal;
}

void execute_uminus(instruction* instr) {
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    assert(rv && lv);

    if (rv->type != number_m) {
        avm_error("uminus expects a number operand\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(lv);  // Καθαρίζουμε τον στόχο πριν γράψουμε
    lv->type = number_m;
    lv->data.numVal = -rv->data.numVal;

    // Αν το αποτέλεσμα είναι πολύ κοντά στο 0.0, κάν' το ακριβώς 0.0
    if (fabs(lv->data.numVal) < 1e-9)
        lv->data.numVal = 0.0;
}

void execute_mod(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* lv  = avm_translate_operand(&instr->result, NULL);

    assert(lv && rv1 && rv2);

    if (rv1->type != number_m || rv2->type != number_m) {
        avm_error("Error: mod expects number operands\n");
        executionFinished = 1;
        return;
    }

    if (rv2->data.numVal == 0.0) {
        avm_warning("Warning: modulo by zero, returning 0\n");
        avm_memcellclear(lv);
        lv->type = number_m;
        lv->data.numVal = 0.0;
        return;
    }

    avm_memcellclear(lv);
    lv->type = number_m;
    
    double dividend = rv1->data.numVal;
    double divisor = rv2->data.numVal;
    
    double result = fmod(dividend, divisor);
    
    // Alpha language modulo semantics:
    // If the absolute result is less than 1, treat it as 0
    if (fabs(result) < 1.0) {
        result = 0.0;
    }
    
    lv->data.numVal = result;
}

int avm_is_true(avm_memcell* m) {
    switch (m->type) {
        case number_m: return m->data.numVal != 0;
        case bool_m: return m->data.boolVal;
        case nil_m: return 0;
        default: return 1;
    }
}

void execute_jeq(instruction* instr) {

    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    if (!rv1 || !rv2) {
        avm_error("jeq: NULL operand");
        executionFinished = 1;
        return;
    }

    unsigned should_jump = 0;

    if (rv1->type == undef_m || rv2->type == undef_m) {
        avm_error("jeq: 'undef' involved in comparison");
        executionFinished = 1;
        return;
    }

    if (rv1->type == nil_m || rv2->type == nil_m) {
        should_jump = (rv1->type == nil_m && rv2->type == nil_m);
    } else if (rv1->type == bool_m || rv2->type == bool_m) {
        should_jump = avm_is_true(rv1) == avm_is_true(rv2);
    } else if (rv1->type != rv2->type) {
        avm_error("jeq: type mismatch in operands");
        executionFinished = 1;
        return;
    } else {
        switch (rv1->type) {
            case number_m: should_jump = (rv1->data.numVal == rv2->data.numVal); break;
            case string_m: should_jump = !strcmp(rv1->data.strVal, rv2->data.strVal); break;
            case table_m:  should_jump = (rv1->data.tableVal == rv2->data.tableVal); break;
            case userfunc_m: should_jump = (rv1->data.funcVal == rv2->data.funcVal); break;
            case libfunc_m: should_jump = !strcmp(rv1->data.libfuncVal, rv2->data.libfuncVal); break;
            default: assert(0);
        }
    }

    if (should_jump) {
        instr_curr = instr->result.val;  // Direct jump, no -1 needed
        return;  // Don't let execute_cycle increment instr_curr
    }
}

void execute_jne(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    if (!rv1 || !rv2) {
        avm_error("jne: NULL operand");
        executionFinished = 1;
        return;
    }

    if (rv1->type == undef_m || rv2->type == undef_m) {
        avm_error("jne: 'undef' operand in comparison");
        executionFinished = 1;
        return;
    }

    unsigned result;
    if (rv1->type == nil_m || rv2->type == nil_m) {
        result = !(rv1->type == nil_m && rv2->type == nil_m);
    } else if (rv1->type == bool_m || rv2->type == bool_m) {
        result = avm_is_true(rv1) != avm_is_true(rv2);
    } else if (rv1->type != rv2->type) {
        result = 1;
    } else {
        switch (rv1->type) {
            case number_m: result = (rv1->data.numVal != rv2->data.numVal); break;
            case string_m: result = strcmp(rv1->data.strVal, rv2->data.strVal) != 0; break;
            case table_m: result = (rv1->data.tableVal != rv2->data.tableVal); break;
            case userfunc_m: result = (rv1->data.funcVal != rv2->data.funcVal); break;
            case libfunc_m: result = strcmp(rv1->data.libfuncVal, rv2->data.libfuncVal) != 0; break;
            default: assert(0);
        }
    }

    if (result) {
        instr_curr = instr->result.val;  // Direct jump, no -1 needed
        return;  // Don't let execute_cycle increment instr_curr
    }
}

void execute_jlt(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    
    if (!rv1 || !rv2) {
        avm_error("jlt: invalid operands");
        executionFinished = 1;
        return;
    }

    // Convert operands to numbers if they are booleans
    double val1, val2;
    
    if (rv1->type == bool_m) {
        val1 = rv1->data.boolVal ? 1.0 : 0.0;
    } else if (rv1->type == number_m) {
        val1 = rv1->data.numVal;
    } else {
        avm_error("jlt: invalid operands");
        executionFinished = 1;
        return;
    }
    
    if (rv2->type == bool_m) {
        val2 = rv2->data.boolVal ? 1.0 : 0.0;
    } else if (rv2->type == number_m) {
        val2 = rv2->data.numVal;
    } else {
        avm_error("jlt: invalid operands");
        executionFinished = 1;
        return;
    }
    
    if (val1 < val2) {
        instr_curr = instr->result.val;  // Direct jump, no -1 needed
        return;  // Don't let execute_cycle increment instr_curr
    }
}

void execute_jle(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    
    if (!rv1 || !rv2) {
        avm_error("jle: invalid operands");
        executionFinished = 1;
        return;
    }

    // Convert operands to numbers if they are booleans
    double val1, val2;
    
    if (rv1->type == bool_m) {
        val1 = rv1->data.boolVal ? 1.0 : 0.0;
    } else if (rv1->type == number_m) {
        val1 = rv1->data.numVal;
    } else {
        avm_error("jle: invalid operands");
        executionFinished = 1;
        return;
    }
    
    if (rv2->type == bool_m) {
        val2 = rv2->data.boolVal ? 1.0 : 0.0;
    } else if (rv2->type == number_m) {
        val2 = rv2->data.numVal;
    } else {
        avm_error("jle: invalid operands");
        executionFinished = 1;
        return;
    }

    if (val1 <= val2) {
        instr_curr = instr->result.val;  // Direct jump, no -1 needed
        return;  // Don't let execute_cycle increment instr_curr
    }
}

void execute_jgt(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    
    if (!rv1 || !rv2) {
        avm_error("jgt: invalid operands");
        executionFinished = 1;
        return;
    }

    // Convert operands to numbers if they are booleans
    double val1, val2;
    
    if (rv1->type == bool_m) {
        val1 = rv1->data.boolVal ? 1.0 : 0.0;
    } else if (rv1->type == number_m) {
        val1 = rv1->data.numVal;
    } else {
        avm_error("jgt: invalid operands");
        executionFinished = 1;
        return;
    }
    
    if (rv2->type == bool_m) {
        val2 = rv2->data.boolVal ? 1.0 : 0.0;
    } else if (rv2->type == number_m) {
        val2 = rv2->data.numVal;
    } else {
        avm_error("jgt: invalid operands");
        executionFinished = 1;
        return;
    }

    if (val1 > val2) {
        instr_curr = instr->result.val;  // Direct jump, no -1 needed
        return;  // Don't let execute_cycle increment instr_curr
    }
}
void execute_jge(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    if (!rv1 || !rv2) {
        avm_error("jge: invalid operands (NULL)");
        executionFinished = 1;
        return;
    }

    double val1, val2;

    // First operand
    if (rv1->type == bool_m)
        val1 = rv1->data.boolVal ? 1.0 : 0.0;
    else if (rv1->type == number_m)
        val1 = rv1->data.numVal;
    else {
        avm_error("jge: invalid operand type for arg1 (expected number or bool)");
        executionFinished = 1;
        return;
    }

    // Second operand
    if (rv2->type == bool_m)
        val2 = rv2->data.boolVal ? 1.0 : 0.0;
    else if (rv2->type == number_m)
        val2 = rv2->data.numVal;
    else {
        avm_error("jge: invalid operand type for arg2 (expected number or bool)");
        executionFinished = 1;
        return;
    }

    if (val1 >= val2) {
        instr_curr = instr->result.val;  // Direct jump, no -1 needed
        return;  // Don't let execute_cycle increment instr_curr
    }
}


unsigned int avm_get_envvalue(unsigned int index) {
    assert(index < AVM_STACKSIZE);
    return stack[index].data.numVal; // assuming environment values are stored as numbers
}

void execute_jump(instruction* instr) {
    if (instr->result.type != label_a) {
        avm_error("jump_: invalid target");
        executionFinished = 1;
        return;
    }
    pc = instr->result.val;
}



void execute_pusharg(instruction* instr) {
    avm_memcell* arg = avm_translate_operand(&instr->arg1, &ax);
    assert(arg);

    avm_memcellclear(&stack[top]);
    avm_assign(&stack[top], arg);
    --top;

    ++totalActuals;
}
void execute_funcenter(instruction* instr) {
    avm_memcell* func = avm_translate_operand(&instr->result, &ax);
    assert(func && func->type == userfunc_m);

    topsp = top;
    totalActuals = 0;
}

void execute_funcexit(instruction* instr) {
    top = topsp;
    topsp = 0;
    pc = avm_get_envvalue(top + SAVED_PC_OFFSET);
}

void execute_newtable(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    assert(lv);

    avm_memcellclear(lv);
    avm_table* t = avm_tablenew();
    lv->type = table_m;
    lv->data.tableVal = t;

    avm_tableincrefcounter(t);
}

void execute_tablegetelem(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* t  = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* i  = avm_translate_operand(&instr->arg2, &bx);
    assert(lv && t && i);

    avm_memcellclear(lv);
    lv->type = nil_m;

    if (t->type != table_m) {
        avm_error("Not a table in getelem");
        executionFinished = 1;
        return;
    }

    avm_memcell* content = avm_tablegetelem(t->data.tableVal, i);
    if (content)
        avm_assign(lv, content);
    else
        printf("Warning: table key not found\n");
}

void execute_tablesetelem(instruction* instr) {
    avm_memcell* t = avm_translate_operand(&instr->result, &ax);
    avm_memcell* i = avm_translate_operand(&instr->arg1, &bx);
    avm_memcell* c = avm_translate_operand(&instr->arg2, NULL);
    assert(t && i && c);

    if (t->type != table_m) {
        avm_error("Not a table in setelem");
        executionFinished = 1;
        return;
    }

    avm_tablesetelem(t->data.tableVal, i, c);
}

void execute_nop(instruction* instr) {
    // No operation
}

void execute_not(instruction* instr) {
    avm_memcell* arg = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* result = avm_translate_operand(&instr->result, NULL);
    assert(arg && result);

    avm_memcellclear(result);
    result->type = bool_m;
    result->data.boolVal = !avm_tobool(arg);
}

void execute_or(instruction* instr) {
    avm_memcell* arg1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* arg2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* result = avm_translate_operand(&instr->result, NULL);
    assert(arg1 && arg2 && result);

    avm_memcellclear(result);
    result->type = bool_m;
    result->data.boolVal = avm_tobool(arg1) || avm_tobool(arg2);
}

void execute_and(instruction* instr) {
    avm_memcell* arg1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* arg2 = avm_translate_operand(&instr->arg2, &bx);
    avm_memcell* result = avm_translate_operand(&instr->result, NULL);
    assert(arg1 && arg2 && result);

    avm_memcellclear(result);
    result->type = bool_m;
    result->data.boolVal = avm_tobool(arg1) && avm_tobool(arg2);
}

void execute_param(instruction* instr) {
    avm_memcell* arg = avm_translate_operand(&instr->arg1, &ax);
    assert(arg);

    // Handle self-assignment properly: if arg points to stack[top], 
    // we need to copy the value before clearing
    if (&stack[top] == arg) {
        // Self-assignment: the value is already at the correct location
        // Just move the stack pointer and increment totalActuals
        --top;
        ++totalActuals;
    } else {
        // Normal case: copy the parameter to the stack
        avm_memcellclear(&stack[top]);
        avm_assign(&stack[top], arg);
        --top;
        ++totalActuals;
    }
}

void execute_getretval(instruction* instr) {
    if (!instr) {
        fprintf(stderr, "[WARNING] Null instruction in getretval\n");
        return;
    }

    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    if (!lv) {
        fprintf(stderr, "[WARNING] Invalid lvalue in getretval (null result)\n");
        return;
    }

    if (retval.type == undef_m) {
        fprintf(stderr, "[WARNING] Skipping getretval: retval is undefined\n");
        return;
    }

    avm_memcellclear(lv);
    avm_assign(lv, &retval);
}

void execute_return(instruction* instr) {
    avm_memcell* ret_val = avm_translate_operand(&instr->arg1, &ax);
    assert(ret_val);

    avm_memcellclear(&retval);
    avm_assign(&retval, ret_val);
}









