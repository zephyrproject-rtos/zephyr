/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 specific kernel interface header
 *
 * This header contains the ARM AArch32 specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/arm/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARCH_H_

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

/* ARM GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { uint32_t name1, name2; }

#include <zephyr/arch/arm/thread.h>
#include <zephyr/arch/arm/exception.h>
#include <zephyr/arch/arm/irq.h>
#include <zephyr/arch/arm/error.h>
#include <zephyr/arch/arm/misc.h>
#include <zephyr/arch/common/addr_types.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/arm/nmi.h>
#include <zephyr/arch/arm/asm_inline.h>
#include <zephyr/arch/common/sys_bitops.h>
#if defined(CONFIG_GDBSTUB)
#include <zephyr/arch/arm/gdbstub.h>
#endif

#ifdef CONFIG_CPU_CORTEX_M
#include <zephyr/arch/arm/cortex_m/cpu.h>
#include <zephyr/arch/arm/cortex_m/memory_map.h>
#include <zephyr/arch/common/sys_io.h>
#elif defined(CONFIG_CPU_AARCH32_CORTEX_R) || defined(CONFIG_CPU_AARCH32_CORTEX_A)
#include <zephyr/arch/arm/cortex_a_r/cpu.h>
#include <zephyr/arch/arm/cortex_a_r/sys_io.h>
#if defined(CONFIG_AARCH32_ARMV8_R)
#include <zephyr/arch/arm/cortex_a_r/lib_helpers.h>
#include <zephyr/arch/arm/cortex_a_r/armv8_timer.h>
#else
#include <zephyr/arch/arm/cortex_a_r/timer.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/fatal_types.h>

enum k_fatal_error_reason_arch {
	/* Cortex-M MEMFAULT exceptions */
	K_ERR_ARM_MEM_GENERIC = K_ERR_ARCH_START,
	K_ERR_ARM_MEM_STACKING,
	K_ERR_ARM_MEM_UNSTACKING,
	K_ERR_ARM_MEM_DATA_ACCESS,
	K_ERR_ARM_MEM_INSTRUCTION_ACCESS,
	K_ERR_ARM_MEM_FP_LAZY_STATE_PRESERVATION,

	/* Cortex-M BUSFAULT exceptions */
	K_ERR_ARM_BUS_GENERIC,
	K_ERR_ARM_BUS_STACKING,
	K_ERR_ARM_BUS_UNSTACKING,
	K_ERR_ARM_BUS_PRECISE_DATA_BUS,
	K_ERR_ARM_BUS_IMPRECISE_DATA_BUS,
	K_ERR_ARM_BUS_INSTRUCTION_BUS,
	K_ERR_ARM_BUS_FP_LAZY_STATE_PRESERVATION,

	/* Cortex-M USAGEFAULT exceptions */
	K_ERR_ARM_USAGE_GENERIC,
	K_ERR_ARM_USAGE_DIV_0,
	K_ERR_ARM_USAGE_UNALIGNED_ACCESS,
	K_ERR_ARM_USAGE_STACK_OVERFLOW,
	K_ERR_ARM_USAGE_NO_COPROCESSOR,
	K_ERR_ARM_USAGE_ILLEGAL_EXC_RETURN,
	K_ERR_ARM_USAGE_ILLEGAL_EPSR,
	K_ERR_ARM_USAGE_UNDEFINED_INSTRUCTION,

	/* Cortex-M SECURE exceptions */
	K_ERR_ARM_SECURE_GENERIC,
	K_ERR_ARM_SECURE_ENTRY_POINT,
	K_ERR_ARM_SECURE_INTEGRITY_SIGNATURE,
	K_ERR_ARM_SECURE_EXCEPTION_RETURN,
	K_ERR_ARM_SECURE_ATTRIBUTION_UNIT,
	K_ERR_ARM_SECURE_TRANSITION,
	K_ERR_ARM_SECURE_LAZY_STATE_PRESERVATION,
	K_ERR_ARM_SECURE_LAZY_STATE_ERROR,

	/* Cortex-A/R exceptions*/
	K_ERR_ARM_UNDEFINED_INSTRUCTION,
	K_ERR_ARM_ALIGNMENT_FAULT,
	K_ERR_ARM_BACKGROUND_FAULT,
	K_ERR_ARM_PERMISSION_FAULT,
	K_ERR_ARM_SYNC_EXTERNAL_ABORT,
	K_ERR_ARM_ASYNC_EXTERNAL_ABORT,
	K_ERR_ARM_SYNC_PARITY_ERROR,
	K_ERR_ARM_ASYNC_PARITY_ERROR,
	K_ERR_ARM_DEBUG_EVENT,
	K_ERR_ARM_TRANSLATION_FAULT,
	K_ERR_ARM_UNSUPPORTED_EXCLUSIVE_ACCESS_FAULT
};

#endif /* _ASMLANGUAGE */

/**
 * @brief Declare the ARCH_STACK_PTR_ALIGN
 *
 * Denotes the required alignment of the stack pointer on public API
 * boundaries
 *
 */
