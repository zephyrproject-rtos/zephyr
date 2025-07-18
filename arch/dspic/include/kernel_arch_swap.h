/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel arch swap functions
 *
 * This file contains function helpers for dspic arch specific thread swap
 * helper functions
 */

#ifndef ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_SWAP_H_
#define ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_SWAP_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/kernel_structs.h>
#include <xc.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "kswap.h"

static inline __attribute__((always_inline)) void z_dspic_save_context(void)
{
	/*Adjust stack for co-operative swap*/
	__asm__ volatile(
		/*Check if in interrupt context*/
		"mov.l w0, [w15++]\n\t"
		"mov.l sr, w0\n\t"
		"and #0xe0, w0\n\t"
		"bra nz, 1f\n\t"

		/*Not in interrupt context*/
		"mov.l [--w15], w0\n\t"
		/*This unlink is needed to match esf*/
		"ulnk\n\t"
		"push RCOUNT\n\t"
		"push.l fsr\n\t"
		"push.l fcr\n\t"
		"mov.l w0, [w15++]\n\t"
		"mov.l w1, [w15++]\n\t"
		"mov.l w2, [w15++]\n\t"
		"mov.l w3, [w15++]\n\t"
		"mov.l w4, [w15++]\n\t"
		"mov.l w5, [w15++]\n\t"
		"mov.l w6, [w15++]\n\t"
		"mov.l w7, [w15++]\n\t"
		"mov.l w8, [w15++]\n\t"
		"push.l f0\n\t"
		"push.l f1\n\t"
		"push.l f2\n\t"
		"push.l f3\n\t"
		"push.l f4\n\t"
		"push.l f5\n\t"
		"push.l f6\n\t"
		"push.l f7\n\t"
		"lnk #0x0\n\t"
		/*in isr lnk is done after esf push*/
		"mov.l w0, [w15++]\n\t"

		/*In interrupt context*/
		"1:\n\t"
		"mov.l [--w15], w0\n\t");

	/* Get the current thread callee_saved context
	 * TODO:
	 * Need to change constant 0x8, 0x28 with offset symbols
	 * ___cpu_t_current_OFFSET, ___thread_t_callee_saved_OFFSET
	 */
	__asm__ volatile("mov.l #__kernel, w0\n\t"
			 "mov.l #0x8, w1\n\t"
			 "add w0, w1, w1\n\t"
			 "mov.l [w1], w2\n\t"
			 "mov.l #0x28, w1\n\t"
			 "add w2, w1, w1\n\t");

	/*Save all callee saved registers*/
	__asm__ volatile("mov.l w8, [w1++]\n\t"
			 "mov.l w9, [w1++]\n\t"
			 "mov.l w10, [w1++]\n\t"
			 "mov.l w11, [w1++]\n\t"
			 "mov.l w12, [w1++]\n\t"
			 "mov.l w13, [w1++]\n\t"
			 "mov.l w14, [w1++]\n\t"

			 "mov.l f8, [w1++]\n\t"
			 "mov.l f9, [w1++]\n\t"
			 "mov.l f10, [w1++]\n\t"
			 "mov.l f11, [w1++]\n\t"
			 "mov.l f12, [w1++]\n\t"
			 "mov.l f13, [w1++]\n\t"
			 "mov.l f14, [w1++]\n\t"
			 "mov.l f15, [w1++]\n\t"
			 "mov.l f16, [w1++]\n\t"
			 "mov.l f17, [w1++]\n\t"
			 "mov.l f18, [w1++]\n\t"
			 "mov.l f19, [w1++]\n\t"
			 "mov.l f20, [w1++]\n\t"
			 "mov.l f21, [w1++]\n\t"
			 "mov.l f22, [w1++]\n\t"
			 "mov.l f23, [w1++]\n\t"
			 "mov.l f24, [w1++]\n\t"
			 "mov.l f25, [w1++]\n\t"
			 "mov.l f26, [w1++]\n\t"
			 "mov.l f27, [w1++]\n\t"
			 "mov.l f28, [w1++]\n\t"
			 "mov.l f29, [w1++]\n\t"
			 "mov.l f30, [w1++]\n\t"
			 "mov.l f31, [w1++]\n\t"

			 "mov.l #RCOUNT, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #CORCON, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #MODCON, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #XMODSRT, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #XMODEND, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #YMODSRT, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #YMODEND, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"
			 "mov.l #XBREV, w2\n\t"
			 "mov.l [w2], [w1++]\n\t"

			 "clr A\n\t"
			 "clr B\n\t"
			 "slac.l A, [W1++]\n\t"
			 "sac.l A, [W1++]\n\t"
			 "suac.l A, [W1++]\n\t"
			 "slac.l B, [W1++]\n\t"
			 "sac.l B, [W1++]\n\t"
			 "suac.l B, [W1++]\n\t"

			 "mov.l w15, [w1++]\n\t"
			 "mov.l w14, [w1++]\n\t"
			 "mov.l #SPLIM, w2\n\t"
			 "mov.l [w2], [w1++]\n\t");
}

