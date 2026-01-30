/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_H_

#include <zephyr/arch/tricore/thread.h>
#include <zephyr/arch/tricore/exception.h>
#include <zephyr/arch/tricore/error.h>
#include <zephyr/arch/tricore/irq.h>
#include <zephyr/arch/tricore/cr.h>

#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/ffs.h>

/* Stack align and MPU data protection region align are both 8*/
#define ARCH_STACK_PTR_ALIGN 8

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>

#if defined(CONFIG_USERSPACE)
#include <zephyr/arch/tricore/syscall.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key = cr_read(TRICORE_ICR);

	__asm__ volatile("disable");

	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	if (arch_irq_unlocked(key)) {
		__asm__ volatile("enable");
	}
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return (key & (1 << 15)) != 0;
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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_ARCH_H_ */
