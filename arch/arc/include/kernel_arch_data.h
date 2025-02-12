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

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <vector_table.h>

#ifndef _ASMLANGUAGE
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ARC_HAS_SECURE
struct arch_esf {
#ifdef CONFIG_ARC_HAS_ZOL
	uintptr_t lp_end;
	uintptr_t lp_start;
	uintptr_t lp_count;
#endif /* CONFIG_ARC_HAS_ZOL */
#ifdef CONFIG_CODE_DENSITY
	/*
	 * Currently unsupported. This is where those registers are
	 * automatically pushed on the stack by the CPU when taking a regular
	 * IRQ.
	 */
	uintptr_t ei_base;
	uintptr_t ldi_base;
	uintptr_t jli_base;
#endif
	uintptr_t r0;
	uintptr_t r1;
	uintptr_t r2;
	uintptr_t r3;
	uintptr_t r4;
	uintptr_t r5;
	uintptr_t r6;
	uintptr_t r7;
	uintptr_t r8;
	uintptr_t r9;
	uintptr_t r10;
	uintptr_t r11;
	uintptr_t r12;
	uintptr_t r13;
	uintptr_t blink;
	uintptr_t pc;
	uintptr_t sec_stat;
	uintptr_t status32;
};
#else
struct arch_esf {
	uintptr_t r0;
	uintptr_t r1;
	uintptr_t r2;
	uintptr_t r3;
	uintptr_t r4;
	uintptr_t r5;
	uintptr_t r6;
	uintptr_t r7;
	uintptr_t r8;
	uintptr_t r9;
	uintptr_t r10;
	uintptr_t r11;
	uintptr_t r12;
	uintptr_t r13;
	uintptr_t blink;
#ifdef CONFIG_ARC_HAS_ZOL
	uintptr_t lp_end;
	uintptr_t lp_start;
	uintptr_t lp_count;
#endif /* CONFIG_ARC_HAS_ZOL */
#ifdef CONFIG_CODE_DENSITY
	/*
	 * Currently unsupported. This is where those registers are
	 * automatically pushed on the stack by the CPU when taking a regular
	 * IRQ.
	 */
	uintptr_t ei_base;
	uintptr_t ldi_base;
	uintptr_t jli_base;
#endif
	uintptr_t pc;
	uintptr_t status32;
};
#endif

typedef struct arch_esf _isf_t;



/* callee-saved registers pushed on the stack, not in k_thread */
struct _callee_saved_stack {
	uintptr_t r13;
	uintptr_t r14;
	uintptr_t r15;
	uintptr_t r16;
	uintptr_t r17;
	uintptr_t r18;
	uintptr_t r19;
	uintptr_t r20;
	uintptr_t r21;
	uintptr_t r22;
	uintptr_t r23;
	uintptr_t r24;
	uintptr_t r25;
	uintptr_t r26;
	uintptr_t fp; /* r27 */

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_ARC_HAS_SECURE
	uintptr_t user_sp;
	uintptr_t kernel_sp;
#else
	uintptr_t user_sp;
#endif
#endif
	/* r28 is the stack pointer and saved separately */
	/* r29 is ILINK and does not need to be saved */
	uintptr_t r30;

#ifdef CONFIG_ARC_HAS_ACCL_REGS
	uintptr_t r58;
#ifndef CONFIG_64BIT
	uintptr_t r59;
#endif /* !CONFIG_64BIT */
#endif

#ifdef CONFIG_FPU_SHARING
	uintptr_t fpu_status;
	uintptr_t fpu_ctrl;
#ifdef CONFIG_FP_FPU_DA
	uintptr_t dpfp2h;
	uintptr_t dpfp2l;
	uintptr_t dpfp1h;
	uintptr_t dpfp1l;
#endif
#endif

#ifdef CONFIG_DSP_SHARING
#ifdef CONFIG_ARC_DSP_BFLY_SHARING
	uintptr_t dsp_fft_ctrl;
	uintptr_t dsp_bfly0;
#endif
	uintptr_t acc0_ghi;
	uintptr_t acc0_glo;
	uintptr_t dsp_ctrl;
#endif

#ifdef CONFIG_ARC_AGU_SHARING
	uintptr_t agu_ap0;
	uintptr_t agu_ap1;
	uintptr_t agu_ap2;
	uintptr_t agu_ap3;
	uintptr_t agu_os0;
	uintptr_t agu_os1;
	uintptr_t agu_mod0;
	uintptr_t agu_mod1;
	uintptr_t agu_mod2;
	uintptr_t agu_mod3;
#ifdef CONFIG_ARC_AGU_MEDIUM
	uintptr_t agu_ap4;
	uintptr_t agu_ap5;
	uintptr_t agu_ap6;
	uintptr_t agu_ap7;
	uintptr_t agu_os2;
	uintptr_t agu_os3;
	uintptr_t agu_mod4;
	uintptr_t agu_mod5;
	uintptr_t agu_mod6;
	uintptr_t agu_mod7;
	uintptr_t agu_mod8;
	uintptr_t agu_mod9;
	uintptr_t agu_mod10;
	uintptr_t agu_mod11;
#endif
#ifdef CONFIG_ARC_AGU_LARGE
	uintptr_t agu_ap8;
	uintptr_t agu_ap9;
	uintptr_t agu_ap10;
	uintptr_t agu_ap11;
	uintptr_t agu_os4;
	uintptr_t agu_os5;
	uintptr_t agu_os6;
	uintptr_t agu_os7;
	uintptr_t agu_mod12;
	uintptr_t agu_mod13;
	uintptr_t agu_mod14;
	uintptr_t agu_mod15;
	uintptr_t agu_mod16;
	uintptr_t agu_mod17;
	uintptr_t agu_mod18;
	uintptr_t agu_mod19;
	uintptr_t agu_mod20;
	uintptr_t agu_mod21;
	uintptr_t agu_mod22;
	uintptr_t agu_mod23;
#endif
#endif
	/*
	 * No need to save r31 (blink), it's either already pushed as the pc or
	 * blink on an irq stack frame.
	 */
};

typedef struct _callee_saved_stack _callee_saved_stack_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_DATA_H_ */
