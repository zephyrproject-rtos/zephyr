/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <spinlock.h>
#include <sys/libc-hooks.h>

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

static struct k_spinlock lock;
static uint8_t max_partitions;

struct k_mem_domain k_mem_domain_default;

#if __ASSERT_ON
static bool check_add_partition(struct k_mem_domain *domain,
				struct k_mem_partition *part)
{

	int i;
	uintptr_t pstart, pend, dstart, dend;

	if (part == NULL) {
		LOG_ERR("NULL k_mem_partition provided");
		return false;
	}

#ifdef CONFIG_EXECUTE_XOR_WRITE
	/* Arches where execution cannot be disabled should always return
	 * false to this check
	 */
	if (K_MEM_PARTITION_IS_EXECUTABLE(part->attr) &&
	    K_MEM_PARTITION_IS_WRITABLE(part->attr)) {
		LOG_ERR("partition is writable and executable <start %lx>",
			part->start);
		return false;
	}
#endif

	if (part->size == 0) {
		LOG_ERR("zero sized partition at %p with base 0x%lx",
			part, part->start);
		return false;
	}

	pstart = part->start;
	pend = part->start + part->size;

	if (pend <= pstart) {
		LOG_ERR("invalid partition %p, wraparound detected. base 0x%lx size %zu",
			part, part->start, part->size);
		return false;
	}

	/* Check that this partition doesn't overlap any existing ones already
	 * in the domain
	 */
	for (i = 0; i < domain->num_partitions; i++) {
		struct k_mem_partition *dpart = &domain->partitions[i];

		if (dpart->size == 0) {
			/* Unused slot */
			continue;
		}

		dstart = dpart->start;
		dend = dstart + dpart->size;

		if (pend > dstart && dend > pstart) {
			LOG_ERR("partition %p base %lx (size %zu) overlaps existing base %lx (size %zu)",
				part, part->start, part->size,
				dpart->start, dpart->size);
			return false;
		}
	}

	return true;
}
#endif

void k_mem_domain_init(struct k_mem_domain *domain, uint8_t num_parts,
		       struct k_mem_partition *parts[])
{
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT(num_parts == 0U || parts != NULL,
		 "parts array is NULL and num_parts is nonzero");
	__ASSERT(num_parts <= max_partitions,
		 "num_parts of %d exceeds maximum allowable partitions (%d)",
		 num_parts, max_partitions);

	key = k_spin_lock(&lock);

	domain->num_partitions = 0U;
	(void)memset(domain->partitions, 0, sizeof(domain->partitions));

	if (num_parts != 0U) {
		uint32_t i;

		for (i = 0U; i < num_parts; i++) {
			__ASSERT(check_add_partition(domain, parts[i]),
				 "invalid partition index %d (%p)",
				 i, parts[i]);

			domain->partitions[i] = *parts[i];
			domain->num_partitions++;
		}
	}

	sys_dlist_init(&domain->mem_domain_q);

#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
	int ret = arch_mem_domain_init(domain);

	/* TODO propagate return values, see #24609.
	 *
	 * Not using an assertion here as this is a memory allocation error
	 */
	if (ret != 0) {
		LOG_ERR("architecture-specific initialization failed for domain %p with %d",
			domain, ret);
		k_panic();
	}
#endif

	k_spin_unlock(&lock, key);
}

void k_mem_domain_destroy(struct k_mem_domain *domain)
{
	k_spinlock_key_t key;
	sys_dnode_t *node, *next_node;

	__ASSERT_NO_MSG(domain != NULL);

	key = k_spin_lock(&lock);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	arch_mem_domain_destroy(domain);
#endif

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		sys_dlist_remove(&thread->mem_domain_info.mem_domain_q_node);
		thread->mem_domain_info.mem_domain = NULL;
	}

	k_spin_unlock(&lock, key);
}

void k_mem_domain_add_partition(struct k_mem_domain *domain,
				struct k_mem_partition *part)
{
	int p_idx;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT(check_add_partition(domain, part),
		 "invalid partition %p", part);

	key = k_spin_lock(&lock);

	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0U) {
			break;
		}
	}

	__ASSERT(p_idx < max_partitions,
		 "no free partition slots available");

	LOG_DBG("add partition base %lx size %zu to domain %p\n",
		part->start, part->size, domain);

	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;

	domain->num_partitions++;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	arch_mem_domain_partition_add(domain, p_idx);
#endif
	k_spin_unlock(&lock, key);
}

void k_mem_domain_remove_partition(struct k_mem_domain *domain,
				  struct k_mem_partition *part)
{
	int p_idx;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT_NO_MSG(part != NULL);

	key = k_spin_lock(&lock);

	/* find a partition that matches the given start and size */
	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		if (domain->partitions[p_idx].start == part->start &&
		    domain->partitions[p_idx].size == part->size) {
			break;
		}
	}

	__ASSERT(p_idx < max_partitions, "no matching partition found");

	LOG_DBG("remove partition base %lx size %zu from domain %p\n",
		part->start, part->size, domain);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	arch_mem_domain_partition_remove(domain, p_idx);
#endif

	/* A zero-sized partition denotes it's a free partition */
	domain->partitions[p_idx].size = 0U;

	domain->num_partitions--;

	k_spin_unlock(&lock, key);
}

void k_mem_domain_add_thread(struct k_mem_domain *domain, k_tid_t thread)
{
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT_NO_MSG(thread != NULL);

	key = k_spin_lock(&lock);
	if (thread->mem_domain_info.mem_domain != NULL) {
		LOG_DBG("remove thread %p from memory domain %p\n",
			thread, thread->mem_domain_info.mem_domain);
		sys_dlist_remove(&thread->mem_domain_info.mem_domain_q_node);
#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
		arch_mem_domain_thread_remove(thread);
#endif
	}

	LOG_DBG("add thread %p to domain %p\n", thread, domain);
	sys_dlist_append(&domain->mem_domain_q,
			 &thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = domain;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	arch_mem_domain_thread_add(thread);
#endif

	k_spin_unlock(&lock, key);
}

void k_mem_domain_remove_thread(k_tid_t thread)
{
	k_mem_domain_add_thread(&k_mem_domain_default, thread);
}

static int init_mem_domain_module(const struct device *arg)
{
	ARG_UNUSED(arg);

	max_partitions = arch_mem_domain_max_partitions_get();
	/*
	 * max_partitions must be less than or equal to
	 * CONFIG_MAX_DOMAIN_PARTITIONS, or would encounter array index
	 * out of bounds error.
	 */
	__ASSERT(max_partitions <= CONFIG_MAX_DOMAIN_PARTITIONS, "");

	k_mem_domain_init(&k_mem_domain_default, 0, NULL);
#ifdef Z_LIBC_PARTITION_EXISTS
	k_mem_domain_add_partition(&k_mem_domain_default, &z_libc_partition);
#endif /* Z_LIBC_PARTITION_EXISTS */

	return 0;
}

SYS_INIT(init_mem_domain_module, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
