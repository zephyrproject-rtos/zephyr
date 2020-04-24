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

#include <devicetree.h>
#include <sw_isr_table.h>
#include <arch/common/ffs.h>
#include <arch/arc/thread.h>
#ifdef CONFIG_CPU_ARCV2
#include <arch/arc/v2/exc.h>
#include <arch/arc/v2/irq.h>
#include <arch/arc/v2/error.h>
#include <arch/arc/v2/misc.h>
#include <arch/arc/v2/aux_regs.h>
#include <arch/arc/v2/arcv2_irq_unit.h>
#include <arch/arc/v2/asm_inline.h>
#include <arch/common/addr_types.h>
#include "v2/sys_io.h"
#ifdef CONFIG_ARC_CONNECT
#include <arch/arc/v2/arc_connect.h>
#endif
#ifdef CONFIG_ARC_HAS_SECURE
#include <arch/arc/v2/secureshield/arc_secure.h>
#endif
#endif

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#define ARCH_STACK_PTR_ALIGN	4

/* Indicate, for a minimally sized MPU region, how large it must be and what
 * its base address must be aligned to.
 *
 * For regions that are NOT the minimum size, this define has no semantics
 * on ARC MPUv2 as its regions must be power of two size and aligned to their
 * own size. On ARC MPUv3, region sizes are arbitrary and this just indicates
 * the required size granularity.
 */
#ifdef CONFIG_ARC_CORE_MPU
#if CONFIG_ARC_MPU_VER == 2
#define Z_ARC_MPU_ALIGN	2048
#elif CONFIG_ARC_MPU_VER == 3
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
#include <arch/arc/v2/mpu/arc_mpu.h>

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

#if CONFIG_ARC_MPU_VER == 2
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT(!(((size) & ((size) - 1))) && (size) >= Z_ARC_MPU_ALIGN \
		 && !((uint32_t)(start) & ((size) - 1)), \
		"the size of the partition must be power of 2" \
		" and greater than or equal to the mpu adddress alignment." \
		"start address of the partition must align with size.")
#elif CONFIG_ARC_MPU_VER == 3
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT((size) % Z_ARC_MPU_ALIGN == 0 && \
		     (size) >= Z_ARC_MPU_ALIGN && \
		     (uint32_t)(start) % Z_ARC_MPU_ALIGN == 0, \
		     "the size of the partition must align with 32" \
		     " and greater than or equal to 32." \
		     "start address of the partition must align with 32.")
#endif
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
