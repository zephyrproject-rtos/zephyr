/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEM_H_
#define _MEM_H_

#ifndef MALIGN
#define MALIGN(x) __attribute__((aligned(x)))
#endif

#ifndef MROUND
#define MROUND(x) (((u32_t)(x)+3) & (~((u32_t)3)))
#endif

void mem_init(void *mem_pool, u16_t mem_size, u16_t mem_count, void **mem_head);
void *mem_acquire(void **mem_head);
void mem_release(void *mem, void **mem_head);

u16_t mem_free_count_get(void *mem_head);
void *mem_get(void *mem_pool, u16_t mem_size, u16_t index);
u16_t mem_index_get(void *mem, void *mem_pool, u16_t mem_size);

void mem_rcopy(u8_t *dst, u8_t const *src, u16_t len);
u8_t mem_nz(u8_t *src, u16_t len);

u32_t mem_ut(void);

#endif /* _MEM_H_ */
