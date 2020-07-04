/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_X86_THREAD_STACK_H
#define ZEPHYR_INCLUDE_ARCH_X86_THREAD_STACK_H

#include <arch/x86/mmustructs.h>

#ifdef CONFIG_X86_64
#define ARCH_STACK_PTR_ALIGN 16UL
#else
#define ARCH_STACK_PTR_ALIGN 4UL
#endif

#ifdef CONFIG_USERSPACE
/* We need a set of page tables for each thread in the system which runs in
 * user mode. For each thread, we have:
 *
 *   - On 32-bit
 *      - a toplevel PD
 *   - On 32-bit (PAE)
 *      - a toplevel PDPT
 *      - a set of PDs for the memory range covered by system RAM
 *   - On 64-bit
 *      - a toplevel PML4
 *      - a set of PDPTs for the memory range covered by system RAM
 *      - a set of PDs for the memory range covered by system RAM
 *   - On all modes:
 *      - a set of PTs for the memory range covered by system RAM
 *
 * Directories and tables for memory ranges outside of system RAM will be
 * shared and not thread-specific.
 *
 * NOTE: We are operating under the assumption that memory domain partitions
 * will not be configured which grant permission to address ranges outside
 * of system RAM.
 *
 * Each of these page tables will be programmed to reflect the memory
 * permission policy for that thread, which will be the union of:
 *
 *   - The boot time memory regions (text, rodata, and so forth)
 *   - The thread's stack buffer
 *   - Partitions in the memory domain configuration (if a member of a
 *     memory domain)
 *
 * The PDPT is fairly small singleton on x86 PAE (32 bytes) and also must
 * be aligned to 32 bytes, so we place it at the highest addresses of the
 * page reserved for the privilege elevation stack. On 64-bit or legacy 32-bit
 * all table entities up to and including the PML4 are page-sized.
 *
 * The page directories and tables require page alignment so we put them as
 * additional fields in the stack object, using the below macros to compute how
 * many pages we need.
 */
#define Z_X86_THREAD_PT_AREA	(Z_X86_NUM_TABLE_PAGES * \
				     (uintptr_t)CONFIG_MMU_PAGE_SIZE)
#else
#define Z_X86_THREAD_PT_AREA	0UL
#endif

#if defined(CONFIG_HW_STACK_PROTECTION) || defined(CONFIG_USERSPACE)
#define Z_X86_STACK_BASE_ALIGN	CONFIG_MMU_PAGE_SIZE
#else
#define Z_X86_STACK_BASE_ALIGN	ARCH_STACK_PTR_ALIGN
#endif

#ifdef CONFIG_USERSPACE
/* If user mode enabled, expand any stack size to fill a page since that is
 * the access control granularity and we don't want other kernel data to
 * unintentionally fall in the latter part of the page
 */
#define Z_X86_STACK_SIZE_ALIGN	CONFIG_MMU_PAGE_SIZE
#else
#define Z_X86_STACK_SIZE_ALIGN	ARCH_STACK_PTR_ALIGN
#endif

#ifndef _ASMLANGUAGE

#ifndef CONFIG_X86_64
struct z_x86_kernel_stack_data {
	/* For 32-bit, a single four-entry page directory pointer table, that
	 * needs to be aligned to 32 bytes.
	 *
	 * 64-bit all the page table entities up to and including the PML4
	 * are page-aligned and we just reserve room for them in
	 * Z_X86_THREAD_PT_AREA.
	 */
	uint8_t ptables[0x20];
} __aligned(0x20);
#endif /* !CONFIG_X86_64 */

/* With both hardware stack protection and userspace enabled, stacks are
 * arranged as follows:
 *
 * High memory addresses
 * +-----------------------------------------+
 * | Thread stack (varies)                   |
 * +-----------------------------------------+
 * | PDPT (32 bytes, 32-bit only)            |
 * | Privilege elevation stack               |
 * |      (4064 or 4096 bytes)               |
 * +-----------------------------------------+
 * | Guard page (4096 bytes)                 |
 * +-----------------------------------------+
 * | User page tables (Z_X86_THREAD_PT_AREA) |
 * +-----------------------------------------+
 * Low Memory addresses
 *
 * Privilege elevation stacks are fixed-size. All the pages containing the
 * thread stack are marked as user-accessible. The guard page is marked
 * read-only to catch stack overflows in supervisor mode.
 *
 * If a thread starts in supervisor mode, the page containing the PDPT and/or
 * privilege elevation stack is also marked read-only.
 *
 * If a thread starts in, or drops down to user mode, the privilege stack page
 * will be marked as present, supervior-only. The page tables will be
 * initialized and used as the active page tables when that thread is active.
 *
 * If KPTI is not enabled, the _main_tss.esp0 field will always be updated
 * updated to point to the top of the privilege elevation stack. Otherwise
 * _main_tss.esp0 always points to the trampoline stack, which handles the
 * page table switch to the kernel PDPT and transplants context to the
 * privileged mode stack.
 */
struct z_x86_thread_stack_header {
#ifdef CONFIG_USERSPACE
	char page_tables[Z_X86_THREAD_PT_AREA];
#endif

#ifdef CONFIG_HW_STACK_PROTECTION
	char guard_page[CONFIG_MMU_PAGE_SIZE];
#endif

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_X86_64
	char privilege_stack[CONFIG_MMU_PAGE_SIZE];
#else
	char privilege_stack[CONFIG_MMU_PAGE_SIZE -
		sizeof(struct z_x86_kernel_stack_data)];

	struct z_x86_kernel_stack_data kernel_data;
#endif /* CONFIG_X86_64 */
#endif /* CONFIG_USERSPACE */
} __packed __aligned(Z_X86_STACK_BASE_ALIGN);

#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_X86_STACK_BASE_ALIGN

#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
	ROUND_UP((size), Z_X86_STACK_SIZE_ALIGN)

#define ARCH_THREAD_STACK_RESERVED \
	sizeof(struct z_x86_thread_stack_header)

#ifdef CONFIG_HW_STACK_PROTECTION
#define ARCH_KERNEL_STACK_RESERVED	CONFIG_MMU_PAGE_SIZE
#define ARCH_KERNEL_STACK_OBJ_ALIGN	CONFIG_MMU_PAGE_SIZE
#else
#define ARCH_KERNEL_STACK_RESERVED	0
#define ARCH_KERNEL_STACK_OBJ_ALIGN	ARCH_STACK_PTR_ALIGN
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_THREAD_STACK_H */
