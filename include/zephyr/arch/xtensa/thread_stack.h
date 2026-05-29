/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_STACK_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_STACK_H_

#include <xtensa/config/core-isa.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_KERNEL_COHERENCE
#define ARCH_STACK_PTR_ALIGN XCHAL_DCACHE_LINESIZE
#else
#define ARCH_STACK_PTR_ALIGN 16
#endif


#ifdef CONFIG_USERSPACE
#ifdef CONFIG_XTENSA_MMU
#define XTENSA_STACK_BASE_ALIGN		CONFIG_MMU_PAGE_SIZE
#define XTENSA_STACK_SIZE_ALIGN		CONFIG_MMU_PAGE_SIZE
#endif
#ifdef CONFIG_XTENSA_MPU
#define XTENSA_STACK_BASE_ALIGN		XCHAL_MPU_ALIGN
#define XTENSA_STACK_SIZE_ALIGN		XCHAL_MPU_ALIGN
#endif
#else
#define XTENSA_STACK_BASE_ALIGN		ARCH_STACK_PTR_ALIGN
#define XTENSA_STACK_SIZE_ALIGN		ARCH_STACK_PTR_ALIGN
#endif

/*
 *
 * High memory addresses
 *
 * +-------------------+ <- thread.stack_info.start + thread.stack_info.size
 * |       TLS         |
 * +-------------------+ <- initial sp (computable with thread.stack_info.delta)
 * |                   |
 * |   Thread stack    |
 * |                   |
 * +-------------------+ <- thread.stack_info.start
 * | Privileged stack  | } CONFIG_MMU_PAGE_SIZE
 * +-------------------+ <- thread.stack_obj
 *
 * Low Memory addresses
 */

#ifndef _ASMLANGUAGE

/* thread stack */

#if defined(CONFIG_USERSPACE) && !defined(CONFIG_GEN_PRIV_STACKS)

/** Struct describing the reserved thread stack space */
struct xtensa_thread_stack_header {
	/** Privilege stack */
	char privilege_stack[CONFIG_PRIVILEGED_STACK_SIZE];
} __packed __aligned(XTENSA_STACK_BASE_ALIGN);

/** Size of reserved thread stack space */
#define ARCH_THREAD_STACK_RESERVED		\
	sizeof(struct xtensa_thread_stack_header)

#endif /* CONFIG_USERSPACE && !CONFIG_GEN_PRIV_STACKS */

#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	XTENSA_STACK_BASE_ALIGN
#define ARCH_THREAD_STACK_SIZE_ADJUST(size)	\
	ROUND_UP((size), XTENSA_STACK_SIZE_ALIGN)

/* kernel stack */
#define ARCH_KERNEL_STACK_OBJ_ALIGN		ARCH_STACK_PTR_ALIGN

#endif /* _ASMLANGUAGE */

#endif
