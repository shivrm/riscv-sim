CFLAGS= -O2
CFILES=src/asm/lexer.c src/asm/parser.c src/asm/emitter.c src/simulator.c src/main.c
OUT=./riscv_asm
CC=clang

riscv_asm: ${CFILES}
	$(CC) ${CFLAGS} ${CFILES} -o ${OUT}

test: riscv_asm
	./test.sh
