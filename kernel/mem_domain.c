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

static struct k_spinlock lock;
static u8_t max_partitions;

#if (defined(CONFIG_EXECUTE_XOR_WRITE) || \
	defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)) && __ASSERT_ON
static bool sane_partition(const struct k_mem_partition *part,
			   const struct k_mem_partition *parts,
			   u32_t num_parts)
{
	bool exec, write;
	u32_t last;
	u32_t i;

	last = part->start + part->size - 1;
	exec = K_MEM_PARTITION_IS_EXECUTABLE(part->attr);
	write = K_MEM_PARTITION_IS_WRITABLE(part->attr);

	if (exec && write) {
		__ASSERT(false,
			"partition is writable and executable <start %lx>",
			 part->start);
		return false;
	}

	for (i = 0U; i < num_parts; i++) {
		bool cur_write, cur_exec;
		u32_t cur_last;

		cur_last = parts[i].start + parts[i].size - 1;

		if (last < parts[i].start || cur_last < part->start) {
			continue;
		}
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
		/* Partitions overlap */
		__ASSERT(false, "overlapping partitions <%lx...%x>, <%lx...%x>",
			part->start, last,
			parts[i].start, cur_last);
		return false;
#endif

		cur_write = K_MEM_PARTITION_IS_WRITABLE(parts[i].attr);
		cur_exec = K_MEM_PARTITION_IS_EXECUTABLE(parts[i].attr);

		if ((cur_write && exec) || (cur_exec && write)) {
			__ASSERT(false, "overlapping partitions are "
				 "writable and executable "
				 "<%lx...%x>, <%lx...%x>",
				 part->start, last,
				 parts[i].start, cur_last);
			return false;
		}
	}

	return true;
}

static inline bool sane_partition_domain(const struct k_mem_domain *domain,
					 const struct k_mem_partition *part)
{
	return sane_partition(part, domain->partitions,
			      domain->num_partitions);
}
#else
#define sane_partition(...) (true)
#define sane_partition_domain(...) (true)
#endif

void k_mem_domain_init(struct k_mem_domain *domain, u8_t num_parts,
		       struct k_mem_partition *parts[])
{
	k_spinlock_key_t key;

	__ASSERT(domain != NULL, "");
	__ASSERT(num_parts == 0U || parts != NULL, "");
	__ASSERT(num_parts <= max_partitions, "");

	key = k_spin_lock(&lock);

	domain->num_partitions = 0U;
	(void)memset(domain->partitions, 0, sizeof(domain->partitions));

	if (num_parts != 0U) {
		u32_t i;

		for (i = 0U; i < num_parts; i++) {
			__ASSERT(parts[i] != NULL, "");
			__ASSERT((parts[i]->start + parts[i]->size) >
				 parts[i]->start,
				 "invalid partition %p size %d",
				 parts[i], parts[i]->size);

#if defined(CONFIG_EXECUTE_XOR_WRITE) || \
	defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
			__ASSERT(sane_partition_domain(domain,
						       parts[i]),
				 "");
#endif
			domain->partitions[i] = *parts[i];
			domain->num_partitions++;
		}
	}

	sys_dlist_init(&domain->mem_domain_q);

	k_spin_unlock(&lock, key);
}

void k_mem_domain_destroy(struct k_mem_domain *domain)
{
	k_spinlock_key_t key;
	sys_dnode_t *node, *next_node;

	__ASSERT(domain != NULL, "");

	key = k_spin_lock(&lock);

	z_arch_mem_domain_destroy(domain);

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

	__ASSERT(domain != NULL, "");
	__ASSERT(part != NULL, "");
	__ASSERT((part->start + part->size) > part->start,
		 "invalid partition %p size %d", part, part->size);

#if defined(CONFIG_EXECUTE_XOR_WRITE) || \
	defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
	__ASSERT(sane_partition_domain(domain, part), "");
#endif

	key = k_spin_lock(&lock);

	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0U) {
			break;
		}
	}

	/* Assert if there is no free partition */
	__ASSERT(p_idx < max_partitions, "");

	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;

	domain->num_partitions++;

	z_arch_mem_domain_partition_add(domain, p_idx);
	k_spin_unlock(&lock, key);
}

void k_mem_domain_remove_partition(struct k_mem_domain *domain,
				  struct k_mem_partition *part)
{
	int p_idx;
	k_spinlock_key_t key;

	__ASSERT(domain != NULL, "");
	__ASSERT(part != NULL, "");

	key = k_spin_lock(&lock);

	/* find a partition that matches the given start and size */
	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		if (domain->partitions[p_idx].start == part->start &&
		    domain->partitions[p_idx].size == part->size) {
			break;
		}
	}

	/* Assert if not found */
	__ASSERT(p_idx < max_partitions, "no matching partition found");

	z_arch_mem_domain_partition_remove(domain, p_idx);

	/* A zero-sized partition denotes it's a free partition */
	domain->partitions[p_idx].size = 0U;

	domain->num_partitions--;

	k_spin_unlock(&lock, key);
}

void k_mem_domain_add_thread(struct k_mem_domain *domain, k_tid_t thread)
{
	k_spinlock_key_t key;

	__ASSERT(domain != NULL, "");
	__ASSERT(thread != NULL, "");
	__ASSERT(thread->mem_domain_info.mem_domain == NULL,
		 "mem domain unset");

	key = k_spin_lock(&lock);

	sys_dlist_append(&domain->mem_domain_q,
			 &thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = domain;

	z_arch_mem_domain_thread_add(thread);

	k_spin_unlock(&lock, key);
}

void k_mem_domain_remove_thread(k_tid_t thread)
{
	k_spinlock_key_t key;

	__ASSERT(thread != NULL, "");
	__ASSERT(thread->mem_domain_info.mem_domain != NULL, "mem domain set");

	key = k_spin_lock(&lock);
	z_arch_mem_domain_thread_remove(thread);

	sys_dlist_remove(&thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = NULL;
	k_spin_unlock(&lock, key);
}

static int init_mem_domain_module(struct device *arg)
{
	ARG_UNUSED(arg);

	max_partitions = z_arch_mem_domain_max_partitions_get();
	/*
	 * max_partitions must be less than or equal to
	 * CONFIG_MAX_DOMAIN_PARTITIONS, or would encounter array index
	 * out of bounds error.
	 */
	__ASSERT(max_partitions <= CONFIG_MAX_DOMAIN_PARTITIONS, "");

	return 0;
}

SYS_INIT(init_mem_domain_module, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
