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
 * (include/arm/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARCH_H_

/* Add include for DTS generated information */
#include <generated_dts_board.h>

/* ARM GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { u32_t name1, name2; }

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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Declare the STACK_ALIGN_SIZE
 *
 * Denotes the required alignment of the stack pointer on public API
 * boundaries
 *
 */
#ifdef CONFIG_STACK_ALIGN_DOUBLE_WORD
#define STACK_ALIGN_SIZE 8
#else
#define STACK_ALIGN_SIZE 4
#endif

/**
 * @brief Declare a minimum MPU guard alignment and size
 *
 * This specifies the minimum MPU guard alignment/size for the MPU. This
 * will be used to denote the guard section of the stack, if it exists.
 *
 * One key note is that this guard results in extra bytes being added to
 * the stack. APIs which give the stack ptr and stack size will take this
 * guard size into account.
 *
 * Stack is allocated, but initial stack pointer is at the end
 * (highest address).  Stack grows down to the actual allocation
 * address (lowest address).  Stack guard, if present, will comprise
 * the lowest MPU_GUARD_ALIGN_AND_SIZE bytes of the stack.
 *
 * As the stack grows down, it will reach the end of the stack when it
 * encounters either the stack guard region, or the stack allocation
 * address.
 *
 * ----------------------- <---- Stack allocation address + stack size +
 * |                     |            MPU_GUARD_ALIGN_AND_SIZE
 * |  Some thread data   | <---- Defined when thread is created
 * |        ...          |
 * |---------------------| <---- Actual initial stack ptr
 * |  Initial Stack Ptr  |       aligned to STACK_ALIGN_SIZE
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |  Stack Ends         |
 * |---------------------- <---- Stack Buffer Ptr from API
 * |  MPU Guard,         |
 * |     if present      |
 * ----------------------- <---- Stack Allocation address
 *
 */
#if defined(CONFIG_MPU_STACK_GUARD)
#define MPU_GUARD_ALIGN_AND_SIZE CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#else
#define MPU_GUARD_ALIGN_AND_SIZE 0
#endif

/**
 * @brief Define alignment of a stack buffer
 *
 * This is used for two different things:
 * 1) Used in checks for stack size to be a multiple of the stack buffer
 *    alignment
 * 2) Used to determine the alignment of a stack buffer
 *
 */
#if defined(CONFIG_USERSPACE)
#define STACK_ALIGN    CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#else
#define STACK_ALIGN    MAX(STACK_ALIGN_SIZE, MPU_GUARD_ALIGN_AND_SIZE)
#endif

/**
 * @brief Calculate power of two ceiling for a buffer size input
 *
 */
#define POW2_CEIL(x) ((1 << (31 - __builtin_clz(x))) < x ?  \
		1 << (31 - __builtin_clz(x) + 1) : \
		1 << (31 - __builtin_clz(x)))

/**
 * @brief Declare a top level thread stack memory region
 *
 * This declares a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN_SIZE and
 * put in * 'noinit' section so that it isn't zeroed at boot
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
#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define _ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(POW2_CEIL(size)) sym[POW2_CEIL(size)]
#else
#define _ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit __aligned(STACK_ALIGN) \
		sym[size+MPU_GUARD_ALIGN_AND_SIZE]
#endif

/**
 * @brief Calculate size of stacks to be allocated in a stack array
 *
 * This macro calculates the size to be allocated for the stacks
 * inside a stack array. It accepts the indicated "size" as a parameter
 * and if required, pads some extra bytes (e.g. for MPU scenarios). Refer
 * K_THREAD_STACK_ARRAY_DEFINE definition to see how this is used.
 *
 * @param size Size of the stack memory region
 */
#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define _ARCH_THREAD_STACK_LEN(size) (POW2_CEIL(size))
#else
#define _ARCH_THREAD_STACK_LEN(size) ((size)+MPU_GUARD_ALIGN_AND_SIZE)
#endif

/**
 * @brief Declare a top level array of thread stack memory regions
 *
 * Create an array of equally sized stacks. See K_THREAD_STACK_DEFINE
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN_SIZE and
 * put in * 'noinit' section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to declare
 * @param size Size of the stack memory region
 */
#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define _ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(POW2_CEIL(size)) \
		sym[nmemb][_ARCH_THREAD_STACK_LEN(size)]
#else
#define _ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(STACK_ALIGN) \
		sym[nmemb][_ARCH_THREAD_STACK_LEN(size)]
#endif

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
#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define _ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(POW2_CEIL(size)) \
		sym[POW2_CEIL(size)]
#else
#define _ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(STACK_ALIGN) \
		sym[size+MPU_GUARD_ALIGN_AND_SIZE]
#endif

/**
 * @brief Return the size in bytes of a stack memory region
 *
 * Convenience macro for passing the desired stack size to k_thread_create()
 * since the underlying implementation may actually create something larger
 * (for instance a guard area).
 *
 * The value returned here is NOT guaranteed to match the 'size' parameter
 * passed to K_THREAD_STACK_DEFINE and related macros.
 *
 * In the case of CONFIG_USERSPACE=y and
 * CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT, the size will be larger than the
 * requested size.
 *
 * In all other configurations, the size will be correct.
 *
 * @param sym Stack memory symbol
 * @return Size of the stack
 */
#define _ARCH_THREAD_STACK_SIZEOF(sym) (sizeof(sym) - MPU_GUARD_ALIGN_AND_SIZE)

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
#define _ARCH_THREAD_STACK_BUFFER(sym) \
		((char *)(sym) + MPU_GUARD_ALIGN_AND_SIZE)

#ifdef CONFIG_ARM_MPU
#ifdef CONFIG_CPU_HAS_ARM_MPU
#include <arch/arm/cortex_m/mpu/arm_mpu.h>
#endif /* CONFIG_CPU_HAS_ARM_MPU */
#ifdef CONFIG_CPU_HAS_NXP_MPU
#include <arch/arm/cortex_m/mpu/nxp_mpu.h>
#endif /* CONFIG_CPU_HAS_NXP_MPU */
#endif /* CONFIG_ARM_MPU */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARCH_H_ */
