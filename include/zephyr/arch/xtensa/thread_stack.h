/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_STACK_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_STACK_H_

#include <xtensa/config/core-isa.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_KERNEL_COHERENCE
#define ARCH_STACK_PTR_ALIGN XCHAL_DCACHE_LINESIZE
#else
#define ARCH_STACK_PTR_ALIGN 16
#endif


#if CONFIG_USERSPACE
#define Z_XTENSA_STACK_BASE_ALIGN		CONFIG_MMU_PAGE_SIZE
#define Z_XTENSA_STACK_SIZE_ALIGN		CONFIG_MMU_PAGE_SIZE
#else
#define Z_XTENSA_STACK_BASE_ALIGN		ARCH_STACK_PTR_ALIGN
#define Z_XTENSA_STACK_SIZE_ALIGN		ARCH_STACK_PTR_ALIGN
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
#ifdef CONFIG_XTENSA_MMU
struct z_xtensa_thread_stack_header {
	char privilege_stack[CONFIG_MMU_PAGE_SIZE];
} __packed __aligned(Z_XTENSA_STACK_BASE_ALIGN);

#define ARCH_THREAD_STACK_RESERVED		\
	sizeof(struct z_xtensa_thread_stack_header)
#endif /* CONFIG_XTENSA_MMU */

#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_XTENSA_STACK_BASE_ALIGN
#define ARCH_THREAD_STACK_SIZE_ADJUST(size)	\
	ROUND_UP((size), Z_XTENSA_STACK_SIZE_ALIGN)

/* kernel stack */
#define ARCH_KERNEL_STACK_RESERVED		0
#define ARCH_KERNEL_STACK_OBJ_ALIGN		ARCH_STACK_PTR_ALIGN

#endif /* _ASMLANGUAGE */

#endif
