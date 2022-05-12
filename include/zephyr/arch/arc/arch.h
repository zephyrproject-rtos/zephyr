/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC specific kernel interface header
 *
 * This header contains the ARC specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_ARCH_H_

#include <zephyr/devicetree.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/arc/thread.h>
#include <zephyr/arch/common/sys_bitops.h>
#include "sys-io-common.h"

#include <zephyr/arch/arc/v2/exc.h>
#include <zephyr/arch/arc/v2/irq.h>
#include <zephyr/arch/arc/v2/misc.h>
#include <zephyr/arch/arc/v2/aux_regs.h>
#include <zephyr/arch/arc/v2/arcv2_irq_unit.h>
#include <zephyr/arch/arc/v2/asm_inline.h>
#include <zephyr/arch/arc/arc_addr_types.h>
#include <zephyr/arch/arc/v2/error.h>

#ifdef CONFIG_ARC_CONNECT
#include <zephyr/arch/arc/v2/arc_connect.h>
#endif

#ifdef CONFIG_ISA_ARCV2
#include "v2/sys_io.h"
#ifdef CONFIG_ARC_HAS_SECURE
#include <zephyr/arch/arc/v2/secureshield/arc_secure.h>
#endif
#endif

#if defined(CONFIG_ARC_FIRQ) && defined(CONFIG_ISA_ARCV3)
#error "Unsupported configuration: ARC_FIRQ and ISA_ARCV3"
#endif

/*
 * We don't allow the configuration with FIRQ enabled and only one interrupt priority level
 * (so all interrupts are FIRQ). Such configuration isn't supported in software and it is not
 * beneficial from the performance point of view.
 */
#if defined(CONFIG_ARC_FIRQ) && CONFIG_NUM_IRQ_PRIO_LEVELS < 2
#error "Unsupported configuration: ARC_FIRQ and (NUM_IRQ_PRIO_LEVELS < 2)"
#endif

#if CONFIG_RGF_NUM_BANKS > 1 && !defined(CONFIG_ARC_FIRQ)
#error "Unsupported configuration: (RGF_NUM_BANKS > 1) and !ARC_FIRQ"
#endif

/*
 * It's required to have more than one interrupt priority level to use second register bank
 * - otherwise all interrupts will use same register bank. Such configuration isn't supported in
 * software and it is not beneficial from the performance point of view.
 */
#if CONFIG_RGF_NUM_BANKS > 1 && CONFIG_NUM_IRQ_PRIO_LEVELS < 2
#error "Unsupported configuration: (RGF_NUM_BANKS > 1) and (NUM_IRQ_PRIO_LEVELS < 2)"
#endif

#if defined(CONFIG_ARC_FIRQ_STACK) && !defined(CONFIG_ARC_FIRQ)
#error "Unsupported configuration: ARC_FIRQ_STACK and !ARC_FIRQ"
#endif

#if defined(CONFIG_ARC_FIRQ_STACK) && CONFIG_RGF_NUM_BANKS < 2
#error "Unsupported configuration: ARC_FIRQ_STACK and (RGF_NUM_BANKS < 2)"
#endif

#if defined(CONFIG_SMP) && !defined(CONFIG_MULTITHREADING)
#error "Non-multithreading mode isn't supported on SMP targets"
#endif

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_64BIT
#define ARCH_STACK_PTR_ALIGN	8
#else
#define ARCH_STACK_PTR_ALIGN	4
#endif /* CONFIG_64BIT */

BUILD_ASSERT(CONFIG_ISR_STACK_SIZE % ARCH_STACK_PTR_ALIGN == 0,
	"CONFIG_ISR_STACK_SIZE must be a multiple of ARCH_STACK_PTR_ALIGN");

BUILD_ASSERT(CONFIG_ARC_EXCEPTION_STACK_SIZE % ARCH_STACK_PTR_ALIGN == 0,
	"CONFIG_ARC_EXCEPTION_STACK_SIZE must be a multiple of ARCH_STACK_PTR_ALIGN");

/* Indicate, for a minimally sized MPU region, how large it must be and what
 * its base address must be aligned to.
 *
 * For regions that are NOT the minimum size, this define has no semantics
 * on ARC MPUv2 as its regions must be power of two size and aligned to their
 * own size. On ARC MPUv4, region sizes are arbitrary and this just indicates
 * the required size granularity.
 */
#ifdef CONFIG_ARC_CORE_MPU
#if CONFIG_ARC_MPU_VER == 2
#define Z_ARC_MPU_ALIGN	2048
#elif (CONFIG_ARC_MPU_VER == 3) || (CONFIG_ARC_MPU_VER == 4) || (CONFIG_ARC_MPU_VER == 6)
#define Z_ARC_MPU_ALIGN	32
#else
#error "Unsupported MPU version"
#endif
#endif

#ifdef CONFIG_MPU_STACK_GUARD
#define Z_ARC_STACK_GUARD_SIZE	Z_ARC_MPU_ALIGN
#else
#define Z_ARC_STACK_GUARD_SIZE	0
#endif

