/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute
 * symbols" in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_DATA_H_

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <vector_table.h>

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include <sys/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ARC_HAS_SECURE
struct _irq_stack_frame {
	uint32_t lp_end;
	uint32_t lp_start;
	uint32_t lp_count;
#ifdef CONFIG_CODE_DENSITY
	/*
	 * Currently unsupported. This is where those registers are
	 * automatically pushed on the stack by the CPU when taking a regular
	 * IRQ.
	 */
	uint32_t ei_base;
	uint32_t ldi_base;
	uint32_t jli_base;
#endif
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t blink;
	uint32_t pc;
	uint32_t sec_stat;
	uint32_t status32;
};
#else
struct _irq_stack_frame {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t blink;
	uint32_t lp_end;
	uint32_t lp_start;
	uint32_t lp_count;
#ifdef CONFIG_CODE_DENSITY
	/*
	 * Currently unsupported. This is where those registers are
	 * automatically pushed on the stack by the CPU when taking a regular
	 * IRQ.
	 */
	uint32_t ei_base;
	uint32_t ldi_base;
	uint32_t jli_base;
#endif
	uint32_t pc;
	uint32_t status32;
};
#endif

typedef struct _irq_stack_frame _isf_t;



/* callee-saved registers pushed on the stack, not in k_thread */
struct _callee_saved_stack {
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t fp; /* r27 */

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_ARC_HAS_SECURE
	uint32_t user_sp;
	uint32_t kernel_sp;
#else
	uint32_t user_sp;
#endif
#endif
	/* r28 is the stack pointer and saved separately */
	/* r29 is ILINK and does not need to be saved */
	uint32_t r30;

#ifdef CONFIG_ARC_HAS_ACCL_REGS
	uint32_t r58;
	uint32_t r59;
#endif

#ifdef CONFIG_FPU_SHARING
	uint32_t fpu_status;
	uint32_t fpu_ctrl;
#ifdef CONFIG_FP_FPU_DA
	uint32_t dpfp2h;
	uint32_t dpfp2l;
	uint32_t dpfp1h;
	uint32_t dpfp1l;
#endif

#endif
	/*
	 * No need to save r31 (blink), it's either alread pushed as the pc or
	 * blink on an irq stack frame.
	 */
};

typedef struct _callee_saved_stack _callee_saved_stack_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_DATA_H_ */
