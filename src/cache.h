/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

typedef struct
{
    uint8_t  v_bit;
    uint32_t order;
    uint64_t tag;
} block_t;

typedef struct 
{
    block_t* blocks;
} set_t;

typedef struct
{
    int lru_tracker;
    int num_sets;
    int num_ways;
    set_t* sets;
} cache_t;

cache_t *cache_new(int sets, int ways);
void cache_destroy(cache_t *c);
int check_cache_hit(cache_t *c, uint64_t addr);
void cache_update(cache_t *c, uint64_t addr);

#endif
