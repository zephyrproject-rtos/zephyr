/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_X86_THREAD_STACK_H
#define ZEPHYR_INCLUDE_ARCH_X86_THREAD_STACK_H

#include <zephyr/arch/x86/mmustructs.h>

#ifdef CONFIG_X86_64
#define ARCH_STACK_PTR_ALIGN 16UL
#else
#define ARCH_STACK_PTR_ALIGN 4UL
#endif

#if defined(CONFIG_X86_STACK_PROTECTION) || defined(CONFIG_USERSPACE) \
	|| defined(CONFIG_THREAD_STACK_MEM_MAPPED)
#define Z_X86_STACK_BASE_ALIGN	CONFIG_MMU_PAGE_SIZE
#else
#define Z_X86_STACK_BASE_ALIGN	ARCH_STACK_PTR_ALIGN
#endif

#if defined(CONFIG_USERSPACE) || defined(CONFIG_THREAD_STACK_MEM_MAPPED)
/* If user mode enabled, expand any stack size to fill a page since that is
 * the access control granularity and we don't want other kernel data to
 * unintentionally fall in the latter part of the page
 *
 * This is also true when memory mapped stacks are used with since
 * access control applies to one page at a time.
 */
#define Z_X86_STACK_SIZE_ALIGN	CONFIG_MMU_PAGE_SIZE
#else
#define Z_X86_STACK_SIZE_ALIGN	ARCH_STACK_PTR_ALIGN
#endif

#ifndef _ASMLANGUAGE
/* With both hardware stack protection and userspace enabled, stacks are
 * arranged as follows:
 *
 * --- Without stack being memory mapped:
 * High memory addresses
 * +-----------------------------------------+
 * | Thread stack (varies)                   |
 * +-----------------------------------------+
 * | Privilege elevation stack               |
 * |   (CONFIG_PRIVILEGED_STACK_SIZE)        |
 * +-----------------------------------------+
 * | Guard page (4096 bytes)                 |
 * |   - 'guard_page' in struct              |
 * |     z_x86_thread_stack_header           |
 * +-----------------------------------------+
 * Low Memory addresses
 *
 * --- With stack being memory mapped:
 * High memory addresses
 * +-----------------------------------------+
 * | Guard page (empty page)                 |
 * +-----------------------------------------+
 * | Thread stack (varies)                   |
 * +-----------------------------------------+
 * | Privilege elevation stack               |
 * |   (CONFIG_PRIVILEGED_STACK_SIZE)        |
 * +-----------------------------------------+
 * | Guard page (empty page)                 |
 * +-----------------------------------------+
 * Low Memory addresses
 *
 * Without memory mapped stacks, the guard page is actually allocated
 * as part of the stack struct, which takes up physical memory during
 * linking.
 *
 * Privilege elevation stacks are fixed-size. All the pages containing the
 * thread stack are marked as user-accessible. The guard page is marked
 * read-only to catch stack overflows in supervisor mode.
 *
 * If a thread starts in supervisor mode, the page containing the
 * privilege elevation stack is also marked read-only.
 *
 * If a thread starts in, or drops down to user mode, the privilege stack page
 * will be marked as present, supervisor-only.
 *
 * If KPTI is not enabled, the _main_tss.esp0 field will always be updated
 * updated to point to the top of the privilege elevation stack. Otherwise
 * _main_tss.esp0 always points to the trampoline stack, which handles the
 * page table switch to the kernel PDPT and transplants context to the
 * privileged mode stack.
 */
struct z_x86_thread_stack_header {
#if defined(CONFIG_X86_STACK_PROTECTION) && !defined(CONFIG_THREAD_STACK_MEM_MAPPED)
	char guard_page[CONFIG_MMU_PAGE_SIZE];
#endif
#ifdef CONFIG_USERSPACE
	char privilege_stack[CONFIG_PRIVILEGED_STACK_SIZE];
#endif /* CONFIG_USERSPACE */
} __packed __aligned(Z_X86_STACK_BASE_ALIGN);

#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_X86_STACK_BASE_ALIGN

#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
	ROUND_UP((size), Z_X86_STACK_SIZE_ALIGN)

#define ARCH_THREAD_STACK_RESERVED \
	sizeof(struct z_x86_thread_stack_header)

#ifdef CONFIG_X86_STACK_PROTECTION
#define ARCH_KERNEL_STACK_RESERVED	CONFIG_MMU_PAGE_SIZE
#define ARCH_KERNEL_STACK_OBJ_ALIGN	CONFIG_MMU_PAGE_SIZE
#else
#define ARCH_KERNEL_STACK_OBJ_ALIGN	ARCH_STACK_PTR_ALIGN
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_THREAD_STACK_H */
