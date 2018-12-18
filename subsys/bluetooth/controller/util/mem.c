/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>

#include "util.h"

#include "mem.h"

void mem_init(void *mem_pool, u16_t mem_size, u16_t mem_count,
	      void **mem_head)
{
	*mem_head = mem_pool;

	/* Store free mem_count after the list's next pointer at an 32-bit
	 * aligned memory location to ensure atomic read/write (in ARM for now).
	 */
	*((u16_t *)MROUND((u8_t *)mem_pool + sizeof(mem_pool))) = mem_count;

	/* Initialize next pointers to form a free list,
	 * next pointer is stored in the first 32-bit of each block
	 */
	(void)memset(((u8_t *)mem_pool + (mem_size * (--mem_count))), 0,
		     sizeof(mem_pool));
	while (mem_count--) {
		u32_t next;

		next = (u32_t)((u8_t *) mem_pool +
			       (mem_size * (mem_count + 1)));
		memcpy(((u8_t *)mem_pool + (mem_size * mem_count)),
		       (void *)&next, sizeof(next));
	}
}

void *mem_acquire(void **mem_head)
{
	if (*mem_head) {
		u16_t free_count;
		void *head;
		void *mem;

		/* Get the free count from the list and decrement it */
		free_count = *((u16_t *)MROUND((u8_t *)*mem_head +
					       sizeof(mem_head)));
		free_count--;

		mem = *mem_head;
		memcpy(&head, mem, sizeof(head));

		/* Store free mem_count after the list's next pointer */
		if (head) {
			*((u16_t *)MROUND((u8_t *)head + sizeof(head))) =
				free_count;
		}

		*mem_head = head;
		return mem;
	}

	return NULL;
}

void mem_release(void *mem, void **mem_head)
{
	u16_t free_count = 0U;

	/* Get the free count from the list and increment it */
	if (*mem_head) {
		free_count = *((u16_t *)MROUND((u8_t *)*mem_head +
					       sizeof(mem_head)));
	}
	free_count++;

	memcpy(mem, mem_head, sizeof(mem));

	/* Store free mem_count after the list's next pointer */
	*((u16_t *)MROUND((u8_t *)mem + sizeof(mem))) = free_count;

	*mem_head = mem;
}

u16_t mem_free_count_get(void *mem_head)
{
	u16_t free_count = 0U;

	/* Get the free count from the list */
	if (mem_head) {
		free_count = *((u16_t *)MROUND((u8_t *)mem_head +
					       sizeof(mem_head)));
	}

	return free_count;
}

void *mem_get(void *mem_pool, u16_t mem_size, u16_t index)
{
	return ((void *)((u8_t *)mem_pool + (mem_size * index)));
}

u16_t mem_index_get(void *mem, void *mem_pool, u16_t mem_size)
{
	return ((u16_t)((u8_t *)mem - (u8_t *)mem_pool) / mem_size);
}

/**
 * @brief  Copy bytes in reverse
 * @details Example: [ 0x11 0x22 0x33 ] -> [ 0x33 0x22 0x11 ]
 */
void mem_rcopy(u8_t *dst, u8_t const *src, u16_t len)
{
	src += len;
	while (len--) {
		*dst++ = *--src;
	}
}

/**
 * @brief Determine if src[0..len-1] contains one or more non-zero bytes
 * @return 0 if all bytes are zero; otherwise 1
 */
u8_t mem_nz(u8_t *src, u16_t len)
{
	while (len--) {
		if (*src++) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Unit test
 */
u32_t mem_ut(void)
{
#define BLOCK_SIZE  MROUND(10)
#define BLOCK_COUNT 10
	u8_t MALIGN(4) pool[BLOCK_COUNT][BLOCK_SIZE];
	void *mem_free;
	void *mem_used;
	u16_t mem_free_count;
	void *mem;

	mem_init(pool, BLOCK_SIZE, BLOCK_COUNT, &mem_free);

	mem_free_count = mem_free_count_get(mem_free);
	if (mem_free_count != BLOCK_COUNT) {
		return 1;
	}

	mem_used = 0;
	while (mem_free_count--) {
		u16_t mem_free_count_current;

		mem = mem_acquire(&mem_free);
		mem_free_count_current = mem_free_count_get(mem_free);
		if (mem_free_count != mem_free_count_current) {
			return 2;
		}

		memcpy(mem, &mem_used, sizeof(mem));
		mem_used = mem;
	}

	mem = mem_acquire(&mem_free);
	if (mem) {
		return 3;
	}

	while (++mem_free_count < BLOCK_COUNT) {
		u16_t mem_free_count_current;

		mem = mem_used;
		memcpy(&mem_used, mem, sizeof(void *));
		mem_release(mem, &mem_free);

		mem_free_count_current = mem_free_count_get(mem_free);
		if ((mem_free_count + 1) != mem_free_count_current) {
			return 4;
		}
	}

	if (mem != mem_free) {
		return 5;
	}

	if (mem_free_count_get(mem_free) != BLOCK_COUNT) {
		return 6;
	}

	return 0;
}
