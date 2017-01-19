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
#define MROUND(x) (((uint32_t)(x)+3) & (~((uint32_t)3)))
#endif

void mem_init(void *mem_pool, uint16_t mem_size, uint16_t mem_count,
	      void **mem_head);
void *mem_acquire(void **mem_head);
void mem_release(void *mem, void **mem_head);

uint16_t mem_free_count_get(void *mem_head);
void *mem_get(void *mem_pool, uint16_t mem_size, uint16_t index);
uint16_t mem_index_get(void *mem, void *mem_pool, uint16_t mem_size);

void mem_rcopy(uint8_t *dst, uint8_t const *src, uint16_t len);
uint8_t mem_is_zero(uint8_t *src, uint16_t len);

uint32_t mem_ut(void);

#endif /* _MEM_H_ */
