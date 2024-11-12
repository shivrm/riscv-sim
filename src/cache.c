#ifndef CACHE_H
#include "cache.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_cache_config(CacheConfig *cfg, char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL){
        printf("Unable to open the file. \n");
        return;
    }
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
    if (strcmp(writeback_policy, "WB") == 0) {
        cfg->writeback_policy = WRITEBACK;
    } else if (strcmp(writeback_policy, "WT") == 0) {
        cfg->writeback_policy = WRITETHROUGH;
    }
    fclose(f);
}

// Initializes the cache (all the lines, blocks inside lines)
void cache_init(Cache *c, CacheConfig *cfg) {
    // Associativity 0 means fully associative 
    if (cfg->associativity == 0) {
        c->associativity = cfg->size / cfg->block_size;
        c->num_lines = 1;
    } else {
        c->associativity = cfg->associativity;
        c->num_lines = cfg->size / (cfg->block_size * cfg->associativity);
    }
    c->block_size = cfg->block_size;


    c->replacement_policy = cfg->replacement_policy;
    c->write_policy = cfg->writeback_policy;

    c->lines = malloc(c->num_lines * sizeof(CacheLine));

    for (int i = 0; i < c->num_lines; i++) { // go through each line, i.e, each set (a set corresponds to an index)
        c->lines[i].entries = malloc(c->associativity * sizeof(CacheEntry));
        for (int j = 0; j < c->associativity; j++) { // go through each block in the set
            c->lines[i].entries[j].valid = 0;
            c->lines[i].entries[j].data = malloc(c->block_size * sizeof(uint8_t));
        }
    }
}

// Selects a block to be replaced from the given line
CacheEntry *cache_evict(Cache *c, CacheLine *line) {
    // If any entries are invalid, replace them
    for (int i = 0; i < c->associativity; i++) {
        if (!line->entries[i].valid) { // each block in a set has its own valid bit. 
            return &line->entries[i];
        }
    }

    CacheEntry *entry = &line->entries[0];

    // Replace a random entry
    if (c->replacement_policy == RANDOM) {
        entry = &line->entries[rand() % c->associativity];
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
        c->writebacks += 1;
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
             tag = addr / (c->block_size * c->num_lines); // find the index, offset and tag of the address

    // If multiple-block access is needed then return directly
    // from memory
    if ((offset + num_bytes) > c->block_size) {
        printf("WARN: Multi-block access; Directly accessing memory.\n");
        uint64_t result = 0;
        for (int i = 0; i < num_bytes; i++) {
            result = (result << 8) + c->mem[addr + num_bytes - i - 1];
        }
        return result;
    }

    CacheLine *line = &c->lines[index]; // Find the right index (right row/set) to look further into
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
        entry = cache_evict(c, line);
        uint64_t block_start = addr - (addr % (c->block_size));
        memcpy(entry->data, &c->mem[block_start], c->block_size * sizeof(uint8_t));
        entry->tag = tag;
        entry->dirty = 0;
        entry->valid = 1;

        // Set insert time
        if (c->replacement_policy == FIFO) {
            entry->insert_time = c->monotime++;
        }
    }

    // Prints log
    // TODO: Writes to a file
    fprintf(c->output_file, "R: Address: 0x%lX, Set: 0x%lX, %s, Tag: 0x%lX, %s\n",
        addr, index, hit? "Hit": "Miss", tag, entry->dirty? "Dirty": "Clean");

    // Set access time
    if (c->replacement_policy == LRU) {
        entry->access_time = c->monotime++;
    }

    // Read the required bytes from cache
    uint8_t *start = &entry->data[offset]; // read the first byte
    uint64_t result = 0;
    for (int i = 0; i < num_bytes; i++) {
        result = (result << 8) + start[num_bytes - i - 1];        
    }

    return result;
}

