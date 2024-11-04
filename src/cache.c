#ifndef CACHE_H
#include "cache.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_cache_config(CacheConfig *cfg, char *filename) {
    FILE *f = fopen(filename, "r");
    fscanf(f, "%lu %lu %lu", &cfg->size, &cfg->block_size, &cfg->associativity);
    char replacement_policy[20] = {0}, writeback_policy[20] = {0};
    fscanf(f, "%s %s", replacement_policy, writeback_policy);
    if (strcmp(replacement_policy, "RANDOM") == 0) {
        cfg->replacement_policy = RANDOM;
    } else if (strcmp(replacement_policy, "LRU") == 0) {
        cfg->replacement_policy = LRU;
    } else if (strcmp(replacement_policy, "FIFO") == 0) {
        cfg->replacement_policy = FIFO;
    }
    if (strcmp(writeback_policy, "WT") == 0) {
        cfg->writeback_policy = WRITEBACK;
    } else if (strcmp(writeback_policy, "WB") == 0) {
        cfg->writeback_policy = WRITETHROUGH;
    }
    fclose(f);
}

// Initializes the cache
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

// Selects a block to be replaced from the given line
CacheEntry *cache_evict(Cache *c, CacheLine *line) {
    // If any entries are invalid, replace them
    for (int i = 0; i < c->associativity; i++) {
        if (!line->entries[i].valid) {
            return &line->entries[i];
        }
    }

    CacheEntry *entry = &line->entries[0];

    // Replace a random entry
    if (c->replacement_policy == RANDOM) {
        // TODO: Implement actual random
        entry = &line->entries[0];
    }

    // Replace first inserted (least insert-time) entry
    if (c->replacement_policy == FIFO) {    
        for (int i = 0; i < c->associativity; i++) {
            if (line->entries[i].insert_time < entry->insert_time) {
                entry = &line->entries[i];
            }
        }
    }

    // Replace least-recently used (least access-time) entry 
    if (c->replacement_policy == LRU) {    
        for (int i = 0; i < c->associativity; i++) {
            if (line->entries[i].access_time < entry->access_time) {
                entry = &line->entries[i];
            }
        }
    }

    // If entry is dirty, write back to memory
    if (entry->dirty) {
        uint64_t index = line - c->lines;
        uint64_t block_start = (entry->tag * c->num_lines * c->block_size) + (index * c->block_size);
        memcpy(&c->mem[block_start], entry->data, c->block_size);
    }

    return entry;
}

// Reads a certain number of bytes
uint64_t cache_read(Cache *c, uint64_t addr, size_t num_bytes) {
    uint64_t offset = addr % c->block_size,
             index = (addr / c->block_size) % c->num_lines,
             tag = addr / (c->block_size * c->num_lines);

    // If multiple-block access is needed then return directly
    // from memory
    if ((offset + num_bytes) > c->num_lines) {
        printf("WARN: Multi-block access; Directly accessing memory.\n");
        uint64_t result = 0;
        for (int i = 0; i < num_bytes; i++) {
            result = (result << 8) + c->mem[addr + i];
        }
        return result;
    }

    CacheLine *line = &c->lines[index];
    CacheEntry *entry;
    int hit = 0;

    // Check if any of the entries in the line match
    for (int i = 0; i < c->associativity; i++) {
        if ((line->entries[i].tag == tag) && (line->entries[i].valid)) {
            entry = &line->entries[i];
            hit = 1;
            c->hits++;
            break;
        }        
    }

    if (!hit) {
        c->misses++;
        
        // Load into cache
        entry = &line->entries[0];
        uint64_t block_start = addr - (addr % (c->block_size * c->num_lines));
        memcpy(entry->data, &c->mem[block_start], c->block_size * sizeof(uint8_t));
        entry->tag = tag;
        entry->valid = 1;

        // Set insert time
        if (c->replacement_policy == FIFO) {
            entry->insert_time = c->monotime++;
        }
    }

    // Prints log
    // TODO: Writes to a file
    printf("R: Address: 0x%lX, Set: 0x%lX, %s, Tag: 0x%lX, %s\n",
        addr, index, hit? "Hit": "Miss", tag, entry->dirty? "Dirty": "Clean");

    // Set access time
    if (c->replacement_policy == LRU) {
        entry->access_time = c->monotime++;
    }

    // Read the required bytes
    uint8_t *start = &entry->data[offset];
    uint64_t result = 0;
    for (int i = 0; i < num_bytes; i++) {
        result = (result << 4) + *(start++);        
    }

    return result;
}

void cache_write(Cache *c, uint64_t addr, uint64_t value, size_t num_bytes) { 
    uint64_t offset = addr % c->block_size,
             index = (addr / c->block_size) % c->num_lines,
             tag = addr / (c->block_size * c->num_lines);

    // If multiple-block access is needed then return directly
    // from memory
    if ((offset + num_bytes) > c->num_lines) {
        // between different blocks
        printf("WARN: Multi-block access; directly accessing memory.\n");
        for (int i = 0; i < num_bytes; i++) {
            c->mem[addr + i] = value % 0xff;
            value >>= 8;
        }
        return;
    }

    CacheLine *line = &c->lines[index];
    CacheEntry *entry;
    int hit = 0;

    // Check if any of the entries in the line match
    for (int i = 0; i < c->associativity; i++) {
        if ((line->entries[i].tag == tag) && (line->entries[i].valid)) {
            entry = &line->entries[i];
            hit = 1;
            c->hits++;
            break;
        }        
    }

    if (!hit) {
        c->misses++;
        
        // If writethrough, then assume no-allocate
        if (c->write_policy == WRITETHROUGH) {
            for (int i = 0; i < num_bytes; i++) {
                c->mem[addr + i] = value % 0xff;
                value >>= 8;
            }
            return; 
        }

        // Load into cache
        entry = &line->entries[0];
        uint64_t block_start = addr - (addr % (c->block_size * c->num_lines));
        memcpy(entry->data, &c->mem[block_start], c->block_size * sizeof(uint8_t));
        entry->tag = tag;
        entry->valid = 1;

        // Set insert time
        if (c->replacement_policy == FIFO) {
            entry->insert_time = c->monotime++;
        }
    }
    
    // Prints log
    // TODO: Writes to a file
    printf("W: Address: 0x%lX, Set: 0x%lX, %s, Tag: 0x%lX, %s\n",
        addr, index, hit? "Hit": "Miss", tag, entry->dirty? "Dirty": "Clean");

    if (c->replacement_policy == LRU) {
        entry->access_time = c->monotime++;
    }

    // In the case of write-through, write to memory
    // and invalidate entry
    if (c->write_policy == WRITETHROUGH) {
        for (int i = 0; i < num_bytes; i++) {
            c->mem[addr + i] = value % 0xff;
            value >>= 8;
        }
        entry->valid = 0;
    // In the case of write-back, write to cache
    } else if (c->write_policy == WRITEBACK) {
        for (int i = 0; i < num_bytes; i++) {
            entry->data[offset + i] = value % 0xff;
            value >>= 8;
        }
    }

    return;
}

// Prints cache statistics
void print_cache_stats(Cache *c) {
    size_t accesses = c->hits + c->misses;
    double hit_rate = (double)c->hits / accesses;
    printf("D-Cache statistics: Accesses=%lu, Hit=%lu, Miss=%lu, Hit Rate=%lf\n", accesses, c->hits, c->misses, hit_rate);
    return;
}