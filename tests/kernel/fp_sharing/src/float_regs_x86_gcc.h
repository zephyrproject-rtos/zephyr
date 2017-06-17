/**
 * @file
 * @brief Intel x86 GCC specific floating point register macros
 */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_X86_GCC_H
#define _FLOAT_REGS_X86_GCC_H

#if !defined(__GNUC__) || !defined(CONFIG_ISA_IA32)
#error __FILE__ goes only with x86 GCC
#endif

#include <toolchain.h>
#include "float_context.h"

/**
 *
 * @brief Load all floating point registers
 *
 * This function loads ALL floating point registers pointed to by @a regs.
 * It is expected that a subsequent call to _store_all_float_registers()
 * will be issued to dump the floating point registers to memory.
 *
 * The format/organization of 'struct fp_register_set'; the generic C test
 * code (main.c) merely treat the register set as an array of bytes.
 *
 * The only requirement is that the arch specific implementations of
 * _load_all_float_registers(), _store_all_float_registers() and
 * _load_then_store_all_float_registers() agree on the format.
 *
 * @return N/A
 */

static inline void _load_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"movdqu  0(%0), %%xmm0\n\t;"
		"movdqu 16(%0), %%xmm1\n\t;"
		"movdqu 32(%0), %%xmm2\n\t;"
		"movdqu 48(%0), %%xmm3\n\t;"
		"movdqu 64(%0), %%xmm4\n\t;"
		"movdqu 80(%0), %%xmm5\n\t;"
		"movdqu 96(%0), %%xmm6\n\t;"
		"movdqu 112(%0), %%xmm7\n\t;"

		"fldt   128(%0)\n\t;"
		"fldt   138(%0)\n\t;"
		"fldt   148(%0)\n\t;"
		"fldt   158(%0)\n\t;"
		"fldt   168(%0)\n\t;"
		"fldt   178(%0)\n\t;"
		"fldt   188(%0)\n\t;"
		"fldt   198(%0)\n\t;"

		: : "r" (regs)
		);
}


/**
 *
 * @brief Load then dump all float registers to memory
 *
 * This function loads ALL floating point registers from the memory buffer
 * specified by @a regs, and then stores them back to that buffer.
 *
 * This routine is called by a high priority thread prior to calling a primitive
 * that pends and triggers a co-operative context switch to a low priority
 * thread. Because the kernel doesn't save floating point context for
 * co-operative context switches, the x87 FPU register stack must be put back
 * in an empty state before the switch occurs in case the next task to perform
 * floating point operations was also co-operatively switched out and simply
 * inherits the existing x87 FPU state (expecting the stack to be empty).
 *
 * @return N/A
 */

static inline void
_load_then_store_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"movdqu  0(%0), %%xmm0\n\t;"
		"movdqu 16(%0), %%xmm1\n\t;"
		"movdqu 32(%0), %%xmm2\n\t;"
		"movdqu 48(%0), %%xmm3\n\t;"
		"movdqu 64(%0), %%xmm4\n\t;"
		"movdqu 80(%0), %%xmm5\n\t;"
		"movdqu 96(%0), %%xmm6\n\t;"
		"movdqu 112(%0), %%xmm7\n\t;"

		"fldt   128(%0)\n\t;"
		"fldt   138(%0)\n\t;"
		"fldt   148(%0)\n\t;"
		"fldt   158(%0)\n\t;"
		"fldt   168(%0)\n\t;"
		"fldt   178(%0)\n\t;"
		"fldt   188(%0)\n\t;"
		"fldt   198(%0)\n\t;"

		/* pop the x87 FPU registers back to memory */

		"fstpt  198(%0)\n\t;"
		"fstpt  188(%0)\n\t;"
		"fstpt  178(%0)\n\t;"
		"fstpt  168(%0)\n\t;"
		"fstpt  158(%0)\n\t;"
		"fstpt  148(%0)\n\t;"
		"fstpt  138(%0)\n\t;"
		"fstpt  128(%0)\n\t;"

		: : "r" (regs)
		);
}


/**
 *
 * @brief Dump all floating point registers to memory
 *
 * This function stores ALL floating point registers to the memory buffer
 * specified by @a regs. It is expected that a previous invocation of
 * _load_all_float_registers() occurred to load all the floating point
 * registers from a memory buffer.
 *
 * @return N/A
 */

static inline void _store_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"movdqu %%xmm0, 0(%0)\n\t;"
		"movdqu %%xmm1, 16(%0)\n\t;"
		"movdqu %%xmm2, 32(%0)\n\t;"
		"movdqu %%xmm3, 48(%0)\n\t;"
		"movdqu %%xmm4, 64(%0)\n\t;"
		"movdqu %%xmm5, 80(%0)\n\t;"
		"movdqu %%xmm6, 96(%0)\n\t;"
		"movdqu %%xmm7, 112(%0)\n\t;"

		"fstpt  198(%0)\n\t;"
		"fstpt  188(%0)\n\t;"
		"fstpt  178(%0)\n\t;"
		"fstpt  168(%0)\n\t;"
		"fstpt  158(%0)\n\t;"
		"fstpt  148(%0)\n\t;"
		"fstpt  138(%0)\n\t;"
		"fstpt  128(%0)\n\t;"

		: : "r" (regs) : "memory"
		);
}
#endif /* _FLOAT_REGS_X86_GCC_H */
