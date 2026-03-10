#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "SymTable.h"
#include "quad.h"
#include "instruction.h"
#include "target_codegen.h"

extern FILE* yyin;
int yyparse(void);



extern void write_binary_file(const char* filename);
extern void write_binary_to_stdout(void);
extern void init_instructions(void);
extern void generate(void);
extern void load_instructions(const char* filename);
extern void load_instructions_from_file(const char* filename);
extern void execute_cycle(void);

int suppress_debug_output = 0;  // Global flag to suppress debug output

#define DEBUG_PRINTF(...) do { \
    if (!suppress_debug_output) { \
        printf(__VA_ARGS__); \
    } \
} while(0)
int main(int argc, char** argv) {
    int suppress_text = (argc < 3 && (argc == 1 || (argc == 2 && argv[1] && !strstr(argv[1], ".abc"))));
    suppress_debug_output = suppress_text;

    SymHash = symtable_init();
    insert_LIBFUNCTS();

    if (argc > 1 && strstr(argv[1], ".abc")) {
        load_instructions_from_file(argv[1]);
        execute_cycle();
        return 0;
    }

    init_quads();

    const char* output_file = "prog.abc";
    if (argc > 2) output_file = argv[2];

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "[main] Cannot read file: %s\n", argv[1]);
            return 1;
        }
    } else {
        yyin = stdin;
    }

    if (yyparse() != 0 || currQuad == 0 || quads == NULL) {
        fprintf(stderr, "[main] Parsing failed or no quads generated.\n");
        return 1;
    }

    if (!suppress_text) {
        print_quads();
    }

    init_instructions();
    generate();

    if (suppress_text) {
        write_binary_to_stdout();
    } else {
        write_binary_file(output_file);
    }

    return 0;
}

