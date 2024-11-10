 #define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Config struct for cache
typedef struct CacheConfig {
    size_t size, block_size, associativity,
    writeback_policy, replacement_policy;
} CacheConfig;

typedef struct CacheEntry {
    int valid, dirty;
    uint64_t tag;
    union { uint64_t insert_time, access_time; }; // Used for replacement (FIFO, LRU policies)
    uint8_t *data;
} CacheEntry;

typedef struct CacheLine {
    CacheEntry *entries;
} CacheLine; 

// Cache struct
// A cache has many lines
// Each line might have multiple entries (depends on associativity)
// Each entry holds consecutive bytes of memory (block)
typedef struct Cache {
    size_t num_lines, block_size, associativity;
    size_t hits, misses, writebacks;
    size_t monotime; // Monotonic counter used to simulate time
    enum {WRITEBACK, WRITETHROUGH} write_policy;
    enum {FIFO, LRU, RANDOM} replacement_policy;

    // Simulator memory
    uint8_t *mem;
    CacheLine *lines;
    
    FILE *output_file;
} Cache;

void load_cache_config(CacheConfig *cfg, char *filename);
void cache_init(Cache *c, CacheConfig *cfg);

uint64_t cache_read(Cache *c, uint64_t addr, size_t num_bytes);
void cache_write(Cache *c, uint64_t addr, uint64_t value, size_t num_bytes);

void print_cache_config(Cache *c);
void cache_invalidate(Cache *c);
void print_cache_stats(Cache *c);
void cache_dump(Cache *c, char *filename);