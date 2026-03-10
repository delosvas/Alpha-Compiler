#ifndef AVM_H
#define AVM_H

#include <stdio.h>
#include <stdlib.h>

#define AVM_STACKSIZE 4096
#define AVM_WIPEOUT 0xABCDEF

#include "avm_memcell.h"
#include "instruction.h"  // Για vmarg

// Library function type definition
typedef void (*library_func_t)(void);

extern avm_memcell stack[AVM_STACKSIZE];
extern unsigned int pc;
extern unsigned int top;
extern unsigned int topsp;
extern unsigned int instr_curr;
extern unsigned int instr_total;
extern int executionFinished;
extern avm_memcell retval;
extern int executionFinished;

// Function declarations
void execute_halt(instruction* instr);
avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg);
void avm_assign(avm_memcell* lv, avm_memcell* rv);
void avm_callsaveenvironment(void);
void avm_calllibfunc(const char* id);
void avm_error(const char* format, ...);
void avm_warning(const char* format, ...);
void avm_memcellclear(avm_memcell* m);
void avm_initstack(void);

// Library function registry
void avm_registerlibfunc(const char* id, library_func_t func);
library_func_t avm_getlibfunc(const char* id);
void avm_initialize(void);

#endif // AVM_H



