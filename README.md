# RISC-V Simulator

A simulator for the RISC-V instruction set architecture, developed as part of CS2323.

# Usage

The project includes a Makefile which can be used to build the project and
test it. To build, run:
```
$ make
```
This produces an executable called `riscv_asm` in the project directory.
The simulator can be run using the following command:
```
$ ./riscv_asm
```

# Project File Structure

```
+-- Makefile
+-- report
| \-- main.tex // Source file for the report
+-- src
| +-- asm // Source code for the assembler
| | +-- emitter.c
| | +-- emitter.h
| | +-- lexer.c
| | +-- lexer.h
| | +-- parser.c
| | +-- parser.h
| | +-- tables.c
| | \-- tables.h
| +-- main.c
| +-- simulator.c
| \-- simulator.h
\-- test // Testcases
```