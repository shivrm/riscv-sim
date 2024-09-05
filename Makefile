CFLAGS= -O2
CFILES=src/lexer.c src/parser.c src/emitter.c src/main.c
OUT=./riscv_asm

riscv_asm: ${CFILES}
	cc ${CFLAGS} ${CFILES} -o ${OUT}

test: riscv_asm
	./test.sh
