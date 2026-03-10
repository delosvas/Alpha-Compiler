#ifndef VM_H
#define VM_H

#include "instruction.h"  // Περιλαμβάνει vmopcode, vmarg, instruction
#include <stdio.h>


// Εκτέλεση bytecode (.abc)
void load_instructions(const char* filename);
void execute_cycle(void);

// Constants tables for VM
extern double* numConsts;
extern unsigned totalNumConsts;
extern char** stringConsts;
extern unsigned totalStringConsts;
extern char** libFuncs;
extern unsigned totalLibFuncs;

#endif // VM_H

