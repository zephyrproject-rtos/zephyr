/**
 * @file
 * @brief ARM64 GCC specific floating point register macros
 */

/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_ARM64_GCC_H
#define _FLOAT_REGS_ARM64_GCC_H

#include <zephyr/toolchain.h>
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
 * _load_all_float_registers() and _store_all_float_registers() agree
 * on the format.
 *
 */
static inline void _load_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
	"ldp	q0,  q1,  [x0, #(16 *  0)]\n\t"
	"ldp	q2,  q3,  [x0, #(16 *  2)]\n\t"
	"ldp	q4,  q5,  [x0, #(16 *  4)]\n\t"
	"ldp	q6,  q7,  [x0, #(16 *  6)]\n\t"
	"ldp	q8,  q9,  [x0, #(16 *  8)]\n\t"
	"ldp	q10, q11, [x0, #(16 * 10)]\n\t"
	"ldp	q12, q13, [x0, #(16 * 12)]\n\t"
	"ldp	q14, q15, [x0, #(16 * 14)]\n\t"
	"ldp	q16, q17, [x0, #(16 * 16)]\n\t"
	"ldp	q18, q19, [x0, #(16 * 18)]\n\t"
	"ldp	q20, q21, [x0, #(16 * 20)]\n\t"
	"ldp	q22, q23, [x0, #(16 * 22)]\n\t"
	"ldp	q24, q25, [x0, #(16 * 24)]\n\t"
	"ldp	q26, q27, [x0, #(16 * 26)]\n\t"
	"ldp	q28, q29, [x0, #(16 * 28)]\n\t"
	"ldp	q30, q31, [x0, #(16 * 30)]"
	:
	: "r" (regs)
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
	__asm__ volatile (
	"stp	q0,  q1,  [x0, #(16 *  0)]\n\t"
	"stp	q2,  q3,  [x0, #(16 *  2)]\n\t"
	"stp	q4,  q5,  [x0, #(16 *  4)]\n\t"
	"stp	q6,  q7,  [x0, #(16 *  6)]\n\t"
	"stp	q8,  q9,  [x0, #(16 *  8)]\n\t"
	"stp	q10, q11, [x0, #(16 * 10)]\n\t"
	"stp	q12, q13, [x0, #(16 * 12)]\n\t"
	"stp	q14, q15, [x0, #(16 * 14)]\n\t"
	"stp	q16, q17, [x0, #(16 * 16)]\n\t"
	"stp	q18, q19, [x0, #(16 * 18)]\n\t"
	"stp	q20, q21, [x0, #(16 * 20)]\n\t"
	"stp	q22, q23, [x0, #(16 * 22)]\n\t"
	"stp	q24, q25, [x0, #(16 * 24)]\n\t"
	"stp	q26, q27, [x0, #(16 * 26)]\n\t"
	"stp	q28, q29, [x0, #(16 * 28)]\n\t"
	"stp	q30, q31, [x0, #(16 * 30)]"
	:
	: "r" (regs)
	: "memory"
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

static inline void _load_then_store_all_float_registers(
						struct fp_register_set *regs)
{
	_load_all_float_registers(regs);
	_store_all_float_registers(regs);
}
#endif /* _FLOAT_REGS_ARM64_GCC_H */
