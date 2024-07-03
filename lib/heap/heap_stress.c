/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

struct z_heap_stress_result {
    uint32_t total_allocs;
    uint32_t successful_allocs;
    uint32_t total_frees;
    size_t accumulated_in_use_bytes;
};

static uint32_t rand32(void)
{
    static uint64_t state = 123456789; /* seed */
    state = state * 2862933555777941757UL + 3037000493UL;
    return (uint32_t)(state >> 32);
}

static bool rand_alloc_choice(struct z_heap_stress_rec *sr)
{
    if (sr->blocks_alloced == 0) {
        return true;
    } else if (sr->blocks_alloced >= sr->nblocks) {
        return false;
    }

    /* Ensure total_bytes fits within uint32_t range */
    assert(sr->total_bytes < 0xffffffffU / 100);

    uint32_t full_pct = (100 * sr->bytes_alloced) / sr->total_bytes;
    uint32_t target = sr->target_percent ? sr->target_percent : 1;
    uint32_t free_chance = 0xffffffffU;

    if (full_pct < sr->target_percent) {
        free_chance = full_pct * (0x80000000U / target);
    }

    return rand32() > free_chance;
}

static size_t rand_alloc_size(struct z_heap_stress_rec *sr)
{
    int scale = 4 + __builtin_clz(rand32());
    return rand32() & ((1U << scale) - 1);
}

static size_t rand_free_choice(struct z_heap_stress_rec *sr)
{
    return rand32() % sr->blocks_alloced;
}

void sys_heap_stress(void *(*alloc_fn)(void *arg, size_t bytes),
                     void (*free_fn)(void *arg, void *p),
                     void *arg, size_t total_bytes,
                     uint32_t op_count,
                     void *scratch_mem, size_t scratch_bytes,
                     int target_percent,
                     struct z_heap_stress_result *result)
{
    assert(alloc_fn != NULL && free_fn != NULL);

    struct z_heap_stress_rec sr = {
        .alloc_fn = alloc_fn,
        .free_fn = free_fn,
        .arg = arg,
        .total_bytes = total_bytes,
        .blocks = scratch_mem,
        .nblocks = scratch_bytes / sizeof(struct z_heap_stress_block),
        .target_percent = target_percent,
    };

    *result = (struct z_heap_stress_result) {0};

    for (uint32_t i = 0; i < op_count; i++) {
        if (rand_alloc_choice(&sr)) {
            size_t sz = rand_alloc_size(&sr);
            void *p = sr.alloc_fn(sr.arg, sz);

            result->total_allocs++;
            if (p != NULL) {
                result->successful_allocs++;
                assert(sr.blocks_alloced < sr.nblocks);
                sr.blocks[sr.blocks_alloced].ptr = p;
                sr.blocks[sr.blocks_alloced].sz = sz;
                sr.blocks_alloced++;
                sr.bytes_alloced += sz;
            } else {
                /* Handle allocation failure */
                i--; /* Retry allocation */
            }
        } else {
            size_t b = rand_free_choice(&sr);
            assert(b < sr.blocks_alloced);
            void *p = sr.blocks[b].ptr;
            size_t sz = sr.blocks[b].sz;

            result->total_frees++;
            sr.blocks[b] = sr.blocks[sr.blocks_alloced - 1];
            sr.blocks_alloced--;
            sr.bytes_alloced -= sz;
            sr.free_fn(sr.arg, p);
        }
        result->accumulated_in_use_bytes += sr.bytes_alloced;
    }
}


