#include <stdio.h>
#include <stdint.h>

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

int main(void) {
    Simulator s = {0};
        
    sim_load(&s, "input.s");

    s.regs[2] = 10;
    s.regs[3] = 20;
    
    sim_run_all(&s);
    printf("%lu\n", s.regs[1]);
}