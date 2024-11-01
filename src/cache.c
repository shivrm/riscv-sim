#ifndef CACHE_H
#include "cache.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cache_init(Cache *c, CacheConfig *cfg) {
    c->num_lines = cfg->size / (cfg->block_size * cfg->associativity);
    c->block_size = cfg->block_size;
    c->associativity = cfg->associativity;

    c->lines = malloc(c->num_lines * sizeof(CacheLine));

    for (int i = 0; i < c->num_lines; i++) {
        c->lines[i].entries = malloc(c->associativity * sizeof(CacheEntry));
        for (int j = 0; j < c->associativity; j++) {
            c->lines[i].entries[j].valid = 0;
            c->lines[i].entries[j].data = malloc(c->block_size * sizeof(uint8_t));
        }
    }
}

uint64_t cache_read(Cache *c, uint64_t addr, size_t num_bytes) {
    uint64_t offset = addr % c->block_size,
             index = (addr / c->block_size) % c->num_lines,
             tag = addr / (c->block_size * c->num_lines);

    if ((offset + num_bytes) > c->num_lines) {
        // between different blocks
    }

    CacheEntry *entry = &c->lines[index].entries[0];
    int hit = 0;

    if ((entry->tag != tag) || (!entry->valid)) {
        // load into cache
        c->misses++;
        uint64_t block_start = addr - (addr % (c->block_size * c->num_lines));
        memcpy(entry->data, &c->mem[block_start], c->block_size * sizeof(uint8_t));
        entry->tag = tag;
        entry->valid = 1;
    } else {
        c->hits++;
        hit = 1;
    }

    printf("R: Address: 0x%lX, Set: 0x%lX, %s, Tag: 0x%lX, %s\n",
        addr, index, hit? "Hit": "Miss", tag, entry->dirty? "Dirty": "Clean");
    
    uint8_t *start = &entry->data[offset];
    uint64_t result = 0;
    for (int i = 0; i < num_bytes; i++) {
        result = (result << 4) + *(start++);        
    }

    return result;
}

void print_cache_stats(Cache *c) {
    size_t accesses = c->hits + c->misses;
    double hit_rate = (double)c->hits / accesses;
    printf("D-Cache statistics: Accesses=%lu, Hit=%lu, Miss=%lu, Hit Rate=%lf\n", accesses, c->hits, c->misses, hit_rate);
    return;
}