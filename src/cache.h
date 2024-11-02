#define CACHE_H

#include <stdlib.h>
#include <stdint.h>

// Config struct for cache
typedef struct CacheConfig {
    size_t size, block_size, associativity,
    replacement_policy, writeback_policy;
} CacheConfig;

typedef struct CacheEntry {
    int valid, dirty;
    uint64_t tag;
    union { uint64_t insert_time, access_time; }; // Used for replacement
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
    size_t hits, misses;
    size_t monotime; // Monotonic counter used to simulate time
    enum {WRITEBACK, WRITETHROUGH} write_policy;
    enum {FIFO, LRU, RANDOM} replacement_policy;

    // Simulator memory
    uint8_t *mem;
    CacheLine *lines;
} Cache;


void cache_init(Cache *c, CacheConfig *cfg);
uint64_t cache_read(Cache *c, uint64_t addr, size_t num_bytes);
void print_cache_stats(Cache *c);