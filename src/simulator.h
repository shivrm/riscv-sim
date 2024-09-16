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

void sim_run(Simulator *s);