/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_DATA_H_

/*
 * Exception/interrupt vector definitions: vectors 20 to 31 are reserved
 * for Intel; vectors 32 to 255 are user defined interrupt vectors.
 */

#define IV_DIVIDE_ERROR 0
#define IV_DEBUG 1
#define IV_NON_MASKABLE_INTERRUPT 2
#define IV_BREAKPOINT 3
#define IV_OVERFLOW 4
#define IV_BOUND_RANGE 5
#define IV_INVALID_OPCODE 6
#define IV_DEVICE_NOT_AVAILABLE 7
#define IV_DOUBLE_FAULT 8
#define IV_COPROC_SEGMENT_OVERRUN 9
#define IV_INVALID_TSS 10
#define IV_SEGMENT_NOT_PRESENT 11
#define IV_STACK_FAULT 12
#define IV_GENERAL_PROTECTION 13
#define IV_PAGE_FAULT 14
#define IV_RESERVED 15
#define IV_X87_FPU_FP_ERROR 16
#define IV_ALIGNMENT_CHECK 17
#define IV_MACHINE_CHECK 18
#define IV_SIMD_FP 19

#define IV_IRQS 32		/* start of vectors available for IRQs */
#define IV_NR_VECTORS 256	/* total number of vectors */

/*
 * EFLAGS/RFLAGS definitions. (RFLAGS is just zero-extended EFLAGS.)
 */

#define EFLAGS_IF	0x00000200U	/* interrupts enabled */
#define EFLAGS_INITIAL	(EFLAGS_IF)

/*
 * Control register definitions.
 */

#define CR0_PG		0x80000000	/* enable paging */
#define CR0_WP		0x00010000	/* honor W bit even when supervisor */
#define CR4_PAE		0x00000020	/* enable PAE */
#define CR4_OSFXSR	0x00000200	/* enable SSE (OS FXSAVE/RSTOR) */

#ifdef CONFIG_X86_64
#include <intel64/kernel_arch_data.h>
#else
#include <ia32/kernel_arch_data.h>
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_DATA_H_ */
