/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/tricore/cr.h>
#include <zephyr/arch/tricore/mpu.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <kernel_internal.h>

#define MPU_STACK_DPR (CONFIG_TRICORE_MPU_DATA_REGIONS - 1)
#define MPU_TEXT_CPR  (CONFIG_TRICORE_MPU_CODE_REGIONS - 1)

#define TRICORE_DPR_L(n) _CONCAT(TRICORE_DPR, _CONCAT(n, _L))
#define TRICORE_DPR_U(n) _CONCAT(TRICORE_DPR, _CONCAT(n, _U))
#define TRICORE_CPR_L(n) _CONCAT(TRICORE_CPR, _CONCAT(n, _L))
#define TRICORE_CPR_U(n) _CONCAT(TRICORE_CPR, _CONCAT(n, _U))
#define TRICORE_DPRE(n)  _CONCAT(TRICORE_DPRE_, n)
#define TRICORE_DPWE(n)  _CONCAT(TRICORE_DPWE_, n)
#define TRICORE_CPXE(n)  _CONCAT(TRICORE_CPXE_, n)

static void _set_dpr(uint8_t region, uintptr_t start_addr, uintptr_t end_addr)
{
#define MPU_DPR_SET(n, ...)                                                                        \
	case n:                                                                                    \
		__asm("mtcr " STRINGIFY(TRICORE_DPR_L(n)) ", %0\n\t" : : "r"(start_addr));         \
		__asm("mtcr " STRINGIFY(TRICORE_DPR_U(n)) ", %0\n\t" : : "r"(end_addr));           \
		break;
	switch (region) {
		LISTIFY(CONFIG_TRICORE_MPU_DATA_REGIONS, MPU_DPR_SET, ())
	}
	__asm volatile("isync" ::: "memory");
}

static void _set_cpr(uint8_t region, uintptr_t start_addr, uintptr_t end_addr)
{
#define MPU_CPR_SET(n, ...)                                                                        \
	case n:                                                                                    \
		__asm("mtcr " STRINGIFY(TRICORE_CPR_L(n)) ", %0\n\t" : : "r"(start_addr));         \
		__asm("mtcr " STRINGIFY(TRICORE_CPR_U(n)) ", %0\n\t" : : "r"(end_addr));           \
		break;
	switch (region) {
		LISTIFY(CONFIG_TRICORE_MPU_CODE_REGIONS, MPU_CPR_SET, ())
	}
	__asm volatile("isync" ::: "memory");
}

static void _set_dpre(uint8_t prs, uint32_t re)
{
#define MPU_DPRE_SET(n, ...)                                                                       \
	case n:                                                                                    \
		__asm("mtcr " STRINGIFY(TRICORE_DPRE(n)) ", %0\n\t" : : "r"(re));                  \
		break;
	switch (prs) {
		LISTIFY(CONFIG_TRICORE_MPU_PROTECTION_SETS, MPU_DPRE_SET, ())
	}
	__asm volatile("isync" ::: "memory");
}

static void _set_dpwe(uint8_t prs, uint32_t we)
{
#define MPU_DPWE_SET(n, ...)                                                                       \
	case n:                                                                                    \
		__asm("mtcr " STRINGIFY(TRICORE_DPWE(n)) ", %0\n\t" : : "r"(we));                  \
		break;
	switch (prs) {
		LISTIFY(CONFIG_TRICORE_MPU_PROTECTION_SETS, MPU_DPWE_SET, ())
	}
	__asm volatile("isync" ::: "memory");
}

static void _set_cpxe(uint8_t prs, uint32_t xe)
{
#define MPU_CPXE_SET(n, ...)                                                                       \
	case n:                                                                                    \
		__asm("mtcr " STRINGIFY(TRICORE_CPXE(n)) ", %0\n\t" : : "r"(xe));                  \
		break;
	switch (prs) {
		LISTIFY(CONFIG_TRICORE_MPU_PROTECTION_SETS, MPU_CPXE_SET, ())
	}
	__asm volatile("isync" ::: "memory");
}

static uint32_t cpr_free = ~0U;
static uint32_t dpr_free = ~0U;
static uint32_t system_dpre;
static uint32_t system_dpwe;
static uint32_t system_cpxe;
#if defined(CONFIG_USERSPACE)
static uint32_t user_cpxe;
static uint32_t user_dpre;
static uint32_t user_dpwe;
static uint8_t partition_slot_max;
static sys_dlist_t loaded_mem_domains = SYS_DLIST_STATIC_INIT(&loaded_mem_domains);
#endif
#if CONFIG_MPU_STACK_GUARD
static uint8_t stack_guard_dpr;
#endif