#ifdef CONFIG_STACK_ALIGN_DOUBLE_WORD
#define ARCH_STACK_PTR_ALIGN 8
#else
#define ARCH_STACK_PTR_ALIGN 4
#endif

/**
 * @brief Declare the minimum alignment for a thread stack
 *
 * Denotes the minimum required alignment of a thread stack.
 *
 * Note:
 * User thread stacks must respect the minimum MPU region
 * alignment requirement.
 */
#if defined(CONFIG_USERSPACE)
#define Z_THREAD_MIN_STACK_ALIGN CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#elif defined(CONFIG_ARM_AARCH32_MMU)
#define Z_THREAD_MIN_STACK_ALIGN CONFIG_ARM_MMU_REGION_MIN_ALIGN_AND_SIZE
#else
#define Z_THREAD_MIN_STACK_ALIGN ARCH_STACK_PTR_ALIGN
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
 * The guard region must include enough space for an exception frame
 * below the trapping region as a stack fault will end up storing
 * the exception data (0x20 bytes) onto the stack below wherever
 * the stack pointer refers, even if that is within the guard region,
 * so we make sure the region is strictly larger than this size by
 * setting it to 0x40 (to respect any power-of-two requirements).
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
 * |  Initial Stack Ptr  |       aligned to ARCH_STACK_PTR_ALIGN
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
/* make sure there's more than enough space for an exception frame */
#if CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE <= 0x20
#define MPU_GUARD_ALIGN_AND_SIZE 0x40
#else
#define MPU_GUARD_ALIGN_AND_SIZE CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#endif
#else
#define MPU_GUARD_ALIGN_AND_SIZE 0
#endif

/**
 * @brief Declare the MPU guard alignment and size for a thread stack
 *        that is using the Floating Point services.
 *
 * For threads that are using the Floating Point services under Shared
 * Registers (CONFIG_FPU_SHARING=y) mode, the exception stack frame may
 * contain both the basic stack frame and the FP caller-saved context,
 * upon exception entry. Therefore, a wide guard region is required to
 * guarantee that stack-overflow detection will always be successful.
 */
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING) \
	&& defined(CONFIG_MPU_STACK_GUARD)
#if CONFIG_MPU_STACK_GUARD_MIN_SIZE_FLOAT <= 0x20
#define MPU_GUARD_ALIGN_AND_SIZE_FLOAT 0x40
#else
#define MPU_GUARD_ALIGN_AND_SIZE_FLOAT CONFIG_MPU_STACK_GUARD_MIN_SIZE_FLOAT
#endif
#else
#define MPU_GUARD_ALIGN_AND_SIZE_FLOAT 0
#endif

/**
 * @brief Define alignment of an MPU guard
 *
 * Minimum alignment of the start address of an MPU guard, depending on
 * whether the MPU architecture enforces a size (and power-of-two) alignment
 * requirement.
 */
#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define Z_MPU_GUARD_ALIGN (MAX(MPU_GUARD_ALIGN_AND_SIZE, \
	MPU_GUARD_ALIGN_AND_SIZE_FLOAT))
#else
#define Z_MPU_GUARD_ALIGN MPU_GUARD_ALIGN_AND_SIZE
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
/* This MPU requires regions to be sized to a power of two, and aligned to
 * their own size. Since an MPU region must be able to cover the entire
 * user-accessible stack buffer, we size/align to match. The privilege
 * mode stack is generated elsewhere in memory.
 */
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_POW2_CEIL(size)
#define ARCH_THREAD_STACK_SIZE_ADJUST(size)	Z_POW2_CEIL(size)
#else
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	MAX(Z_THREAD_MIN_STACK_ALIGN, \
						    Z_MPU_GUARD_ALIGN)
#ifdef CONFIG_USERSPACE
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
	ROUND_UP(size, CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
#endif
#endif

#ifdef CONFIG_MPU_STACK_GUARD
/* Kernel-only stacks need an MPU guard region programmed at the beginning of
 * the stack object, so align the object appropriately.
 */
#define ARCH_KERNEL_STACK_RESERVED	MPU_GUARD_ALIGN_AND_SIZE
#define ARCH_KERNEL_STACK_OBJ_ALIGN	Z_MPU_GUARD_ALIGN
#endif

/* On arm, all MPU guards are carve-outs. */
#define ARCH_THREAD_STACK_RESERVED 0

/* Legacy case: retain containing extern "C" with C++ */
#ifdef CONFIG_ARM_MPU
#ifdef CONFIG_CPU_HAS_ARM_MPU
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#endif /* CONFIG_CPU_HAS_ARM_MPU */
#ifdef CONFIG_CPU_HAS_NXP_MPU
#include <zephyr/arch/arm/mpu/nxp_mpu.h>
#endif /* CONFIG_CPU_HAS_NXP_MPU */
#endif /* CONFIG_ARM_MPU */
#ifdef CONFIG_ARM_AARCH32_MMU
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#endif /* CONFIG_ARM_AARCH32_MMU */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARCH_H_ */
