#define CACHE_H

#include <stdlib.h>
#include <stdint.h>

typedef struct CacheConfig {
    size_t size, block_size, associativity,
    replacement_policy, writeback_policy;
} CacheConfig;

typedef struct CacheEntry {
    int valid, dirty;
    uint64_t tag, extra;
    uint8_t *data;
} CacheEntry;

typedef struct CacheLine {
    CacheEntry *entries;
} CacheLine; 

typedef struct Cache {
    size_t num_lines, block_size, associativity;
    size_t hits, misses;
    enum {WRITEBACK, WRITETHROUGH} write_policy;
    enum {FIFO, LRU, RANDOM} replacement_policy;

    // Simulator memory
    uint8_t *mem;
    CacheLine *lines;
} Cache;


void cache_init(Cache *c, CacheConfig *cfg);
uint64_t cache_read(Cache *c, uint64_t addr, size_t num_bytes);
void print_cache_stats(Cache *c);