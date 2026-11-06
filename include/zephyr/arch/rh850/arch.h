/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 specific kernel interface header
 *
 * Included by the architecture abstraction header include/zephyr/arch/cpu.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ARCH_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/arch/rh850/thread.h>
#include <zephyr/arch/rh850/exception.h>
#include <zephyr/arch/rh850/error.h>
#include <zephyr/arch/rh850/irq.h>
#include <zephyr/arch/rh850/sysreg.h>

#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/ffs.h>

/** @cond INTERNAL_HIDDEN */
#define ARCH_STACK_PTR_ALIGN 4
/** @endcond */

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* The key is PSW.ID before the lock: 0 means interrupts were enabled */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key = rh850_sysreg_read(RH850_SR_PSW) & RH850_PSW_ID;

	__asm__ volatile("di" ::: "memory");

	return key;
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return (key & RH850_PSW_ID) == 0U;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	if (arch_irq_unlocked(key)) {
		__asm__ volatile("ei" ::: "memory");
	}
}

static inline bool arch_cpu_irqs_are_enabled(void)
{
	return (rh850_sysreg_read(RH850_SR_PSW) & RH850_PSW_ID) == 0U;
}

extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ARCH_H_ */
