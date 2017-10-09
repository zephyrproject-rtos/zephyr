/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <nano_internal.h>
#include <misc/__assert.h>

static u8_t max_partitions;

static void ensure_w_xor_x(u32_t attrs)
{
#if defined(CONFIG_EXECUTE_XOR_WRITE) && __ASSERT_ON
	bool writable = K_MEM_PARTITION_IS_WRITABLE(attrs);
	bool executable = K_MEM_PARTITION_IS_EXECUTABLE(attrs);

	__ASSERT(writable != executable, "writable page not executable");
#else
	ARG_UNUSED(attrs);
#endif
}

void k_mem_domain_init(struct k_mem_domain *domain, u32_t num_parts,
		struct k_mem_partition *parts[])
{
	unsigned int key;

	__ASSERT(domain && (!num_parts || parts), "");
	__ASSERT(num_parts <= max_partitions, "");

	key = irq_lock();

	domain->num_partitions = num_parts;
	memset(domain->partitions, 0, sizeof(domain->partitions));

	if (num_parts) {
		u32_t i;

		for (i = 0; i < num_parts; i++) {
			__ASSERT(parts[i], "");

			ensure_w_xor_x(parts[i]->attr);

			domain->partitions[i] = *parts[i];
		}
	}

	sys_dlist_init(&domain->mem_domain_q);

	irq_unlock(key);
}

void k_mem_domain_destroy(struct k_mem_domain *domain)
{
	unsigned int key;
	sys_dnode_t *node, *next_node;

	__ASSERT(domain, "");

	key = irq_lock();

	/* Handle architecture specifc destroy only if it is the current thread*/
	if (_current->mem_domain_info.mem_domain == domain) {
		_arch_mem_domain_destroy(domain);
	}

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		sys_dlist_remove(&thread->mem_domain_info.mem_domain_q_node);
		thread->mem_domain_info.mem_domain = NULL;
	}

	irq_unlock(key);
}

void k_mem_domain_add_partition(struct k_mem_domain *domain,
			       struct k_mem_partition *part)
{
	int p_idx;
	unsigned int key;

	__ASSERT(domain && part, "");
	__ASSERT(part->start + part->size > part->start, "");

	ensure_w_xor_x(part->attr);

	key = irq_lock();

	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0) {
			break;
		}
	}

	/* Assert if there is no free partition */
	__ASSERT(p_idx < max_partitions, "");

	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;

	domain->num_partitions++;

	irq_unlock(key);
}

void k_mem_domain_remove_partition(struct k_mem_domain *domain,
				  struct k_mem_partition *part)
{
	int p_idx;
	unsigned int key;

	__ASSERT(domain && part, "");

	key = irq_lock();

	/* find a partition that matches the given start and size */
	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		if (domain->partitions[p_idx].start == part->start &&
		    domain->partitions[p_idx].size == part->size) {
			break;
		}
	}

	/* Assert if not found */
	__ASSERT(p_idx < max_partitions, "");

	/* Handle architecture specifc remove only if it is the current thread*/
	if (_current->mem_domain_info.mem_domain == domain) {
		_arch_mem_domain_partition_remove(domain, p_idx);
	}

	domain->partitions[p_idx].start = 0;
	domain->partitions[p_idx].size = 0;
	domain->partitions[p_idx].attr = 0;

	domain->num_partitions--;

	irq_unlock(key);
}

void k_mem_domain_add_thread(struct k_mem_domain *domain, k_tid_t thread)
{
	unsigned int key;

	__ASSERT(domain && thread && !thread->mem_domain_info.mem_domain, "");

	key = irq_lock();

	sys_dlist_append(&domain->mem_domain_q,
			 &thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = domain;

	irq_unlock(key);
}

void k_mem_domain_remove_thread(k_tid_t thread)
{
	unsigned int key;

	__ASSERT(thread && thread->mem_domain_info.mem_domain, "");

	key = irq_lock();

	sys_dlist_remove(&thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = NULL;

	irq_unlock(key);
}

static int init_mem_domain_module(struct device *arg)
{
	ARG_UNUSED(arg);

	max_partitions = _arch_mem_domain_max_partitions_get();
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
