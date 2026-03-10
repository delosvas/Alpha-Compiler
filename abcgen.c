#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction.h"
#include "generate.h"
#include "quad.h"
#include "consts.h"
#include "target_codegen.h"

// Externals
extern quad* quads;
extern unsigned total;
extern unsigned currQuad;

extern instruction* instructions;
extern unsigned instr_total;

extern double numConsts[];
extern unsigned totalNumConsts;

extern const char* stringConsts[];
extern unsigned totalStringConsts;

extern const char* libFuncs[];
extern unsigned totalLibFuncs;


void generate_all(void) {
    for (currQuad = 0; currQuad < total; ++currQuad) {
        quad* q = &quads[currQuad];
        switch (q->op) {
            case assign:         generate_ASSIGN(q); break;
            case add:            generate_ADD(q); break;
            case sub:            generate_SUB(q); break;
            case mul:            generate_MUL(q); break;
            case div_:           generate_DIV(q); break;
            case mod:            generate_MOD(q); break;
            case uminus:         generate_UMINUS(q); break;
            case and:            generate_AND(q); break;
            case or_:            generate_OR(q); break;
            case not_:           generate_NOT(q); break;
            case if_eq:          generate_IF_EQ(q); break;
            case if_noteq:       generate_IF_NOTEQ(q); break;
            case if_lesseq:      generate_IF_LESSEQ(q); break;
            case if_greatereq:   generate_IF_GREATEREQ(q); break;
            case if_less:        generate_IF_LESS(q); break;
            case if_greater:     generate_IF_GREATER(q); break;
            case jump_:          generate_JUMP(q); break;
            case call:           generate_CALL(q); break;
            case param:          generate_PARAM(q); break;
            case return_:        generate_RETURN(q); break;
            case getretval:      generate_GETRETVAL(q); break;
            case funcstart:      generate_FUNCSTART(q); break;
            case funcend:        generate_FUNCEND(q); break;
            case tablecreate:    generate_NEWTABLE(q); break;
            case tablegetelem:   generate_TABLEGETELEM(q); break;
            case tablesetelem:   generate_TABLESETELEM(q); break;
            default:
                fprintf(stderr, "❌ Unknown quad op: %d at line %u\n", q->op, q->line);
                exit(1);
        }
        printf("⚙️ Generating for quad %u: opcode %d\n", currQuad, q->op);
    }

    patch_incomplete_jumps();

    //  Add HALT instruction to stop VM execution
    emit_instr(halt_v, NULL, NULL, NULL);
}
void write_abc_file(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Cannot open output .abc file");
        exit(1);
    }

    fwrite("ALVM", sizeof(char), 4, f);
    fwrite(&instr_total, sizeof(unsigned), 1, f);

    for (unsigned i = 0; i < instr_total; ++i) {
        instruction* instr = &instructions[i];
        fwrite(&instr->opcode, sizeof(uint8_t), 1, f);
        fwrite(&instr->result.type, sizeof(uint8_t), 1, f);
        fwrite(&instr->result.val, sizeof(uint32_t), 1, f);
        fwrite(&instr->arg1.type, sizeof(uint8_t), 1, f);
        fwrite(&instr->arg1.val, sizeof(uint32_t), 1, f);
        fwrite(&instr->arg2.type, sizeof(uint8_t), 1, f);
        fwrite(&instr->arg2.val, sizeof(uint32_t), 1, f);
    }

    fwrite(&totalNumConsts, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalNumConsts; ++i)
        fwrite(&numConsts[i], sizeof(double), 1, f);

    fwrite(&totalStringConsts, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        unsigned len = strlen(stringConsts[i]);
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(stringConsts[i], sizeof(char), len, f);
    }

    fwrite(&totalLibFuncs, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        unsigned len = strlen(libFuncs[i]);
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(libFuncs[i], sizeof(char), len, f);
    }

    fclose(f);
    printf(" Wrote %u instructions to %s\n", instr_total, filename);
}
void load_quads_from_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Failed to open quads file");
        exit(1);
    }

    unsigned capacity = 64;
    quads = malloc(capacity * sizeof(quad));
    if (!quads) {
        perror("malloc failed");
        exit(1);
    }

    total = 0;
    while (!feof(f)) {
        if (total >= capacity) {
            capacity *= 2;
            quads = realloc(quads, capacity * sizeof(quad));
            if (!quads) {
                perror("realloc failed");
                exit(1);
            }
        }

        int op, label;
        if (fscanf(f, "%d", &op) != 1) break;

        quad* q = &quads[total];
        memset(q, 0, sizeof(quad));
        q->op = (iopcode)op;

        fscanf(f, "%*s %*s %*s %d", &label);
        q->label = label;

        q->line = total;
        total++;
    }

    fclose(f);
    printf(" Loaded %u quads from %s\n", total, filename);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input_quads.txt output.abc\n", argv[0]);
        return 1;
    }

    const char* input = argv[1];
    const char* output = argv[2];

    // 📥 Φόρτωσε τα quads από το αρχείο
    load_quads_from_file(input);

    // ⚙️ Μετατροπή quads -> AVM instructions
    generate_all();

    // 💾 Αποθήκευση .abc αρχείου
    write_abc_file(output);

    return 0;
}




