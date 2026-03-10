# Wanky-Compiler

A custom-built, "wanky" compiler that translates a high-level programming language into instructions for a custom Virtual Machine (VM). 

This project was built from scratch and demonstrates the full compilation pipeline, including lexical analysis, syntax parsing, semantic analysis, intermediate code generation (quads), and target code generation for the custom VM.

## Architecture

The compilation process is separated into multiple distinct phases:

1. **Lexical Analysis (Scanner)**: Flex (`.l`) is used to tokenize the source code.
2. **Syntax Analysis (Parser)**: Bison (`.y`) is used to define the grammar and build the syntax tree.
3. **Semantic Analysis**: Builds symbol tables, checks scopes, handles function definitions/calls, and catches semantic errors.
4. **Intermediate Code Generation**: Translates the syntax tree into intermediate representation (IR) using Quads.
5. **Target Code Generation**: Translates the IR Quads into target instructions for the custom Virtual Machine.
6. **Virtual Machine (VM)**: A custom-built VM that loads and executes the generated target instructions. It includes a memory stack, environment registers, and built-in library functions.

## Features

- Custom symbol table management with scope resolution.
- Support for variables, arrays (tables), functions, and closures.
- Control flow constructs (if, while, for).
- Intermediate code generation (Quads) for optimization and debugging.
- A fully functional Virtual Machine capable of executing the compiled target code.
- Extensive memory management and runtime error checking within the VM.

## Building and Running

Ensure you have `flex`, `bison`, and a C compiler (like `gcc`) installed.

1. **Build**: Run `make` in the root directory. This will generate the parser, scanner, and compile all components into the final compiler/VM executables.
2. **Compile**: `./compiler <source_file>` to generate the target code.
3. **Execute**: Run the generated target code using the custom VM executable.

*(Note: The exact build commands and executable names may depend on the specific `Makefile` configuration.)*

## Quirks and "Wankiness"

As a custom-built, experimental compiler, it has its fair share of quirks! Some language features might behave unexpectedly, and error messages can sometimes be a bit cryptic. It's a "wanky" but functional demonstration of compiler design principles.
