#include <stdio.h>
#include <stdlib.h>
#include "instruction.h"
#include "vm.h"

// Declare global variables as external
extern instruction* instructions;
extern unsigned instr_total;
extern unsigned instr_curr;

// Function to create a test instruction
instruction create_test_instruction(vmopcode op, vmarg_t res_type, unsigned res_val,
                                  vmarg_t arg1_type, unsigned arg1_val,
                                  vmarg_t arg2_type, unsigned arg2_val) {
    instruction instr;
    instr.opcode = op;
    instr.result.type = res_type;
    instr.result.val = res_val;
    instr.arg1.type = arg1_type;
    instr.arg1.val = arg1_val;
    instr.arg2.type = arg2_type;
    instr.arg2.val = arg2_val;
    instr.srcLine = 0;
    return instr;
}

int main() {
    // Initialize instructions array
    instructions = (instruction*)malloc(10 * sizeof(instruction));
    if (!instructions) {
        printf("Error: Failed to allocate memory for instructions\n");
        return 1;
    }

    // Create some test instructions
    // 1. Assign a number to a local variable
    instructions[instr_total++] = create_test_instruction(
        assign_v, local_a, 0,    // result: local[0]
        number_a, 42,            // arg1: number 42
        nil_a, 0                 // arg2: not used
    );

    // 2. Add two numbers
    instructions[instr_total++] = create_test_instruction(
        add_v, local_a, 1,       // result: local[1]
        number_a, 10,            // arg1: number 10
        number_a, 20             // arg2: number 20
    );

    // 3. Call a function
    instructions[instr_total++] = create_test_instruction(
        call_v, libfunc_a, 0,    // result: libfunc[0]
        nil_a, 0,                // arg1: not used
        nil_a, 0                 // arg2: not used
    );

    // 4. Jump instruction
    instructions[instr_total++] = create_test_instruction(
        jump_v, label_a, 6,      // result: label 6
        nil_a, 0,                // arg1: not used
        nil_a, 0                 // arg2: not used
    );

    // 5. NOP instruction
    instructions[instr_total++] = create_test_instruction(
        nop_v, nil_a, 0,         // result: not used
        nil_a, 0,                // arg1: not used
        nil_a, 0                 // arg2: not used
    );

    printf("Starting VM execution with %d instructions...\n", instr_total);
    
    // Execute instructions one by one
    while (instr_curr < instr_total) {
        execute_cycle();
    }

    // Cleanup
    free(instructions);
    return 0;
} 
