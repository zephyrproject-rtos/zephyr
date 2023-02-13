/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/sys/__assert.h>
#include <stdbool.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

struct k_spinlock z_mem_domain_lock;
static uint8_t max_partitions;

struct k_mem_domain k_mem_domain_default;

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

	if (part->size == 0U) {
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

		if (dpart->size == 0U) {
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

int k_mem_domain_init(struct k_mem_domain *domain, uint8_t num_parts,
		      struct k_mem_partition *parts[])
{
	k_spinlock_key_t key;
	int ret = 0;

	CHECKIF(domain == NULL) {
		ret = -EINVAL;
		goto out;
	}

	CHECKIF(!(num_parts == 0U || parts != NULL)) {
		LOG_ERR("parts array is NULL and num_parts is nonzero");
		ret = -EINVAL;
		goto out;
	}

	CHECKIF(!(num_parts <= max_partitions)) {
		LOG_ERR("num_parts of %d exceeds maximum allowable partitions (%d)",
			num_parts, max_partitions);
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&z_mem_domain_lock);

	domain->num_partitions = 0U;
	(void)memset(domain->partitions, 0, sizeof(domain->partitions));
	sys_dlist_init(&domain->mem_domain_q);

#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
	ret = arch_mem_domain_init(domain);

	if (ret != 0) {
		LOG_ERR("architecture-specific initialization failed for domain %p with %d",
			domain, ret);
		ret = -ENOMEM;
		goto unlock_out;
	}
#endif
	if (num_parts != 0U) {
		uint32_t i;

		for (i = 0U; i < num_parts; i++) {
			CHECKIF(!check_add_partition(domain, parts[i])) {
				LOG_ERR("invalid partition index %d (%p)",
					i, parts[i]);
				ret = -EINVAL;
				goto unlock_out;
			}

			domain->partitions[i] = *parts[i];
			domain->num_partitions++;
#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
			int ret2 = arch_mem_domain_partition_add(domain, i);

			ARG_UNUSED(ret2);
			CHECKIF(ret2 != 0) {
				ret = ret2;
			}
#endif
		}
	}

unlock_out:
	k_spin_unlock(&z_mem_domain_lock, key);

out:
	return ret;
}

int k_mem_domain_add_partition(struct k_mem_domain *domain,
			       struct k_mem_partition *part)
{
	int p_idx;
	k_spinlock_key_t key;
	int ret = 0;

	CHECKIF(domain == NULL) {
		ret = -EINVAL;
		goto out;
	}

	CHECKIF(!check_add_partition(domain, part)) {
		LOG_ERR("invalid partition %p", part);
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&z_mem_domain_lock);

	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0U) {
			break;
		}
	}

	CHECKIF(!(p_idx < max_partitions)) {
		LOG_ERR("no free partition slots available");
		ret = -ENOSPC;
		goto unlock_out;
	}

	LOG_DBG("add partition base %lx size %zu to domain %p\n",
		part->start, part->size, domain);

	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;

	domain->num_partitions++;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_partition_add(domain, p_idx);
#endif

unlock_out:
	k_spin_unlock(&z_mem_domain_lock, key);

out:
	return ret;
}

int k_mem_domain_remove_partition(struct k_mem_domain *domain,
				  struct k_mem_partition *part)
{
	int p_idx;
	k_spinlock_key_t key;
	int ret = 0;

	CHECKIF((domain == NULL) || (part == NULL)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&z_mem_domain_lock);

	/* find a partition that matches the given start and size */
	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		if (domain->partitions[p_idx].start == part->start &&
		    domain->partitions[p_idx].size == part->size) {
			break;
		}
	}

	CHECKIF(!(p_idx < max_partitions)) {
		LOG_ERR("no matching partition found");
		ret = -ENOENT;
		goto unlock_out;
	}

	LOG_DBG("remove partition base %lx size %zu from domain %p\n",
		part->start, part->size, domain);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_partition_remove(domain, p_idx);
#endif

	/* A zero-sized partition denotes it's a free partition */
	domain->partitions[p_idx].size = 0U;

	domain->num_partitions--;

unlock_out:
	k_spin_unlock(&z_mem_domain_lock, key);

out:
	return ret;
}

static int add_thread_locked(struct k_mem_domain *domain,
			     k_tid_t thread)
{
	int ret = 0;

	__ASSERT_NO_MSG(domain != NULL);
	__ASSERT_NO_MSG(thread != NULL);

	LOG_DBG("add thread %p to domain %p\n", thread, domain);
	sys_dlist_append(&domain->mem_domain_q,
			 &thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = domain;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_thread_add(thread);
#endif

	return ret;
}

static int remove_thread_locked(struct k_thread *thread)
{
	int ret = 0;

	__ASSERT_NO_MSG(thread != NULL);
	LOG_DBG("remove thread %p from memory domain %p\n",
		thread, thread->mem_domain_info.mem_domain);
	sys_dlist_remove(&thread->mem_domain_info.mem_domain_q_node);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_thread_remove(thread);
#endif

	return ret;
}

/* Called from thread object initialization */
void z_mem_domain_init_thread(struct k_thread *thread)
{
	int ret;
	k_spinlock_key_t key = k_spin_lock(&z_mem_domain_lock);

	/* New threads inherit memory domain configuration from parent */
	ret = add_thread_locked(_current->mem_domain_info.mem_domain, thread);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);

	k_spin_unlock(&z_mem_domain_lock, key);
}

/* Called when thread aborts during teardown tasks. sched_spinlock is held */
void z_mem_domain_exit_thread(struct k_thread *thread)
{
	int ret;

	k_spinlock_key_t key = k_spin_lock(&z_mem_domain_lock);

	ret = remove_thread_locked(thread);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);

	k_spin_unlock(&z_mem_domain_lock, key);
}

int k_mem_domain_add_thread(struct k_mem_domain *domain, k_tid_t thread)
{
	int ret = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&z_mem_domain_lock);
	if (thread->mem_domain_info.mem_domain != domain) {
		ret = remove_thread_locked(thread);

		if (ret == 0) {
			ret = add_thread_locked(domain, thread);
		}
	}
	k_spin_unlock(&z_mem_domain_lock, key);

	return ret;
}

static int init_mem_domain_module(const struct device *arg)
{
	int ret;

	ARG_UNUSED(arg);
	ARG_UNUSED(ret);

	max_partitions = arch_mem_domain_max_partitions_get();
	/*
	 * max_partitions must be less than or equal to
	 * CONFIG_MAX_DOMAIN_PARTITIONS, or would encounter array index
	 * out of bounds error.
	 */
	__ASSERT(max_partitions <= CONFIG_MAX_DOMAIN_PARTITIONS, "");

	ret = k_mem_domain_init(&k_mem_domain_default, 0, NULL);
	__ASSERT(ret == 0, "failed to init default mem domain");

#ifdef Z_LIBC_PARTITION_EXISTS
	ret = k_mem_domain_add_partition(&k_mem_domain_default,
					 &z_libc_partition);
	__ASSERT(ret == 0, "failed to add default libc mem partition");
#endif /* Z_LIBC_PARTITION_EXISTS */

	return 0;
}

SYS_INIT(init_mem_domain_module, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
