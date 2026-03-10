#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instruction.h"
#include "vm.h"

extern instruction* instructions;
extern unsigned instr_curr;
extern unsigned instr_total;

// External references to VM constants tables
extern double* numConsts;
extern unsigned totalNumConsts;
extern char** stringConsts;
extern unsigned totalStringConsts;
extern char** libFuncs;
extern unsigned totalLibFuncs;

typedef struct {
    const char* name;
    vmopcode opcode;
} opcode_entry;

static opcode_entry opcode_table[] = {
    {"assign", assign_v},
    {"add", add_v},
    {"sub", sub_v},
    {"mul", mul_v},
    {"div", div_v},
    {"mod", mod_v},
    {"jeq", jeq_v},
    {"jne", jne_v},
    {"jle", jle_v},
    {"jge", jge_v},
    {"jlt", jlt_v},
    {"jgt", jgt_v},
    {"jump", jump_v},
    {"call", call_v},
    {"pusharg", pusharg_v},
    {"funcenter", funcenter_v},
    {"funcexit", funcexit_v},
    {"newtable", newtable_v},
    {"tablegetelem", tablegetelem_v},
    {"tablesetelem", tablesetelem_v},
    {"nop", nop_v},
    {"not", not_v},
    {"or", or_v},
    {"and", and_v},
    {"param", param_v},
    {"getretval", getretval_v},
    {"return", return_v},
    {"halt", halt_v}, 
    {NULL, -1}
};

vmopcode get_opcode_from_string(const char* str) {
    for (int i = 0; opcode_table[i].name; ++i) {
        if (strcmp(str, opcode_table[i].name) == 0)
            return opcode_table[i].opcode;
    }
    return -1;
}
void load_instructions_from_file(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("Error opening .abc file");
        exit(1);
    }

    // (Optional) magic number check
    char magic[4];
    if (fread(magic, sizeof(char), 4, f) != 4 || memcmp(magic, "ALVM", 4) != 0) {
        fprintf(stderr, "❌ Invalid or corrupt .abc file (missing magic header)\n");
        fclose(f);
        exit(1);
    }

    if (fread(&instr_total, sizeof(unsigned), 1, f) != 1) {
        fprintf(stderr, "Failed to read instruction count\n");
        fclose(f);
        exit(1);
    }

    if (instr_total == 0 || instr_total > 1000000) {
        fprintf(stderr, "❌ Suspicious instruction count: %u\n", instr_total);
        fclose(f);
        exit(1);
    }

    instructions = calloc(instr_total, sizeof(instruction));
    if (!instructions) {
        fprintf(stderr, "Failed to allocate memory for instructions\n");
        fclose(f);
        exit(1);
    }

    for (instr_curr = 0; instr_curr < instr_total; ++instr_curr) {
        instruction* instr = &instructions[instr_curr];
        fread(&instr->opcode, sizeof(uint8_t), 1, f);
        fread(&instr->result.type, sizeof(uint8_t), 1, f);
        fread(&instr->result.val, sizeof(uint32_t), 1, f);
        fread(&instr->arg1.type, sizeof(uint8_t), 1, f);
        fread(&instr->arg1.val, sizeof(uint32_t), 1, f);
        fread(&instr->arg2.type, sizeof(uint8_t), 1, f);
        fread(&instr->arg2.val, sizeof(uint32_t), 1, f);
    }

    // Number constants
    fread(&totalNumConsts, sizeof(unsigned), 1, f);
    if (totalNumConsts > 0) {
        numConsts = malloc(totalNumConsts * sizeof(double));
        if (!numConsts) {
            fprintf(stderr, "Failed to allocate memory for number constants\n");
            fclose(f);
            exit(1);
        }
        for (unsigned i = 0; i < totalNumConsts; ++i) {
            fread(&numConsts[i], sizeof(double), 1, f);
        }
    }

    // String constants
    fread(&totalStringConsts, sizeof(unsigned), 1, f);
    if (totalStringConsts > 0) {
        stringConsts = malloc(totalStringConsts * sizeof(char*));
        if (!stringConsts) {
            fprintf(stderr, "Failed to allocate memory for string constants\n");
            fclose(f);
            exit(1);
        }
        for (unsigned i = 0; i < totalStringConsts; ++i) {
            unsigned len;
            fread(&len, sizeof(unsigned), 1, f);
            stringConsts[i] = malloc((len + 1) * sizeof(char));
            if (!stringConsts[i]) {
                fprintf(stderr, "Failed to allocate memory for string %u\n", i);
                fclose(f);
                exit(1);
            }
            fread(stringConsts[i], sizeof(char), len, f);
            stringConsts[i][len] = '\0'; // null-terminate
        }
    }

    // Library functions
    fread(&totalLibFuncs, sizeof(unsigned), 1, f);
    if (totalLibFuncs > 0) {
        libFuncs = malloc(totalLibFuncs * sizeof(char*));
        if (!libFuncs) {
            fprintf(stderr, "Failed to allocate memory for library functions\n");
            fclose(f);
            exit(1);
        }
        for (unsigned i = 0; i < totalLibFuncs; ++i) {
            unsigned len;
            fread(&len, sizeof(unsigned), 1, f);
            libFuncs[i] = malloc((len + 1) * sizeof(char));
            if (!libFuncs[i]) {
                fprintf(stderr, "Failed to allocate memory for library function name %u\n", i);
                fclose(f);
                exit(1);
            }
            fread(libFuncs[i], sizeof(char), len, f);
            libFuncs[i][len] = '\0'; // null-terminate
        }
    }

    fclose(f);
    printf(" Loaded %u instructions from %s\n", instr_total, filename);
    printf(" Loaded %u number constants, %u string constants, %u library functions\n",
           totalNumConsts, totalStringConsts, totalLibFuncs);
}


