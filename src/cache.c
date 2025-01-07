/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "pipe.h"
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>

block_t* blocks_new(int num_ways) {
    block_t* blocks = malloc(num_ways * sizeof(block_t));
    for (int i = 0; i < num_ways; i++) {
        blocks[i].tag   = 0;
        blocks[i].order = 0;
        blocks[i].v_bit = 0;
    }
    return blocks;
}

set_t* sets_new(int num_sets, int num_ways) {
    set_t* sets = malloc(num_sets * sizeof(set_t));
    for (int i = 0; i < num_sets; i++) {
        sets[i].blocks = blocks_new(num_ways);
    }
    return sets;
}

cache_t *cache_new(int sets, int ways)
{
    cache_t* cache  = malloc(sizeof(cache_t));
    cache->num_sets = sets;
    cache->num_ways = ways;
    cache->sets     = sets_new(sets, ways);
    cache->lru_tracker = 1;
    return cache;
}

void free_sets(set_t* sets, int num_sets, int num_ways) {
    for (int i = 0; i < num_sets; i++) {
        free(sets[i].blocks);
    }
    free(sets);
    return;
}

void cache_destroy(cache_t *c)
{
    free_sets(c->sets, c->num_sets, c->num_ways);
    free(c);
    return;
}

int calc_bits(int num) {
    if (num == 4) {
        return 2;
    } else if (num == 8) {
        return 3;
    } else if (num == 64) {
        return 6;
    } else if (num == 256) {
        return 8;
    }
    printf("Unexpected num in calc_bits\n");
    return -1;
}

uint64_t find_tag(cache_t* c,uint64_t addr) {
    int set_bits   = calc_bits(c->num_sets);
    int block_bits = 5;
    uint64_t tag   = addr >> (set_bits + block_bits);
    return tag;
}

int find_set(cache_t* c, uint64_t addr) {
    int set_bits   = calc_bits(c->num_sets);
    int block_bits = 5;
    uint64_t set   = addr >> block_bits;
    uint64_t mask  = ((uint64_t) -1) >> (64 - set_bits);
    set = set & mask;
    return (int) set;
}

int find_suitable_block(cache_t* c, int set_i) {
    int oldest = 0;
    set_t set = c->sets[set_i];
    for (int i = 0; i < c->num_ways; i++) {
        block_t cur_block  = set.blocks[i];
        uint8_t cur_v_bit  = cur_block.v_bit;
        uint32_t cur_order = cur_block.order;
        if (!cur_v_bit) return i;
        if (cur_order < set.blocks[oldest].order) oldest = i;
    }
    return oldest;
}

void update_vals(cache_t* c, int set_i, int block_i, uint64_t tag) {
    c->sets[set_i].blocks[block_i].tag   = tag;
    c->sets[set_i].blocks[block_i].v_bit = 1;
    c->sets[set_i].blocks[block_i].order = c->lru_tracker;
    c->lru_tracker += 1;
    
    return;
}

void cache_update(cache_t *c, uint64_t addr)
{
    int set_i    = find_set(c, addr);
    uint64_t tag = find_tag(c, addr);
    printf("Updating cache... Set index: %d, Tag: %ld\n", set_i, tag);
    int suitable_block = find_suitable_block(c, set_i);
    update_vals(c, set_i, suitable_block, tag);
    return;
    
}

int check_hit(cache_t* c, int set_i, uint64_t tag) {
    set_t set = c->sets[set_i];
    for (int i = 0; i < c->num_ways; i++) {
        block_t cur_block = set.blocks[i];
        uint8_t cur_v_bit = cur_block.v_bit;
        uint64_t cur_tag  = cur_block.tag;
        if (cur_v_bit) {
            if (cur_tag == tag) {
                c->sets[set_i].blocks[i].order = c->lru_tracker;
                c->lru_tracker += 1;
                return 1;
            }
        }
    }
    return 0;
}

int check_cache_hit(cache_t *c, uint64_t addr) {
    int set_i    = find_set(c, addr);
    uint64_t tag = find_tag(c, addr);
    printf("Checking cache... Set index: %d, Tag: %ld\n", set_i, tag);
    return check_hit(c, set_i, tag);
}
























