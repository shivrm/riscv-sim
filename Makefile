
CFLAGS= -O2
CFILES=src/asm/lexer.c src/asm/parser.c src/asm/emitter.c src/cache.c src/simulator.c src/main.c
OUT=./riscv_sim
CC=clang

riscv_sim: ${CFILES}
	$(CC) ${CFLAGS} ${CFILES} -o ${OUT}

#PHONY: riscv_sim