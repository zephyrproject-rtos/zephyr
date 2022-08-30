/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/wait_q.h>
#include <zephyr/sys/dlist.h>
#include <ksched.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>

/**
 * @brief Initialize kernel memory slab subsystem.
 *
 * Perform any initialization of memory slabs that wasn't done at build time.
 * Currently this just involves creating the list of free blocks for each slab.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p slab contains invalid configuration and/or values.
 */
static int create_free_list(struct k_mem_slab *slab)
{
	uint32_t j;
	char *p;

	/* blocks must be word aligned */
	CHECKIF(((slab->block_size | (uintptr_t)slab->buffer) &
				(sizeof(void *) - 1)) != 0U) {
		return -EINVAL;
	}

	slab->free_list = NULL;
	p = slab->buffer;

	for (j = 0U; j < slab->num_blocks; j++) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p += slab->block_size;
	}
	return 0;
}

/**
 * @brief Complete initialization of statically defined memory slabs.
 *
 * Perform any initialization that wasn't done at build time.
 *
 * @return 0 on success, fails otherwise.
 */
static int init_mem_slab_module(const struct device *dev)
{
	int rc = 0;
	ARG_UNUSED(dev);

	STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
		rc = create_free_list(slab);
		if (rc < 0) {
			goto out;
		}
		z_object_init(slab);
	}

out:
	return rc;
}

SYS_INIT(init_mem_slab_module, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	int rc = 0;

	slab->num_blocks = num_blocks;
	slab->block_size = block_size;
	slab->buffer = buffer;
	slab->num_used = 0U;
	slab->lock = (struct k_spinlock) {};

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	slab->max_used = 0U;
#endif

	rc = create_free_list(slab);
	if (rc < 0) {
		goto out;
	}

	z_waitq_init(&slab->wait_q);
	z_object_init(slab);
out:
	SYS_PORT_TRACING_OBJ_INIT(k_mem_slab, slab, rc);

	return rc;
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&slab->lock);
	int result;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mem_slab, alloc, slab, timeout);

	if (slab->free_list != NULL) {
		/* take a free block */
		*mem = slab->free_list;
		slab->free_list = *(char **)(slab->free_list);
		slab->num_used++;

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
		slab->max_used = MAX(slab->num_used, slab->max_used);
#endif

		result = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT) ||
		   !IS_ENABLED(CONFIG_MULTITHREADING)) {
		/* don't wait for a free block to become available */
		*mem = NULL;
		result = -ENOMEM;
	} else {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_mem_slab, alloc, slab, timeout);

		/* wait for a free block or timeout */
		result = z_pend_curr(&slab->lock, key, &slab->wait_q, timeout);
		if (result == 0) {
			*mem = _current->base.swap_data;
		}

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, alloc, slab, timeout, result);

		return result;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, alloc, slab, timeout, result);

	k_spin_unlock(&slab->lock, key);

	return result;
}

void k_mem_slab_free(struct k_mem_slab *slab, void **mem)
{
	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mem_slab, free, slab);
	if (slab->free_list == NULL && IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct k_thread *pending_thread = z_unpend_first_thread(&slab->wait_q);

		if (pending_thread != NULL) {
			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, free, slab);

			z_thread_return_value_set_with_data(pending_thread, 0, *mem);
			z_ready_thread(pending_thread);
			z_reschedule(&slab->lock, key);
			return;
		}
	}
	**(char ***) mem = slab->free_list;
	slab->free_list = *(char **) mem;
	slab->num_used--;

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, free, slab);

	k_spin_unlock(&slab->lock, key);
}

int k_mem_slab_runtime_stats_get(struct k_mem_slab *slab, struct sys_memory_stats *stats)
{
	if ((slab == NULL) || (stats == NULL)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	stats->allocated_bytes = slab->num_used * slab->block_size;
	stats->free_bytes = (slab->num_blocks - slab->num_used) * slab->block_size;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	stats->max_allocated_bytes = slab->max_used * slab->block_size;
#else
	stats->max_allocated_bytes = 0;
#endif

	k_spin_unlock(&slab->lock, key);

	return 0;
}

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
int k_mem_slab_runtime_stats_reset_max(struct k_mem_slab *slab)
{
	if (slab == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	slab->max_used = slab->num_used;

	k_spin_unlock(&slab->lock, key);

	return 0;
}
#endif
