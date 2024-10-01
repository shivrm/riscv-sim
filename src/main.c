#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

int main(void) {
    Simulator s;
    sim_init(&s);

    while (1) {
        char input[100] = "\0";
        scanf("%s", input);

        if (strcmp(input, "load") == 0) {
            char filename[100] = "\0";
            scanf("%s", filename);
            sim_init(&s);
            sim_load(&s, filename); 
        } else if (strcmp(input, "run") == 0) {
            sim_run(&s);
        } else if (strcmp(input, "regs") == 0) {
            sim_regs(&s);
        } else if (strcmp(input, "mem") == 0) {
            int start, count;
            scanf(" %x %d", &start, &count);
            sim_mem(&s, start, count);
        } else if (strcmp(input, "step") == 0) {
            sim_step(&s);
        } else if (strcmp(input, "break") == 0) {
            int line;
            scanf(" %d", &line);
            sim_add_breakpoint(&s, line);
        } else if (strcmp(input, "del") == 0) {
            scanf("%s", input);;
            if (strcmp(input, "break") != 0) continue;
            
            int line;
            scanf(" %d", &line);
            sim_remove_breakpoint(&s, line);  
        } else if (strcmp(input, "show-stack") == 0) {
            sim_show_stack(&s);
        } else if (strcmp(input, "exit") == 0) {
            printf("Exited the simulator\n");
            break;
        }
        printf("\n");
    }

    return 0;
}