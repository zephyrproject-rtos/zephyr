/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>
/* private kernel APIs */
#include <ksched.h>
#include <wait_q.h>

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
static struct k_obj_type obj_type_mem_slab;

#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB

static int k_mem_slab_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	__ASSERT((obj_core != NULL) && (stats != NULL), "NULL parameter");

	struct k_mem_slab *slab;
	k_spinlock_key_t   key;

	slab = CONTAINER_OF(obj_core, struct k_mem_slab, obj_core);
	key = k_spin_lock(&slab->lock);
	memcpy(stats, &slab->info, sizeof(slab->info));
	k_spin_unlock(&slab->lock, key);

	return 0;
}

static int k_mem_slab_stats_query(struct k_obj_core *obj_core, void *stats)
{
	__ASSERT((obj_core != NULL) && (stats != NULL), "NULL parameter");

	struct k_mem_slab *slab;
	k_spinlock_key_t   key;
	struct sys_memory_stats *ptr = stats;

	slab = CONTAINER_OF(obj_core, struct k_mem_slab, obj_core);
	key = k_spin_lock(&slab->lock);
	ptr->free_bytes = (slab->info.num_blocks - slab->info.num_used) *
			  slab->info.block_size;
	ptr->allocated_bytes = slab->info.num_used * slab->info.block_size;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	ptr->max_allocated_bytes = slab->info.max_used * slab->info.block_size;
#else
	ptr->max_allocated_bytes = 0;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */
	k_spin_unlock(&slab->lock, key);

	return 0;
}

static int k_mem_slab_stats_reset(struct k_obj_core *obj_core)
{
	__ASSERT(obj_core != NULL, "NULL parameter");

	struct k_mem_slab *slab;
	k_spinlock_key_t   key;

	slab = CONTAINER_OF(obj_core, struct k_mem_slab, obj_core);
	key = k_spin_lock(&slab->lock);

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	slab->info.max_used = slab->info.num_used;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

	k_spin_unlock(&slab->lock, key);

	return 0;
}

static struct k_obj_core_stats_desc mem_slab_stats_desc = {
	.raw_size = sizeof(struct k_mem_slab_info),
	.query_size = sizeof(struct sys_memory_stats),
	.raw   = k_mem_slab_stats_raw,
	.query = k_mem_slab_stats_query,
	.reset = k_mem_slab_stats_reset,
	.disable = NULL,
	.enable = NULL,
};
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */

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
	CHECKIF(((slab->info.block_size | (uintptr_t)slab->buffer) &
				(sizeof(void *) - 1)) != 0U) {
		return -EINVAL;
	}

	slab->free_list = NULL;
	p = slab->buffer;

	for (j = 0U; j < slab->info.num_blocks; j++) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p += slab->info.block_size;
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
static int init_mem_slab_obj_core_list(void)
{
	int rc = 0;

	/* Initialize mem_slab object type */

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
	z_obj_type_init(&obj_type_mem_slab, K_OBJ_TYPE_MEM_SLAB_ID,
			offsetof(struct k_mem_slab, obj_core));
#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB
	k_obj_type_stats_init(&obj_type_mem_slab, &mem_slab_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */

	/* Initialize statically defined mem_slabs */

	STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
		rc = create_free_list(slab);
		if (rc < 0) {
			goto out;
		}
		k_object_init(slab);

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
		k_obj_core_init_and_link(K_OBJ_CORE(slab), &obj_type_mem_slab);
#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB
		k_obj_core_stats_register(K_OBJ_CORE(slab), &slab->info,
					  sizeof(struct k_mem_slab_info));
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */
	}

out:
	return rc;
}

SYS_INIT(init_mem_slab_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	int rc;

	slab->info.num_blocks = num_blocks;
	slab->info.block_size = block_size;
	slab->buffer = buffer;
	slab->info.num_used = 0U;
	slab->lock = (struct k_spinlock) {};

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	slab->info.max_used = 0U;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

	rc = create_free_list(slab);
	if (rc < 0) {
		goto out;
	}

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
	k_obj_core_init_and_link(K_OBJ_CORE(slab), &obj_type_mem_slab);
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */
#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB
	k_obj_core_stats_register(K_OBJ_CORE(slab), &slab->info,
				  sizeof(struct k_mem_slab_info));
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */

	z_waitq_init(&slab->wait_q);
	k_object_init(slab);
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
		slab->info.num_used++;

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
		slab->info.max_used = MAX(slab->info.num_used,
					  slab->info.max_used);
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

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

void k_mem_slab_free(struct k_mem_slab *slab, void *mem)
{
	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	__ASSERT(((char *)mem >= slab->buffer) &&
		 ((((char *)mem - slab->buffer) % slab->info.block_size) == 0) &&
		 ((char *)mem <= (slab->buffer + (slab->info.block_size *
						  (slab->info.num_blocks - 1)))),
		 "Invalid memory pointer provided");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mem_slab, free, slab);
	if (slab->free_list == NULL && IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct k_thread *pending_thread = z_unpend_first_thread(&slab->wait_q);

		if (pending_thread != NULL) {
			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, free, slab);

			z_thread_return_value_set_with_data(pending_thread, 0, mem);
			z_ready_thread(pending_thread);
			z_reschedule(&slab->lock, key);
			return;
		}
	}
	*(char **) mem = slab->free_list;
	slab->free_list = (char *) mem;
	slab->info.num_used--;

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, free, slab);

	k_spin_unlock(&slab->lock, key);
}

int k_mem_slab_runtime_stats_get(struct k_mem_slab *slab, struct sys_memory_stats *stats)
{
	if ((slab == NULL) || (stats == NULL)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	stats->allocated_bytes = slab->info.num_used * slab->info.block_size;
	stats->free_bytes = (slab->info.num_blocks - slab->info.num_used) *
			    slab->info.block_size;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	stats->max_allocated_bytes = slab->info.max_used *
				     slab->info.block_size;
#else
	stats->max_allocated_bytes = 0;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

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

	slab->info.max_used = slab->info.num_used;

	k_spin_unlock(&slab->lock, key);

	return 0;
}
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */
