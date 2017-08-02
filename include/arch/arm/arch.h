/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM specific kernel interface header
 *
 * This header contains the ARM specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/arc/cpu.h)
 */

#ifndef _ARM_ARCH__H_
#define _ARM_ARCH__H_

/* Add include for DTS generated information */
#include <generated_dts_board.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ARM GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { u32_t name1, name2; }

/* APIs need to support non-byte addressable architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#ifdef CONFIG_CPU_CORTEX_M
#include <arch/arm/cortex_m/exc.h>
#include <arch/arm/cortex_m/irq.h>
#include <arch/arm/cortex_m/error.h>
#include <arch/arm/cortex_m/misc.h>
#include <arch/arm/cortex_m/memory_map.h>
#include <arch/arm/cortex_m/asm_inline.h>
#include <arch/arm/cortex_m/addr_types.h>
#include <arch/arm/cortex_m/sys_io.h>
#include <arch/arm/cortex_m/nmi.h>
#endif
#if defined(CONFIG_MPU_STACK_GUARD)
#define STACK_ALIGN  32
#else  /* CONFIG_MPU_STACK_GUARD */
#define STACK_ALIGN  4
#endif

/**
 * @brief Declare a toplevel thread stack memory region
 *
 * This declares a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN and put in
 * 'noinit' section so that it isn't zeroed at boot
 *
 * The declared symbol will always be a character array which can be passed to
 * k_thread_create, but should otherwise not be manipulated.
 *
 * It is legal to precede this definition with the 'static' keyword.
 *
 * It is NOT legal to take the sizeof(sym) and pass that to the stackSize
 * parameter of k_thread_create(), it may not be the same as the
 * 'size' parameter. Use K_THREAD_STACK_SIZEOF() instead.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define _ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit __aligned(STACK_ALIGN) \
		sym[size+STACK_ALIGN]

/**
 * @brief Declare a toplevel array of thread stack memory regions
 *
 * Create an array of equally sized stacks. See K_THREAD_STACK_DEFINE
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN and put in
 * 'noinit' section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to declare
 * @param size Size of the stack memory region
 */
#define _ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit __aligned(STACK_ALIGN) \
		sym[nmemb][size+STACK_ALIGN]

/**
 * @brief Declare an embedded stack memory region
 *
 * Used for stacks embedded within other data structures. Use is highly
 * discouraged but in some cases necessary. For memory protection scenarios,
 * it is very important that any RAM preceding this member not be writable
 * by threads else a stack overflow will lead to silent corruption. In other
 * words, the containing data structure should live in RAM owned by the kernel.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define _ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(STACK_ALIGN) \
		sym[size+STACK_ALIGN]

/**
 * @brief Return the size in bytes of a stack memory region
 *
 * Convenience macro for passing the desired stack size to k_thread_create()
 * since the underlying implementation may actually create something larger
 * (for instance a guard area).
 *
 * The value returned here is guaranteed to match the 'size' parameter
 * passed to K_THREAD_STACK_DEFINE and related macros.
 *
 * @param sym Stack memory symbol
 * @return Size of the stack
 */
#define _ARCH_THREAD_STACK_SIZEOF(sym) (sizeof(sym) - STACK_ALIGN)

/**
 * @brief Get a pointer to the physical stack buffer
 *
 * Convenience macro to get at the real underlying stack buffer used by
 * the CPU. Guaranteed to be a character pointer of size K_THREAD_STACK_SIZEOF.
 * This is really only intended for diagnostic tools which want to examine
 * stack memory contents.
 *
 * @param sym Declared stack symbol name
 * @return The buffer itself, a char *
 */
#define _ARCH_THREAD_STACK_BUFFER(sym) ((char *)(sym + STACK_ALIGN))

#ifdef __cplusplus
}
#endif

#endif /* _ARM_ARCH__H_ */