void cache_write(Cache *c, uint64_t addr, uint64_t value, size_t num_bytes) { 
    uint64_t offset = addr % c->block_size,
             index = (addr / c->block_size) % c->num_lines,
             tag = addr / (c->block_size * c->num_lines);

    // If multiple-block access is needed then return directly
    // from memory
    if ((offset + num_bytes) > c->block_size) {
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
    
        // If writethrough, then assume no-allocate; and write directly to memory
        if (c->write_policy == WRITETHROUGH) {
            c->writebacks += 1;
            for (int i = 0; i < num_bytes; i++) {
                c->mem[addr + i] = value % 0xff; // taking the remainder when divided by 255
                value >>= 8;
            }
    
            fprintf(c->output_file, "W: Address: 0x%lX, Set: 0x%lX, %s, Tag: 0x%lX, %s\n",
                addr, index, "Miss", tag, "Clean");
            return; 
        }

        // Load into cache
        entry = cache_evict(c, line); // evict some block in the line to make room for the new block we're writing to
        uint64_t block_start = addr - (addr % (c->block_size));
        memcpy(entry->data, &c->mem[block_start], c->block_size * sizeof(uint8_t));
        entry->tag = tag;
        entry->dirty = 0;
        entry->valid = 1;

        // Set insert time
        if (c->replacement_policy == FIFO) {
            entry->insert_time = c->monotime++;
        }
    }
    
    // Prints log
    fprintf(c->output_file, "W: Address: 0x%lX, Set: 0x%lX, %s, Tag: 0x%lX, %s\n",
        addr, index, hit? "Hit": "Miss", tag, entry->dirty? "Dirty": "Clean");

    if (c->replacement_policy == LRU) {
        entry->access_time = c->monotime++;
    } // update the access time of that particular entry

    // In the case of write-through, write to memory and cache
    if (c->write_policy == WRITETHROUGH) {
        c->writebacks += 1;
        for (int i = 0; i < num_bytes; i++) {
            c->mem[addr + i] = value % 0xff;
            value >>= 8;
        }
    } else {
        entry->dirty = 1;
    }
    for (int i = 0; i < num_bytes; i++) {
        entry->data[offset + i] = value % 0xff;
        value >>= 8;
    }


    return;
}

void cache_invalidate(Cache *c) {
    for (int i = 0; i < c->num_lines; i++) {
        for (int j = 0; j < c->associativity; j++) {
            CacheEntry *entry = &c->lines[i].entries[j];
            
            // Dump if entry is dirty
            if (entry->dirty) {
                c->writebacks += 1;
                uint64_t block_start = (entry->tag * c->num_lines * c->block_size) + (i * c->block_size);
                memcpy(&c->mem[block_start], entry->data, c->block_size);
            }

            c->lines[i].entries[j].valid = 0;
        }
    }
}

void cache_dump(Cache *c, char *filename) {
    FILE *f = fopen(filename, "w");
    for (int i = 0; i < c->num_lines; i++) {
        for (int j = 0; j < c->associativity; j++) {
            CacheEntry entry = c->lines[i].entries[j];
            if (entry.valid)
                fprintf(f, "Set: 0x%X, Tag: 0x%lx, %s\n", i, entry.tag, entry.dirty? "Dirty": "Clean");
        }
    }
    fclose(f);
}

void print_cache_config(Cache *c) {
    printf("Cache Size: %zu\n", c->block_size * c->num_lines * c->associativity);
    printf("Block Size: %zu\n", c->block_size);
    printf("Associativity: %zu\n", c->associativity);
    
    char *rp;
    switch (c->replacement_policy) {
        case RANDOM: rp = "RANDOM"; break;
        case LRU: rp = "LRU"; break;
        case FIFO: rp = "FIFO"; break;
    }
    printf("Replacement Policy: %s\n", rp);

    char *wp;
    switch (c->write_policy) {
        case WRITEBACK: wp = "WRITEBACK"; break;
        case WRITETHROUGH: wp = "WRITETHROUGH"; break;
    }
    printf("Write Back Policy: %s\n", wp);
}

// Prints cache statistics
void print_cache_stats(Cache *c) {
    size_t accesses = c->hits + c->misses;
    double hit_rate = (double)c->hits / accesses;
    printf("D-Cache statistics: Accesses=%lu, Hit=%lu, Miss=%lu, Hit Rate=%.2lf\n", accesses, c->hits, c->misses, hit_rate);
    return;
}