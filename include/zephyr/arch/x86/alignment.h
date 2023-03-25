/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identfier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ALIGNMENT_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ALIGNMENT_H_

#ifdef CONFIG_X86_64
#include <zephyr/arch/x86/intel64/alignment.h>
#else
#include <zephyr/arch/x86/ia32/alignment.h>
#endif

#if defined(CONFIG_HW_STACK_PROTECTION) || defined(CONFIG_USERSPACE)
#define Z_X86_STACK_BASE_ALIGN  CONFIG_MMU_PAGE_SIZE
#else
#define Z_X86_STACK_BASE_ALIGN  ARCH_STACK_PTR_ALIGN
#endif

#ifdef CONFIG_USERSPACE

/*
 * If user mode is enabled, expand any stack size to fill a page since that is
 * the access control granularity and we don't want other kernel data to
 * unintentionally fall in the latter part of the page.
 */

#define Z_X86_STACK_SIZE_ALIGN  CONFIG_MMU_PAGE_SIZE
#else
#define Z_X86_STACK_SIZE_ALIGN  ARCH_STACK_PTR_ALIGN
#endif

#define ARCH_THREAD_STACK_OBJ_ALIGN(size)       Z_X86_STACK_BASE_ALIGN

#ifdef CONFIG_HW_STACK_PROTECTION
#define ARCH_KERNEL_STACK_OBJ_ALIGN     CONFIG_MMU_PAGE_SIZE
#else
#define ARCH_KERNEL_STACK_OBJ_ALIGN     ARCH_STACK_PTR_ALIGN
#endif


#endif  /* ZEPHYR_INCLUDE_ARCH_X86_ALIGNMENT_H_ */
