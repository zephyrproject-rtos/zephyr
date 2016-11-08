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
#include <kernel_structs.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <misc/dlist.h>
#include <ksched.h>
#include <init.h>

extern struct k_mem_slab _k_mem_slab_list_start[];
extern struct k_mem_slab _k_mem_slab_list_end[];

struct k_mem_slab *_trace_list_k_mem_slab;

/**
 * @brief Initialize kernel memory slab subsystem.
 *
 * Perform any initialization of memory slabs that wasn't done at build time.
 * Currently this just involves creating the list of free blocks for each slab.
 *
 * @return N/A
 */
static void create_free_list(struct k_mem_slab *slab)
{
	char *p;
	int j;

	slab->free_list = NULL;
	p = slab->buffer;

	for (j = 0; j < slab->num_blocks; j++) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p += slab->block_size;
	}
}

/**
 * @brief Complete initialization of statically defined memory slabs.
 *
 * Perform any initialization that wasn't done at build time.
 *
 * @return N/A
 */
static int init_mem_slab_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_mem_slab *slab;

	for (slab = _k_mem_slab_list_start;
	     slab < _k_mem_slab_list_end;
	     slab++) {
		create_free_list(slab);
		SYS_TRACING_OBJ_INIT(k_mem_slab, slab);
	}
	return 0;
}

SYS_INIT(init_mem_slab_module, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

void k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	slab->num_blocks = num_blocks;
	slab->block_size = block_size;
	slab->buffer = buffer;
	slab->num_used = 0;
	create_free_list(slab);
	sys_dlist_init(&slab->wait_q);
	SYS_TRACING_OBJ_INIT(k_mem_slab, slab);
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, int32_t timeout)
{
	unsigned int key = irq_lock();
	int result;

	if (slab->free_list != NULL) {
		/* take a free block */
		*mem = slab->free_list;
		slab->free_list = *(char **)(slab->free_list);
		slab->num_used++;
		result = 0;
	} else if (timeout == K_NO_WAIT) {
		/* don't wait for a free block to become available */
		*mem = NULL;
		result = -ENOMEM;
	} else {
		/* wait for a free block or timeout */
		_pend_current_thread(&slab->wait_q, timeout);
		result = _Swap(key);
		if (result == 0) {
			*mem = _current->base.swap_data;
		}
		return result;
	}

	irq_unlock(key);

	return result;
}

void k_mem_slab_free(struct k_mem_slab *slab, void **mem)
{
	int key = irq_lock();
	struct k_thread *pending_thread = _unpend_first_thread(&slab->wait_q);

	if (pending_thread) {
		_set_thread_return_value_with_data(pending_thread, 0, *mem);
		_abort_thread_timeout(pending_thread);
		_ready_thread(pending_thread);
		if (_must_switch_threads()) {
			_Swap(key);
			return;
		}
	} else {
		**(char ***)mem = slab->free_list;
		slab->free_list = *(char **)mem;
		slab->num_used--;
	}

	irq_unlock(key);
}
