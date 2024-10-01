#define SIMULATOR_H

#ifndef EMITTER_H
#include "asm/emitter.h"
#endif

#define MEM_SIZE 0x20000

typedef struct BreakPointVec {
    size_t len, cap;
    int *data;
} BreakPointVec;

typedef struct Simulator {
    uint64_t pc, regs[32];    
    uint8_t mem[MEM_SIZE];
    char *src; 
    ParseNode *nodes;
    LabelVec *labels;
    BreakPointVec *breaks;
} Simulator;

void sim_init(Simulator *s);
int sim_load(Simulator *s, char *file);
void sim_run_one(Simulator *s);
void sim_step(Simulator *s);
void sim_run(Simulator *s);
void sim_regs(Simulator *s);
void sim_mem(Simulator *s, int start, int count);
void sim_add_breakpoint(Simulator *s, int line);
void sim_remove_breakpoint(Simulator *s, int line);