static int mpu_configure_region(const struct tricore_mpu_region *section)
{
	uint8_t cpr = __builtin_ctz(cpr_free);
	uint8_t dpr = __builtin_ctz(dpr_free);

	if (section->flags & (TRICORE_MPU_ACCESS_P_X | TRICORE_MPU_ACCESS_U_X)) {
		if (cpr >= CONFIG_TRICORE_MPU_CODE_REGIONS) {
			return -1;
		}
		cpr_free &= ~(1U << cpr);
		_set_cpr(cpr, section->start, section->end);
		system_cpxe |= (section->flags & TRICORE_MPU_ACCESS_P_X) ? (1U << cpr) : 0U;
#if defined(CONFIG_USERSPACE)
		user_cpxe |= (section->flags & TRICORE_MPU_ACCESS_U_X) ? (1U << cpr) : 0U;
#endif
	}
	if (section->flags & TRICORE_MPU_ACCESS_P_RW_U_RW) {
		if (dpr >= CONFIG_TRICORE_MPU_DATA_REGIONS) {
			return -1;
		}
		dpr_free &= ~(1U << dpr);
		_set_dpr(dpr, section->start, section->end);
		system_dpre |= (section->flags & TRICORE_MPU_ACCESS_P_R) ? (1U << dpr) : 0U;
		system_dpwe |= (section->flags & TRICORE_MPU_ACCESS_P_W) ? (1U << dpr) : 0U;
#if defined(CONFIG_USERSPACE)
		user_dpre |= (section->flags & TRICORE_MPU_ACCESS_U_R) ? (1U << dpr) : 0U;
		user_dpwe |= (section->flags & TRICORE_MPU_ACCESS_U_W) ? (1U << dpr) : 0U;
#endif
	}

	return 0;
}

void z_tricore_mpu_enable(void)
{
	uint32_t corecon = cr_read(TRICORE_CORECON);

	corecon |= (1U << 1);
	cr_write(TRICORE_CORECON, corecon);
}

void z_tricore_mpu_disable(void)
{
	uint32_t corecon = cr_read(TRICORE_CORECON);

	corecon &= ~(1U << 1);
	cr_write(TRICORE_CORECON, corecon);
}

#if CONFIG_MPU_STACK_GUARD
void z_tricore_mpu_stackguard_disable(struct k_thread *thread)
{
	size_t i;

	ARG_UNUSED(thread);

	for (i = 0; i < mpu_config.num_regions; i++) {
		_set_dpr(i, mpu_config.regions[i].start, mpu_config.regions[i].end);
	}
}

void z_tricore_mpu_stackguard_enable(struct k_thread *thread)
{
	uint32_t guard_start;
	uint32_t guard_end;
	uint32_t i;

	if (thread != NULL) {
		/* The guard sits right below stack_info.start */
		guard_end = thread->stack_info.start;
		guard_start = guard_end - Z_TRICORE_STACK_GUARD_SIZE;
	} else {
		/* The ISR stack has its guard at the bottom */
		guard_start = (uintptr_t)&z_interrupt_stacks[0];
		guard_end = guard_start + Z_TRICORE_STACK_GUARD_SIZE;
	}

	for (i = 0; i < mpu_config.num_regions; i++) {
		const struct tricore_mpu_region *region = &mpu_config.regions[i];

		if (guard_start >= region->end || guard_end <= region->start) {
			continue;
		}
		_set_dpr(i, region->start, guard_start);
		_set_dpr(stack_guard_dpr, guard_end, region->end);
	}
}
#endif

#if defined(CONFIG_USERSPACE)
/* K_USER is set before arch_user_mode_enter() moves the thread to set 1 */
static bool _uses_user_set(const struct k_thread *thread)
{
	return ((thread->base.user_options & K_USER) != 0) && (thread->arch.prs != 0U);
}

/* Unload the oldest domain other than @p keep and the current one */
static bool _unload_domain(const struct k_mem_domain *keep)
{
	const struct k_mem_domain *running = _current->mem_domain_info.mem_domain;
	struct k_mem_domain *d;

	SYS_DLIST_FOR_EACH_CONTAINER(&loaded_mem_domains, d, arch.loaded_node) {
		if ((d == keep) || (d == running)) {
			continue;
		}
		sys_dlist_remove(&d->arch.loaded_node);
		dpr_free |= (d->arch.dpre | d->arch.dpwe) &
			    ~(system_dpre | system_dpwe | user_dpre | user_dpwe |
			      (1U << MPU_STACK_DPR));
		cpr_free |= d->arch.cpxe & ~(system_cpxe | user_cpxe);
		return true;
	}

	return false;
}

