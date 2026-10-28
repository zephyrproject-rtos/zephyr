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
	}
	if (section->flags & TRICORE_MPU_ACCESS_P_RW_U_RW) {
		if (dpr >= CONFIG_TRICORE_MPU_DATA_REGIONS) {
			return -1;
		}
		dpr_free &= ~(1U << dpr);
		_set_dpr(dpr, section->start, section->end);
		system_dpre |= (section->flags & TRICORE_MPU_ACCESS_P_R) ? (1U << dpr) : 0U;
		system_dpwe |= (section->flags & TRICORE_MPU_ACCESS_P_W) ? (1U << dpr) : 0U;
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

void z_tricore_mpu_configure_kernel_thread(struct k_thread *thread)
{

#if CONFIG_MPU_STACK_GUARD
	z_tricore_mpu_stackguard_enable(thread);
#endif
	_set_dpre(thread->arch.prs, system_dpre);
	_set_dpwe(thread->arch.prs, system_dpwe);
	_set_cpxe(thread->arch.prs, system_cpxe);
}

void z_tricore_mpu_configure_thread(struct k_thread *thread)
{
	__ASSERT(thread != NULL, "Thread pointer cannot be NULL");

	z_tricore_mpu_configure_kernel_thread(thread);
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
	}

	_set_dpre(0, system_dpre);
	_set_dpwe(0, system_dpwe);
	_set_cpxe(0, system_cpxe);

	z_tricore_mpu_enable();
}
