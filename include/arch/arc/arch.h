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

#ifndef _ARC_ARCH__H_
#define _ARC_ARCH__H_

#ifdef __cplusplus
extern "C" {
#endif

/* APIs need to support non-byte addressable architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#include <generated_dts_board.h>
#include <sw_isr_table.h>
#ifdef CONFIG_CPU_ARCV2
#include <arch/arc/v2/exc.h>
#include <arch/arc/v2/irq.h>
#include <arch/arc/v2/ffs.h>
#include <arch/arc/v2/error.h>
#include <arch/arc/v2/misc.h>
#include <arch/arc/v2/aux_regs.h>
#include <arch/arc/v2/arcv2_irq_unit.h>
#include <arch/arc/v2/asm_inline.h>
#include <arch/arc/v2/addr_types.h>
#endif

#if defined(CONFIG_MPU_STACK_GUARD)
#if defined(CONFIG_ARC_CORE_MPU)
#if CONFIG_ARC_MPU_VER == 2
/*
 * The minimum MPU region of MPU v2 is 2048 bytes. The
 * start address of MPU region should be aligned to the
 * region size
 */
/* The STACK_GUARD_SIZE is the size of stack guard region */
#define STACK_ALIGN  2048
#define STACK_GUARD_SIZE 2048
#elif CONFIG_ARC_MPU_VER == 3
#define STACK_ALIGN 32
#define STACK_GUARD_SIZE 32
#endif
#else /* CONFIG_ARC_CORE_MPU */
#error "Unsupported STACK_ALIGN"
#endif
#else  /* CONFIG_MPU_STACK_GUARD */
#define STACK_ALIGN  4
#define STACK_GUARD_SIZE 0
#endif

#define _ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit __aligned(STACK_ALIGN) \
		sym[size+STACK_GUARD_SIZE]

#define _ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit __aligned(STACK_ALIGN) \
		sym[nmemb][size+STACK_GUARD_SIZE]

#define _ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(STACK_ALIGN) \
		sym[size+STACK_GUARD_SIZE]

#define _ARCH_THREAD_STACK_SIZEOF(sym) (sizeof(sym) - STACK_GUARD_SIZE)

#define _ARCH_THREAD_STACK_BUFFER(sym) ((char *)(sym + STACK_GUARD_SIZE))

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_ARC_MPU
#ifndef _ASMLANGUAGE
#include <arch/arc/v2/mpu/arc_mpu.h>

#define K_MEM_PARTITION_P_NA_U_NA	AUX_MPU_RDP_N
#define K_MEM_PARTITION_P_RW_U_RW	(AUX_MPU_RDP_UW | AUX_MPU_RDP_UR | \
					 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR)
#define K_MEM_PARTITION_P_RW_U_RO	(AUX_MPU_RDP_UR | \
					 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR)
#define K_MEM_PARTITION_P_RW_U_NA	(AUX_MPU_RDP_KW | AUX_MPU_RDP_KR)
#define K_MEM_PARTITION_P_RO_U_RO	(AUX_MPU_RDP_UR | AUX_MPU_RDP_KR)
#define K_MEM_PARTITION_P_RO_U_NA	(AUX_MPU_RDP_KR)

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX	(AUX_MPU_RDP_UW | AUX_MPU_RDP_UR | \
					 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR | \
					 AUX_MPU_RDP_KE | AUX_MPU_RDP_UE)
#define K_MEM_PARTITION_P_RWX_U_RX	(AUX_MPU_RDP_UR | \
					 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR | \
					 AUX_MPU_RDP_KE | AUX_MPU_RDP_UE)
#define K_MEM_PARTITION_P_RX_U_RX	(AUX_MPU_RDP_UR | \
					 AUX_MPU_RDP_KR | \
					 AUX_MPU_RDP_KE | AUX_MPU_RDP_UE)

#define K_MEM_PARTITION_IS_WRITABLE(attr) \
	({ \
		int __is_writable__; \
		attr &= (AUX_MPU_RDP_UW | AUX_MPU_RDP_KW); \
		switch (attr) { \
		case (AUX_MPU_RDP_UW | AUX_MPU_RDP_KW): \
		case AUX_MPU_RDP_UW: \
		case AUX_MPU_RDP_KW: \
			__is_writable__ = 1; \
			break; \
		default: \
			__is_writable__ = 0; \
			break; \
		} \
		__is_writable__; \
	})
#define K_MEM_PARTITION_IS_EXECUTABLE(attr) \
	((attr) & (AUX_MPU_RDP_KE | AUX_MPU_RDP_UE))

#endif /* _ASMLANGUAGE */

#if CONFIG_ARC_MPU_VER == 2
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT_MSG(!(((size) & ((size) - 1))) && (size) >= STACK_ALIGN \
		 && !((u32_t)(start) & ((size) - 1)), \
		"the size of the partition must be power of 2" \
		" and greater than or equal to the mpu adddress alignment." \
		"start address of the partition must align with size.")
#elif CONFIG_ARC_MPU_VER == 3
#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT_MSG((size) % STACK_ALIGN == 0 && (size) >= STACK_ALIGN \
		 && (u32_t)(start) % STACK_ALIGN == 0, \
		"the size of the partition must align with 32" \
		" and greater than or equal to 32." \
		"start address of the partition must align with 32.")
#endif
#endif /* CONFIG_ARC_MPU*/
#endif /* CONFIG_USERSPACE */

#ifndef _ASMLANGUAGE
/* Typedef for the k_mem_partition attribute*/
typedef u32_t k_mem_partition_attr_t;
#endif /* _ASMLANGUAGE */

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE
/* Syscall invocation macros. arc-specific machine constraints used to ensure
 * args land in the proper registers. Currently, they are all stub functions
 * just for enabling CONFIG_USERSPACE on arc w/o errors.
 */

static inline u32_t _arch_syscall_invoke6(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t arg4, u32_t arg5, u32_t arg6,
					  u32_t call_id)
{
	return 0;
}

static inline u32_t _arch_syscall_invoke5(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t arg4, u32_t arg5, u32_t call_id)
{
	return 0;
}

static inline u32_t _arch_syscall_invoke4(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t arg4, u32_t call_id)
{
	return 0;
}

static inline u32_t _arch_syscall_invoke3(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t call_id)
{
	return 0;
}

static inline u32_t _arch_syscall_invoke2(u32_t arg1, u32_t arg2, u32_t call_id)
{
	return 0;
}

static inline u32_t _arch_syscall_invoke1(u32_t arg1, u32_t call_id)
{
	return 0;
}

static inline u32_t _arch_syscall_invoke0(u32_t call_id)
{
	return 0;
}

static inline int _arch_is_user_context(void)
{
	return 0;
}
#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#ifdef __cplusplus
}
#endif
#endif /* _ARC_ARCH__H_ */
