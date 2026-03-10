#include <stdio.h>
#include <stdlib.h>
#include "instruction.h"
#include "vm.h"
#include "abc_loader.h"
#include "avm.h"

// Constants tables for VM
double* numConsts = NULL;
unsigned totalNumConsts = 0;
char** stringConsts = NULL;
unsigned totalStringConsts = 0;
char** libFuncs = NULL;
unsigned totalLibFuncs = 0;

// External VM state variables
extern unsigned totalActuals;

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <prog.abc>\n", argv[0]);
        return 1;
    }

    avm_initstack();                     // Initialize stack
    avm_initialize();                    // Initialize and register library functions
    
    top = AVM_STACKSIZE - 1;             // Stack starts from the top
    topsp = top;                         // Frame pointer starts at top too
    pc = 0;                             // Program counter starts at 0
    executionFinished = 0;
    totalActuals = 0;                   // Reset function call arguments
    
    load_instructions_from_file(argv[1]);

    instr_curr = 0;                      // Start execution from first instruction

    while (!executionFinished) {
        execute_cycle();
    }

    return 0;
}

