#include <stdio.h>
#include <stdint.h>

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

int main(void) {
    Simulator s = {0};
        
    sim_load(&s, "input.s");
    for (int i = 0; i <= 31; i++) {
        // Append a new line to registers in each loop iteration
        s.regs[i] = 0;
    }
    //s.regs[2] = 10;
    //s.regs[3] = 20;
    char registers[600] = {0};
    printf("hi\n");
    sim_run_all(&s);
    //printf("%llx\n", s.regs[1]);

    regs(&s, registers);

    printf("%s\n", registers);

    char* string;
    string = mem(&s, 0x10000, 4, string);
    printf("%s\n", string);
    free(string);
}