/* Kernel-only stacks have the following layout if a stack guard is enabled:
 *
 * +------------+ <- thread.stack_obj
 * | Guard      | } Z_ARC_STACK_GUARD_SIZE
 * +------------+ <- thread.stack_info.start
 * | Kernel     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#ifdef CONFIG_MPU_STACK_GUARD
#define ARCH_KERNEL_STACK_RESERVED	Z_ARC_STACK_GUARD_SIZE
#define ARCH_KERNEL_STACK_OBJ_ALIGN	Z_ARC_MPU_ALIGN
#endif

#ifdef CONFIG_USERSPACE
/* Any thread running In user mode will have full access to the region denoted
 * by thread.stack_info.
 *
 * Thread-local storage is at the very highest memory locations of this area.
 * Memory for TLS and any initial random stack pointer offset is captured
 * in thread.stack_info.delta.
 */
#ifdef CONFIG_MPU_STACK_GUARD
/* MPU guards are only supported with V3 MPU and later. In this configuration
 * the stack object will contain the MPU guard, the privilege stack, and then
 * the stack buffer in that order:
 *
 * +------------+ <- thread.stack_obj
 * | Guard      | } Z_ARC_STACK_GUARD_SIZE
 * +------------+ <- thread.arch.priv_stack_start
 * | Priv Stack | } CONFIG_PRIVILEGED_STACK_SIZE
 * +------------+ <- thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_THREAD_STACK_RESERVED	(Z_ARC_STACK_GUARD_SIZE + \
					 CONFIG_PRIVILEGED_STACK_SIZE)
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_ARC_MPU_ALIGN
/* We need to be able to exactly cover the stack buffer with an MPU region,
 * so round its size up to the required granularity of the MPU
 */
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
		(ROUND_UP((size), Z_ARC_MPU_ALIGN))
BUILD_ASSERT(CONFIG_PRIVILEGED_STACK_SIZE % Z_ARC_MPU_ALIGN == 0,
	     "improper privilege stack size");
#else /* !CONFIG_MPU_STACK_GUARD */
/* Userspace enabled, but supervisor stack guards are not in use */
#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
/* Use defaults for everything. The privilege elevation stack is located
 * in another area of memory generated at build time by gen_kobject_list.py
 *
 * +------------+ <- thread.arch.priv_stack_start
 * | Priv Stack | } Z_KERNEL_STACK_LEN(CONFIG_PRIVILEGED_STACK_SIZE)
 * +------------+
 *
 * +------------+ <- thread.stack_obj = thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
		Z_POW2_CEIL(ROUND_UP((size), Z_ARC_MPU_ALIGN))
#define ARCH_THREAD_STACK_OBJ_ALIGN(size) \
		ARCH_THREAD_STACK_SIZE_ADJUST(size)
#define ARCH_THREAD_STACK_RESERVED		0
#else /* !CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT */
/* Reserved area of the thread object just contains the privilege stack:
 *
 * +------------+ <- thread.stack_obj = thread.arch.priv_stack_start
 * | Priv Stack | } CONFIG_PRIVILEGED_STACK_SIZE
 * +------------+ <- thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_THREAD_STACK_RESERVED		CONFIG_PRIVILEGED_STACK_SIZE
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
		(ROUND_UP((size), Z_ARC_MPU_ALIGN))
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_ARC_MPU_ALIGN

BUILD_ASSERT(CONFIG_PRIVILEGED_STACK_SIZE % Z_ARC_MPU_ALIGN == 0,
	     "improper privilege stack size");
#endif /* CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT */
#endif /* CONFIG_MPU_STACK_GUARD */

#else /* !CONFIG_USERSPACE */

#ifdef CONFIG_MPU_STACK_GUARD
/* Only supported on ARC MPU V3 and higher. Reserve some memory for the stack
 * guard. This is just a minimally-sized region at the beginning of the stack
 * object, which is programmed to produce an exception if written to.
 *
 * +------------+ <- thread.stack_obj
 * | Guard      | } Z_ARC_STACK_GUARD_SIZE
 * +------------+ <- thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_THREAD_STACK_RESERVED		Z_ARC_STACK_GUARD_SIZE
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_ARC_MPU_ALIGN
/* Default for ARCH_THREAD_STACK_SIZE_ADJUST */
#else /* !CONFIG_MPU_STACK_GUARD */
/* No stack guard, no userspace, Use defaults for everything. */
#endif /* CONFIG_MPU_STACK_GUARD */
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_ARC_MPU

/* Legacy case: retain containing extern "C" with C++ */
#include <zephyr/arch/arc/v2/mpu/arc_mpu.h>

#define K_MEM_PARTITION_P_NA_U_NA	AUX_MPU_ATTR_N
#define K_MEM_PARTITION_P_RW_U_RW	(AUX_MPU_ATTR_UW | AUX_MPU_ATTR_UR | \
					 AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR)
#define K_MEM_PARTITION_P_RW_U_RO	(AUX_MPU_ATTR_UR | \
					 AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR)
