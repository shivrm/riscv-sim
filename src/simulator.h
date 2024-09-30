#define SIMULATOR_H

#ifndef EMITTER_H
#include "asm/emitter.h"
#endif

#define MEM_SIZE 0x20000

typedef struct Simulator {
    uint64_t pc, regs[32];    
    uint8_t mem[MEM_SIZE];
    char *src; 
    ParseNode *nodes;
    LabelVec *labels;
} Simulator;

int sim_load(Simulator *s, char *file);
void sim_run(Simulator *s);
void sim_run_all(Simulator *s);
void regs(Simulator *s, char* registers);
char* mem(Simulator *s, int address, int count, char* string);