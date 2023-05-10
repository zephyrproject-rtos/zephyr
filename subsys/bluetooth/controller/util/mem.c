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

void mem_init(void *mem_pool, uint16_t mem_size, uint16_t mem_count,
	      void **mem_head)
{
	*mem_head = mem_pool;

	/* Store free mem_count after the list's next pointer at an 32-bit
	 * aligned memory location to ensure atomic read/write (in ARM for now).
	 */
	*((uint16_t *)MROUND((uint8_t *)mem_pool + sizeof(mem_pool))) = mem_count;

	/* Initialize next pointers to form a free list,
	 * next pointer is stored in the first 32-bit of each block
	 */
	(void)memset(((uint8_t *)mem_pool + (mem_size * (--mem_count))), 0,
		     sizeof(mem_pool));
	while (mem_count--) {
		uint32_t next;

		next = (uint32_t)((uint8_t *) mem_pool +
			       (mem_size * (mem_count + 1)));
		memcpy(((uint8_t *)mem_pool + (mem_size * mem_count)),
		       (void *)&next, sizeof(next));
	}
}

void *mem_acquire(void **mem_head)
{
	if (*mem_head) {
		uint16_t free_count;
		void *head;
		void *mem;

		/* Get the free count from the list and decrement it */
		free_count = *((uint16_t *)MROUND((uint8_t *)*mem_head +
					       sizeof(mem_head)));
		free_count--;

		mem = *mem_head;
		memcpy(&head, mem, sizeof(head));

		/* Store free mem_count after the list's next pointer */
		if (head) {
			*((uint16_t *)MROUND((uint8_t *)head + sizeof(head))) =
				free_count;
		}

		*mem_head = head;
		return mem;
	}

	return NULL;
}

void mem_release(void *mem, void **mem_head)
{
	uint16_t free_count = 0U;

	/* Get the free count from the list and increment it */
	if (*mem_head) {
		free_count = *((uint16_t *)MROUND((uint8_t *)*mem_head +
					       sizeof(mem_head)));
	}
	free_count++;

	memcpy(mem, mem_head, sizeof(mem));

	/* Store free mem_count after the list's next pointer */
	*((uint16_t *)MROUND((uint8_t *)mem + sizeof(mem))) = free_count;

	*mem_head = mem;
}

uint16_t mem_free_count_get(void *mem_head)
{
	uint16_t free_count = 0U;

	/* Get the free count from the list */
	if (mem_head) {
		free_count = *((uint16_t *)MROUND((uint8_t *)mem_head +
					       sizeof(mem_head)));
	}

	return free_count;
}

void *mem_get(const void *mem_pool, uint16_t mem_size, uint16_t index)
{
	return ((void *)((uint8_t *)mem_pool + (mem_size * index)));
}

uint16_t mem_index_get(const void *mem, const void *mem_pool, uint16_t mem_size)
{
	return ((uint8_t *)mem - (uint8_t *)mem_pool) / mem_size;
}

/**
 * @brief  Copy bytes in reverse
 * @details Example: [ 0x11 0x22 0x33 ] -> [ 0x33 0x22 0x11 ]
 */
void mem_rcopy(uint8_t *dst, uint8_t const *src, uint16_t len)
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
uint8_t mem_nz(uint8_t *src, uint16_t len)
{
	while (len--) {
		if (*src++) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief XOR bytes
 */
inline void mem_xor_n(uint8_t *dst, uint8_t *src1, uint8_t *src2, uint16_t len)
{
	while (len--) {
		*dst++ = *src1++ ^ *src2++;
	}
}

/**
 * @brief XOR 32-bits
 */
void mem_xor_32(uint8_t *dst, uint8_t *src1, uint8_t *src2)
{
	mem_xor_n(dst, src1, src2, 4U);
}

/**
 * @brief Unit test
 */
uint32_t mem_ut(void)
{
#define BLOCK_SIZE  MROUND(10)
#define BLOCK_COUNT 10
	uint8_t MALIGN(4) pool[BLOCK_COUNT][BLOCK_SIZE];
	void *mem_free;
	void *mem_used;
	uint16_t mem_free_count;
	void *mem;

	mem_init(pool, BLOCK_SIZE, BLOCK_COUNT, &mem_free);

	mem_free_count = mem_free_count_get(mem_free);
	if (mem_free_count != BLOCK_COUNT) {
		return 1;
	}

	mem_used = 0;
	while (mem_free_count--) {
		uint16_t mem_free_count_current;

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
		uint16_t mem_free_count_current;

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
