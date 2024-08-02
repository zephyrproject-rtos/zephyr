/**
 * @file
 * @brief RISCV GCC specific floating point register macros
 */

/*
 * Copyright (c) 2019, Huang Qi <757509347@qq.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_RISCV_GCC_H
#define _FLOAT_REGS_RISCV_GCC_H

#if !defined(__GNUC__) || !defined(CONFIG_RISCV)
#error __FILE__ goes only with RISCV GCC
#endif

#include <zephyr/toolchain.h>
#include "float_context.h"

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
#define RV_FPREG_WIDTH 8
#define RV_FPREG_SAVE "fsd "
#define RV_FPREG_LOAD "fld "
#else
#define RV_FPREG_WIDTH 4
#define RV_FPREG_SAVE "fsw "
#define RV_FPREG_LOAD "flw "
#endif

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
 * _load_all_float_registers() and _store_all_float_registers() agree
 * on the format.
 *
 */
static inline void _load_all_float_registers(struct fp_register_set *regs)
{
	__asm__(
		"mv  t0, %0\n"
		"mv  t1, %1\n"
		RV_FPREG_LOAD "f0, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f1, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f2, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f3, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f4, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f5, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f6, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f7, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f8, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f9, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f10, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f11, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f12, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f13, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f14, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f15, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f16, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f17, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f18, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f19, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f20, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f21, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f22, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f23, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f24, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f25, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f26, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f27, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f28, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f29, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f30, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_LOAD "f31, 0(t0)\n"
		:
		: "r"(regs), "r"(RV_FPREG_WIDTH)
		: "t0", "t1"
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
 */

static inline void _store_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile(
		"mv t0, %0\n\t"
		"mv t1, %1\n\t"
		RV_FPREG_SAVE "f0, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f1, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f2, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f3, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f4, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f5, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f6, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f7, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f8, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f9, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f10, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f11, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f12, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f13, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f14, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f15, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f16, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f17, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f18, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f19, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f20, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f21, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f22, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f23, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f24, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f25, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f26, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f27, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f28, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f29, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f30, 0(t0)\n"
		"add t0, t0, t1\n"
		RV_FPREG_SAVE "f31, 0(t0)\n"
		:
		: "r"(regs), "r"(RV_FPREG_WIDTH)
		: "t0", "t1", "memory"
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
 * thread.
 *
 */

static inline void _load_then_store_all_float_registers(struct fp_register_set
							*regs)
{
	_load_all_float_registers(regs);
	_store_all_float_registers(regs);
}
#endif /* _FLOAT_REGS_ARC_GCC_H */
