FLEX   = flex
BISON  = bison
CC     = gcc

LEXER  = al.l
PARSER = parser.y

COMPILER_EXEC = calc.out
VM_EXEC       = alpha_vm
TEST_VM_EXEC  = test_vm
ABCGEN_EXEC   = abcgen

SRC_FILES = al.c parser.c quad.c SymTable.c \
            consts.c generate.c instruction.c \
            vm.c target_codegen.c abc_loader.c main.c utils.c \
            main_vm.c test_vm.c avm_memcell.c avm_libfuncs.c \
            avm_table.c

COMPILER_SRC = $(filter-out main_vm.c test_vm.c, $(SRC_FILES))
VM_SRC       = $(filter-out main.c test_vm.c consts.c target_codegen.c generate.c, $(SRC_FILES))
TEST_VM_SRC  = vm.c instruction.c test_vm.c avm_memcell.c avm_libfuncs.c avm_table.c consts.c
ABCGEN_SRC = abcgen.c quad_stub.c generate.c instruction.c target_codegen.c consts.c

DEPS = parser.h SymTable.h \
       consts.h generate.h instruction.h \
       vm.h target_codegen.h utils.h abc_loader.h \
       avm_memcell.h avm_libfuncs.h avm_table.h

all: $(COMPILER_EXEC) $(VM_EXEC) $(TEST_VM_EXEC) $(ABCGEN_EXEC)

$(COMPILER_EXEC): $(COMPILER_SRC)
	$(CC) -g -DCALC_OUT_BUILD -o $@ $^ -lm

$(VM_EXEC): $(VM_SRC)
	$(CC) -g -o $@ $^ -lm

$(TEST_VM_EXEC): $(TEST_VM_SRC)
	$(CC) -g -o $@ $^ -lm

$(ABCGEN_EXEC): $(ABCGEN_SRC)
	$(CC) -g -o $@ $^ -Wall

al.c: $(LEXER)
	$(FLEX) --outfile=al.c $(LEXER)

parser.c parser.h: $(PARSER)
	$(BISON) -v --yacc --defines --output=parser.c $(PARSER)

run-compiler:
	@echo "Usage: make run-compiler FILE=filename"
	@exit 1

run-compiler-file:
	./$(COMPILER_EXEC) < $(FILE)

run-vm:
	@echo "Usage: make run-vm FILE=filename"
	@exit 1

run-vm-file:
	./$(VM_EXEC) $(FILE)

run-test-vm:
	./$(TEST_VM_EXEC)

testall:
	@for file in Working/*.asc; do \
		echo "==== Running $$file ===="; \
		./$(COMPILER_EXEC) < "$$file"; \
		echo; \
	done

clean:
	rm -f al.c parser.c parser.h parser.output \
		$(COMPILER_EXEC) $(VM_EXEC) $(TEST_VM_EXEC) $(ABCGEN_EXEC) \
		*.out *.o *.abc