static int _alloc_region(uint32_t *free_mask, const struct k_mem_domain *keep)
{
	uint8_t r;

	while (*free_mask == 0U) {
		if (!_unload_domain(keep)) {
			return -ENOSPC;
		}
	}

	r = __builtin_ctz(*free_mask);
	*free_mask &= ~(1U << r);

	return r;
}
#endif

void z_tricore_mpu_configure_kernel_thread(struct k_thread *thread)
{
#if defined(CONFIG_USERSPACE)
	__ASSERT(!_uses_user_set(thread), "Kernel thread expected");
#endif

#if CONFIG_MPU_STACK_GUARD
	z_tricore_mpu_stackguard_enable(thread);
#endif
	_set_dpre(thread->arch.prs, system_dpre);
	_set_dpwe(thread->arch.prs, system_dpwe);
	_set_cpxe(thread->arch.prs, system_cpxe);
}

#if defined(CONFIG_USERSPACE)
void z_tricore_mpu_configure_user_thread(struct k_thread *thread)
{
	struct k_mem_domain *mem_domain = thread->mem_domain_info.mem_domain;
	uint8_t i;

	__ASSERT(_uses_user_set(thread), "User thread expected");

	_set_dpr(MPU_STACK_DPR, thread->stack_info.start,
		 thread->stack_info.start + thread->stack_info.size);

	if (sys_dnode_is_linked(&mem_domain->arch.loaded_node)) {
		uint8_t prs = thread->arch.prs;
		uint32_t dpre = mem_domain->arch.dpre | (1U << MPU_STACK_DPR);
		uint32_t dpwe = mem_domain->arch.dpwe | (1U << MPU_STACK_DPR);
		uint32_t cpxe = mem_domain->arch.cpxe;

		_set_dpre(prs, dpre);
		_set_dpwe(prs, dpwe);
		_set_cpxe(prs, cpxe);
		return;
	}

	mem_domain->arch.dpwe = user_dpwe;
	mem_domain->arch.dpre = user_dpre;
	mem_domain->arch.cpxe = user_cpxe;

	for (i = 0; i < ARRAY_SIZE(mem_domain->partitions); i++) {
		struct k_mem_partition *partition = &mem_domain->partitions[i];

		if (partition->size == 0) {
			continue;
		}

		if (partition->attr.access_rights &
		    (TRICORE_MPU_ACCESS_U_R | TRICORE_MPU_ACCESS_U_W)) {
			int dpr_offset = _alloc_region(&dpr_free, mem_domain);

			__ASSERT(dpr_offset >= 0, "No DPR for partition %p", partition);
			if (dpr_offset < 0) {
				continue;
			}

			_set_dpr(dpr_offset, (uintptr_t)partition->start,
				 (uintptr_t)(partition->start + partition->size));

			if (partition->attr.access_rights & TRICORE_MPU_ACCESS_U_W) {
				mem_domain->arch.dpwe |= (1U << dpr_offset);
			}
			if (partition->attr.access_rights & TRICORE_MPU_ACCESS_U_R) {
				mem_domain->arch.dpre |= (1U << dpr_offset);
			}
		}
		if (partition->attr.access_rights & TRICORE_MPU_ACCESS_U_X) {
			int cpr_offset = _alloc_region(&cpr_free, mem_domain);

			__ASSERT(cpr_offset >= 0, "No CPR for partition %p", partition);
			if (cpr_offset < 0) {
				continue;
			}

			_set_cpr(cpr_offset, (uintptr_t)partition->start,
				 (uintptr_t)(partition->start + partition->size));

			mem_domain->arch.cpxe |= (1U << cpr_offset);
		}
	}

	sys_dlist_append(&loaded_mem_domains, &mem_domain->arch.loaded_node);

	{
		uint8_t prs = thread->arch.prs;
		uint32_t dpre = mem_domain->arch.dpre | (1U << MPU_STACK_DPR);
		uint32_t dpwe = mem_domain->arch.dpwe | (1U << MPU_STACK_DPR);
		uint32_t cpxe = mem_domain->arch.cpxe;

		_set_dpre(prs, dpre);
		_set_dpwe(prs, dpwe);
		_set_cpxe(prs, cpxe);
	}
}
#endif

