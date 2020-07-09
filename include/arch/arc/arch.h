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

#ifdef __cplusplus
extern "C" {
#endif

#define ARCH_STACK_PTR_ALIGN	4

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	#if defined(CONFIG_ARC_CORE_MPU)
		#if CONFIG_ARC_MPU_VER == 2
		/*
		 * The minimum MPU region of MPU v2 is 2048 bytes. The
		 * start address of MPU region should be aligned to the
		 * region size
		 */
			#define STACK_ALIGN  2048
		#elif CONFIG_ARC_MPU_VER == 3
			#define STACK_ALIGN 32
		#else
			#error "Unsupported MPU version"
		#endif /* CONFIG_ARC_MPU_VER */

	#else /* CONFIG_ARC_CORE_MPU */
		#error "Requires to enable MPU"
	#endif

#else  /* CONFIG_MPU_STACK_GUARD  || CONFIG_USERSPACE */
	#define STACK_ALIGN  4
#endif

#if defined(CONFIG_MPU_STACK_GUARD)
	#if CONFIG_ARC_MPU_VER == 3
	#define STACK_GUARD_SIZE 32
	#endif
#else /* CONFIG_MPU_STACK_GUARD */
	#define STACK_GUARD_SIZE 0
#endif /* CONFIG_MPU_STACK_GUARD */


#define STACK_SIZE_ALIGN(x) ROUND_UP(x, STACK_ALIGN)

/**
 * @brief Calculate power of two ceiling for a buffer size input
 *
 */
#define POW2_CEIL(x) ((1 << (31 - __builtin_clz(x))) < x ?  \
		1 << (31 - __builtin_clz(x) + 1) : \
		1 << (31 - __builtin_clz(x)))

#if CONFIG_ARC_MPU_VER == 2
/* MPUv2 requires
 *  - region size must be power of 2 and >= 2048
 *  - region start must be aligned to its size
 */
#define Z_ARC_MPU_SIZE_ALIGN(size) POW2_CEIL(STACK_SIZE_ALIGN(size))
#elif CONFIG_ARC_MPU_VER == 3
/* MPUv3 requires
 * - region size must be 32 bytes aligned
 * - region start must be 32 bytes aligned
 */
#define Z_ARC_MPU_SIZE_ALIGN(size) STACK_SIZE_ALIGN(size)
#endif



#if  defined(CONFIG_USERSPACE)

#if CONFIG_ARC_MPU_VER == 2
/* MPU stack guard does not works for MPUv2 as it uses GEN_PRIV_STACK */
#define ARCH_THREAD_STACK_RESERVED 0
#define Z_PRIVILEGE_STACK_ALIGN CONFIG_PRIVILEGED_STACK_SIZE
/*
 * user stack are protected using MPU regions, so need to adhere to
 * MPU start, size alignment
 */
#define Z_ARC_THREAD_STACK_ALIGN(size)	Z_ARC_MPU_SIZE_ALIGN(size)
#define ARCH_THREAD_STACK_LEN(size) Z_ARC_MPU_SIZE_ALIGN(size)

#elif CONFIG_ARC_MPU_VER == 3
#define ARCH_THREAD_STACK_RESERVED \
		(STACK_GUARD_SIZE + CONFIG_PRIVILEGED_STACK_SIZE)
#define Z_PRIVILEGE_STACK_ALIGN (STACK_ALIGN)

#define Z_ARC_THREAD_STACK_ALIGN(size) (STACK_ALIGN)
#define ARCH_THREAD_STACK_LEN(size) \
		(Z_ARC_MPU_SIZE_ALIGN(size) + ARCH_THREAD_STACK_RESERVED)

#endif /* CONFIG_ARC_MPU_VER == 2 */

#else /* CONFIG_USERSPACE */
/*
 * For MPU STACK_GUARD  kernel stacks do not need a MPU region to protect,
 * only guard needs to be protected and aligned. For MPUv3, MPU_STACK_GUARD
 * requires start 32 bytes aligned, also for size which is decided by stack
 * array and USERSPACE.
 *
 * When no-mpu and no USERSPACE/MPU_STACK_GUARD, everything is 4 bytes
 * aligned
 */
#define ARCH_THREAD_STACK_RESERVED (STACK_GUARD_SIZE)

#define Z_ARC_THREAD_STACK_ALIGN(size) (STACK_ALIGN)
#define ARCH_THREAD_STACK_LEN(size) \
		(STACK_SIZE_ALIGN(size) + ARCH_THREAD_STACK_RESERVED)
#endif /* CONFIG_USERSPACE */

#define Z_ARC_THREAD_STACK_ARRAY_LEN(size) ARCH_THREAD_STACK_LEN(size)


#define ARCH_THREAD_STACK_DEFINE(sym, size) \
		struct z_thread_stack_element __noinit \
		__aligned(Z_ARC_THREAD_STACK_ALIGN(size)) \
		sym[ARCH_THREAD_STACK_LEN(size)]

#define ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
		struct z_thread_stack_element __noinit \
		__aligned(Z_ARC_THREAD_STACK_ALIGN(size)) \
		sym[nmemb][Z_ARC_THREAD_STACK_ARRAY_LEN(size)]

#define ARCH_THREAD_STACK_MEMBER(sym, size) \
		struct z_thread_stack_element \
		__aligned(Z_ARC_THREAD_STACK_ALIGN(size)) \
		sym[ARCH_THREAD_STACK_LEN(size)]

#define ARCH_THREAD_STACK_SIZEOF(sym) \
		(sizeof(sym) - ARCH_THREAD_STACK_RESERVED)

#define ARCH_THREAD_STACK_BUFFER(sym) \
		((char *)(sym))

#ifdef CONFIG_ARC_MPU
#ifndef _ASMLANGUAGE

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

#endif /* _ASMLANGUAGE */

#if CONFIG_ARC_MPU_VER == 2
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT(!(((size) & ((size) - 1))) && (size) >= STACK_ALIGN \
		 && !((uint32_t)(start) & ((size) - 1)), \
		"the size of the partition must be power of 2" \
		" and greater than or equal to the mpu adddress alignment." \
		"start address of the partition must align with size.")
#elif CONFIG_ARC_MPU_VER == 3
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT((size) % STACK_ALIGN == 0 && (size) >= STACK_ALIGN \
		 && (uint32_t)(start) % STACK_ALIGN == 0, \
		"the size of the partition must align with 32" \
		" and greater than or equal to 32." \
		"start address of the partition must align with 32.")
#endif
#endif /* CONFIG_ARC_MPU*/

#ifndef _ASMLANGUAGE
/* Typedef for the k_mem_partition attribute*/
typedef uint32_t k_mem_partition_attr_t;

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_ARCH_ARC_ARCH_H_ */
