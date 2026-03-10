#include "SymTable.h"
#include <stdio.h>
#include "instruction.h"
#include "consts.h"
#include <string.h>

// External declarations for constants from consts.c
extern double numConsts[];
extern unsigned totalNumConsts;
extern const char* stringConsts[];
extern unsigned totalStringConsts;
extern const char* libFuncs[];
extern unsigned totalLibFuncs;

void insert_LIBFUNCTS() {
    symtable_insert_direct(0, 0, LIBFUNC, "print");
    symtable_insert_direct(0, 0, LIBFUNC, "input");
    symtable_insert_direct(0, 0, LIBFUNC, "objectmemberkeys");
    symtable_insert_direct(0, 0, LIBFUNC, "objecttotalmembers");
    symtable_insert_direct(0, 0, LIBFUNC, "objectcopy");
    symtable_insert_direct(0, 0, LIBFUNC, "totalarguments");
    symtable_insert_direct(0, 0, LIBFUNC, "argument");
    symtable_insert_direct(0, 0, LIBFUNC, "typeof");
    symtable_insert_direct(0, 0, LIBFUNC, "strtonum");
    symtable_insert_direct(0, 0, LIBFUNC, "sqrt");
    symtable_insert_direct(0, 0, LIBFUNC, "cos");
    symtable_insert_direct(0, 0, LIBFUNC, "sin");
}

void write_binary_file(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return;
    }

    // Write magic header
    fwrite("ALVM", sizeof(char), 4, f);
    // Write instruction count
    fwrite(&instr_curr, sizeof(unsigned), 1, f);
    
    // Write instructions field-by-field to match abc_loader.c reading format
    for (unsigned i = 0; i < instr_curr; ++i) {
        instruction* instr = &instructions[i];
        fwrite(&instr->opcode, sizeof(uint8_t), 1, f);
        
        fwrite(&instr->result.type, sizeof(uint8_t), 1, f);
        fwrite(&instr->result.val, sizeof(uint32_t), 1, f);
        
        fwrite(&instr->arg1.type, sizeof(uint8_t), 1, f);
        fwrite(&instr->arg1.val, sizeof(uint32_t), 1, f);
        
        fwrite(&instr->arg2.type, sizeof(uint8_t), 1, f);
        fwrite(&instr->arg2.val, sizeof(uint32_t), 1, f);
    }

    // Write constants tables
    // 1. Number constants
    fwrite(&totalNumConsts, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalNumConsts; ++i) {
        fwrite(&numConsts[i], sizeof(double), 1, f);
    }
    
    // 2. String constants
    fwrite(&totalStringConsts, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        unsigned len = strlen(stringConsts[i]) + 1; // +1 for null terminator
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(stringConsts[i], sizeof(char), len, f);
    }
    
    // 3. Library function names
    fwrite(&totalLibFuncs, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        unsigned len = strlen(libFuncs[i]) + 1; // +1 for null terminator
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(libFuncs[i], sizeof(char), len, f);
    }

    fclose(f);
    printf("[output] Binary file \"%s\" written successfully with constants.\n", filename);
}

void write_binary_to_stdout(void) {
    // Write magic header
    fwrite("ALVM", sizeof(char), 4, stdout);
    // Write instruction count
    fwrite(&instr_curr, sizeof(unsigned), 1, stdout);
    
    // Write instructions
    for (unsigned i = 0; i < instr_curr; ++i) {
        instruction* instr = &instructions[i];
        fwrite(&instr->opcode, sizeof(uint8_t), 1, stdout);
        
        fwrite(&instr->result.type, sizeof(uint8_t), 1, stdout);
        fwrite(&instr->result.val, sizeof(uint32_t), 1, stdout);
        
        fwrite(&instr->arg1.type, sizeof(uint8_t), 1, stdout);
        fwrite(&instr->arg1.val, sizeof(uint32_t), 1, stdout);
        
        fwrite(&instr->arg2.type, sizeof(uint8_t), 1, stdout);
        fwrite(&instr->arg2.val, sizeof(uint32_t), 1, stdout);
    }
    
    // Write constants tables
    // 1. Number constants
    fwrite(&totalNumConsts, sizeof(unsigned), 1, stdout);
    for (unsigned i = 0; i < totalNumConsts; ++i) {
        fwrite(&numConsts[i], sizeof(double), 1, stdout);
    }
    
    // 2. String constants
    fwrite(&totalStringConsts, sizeof(unsigned), 1, stdout);
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        unsigned len = strlen(stringConsts[i]) + 1;
        fwrite(&len, sizeof(unsigned), 1, stdout);
        fwrite(stringConsts[i], sizeof(char), len, stdout);
    }
    
    // 3. Library function names
    fwrite(&totalLibFuncs, sizeof(unsigned), 1, stdout);
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        unsigned len = strlen(libFuncs[i]) + 1;
        fwrite(&len, sizeof(unsigned), 1, stdout);
        fwrite(libFuncs[i], sizeof(char), len, stdout);
    }
    
    fflush(stdout);
}