void z_tricore_mpu_configure_thread(struct k_thread *thread)
{
	__ASSERT(thread != NULL, "Thread pointer cannot be NULL");

#if CONFIG_USERSPACE
	if (_uses_user_set(thread)) {
		z_tricore_mpu_configure_user_thread(thread);
	} else {
		z_tricore_mpu_configure_kernel_thread(thread);
	}
#else
	z_tricore_mpu_configure_kernel_thread(thread);
#endif
}

void z_tricore_mpu_init(void)
{
	size_t i;

	for (i = 0; i < mpu_config.num_regions; i++) {
		mpu_configure_region(&mpu_config.regions[i]);
	}

#if CONFIG_MPU_STACK_GUARD
	stack_guard_dpr = __builtin_ctz(dpr_free);
	dpr_free &= ~(1U << stack_guard_dpr);
	system_dpre |= (1U << stack_guard_dpr);
	system_dpwe |= (1U << stack_guard_dpr);

	z_tricore_mpu_stackguard_enable(NULL);
#endif

	{
		uint32_t dpr_hw_mask = (CONFIG_TRICORE_MPU_DATA_REGIONS >= 32) ?
			UINT32_MAX : ((1U << CONFIG_TRICORE_MPU_DATA_REGIONS) - 1U);
		uint32_t cpr_hw_mask = (CONFIG_TRICORE_MPU_CODE_REGIONS >= 32) ?
			UINT32_MAX : ((1U << CONFIG_TRICORE_MPU_CODE_REGIONS) - 1U);

		dpr_free &= dpr_hw_mask;
		cpr_free &= cpr_hw_mask;
#if defined(CONFIG_USERSPACE)
		dpr_free &= ~(1U << MPU_STACK_DPR);
		partition_slot_max = __builtin_popcount(dpr_free);
#endif
	}

	_set_dpre(0, system_dpre);
	_set_dpwe(0, system_dpwe);
	_set_cpxe(0, system_cpxe);

	z_tricore_mpu_enable();
}

#if CONFIG_USERSPACE
int arch_mem_domain_init(struct k_mem_domain *domain)
{
	domain->arch.cpxe = 0;
	domain->arch.dpre = 0;
	domain->arch.dpwe = 0;
	sys_dnode_init(&domain->arch.loaded_node);

	return 0;
}

int arch_mem_domain_max_partitions_get(void)
{
	int max = (int)partition_slot_max;

	if (max > CONFIG_MAX_DOMAIN_PARTITIONS) {
		max = CONFIG_MAX_DOMAIN_PARTITIONS;
	}
	return max;
}

static uint32_t _get_dpre(uint8_t prs)
{
	uint32_t v = 0;

#define MPU_DPRE_GET(n, ...)                                                                       \
	case n:                                                                                    \
		v = cr_read(TRICORE_DPRE(n));                                                      \
		break;
	switch (prs) {
		LISTIFY(CONFIG_TRICORE_MPU_PROTECTION_SETS, MPU_DPRE_GET, ())
	}
	return v;
}

static uint32_t _get_dpwe(uint8_t prs)
{
	uint32_t v = 0;

#define MPU_DPWE_GET(n, ...)                                                                       \
	case n:                                                                                    \
		v = cr_read(TRICORE_DPWE(n));                                                      \
		break;
	switch (prs) {
		LISTIFY(CONFIG_TRICORE_MPU_PROTECTION_SETS, MPU_DPWE_GET, ())
	}
	return v;
}

static void _get_dpr(uint8_t region, uintptr_t *start, uintptr_t *end)
{
#define MPU_DPR_GET(n, ...)                                                                        \
	case n:                                                                                    \
		*start = cr_read(TRICORE_DPR_L(n));                                                \
		*end = cr_read(TRICORE_DPR_U(n));                                                  \
		break;
	switch (region) {
		LISTIFY(CONFIG_TRICORE_MPU_DATA_REGIONS, MPU_DPR_GET, ())
	}
}

