/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       sh_mem.c
 *
 * COMPONENT
 *
 *         IPC Stack for uAMP systems.
 *
 * DESCRIPTION
 *
 *       Header file for fixed buffer size memory management service. Currently
 *       it is being used to manage shared memory.
 *
 **************************************************************************/
#ifndef SH_MEM_H_
#define SH_MEM_H_

#include <metal/mutex.h>

#if defined __cplusplus
extern "C" {
#endif

/* Macros */
#define BITMAP_WORD_SIZE         (sizeof(unsigned long) << 3)
#define WORD_SIZE                sizeof(unsigned long)
#define WORD_ALIGN(a)            (((a) & (WORD_SIZE-1)) != 0)? \
                                 (((a) & (~(WORD_SIZE-1))) + sizeof(unsigned long)):(a)
#define SH_MEM_POOL_LOCATE_BITMAP(pool,idx) ((unsigned char *) pool \
                                             + sizeof(struct sh_mem_pool) \
                                             + (BITMAP_WORD_SIZE * idx))

/*
 * This structure represents a  shared memory pool.
 *
 * @start_addr      - start address of shared memory region
 * @lock            - lock to ensure exclusive access
 * @size            - size of shared memory*
 * @buff_size       - size of each buffer
 * @total_buffs     - total number of buffers in shared memory region
 * @used_buffs      - number of used buffers
 * @bmp_size        - size of bitmap array
 *
 */

struct sh_mem_pool {
	void *start_addr;
	metal_mutex_t lock;
	unsigned int size;
	unsigned int buff_size;
	unsigned int total_buffs;
	unsigned int used_buffs;
	unsigned int bmp_size;
};

/* APIs */
struct sh_mem_pool *sh_mem_create_pool(void *start_addr, unsigned int size,
				       unsigned int buff_size);
void sh_mem_delete_pool(struct sh_mem_pool *pool);
void *sh_mem_get_buffer(struct sh_mem_pool *pool);
void sh_mem_free_buffer(void *ptr, struct sh_mem_pool *pool);
int get_first_zero_bit(unsigned long value);

#if defined __cplusplus
}
#endif

#endif				/* SH_MEM_H_ */
