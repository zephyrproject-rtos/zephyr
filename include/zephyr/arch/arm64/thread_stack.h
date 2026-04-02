/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_STACK_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_STACK_H_

#include <zephyr/arch/arm64/mm.h>

#define ARCH_STACK_PTR_ALIGN			16

#if defined(CONFIG_USERSPACE) || defined(CONFIG_ARM64_STACK_PROTECTION)
#define Z_ARM64_STACK_BASE_ALIGN		MEM_DOMAIN_ALIGN_AND_SIZE
#define Z_ARM64_STACK_SIZE_ALIGN		MEM_DOMAIN_ALIGN_AND_SIZE
#else
#define Z_ARM64_STACK_BASE_ALIGN		ARCH_STACK_PTR_ALIGN
#define Z_ARM64_STACK_SIZE_ALIGN		ARCH_STACK_PTR_ALIGN
#endif

#if defined(CONFIG_ARM64_STACK_PROTECTION)
#define Z_ARM64_STACK_GUARD_SIZE		MEM_DOMAIN_ALIGN_AND_SIZE
#define Z_ARM64_K_STACK_BASE_ALIGN		MEM_DOMAIN_ALIGN_AND_SIZE
#else
#define Z_ARM64_STACK_GUARD_SIZE		0
#define Z_ARM64_K_STACK_BASE_ALIGN		ARCH_STACK_PTR_ALIGN
#endif

/*
 * [ see also comments in arch/arm64/core/thread.c ]
 *
 * High memory addresses
 *
 * +-------------------+ <- thread.stack_info.start + thread.stack_info.size
 * |       TLS         |
 * +-------------------+ <- initial sp (computable with thread.stack_info.delta)
 * |                   |
 * |    Used stack     |
 * |                   |
 * +...................+ <- thread's current stack pointer
 * |                   |
 * |   Unused stack    |
 * |                   |
 * +-------------------+ <- thread.stack_info.start
 * | Privileged stack  | } K_(THREAD|KERNEL)_STACK_RESERVED
 * +-------------------+ <- thread stack limit (update on every context switch)
 * |    Stack guard    | } Z_ARM64_STACK_GUARD_SIZE (protected by MMU/MPU)
 * +-------------------+ <- thread.stack_obj
 *
 * Low Memory addresses
 */

/* thread stack */
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_ARM64_STACK_BASE_ALIGN
#define ARCH_THREAD_STACK_SIZE_ADJUST(size)	\
	ROUND_UP((size), Z_ARM64_STACK_SIZE_ALIGN)
#define ARCH_THREAD_STACK_RESERVED		CONFIG_PRIVILEGED_STACK_SIZE + \
	Z_ARM64_STACK_GUARD_SIZE

/* kernel stack */
#define ARCH_KERNEL_STACK_RESERVED		Z_ARM64_STACK_GUARD_SIZE
#define ARCH_KERNEL_STACK_OBJ_ALIGN		Z_ARM64_K_STACK_BASE_ALIGN

#ifndef _ASMLANGUAGE

struct z_arm64_thread_stack_header {
	char privilege_stack[CONFIG_PRIVILEGED_STACK_SIZE];
} __packed __aligned(Z_ARM64_STACK_BASE_ALIGN);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_STACK_H_ */