/* Other threads pick up the change when they are switched in */
static void _domain_reapply_current(struct k_mem_domain *domain)
{
	struct k_thread *t = _current;
	uint32_t stack_bit = 1U << MPU_STACK_DPR;

	if (!_uses_user_set(t) || (t->mem_domain_info.mem_domain != domain)) {
		return;
	}

	_set_dpre(t->arch.prs, domain->arch.dpre | stack_bit);
	_set_dpwe(t->arch.prs, domain->arch.dpwe | stack_bit);
	_set_cpxe(t->arch.prs, domain->arch.cpxe);
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];
	uint32_t rights;

	if (!sys_dnode_is_linked(&domain->arch.loaded_node)) {
		return 0;
	}
	if (partition->size == 0U) {
		return 0;
	}

	rights = partition->attr.access_rights;

	if (rights & (TRICORE_MPU_ACCESS_U_R | TRICORE_MPU_ACCESS_U_W)) {
		int dpr_off = _alloc_region(&dpr_free, domain);

		if (dpr_off < 0) {
			return dpr_off;
		}
		_set_dpr(dpr_off, (uintptr_t)partition->start,
			 (uintptr_t)(partition->start + partition->size));
		if (rights & TRICORE_MPU_ACCESS_U_R) {
			domain->arch.dpre |= (1U << dpr_off);
		}
		if (rights & TRICORE_MPU_ACCESS_U_W) {
			domain->arch.dpwe |= (1U << dpr_off);
		}
	}
	if (rights & TRICORE_MPU_ACCESS_U_X) {
		int cpr_off = _alloc_region(&cpr_free, domain);

		if (cpr_off < 0) {
			return cpr_off;
		}
		_set_cpr(cpr_off, (uintptr_t)partition->start,
			 (uintptr_t)(partition->start + partition->size));
		domain->arch.cpxe |= (1U << cpr_off);
	}

	_domain_reapply_current(domain);
	return 0;
}

static int _find_slot_by_bounds(uintptr_t start, uintptr_t end, uint32_t mask,
				bool data)
{
	while (mask != 0U) {
		uint8_t r = __builtin_ctz(mask);
		uintptr_t rs = 0U;
		uintptr_t re = 0U;

		mask &= ~(1U << r);
		if (data) {
			_get_dpr(r, &rs, &re);
		} else {
#define MPU_CPR_GET2(n, ...)                                                                       \
			case n:                                                                    \
				rs = cr_read(TRICORE_CPR_L(n));                                    \
				re = cr_read(TRICORE_CPR_U(n));                                    \
				break;
			switch (r) {
				LISTIFY(CONFIG_TRICORE_MPU_CODE_REGIONS, MPU_CPR_GET2, ())
			}
		}
		if (rs == start && re == end) {
			return (int)r;
		}
	}
	return -1;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];
	uintptr_t start;
	uintptr_t end;
	uint32_t mask;
	int slot;

	if (!sys_dnode_is_linked(&domain->arch.loaded_node)) {
		return 0;
	}
	if (partition->size == 0U) {
		return 0;
	}

	start = (uintptr_t)partition->start;
	end = start + partition->size;

	mask = (domain->arch.dpre | domain->arch.dpwe) &
	       ~(user_dpre | user_dpwe | (1U << MPU_STACK_DPR));
	slot = _find_slot_by_bounds(start, end, mask, true);
	if (slot >= 0) {
		uint32_t bit = 1U << (uint8_t)slot;

		domain->arch.dpre &= ~bit;
		domain->arch.dpwe &= ~bit;
		dpr_free |= bit;
		_set_dpr((uint8_t)slot, 0U, 0U);
	}

	mask = domain->arch.cpxe & ~(user_cpxe | (1U << MPU_TEXT_CPR));
	slot = _find_slot_by_bounds(start, end, mask, false);
	if (slot >= 0) {
		uint32_t bit = 1U << (uint8_t)slot;

		domain->arch.cpxe &= ~bit;
		cpr_free |= bit;
		_set_cpr((uint8_t)slot, 0U, 0U);
	}

	_domain_reapply_current(domain);
	return 0;
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	if (!_uses_user_set(thread)) {
		return 0;
	}
	if (thread == _current) {
		z_tricore_mpu_configure_user_thread(thread);
	}
	return 0;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	ARG_UNUSED(thread);
	return 0;
}

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end;
	uint32_t mask;
	uint8_t prs;

	if (size == 0) {
		return 0;
	}

	end = start + size;
	if (end < start) {
		return -1;
	}

	prs = _current->arch.prs;
	mask = write ? _get_dpwe(prs) : _get_dpre(prs);

	while (mask != 0U) {
		uint8_t region = __builtin_ctz(mask);
		uintptr_t rs = 0U;
		uintptr_t re = 0U;

		mask &= ~(1U << region);
		_get_dpr(region, &rs, &re);
		if (rs == re) {
			continue;
		}
		if (start >= rs && end <= re) {
			return 0;
		}
	}

	return -1;
}
#endif
