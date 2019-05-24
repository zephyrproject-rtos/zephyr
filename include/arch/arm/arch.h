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

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
/* Guard is 'carved-out' of the thread stack region, and the supervisor
 * mode stack is allocated elsewhere by gen_priv_stack.py
 */
#define Z_ARCH_THREAD_STACK_RESERVED 0
#else
#define Z_ARCH_THREAD_STACK_RESERVED MPU_GUARD_ALIGN_AND_SIZE
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define Z_ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(POW2_CEIL(size)) sym[POW2_CEIL(size)]
#else
#define Z_ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit __aligned(STACK_ALIGN) \
		sym[size+MPU_GUARD_ALIGN_AND_SIZE]
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define Z_ARCH_THREAD_STACK_LEN(size) (POW2_CEIL(size))
#else
#define Z_ARCH_THREAD_STACK_LEN(size) ((size)+MPU_GUARD_ALIGN_AND_SIZE)
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define Z_ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(POW2_CEIL(size)) \
		sym[nmemb][Z_ARCH_THREAD_STACK_LEN(size)]
#else
#define Z_ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(STACK_ALIGN) \
		sym[nmemb][Z_ARCH_THREAD_STACK_LEN(size)]
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define Z_ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(POW2_CEIL(size)) \
		sym[POW2_CEIL(size)]
#else
#define Z_ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(STACK_ALIGN) \
		sym[size+MPU_GUARD_ALIGN_AND_SIZE]
#endif

#define Z_ARCH_THREAD_STACK_SIZEOF(sym) (sizeof(sym) - MPU_GUARD_ALIGN_AND_SIZE)

#define Z_ARCH_THREAD_STACK_BUFFER(sym) \
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
