/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 specific syscall header
 *
 * This header contains the ARM64 specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/aarch64/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_AARCH64_ARM_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_AARCH64_ARM_SYSCALL_H_

#define _SVC_CALL_CONTEXT_SWITCH	0
#define _SVC_CALL_IRQ_OFFLOAD		1
#define _SVC_CALL_RUNTIME_EXCEPT	2

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>
#include <arch/arm/aarch64/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool arch_is_user_context(void)
{
	uint64_t tpidrro_el0;

	__asm__ volatile("mrs %0, tpidrro_el0" : "=r" (tpidrro_el0));

	return (tpidrro_el0 != 0x0);
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_SYSCALL_H_ */
