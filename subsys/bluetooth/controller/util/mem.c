/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <string.h>

#include "defines.h"

#include "mem.h"

void mem_init(void *mem_pool, uint16_t mem_size, uint16_t mem_count,
	      void **mem_head)
{
	*mem_head = mem_pool;

	/* Store free mem_count after the list's next pointer */
	memcpy(((uint8_t *)mem_pool + sizeof(mem_pool)),
		 (uint8_t *)&mem_count, sizeof(mem_count));

	/* Initialize next pointers to form a free list,
	 * next pointer is stored in the first 32-bit of each block
	 */
	memset(((uint8_t *)mem_pool + (mem_size * (--mem_count))), 0,
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
		void *mem;

		/* Get the free count from the list and decrement it */
		memcpy((void *)&free_count,
			 ((uint8_t *)*mem_head + sizeof(mem_head)),
			 sizeof(free_count));
		free_count--;

		mem = *mem_head;
		memcpy(mem_head, mem, sizeof(mem_head));

		/* Store free mem_count after the list's next pointer */
		if (*mem_head) {
			memcpy(((uint8_t *)*mem_head + sizeof(mem_head)),
				 (uint8_t *)&free_count, sizeof(free_count));
		}

		return mem;
	}

	return NULL;
}

void mem_release(void *mem, void **mem_head)
{
	uint16_t free_count = 0;

	/* Get the free count from the list and increment it */
	if (*mem_head) {
		memcpy(&free_count, ((uint8_t *)*mem_head + sizeof(mem_head)),
		       sizeof(free_count));
	}
	free_count++;

	memcpy(mem, mem_head, sizeof(mem));
	*mem_head = mem;

	/* Store free mem_count after the list's next pointer */
	memcpy(((uint8_t *)*mem_head + sizeof(mem_head)),
		 (uint8_t *)&free_count, sizeof(free_count));
}

uint16_t mem_free_count_get(void *mem_head)
{
	uint16_t free_count = 0;

	/* Get the free count from the list */
	if (mem_head) {
		memcpy(&free_count, ((uint8_t *)mem_head + sizeof(mem_head)),
		       sizeof(free_count));
	}

	return free_count;
}

void *mem_get(void *mem_pool, uint16_t mem_size, uint16_t index)
{
	return ((void *)((uint8_t *)mem_pool + (mem_size * index)));
}

uint16_t mem_index_get(void *mem, void *mem_pool, uint16_t mem_size)
{
	return ((uint16_t)((uint8_t *)mem - (uint8_t *)mem_pool) /
		mem_size);
}

void mem_rcopy(uint8_t *dst, uint8_t const *src, uint16_t len)
{
	src += len;
	while (len--) {
		*dst++ = *--src;
	}
}

uint8_t mem_is_zero(uint8_t *src, uint16_t len)
{
	while (len--) {
		if (*src++) {
			return 1;
		}
	}

	return 0;
}

uint32_t mem_ut(void)
{
#define BLOCK_SIZE  ALIGN4(10)
#define BLOCK_COUNT 10
	uint8_t ALIGNED(4) pool[BLOCK_COUNT][BLOCK_SIZE];
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