#define K_MEM_PARTITION_P_RW_U_NA	(AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR)
#define K_MEM_PARTITION_P_RO_U_RO	(AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR)
#define K_MEM_PARTITION_P_RO_U_NA	(AUX_MPU_ATTR_KR)

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX	(AUX_MPU_ATTR_UW | AUX_MPU_ATTR_UR | \
					 AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR | \
					 AUX_MPU_ATTR_KE | AUX_MPU_ATTR_UE)
#define K_MEM_PARTITION_P_RWX_U_RX	(AUX_MPU_ATTR_UR | \
					 AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR | \
					 AUX_MPU_ATTR_KE | AUX_MPU_ATTR_UE)
#define K_MEM_PARTITION_P_RX_U_RX	(AUX_MPU_ATTR_UR | \
					 AUX_MPU_ATTR_KR | \
					 AUX_MPU_ATTR_KE | AUX_MPU_ATTR_UE)

#define K_MEM_PARTITION_IS_WRITABLE(attr) \
	({ \
		int __is_writable__; \
		switch (attr & (AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW)) { \
		case (AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW): \
		case AUX_MPU_ATTR_UW: \
		case AUX_MPU_ATTR_KW: \
			__is_writable__ = 1; \
			break; \
		default: \
			__is_writable__ = 0; \
			break; \
		} \
		__is_writable__; \
	})
#define K_MEM_PARTITION_IS_EXECUTABLE(attr) \
	((attr) & (AUX_MPU_ATTR_KE | AUX_MPU_ATTR_UE))

/*
 * BUILD_ASSERT in case of MWDT is a bit more picky in performing compile-time check.
 * For example it can't evaluate variable address at build time like GCC toolchain can do.
 * That's why we provide custom _ARCH_MEM_PARTITION_ALIGN_CHECK implementation for MWDT toolchain
 * with additional check for arguments availability in compile time.
 */
#ifdef __CCAC__
#define IS_BUILTIN_MWDT(val) __builtin_constant_p((uintptr_t)(val))
#if CONFIG_ARC_MPU_VER == 2 || CONFIG_ARC_MPU_VER == 3 || CONFIG_ARC_MPU_VER == 6
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size)						\
	BUILD_ASSERT(IS_BUILTIN_MWDT(size) ? !((size) & ((size) - 1)) : 1,			\
		"partition size must be power of 2");						\
	BUILD_ASSERT(IS_BUILTIN_MWDT(size) ? (size) >= Z_ARC_MPU_ALIGN : 1,			\
		"partition size must be >= mpu address alignment.");				\
	BUILD_ASSERT(IS_BUILTIN_MWDT(size) ? IS_BUILTIN_MWDT(start) ?				\
		!((uintptr_t)(start) & ((size) - 1)) : 1 : 1,					\
		"partition start address must align with size.")
#elif CONFIG_ARC_MPU_VER == 4
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size)						\
	BUILD_ASSERT(IS_BUILTIN_MWDT(size) ? (size) % Z_ARC_MPU_ALIGN == 0 : 1,			\
		"partition size must align with " STRINGIFY(Z_ARC_MPU_ALIGN));			\
	BUILD_ASSERT(IS_BUILTIN_MWDT(size) ? (size) >= Z_ARC_MPU_ALIGN : 1,			\
		"partition size must be >= " STRINGIFY(Z_ARC_MPU_ALIGN));			\
	BUILD_ASSERT(IS_BUILTIN_MWDT(start) ? (uintptr_t)(start) % Z_ARC_MPU_ALIGN == 0 : 1,	\
		"partition start address must align with " STRINGIFY(Z_ARC_MPU_ALIGN))
#endif
#else /* __CCAC__ */
#if CONFIG_ARC_MPU_VER == 2 || CONFIG_ARC_MPU_VER == 3 || CONFIG_ARC_MPU_VER == 6
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size)						\
	BUILD_ASSERT(!((size) & ((size) - 1)),							\
		"partition size must be power of 2");						\
	BUILD_ASSERT((size) >= Z_ARC_MPU_ALIGN,							\
		"partition size must be >= mpu address alignment.");				\
	BUILD_ASSERT(!((uintptr_t)(start) & ((size) - 1)),					\
		"partition start address must align with size.")
#elif CONFIG_ARC_MPU_VER == 4
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size)						\
	BUILD_ASSERT((size) % Z_ARC_MPU_ALIGN == 0,						\
		"partition size must align with " STRINGIFY(Z_ARC_MPU_ALIGN));			\
	BUILD_ASSERT((size) >= Z_ARC_MPU_ALIGN,							\
		"partition size must be >= " STRINGIFY(Z_ARC_MPU_ALIGN));			\
	BUILD_ASSERT((uintptr_t)(start) % Z_ARC_MPU_ALIGN == 0,					\
		"partition start address must align with " STRINGIFY(Z_ARC_MPU_ALIGN))
#endif
#endif /* __CCAC__ */
#endif /* CONFIG_ARC_MPU*/

/* Typedef for the k_mem_partition attribute*/
typedef uint32_t k_mem_partition_attr_t;

static ALWAYS_INLINE void arch_nop(void)
{
	__builtin_arc_nop();
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_ARCH_ARC_ARCH_H_ */