static inline __attribute__((always_inline)) void z_dspic_restore_context(void)
{
	/* Get the current thread callee_saved context
	 * TODO:
	 * Need to change constant 0x8, 0x28 with offset symbols
	 * ___cpu_t_current_OFFSET, ___thread_t_callee_saved_OFFSET
	 */
	__asm__ volatile("mov.l #__kernel, w0\n\t"
			 "mov.l #0x8, w1\n\t"
			 "add w0, w1, w1\n\t"
			 "mov.l [w1], w2\n\t"
			 "mov.l #0x28, w1\n\t"
			 "add w2, w1, w1\n\t");

	/*Restore all registers*/
	__asm__ volatile("mov.l [w1++], w8\n\t"
			 "mov.l [w1++], w9\n\t"
			 "mov.l [w1++], w10\n\t"
			 "mov.l [w1++], w11\n\t"
			 "mov.l [w1++], w12\n\t"
			 "mov.l [w1++], w13\n\t"
			 "mov.l [w1++], w14\n\t"

			 "mov.l [w1++], f8\n\t"
			 "mov.l [w1++], f9\n\t"
			 "mov.l [w1++], f10\n\t"
			 "mov.l [w1++], f11\n\t"
			 "mov.l [w1++], f12\n\t"
			 "mov.l [w1++], f13\n\t"
			 "mov.l [w1++], f14\n\t"
			 "mov.l [w1++], f15\n\t"
			 "mov.l [w1++], f16\n\t"
			 "mov.l [w1++], f17\n\t"
			 "mov.l [w1++], f18\n\t"
			 "mov.l [w1++], f19\n\t"
			 "mov.l [w1++], f20\n\t"
			 "mov.l [w1++], f21\n\t"
			 "mov.l [w1++], f22\n\t"
			 "mov.l [w1++], f23\n\t"
			 "mov.l [w1++], f24\n\t"
			 "mov.l [w1++], f25\n\t"
			 "mov.l [w1++], f26\n\t"
			 "mov.l [w1++], f27\n\t"
			 "mov.l [w1++], f28\n\t"
			 "mov.l [w1++], f29\n\t"
			 "mov.l [w1++], f30\n\t"
			 "mov.l [w1++], f31\n\t"

			 "mov.l #RCOUNT, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #CORCON, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #MODCON, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #XMODSRT, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #XMODEND, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #YMODSRT, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #YMODEND, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"
			 "mov.l #XBREV, w2\n\t"
			 "mov.l [w1++], [w2]\n\t"

			 "clr A\n\t"
			 "clr B\n\t"
			 "slac.l A, [W1++]\n\t"
			 "sac.l A, [W1++]\n\t"
			 "suac.l A, [W1++]\n\t"
			 "slac.l B, [W1++]\n\t"
			 "sac.l B, [W1++]\n\t"
			 "suac.l B, [W1++]\n\t"

			 "mov.l [w1++], w15\n\t"
			 "mov.l [w1++], w14\n\t"
			 "mov.l #SPLIM, w2\n\t"
			 "mov.l [w1++], [w2]\n\t");

	/*pop exception/swap saved stack frame*/
	__asm__ volatile(
		/* Check context and only pop the
		 * esf if in thread context
		 */
		"mov.l w0, [w15++]\n\t"
		"mov.l sr, w0\n\t"
		"and #0xe0, w0\n\t"
		"mov.l [--w15], w0\n\t"
		"bra nz, 1f\n\t"

		/*in isr the unlink is done before esf pop*/
		"ulnk\n\t"
		/*Thread context*/
		"pop.l f7\n\t"
		"pop.l f6\n\t"
		"pop.l f5\n\t"
		"pop.l f4\n\t"
		"pop.l f3\n\t"
		"pop.l f2\n\t"
		"pop.l f1\n\t"
		"pop.l f0\n\t"
		"mov.l [--w15], w8\n\t"
		"mov.l [--w15], w7\n\t"
		"mov.l [--w15], w6\n\t"
		"mov.l [--w15], w5\n\t"
		"mov.l [--w15], w4\n\t"
		"mov.l [--w15], w3\n\t"
		"mov.l [--w15], w2\n\t"
		"mov.l [--w15], w1\n\t"
		"mov.l [--w15], w0\n\t"
		"pop.l fcr\n\t"
		"pop.l fsr\n\t"
		"pop RCOUNT\n\t"
		"lnk #0x0\n\t"

		/*Interrupt context*/
		"1:\n\t"
		"nop\n\t");
}

/* routine which swaps the context. Needs to be written in assembly */
static inline __attribute__((always_inline)) void z_dspic_do_swap(void)
{
	z_dspic_save_context();

    /*Switch to next task in queue*/
	z_current_thread_set(_kernel.ready_q.cache);

	z_dspic_restore_context();
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_SWAP_H_ */
