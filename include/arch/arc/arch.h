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


#ifdef __cplusplus
}
#endif
#endif /* _ARC_ARCH__H_ */
