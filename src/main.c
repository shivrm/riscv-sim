#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef CACHE_H
#include "cache.h"
#endif

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

int main(void) {
    srand(time(NULL));

    Simulator s;
    while (1) {
        char input[100] = "\0";
        scanf("%s", input);
        if (strcmp(input, "cache_sim") == 0) {
            scanf("%s", input);  // Read the next word (enable/disable/status/invalidate/dump/stats)
            if (strcmp(input, "enable") == 0) {
                printf("\n");
                char config_file[100] = "\0";
                scanf("%s", config_file);
                
                s.cache_enabled = 1;
                load_cache_config(&s.cache_cfg, config_file);
                sim_init(&s);
                print_cache_config(s.cache);
                printf("%d\n", s.cache->num_lines);
            } 
            else if (strcmp(input, "disable") == 0) {
                s.cache_enabled = 0;
                printf("Cache simulator disabled.\n");
            } 
            else if (strcmp(input, "status") == 0) {
                printf("\n");
                printf("%s", s.cache_enabled ? "Cache enabled.\n" : "Cache disabled.\n");
                if (s.cache_enabled) {
                    print_cache_config(s.cache);
                    printf("%d\n", s.cache->num_lines);
                }
            }
            else if (strcmp(input, "invalidate") == 0){
                cache_invalidate(s.cache);
            }
            else if (strcmp(input, "dump") == 0){ // only take care of the valid entries
                printf("\n");
                char dump_file[100] = "\0";
                scanf("%s", dump_file);
                cache_dump(s.cache, dump_file);
            }
            else if (strcmp(input, "stats") == 0){
                print_cache_stats(s.cache);
            }
        }

        else if (strcmp(input, "load") == 0) {
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

    fclose(s.cache->output_file);
    return 0;
}