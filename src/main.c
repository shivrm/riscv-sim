#include <stdio.h>
#include <stdint.h>

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

int main(void) {
    Simulator s = {0};
    
    // add x1, x2, x3
    ((uint32_t*)s.mem)[0] = 0x003100b3;

    s.regs[2] = 10;
    s.regs[3] = 20;


    sim_run(&s);
    printf("%lu\n", s.regs[1]);
}