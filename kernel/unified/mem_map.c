/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

#include <kernel.h>
#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <misc/dlist.h>
#include <ksched.h>
#include <init.h>

extern struct k_mem_map _k_mem_map_ptr_start[];
extern struct k_mem_map _k_mem_map_ptr_end[];

/**
 * @brief Initialize kernel memory map subsystem.
 *
 * Perform any initialization of memory maps that wasn't done at build time.
 * Currently this just involves creating the list of free blocks for each map.
 *
 * @return N/A
 */
static void create_free_list(struct k_mem_map *map)
{
	char *p;
	int j;

	map->free_list = NULL;
	p = map->buffer;

	for (j = 0; j < map->num_blocks; j++) {
		*(char **)p = map->free_list;
		map->free_list = p;
		p += map->block_size;
	}
}

/**
 * @brief Complete initialization of statically defined memory maps.
 *
 * Perform any initialization that wasn't done at build time.
 *
 * @return N/A
 */
static int init_mem_map_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_mem_map *map;

	for (map = _k_mem_map_ptr_start; map < _k_mem_map_ptr_end; map++) {
		create_free_list(map);
	}
	return 0;
}

void k_mem_map_init(struct k_mem_map *map, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	map->num_blocks = num_blocks;
	map->block_size = block_size;
	map->buffer = buffer;
	map->num_used = 0;
	create_free_list(map);
	sys_dlist_init(&map->wait_q);
	SYS_TRACING_OBJ_INIT(mem_map, map);
}

int k_mem_map_alloc(struct k_mem_map *map, void **mem, int32_t timeout)
{
	unsigned int key = irq_lock();
	int result;

	if (map->free_list != NULL) {
		/* take a free block */
		*mem = map->free_list;
		map->free_list = *(char **)(map->free_list);
		map->num_used++;
		result = 0;
	} else if (timeout == K_NO_WAIT) {
		/* don't wait for a free block to become available */
		*mem = NULL;
		result = -ENOMEM;
	} else {
		/* wait for a free block or timeout */
		_pend_current_thread(&map->wait_q, timeout);
		result = _Swap(key);
		if (result == 0) {
			*mem = _current->swap_data;
		}
		return result;
	}

	irq_unlock(key);

	return result;
}

void k_mem_map_free(struct k_mem_map *map, void **mem)
{
	int key = irq_lock();
	struct k_thread *pending_thread = _unpend_first_thread(&map->wait_q);

	if (pending_thread) {
		_set_thread_return_value_with_data(pending_thread, 0, *mem);
		_abort_thread_timeout(pending_thread);
		_ready_thread(pending_thread);
		if (_must_switch_threads()) {
			_Swap(key);
			return;
		}
	} else {
		**(char ***)mem = map->free_list;
		map->free_list = *(char **)mem;
		map->num_used--;
	}

	irq_unlock(key);
}

SYS_INIT(init_mem_map_module, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
