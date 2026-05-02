/**
 * @file
 * @brief ARM GCC specific floating point register macros
 */

/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_ARM_GCC_H
#define _FLOAT_REGS_ARM_GCC_H

#include <zephyr/toolchain.h>
#include "float_context.h"

#if defined(CONFIG_VFP_FEATURE_REGS_S64_D32)

static inline void _load_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"vldmia %0, {d0-d15};\n\t"
		"vldmia %1, {d16-d31};\n\t"
		: : "r" (&regs->fp_volatile), "r" (&regs->fp_non_volatile)
		);
}

static inline void _store_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"vstmia %0, {d0-d15};\n\t"
		"vstmia %1, {d16-d31};\n\t"
		: : "r" (&regs->fp_volatile), "r" (&regs->fp_non_volatile)
		: "memory"
		);
}

static inline void _load_then_store_all_float_registers(struct fp_register_set *regs)
{
	_load_all_float_registers(regs);
	_store_all_float_registers(regs);
}

#else

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
		"vldmia %0, {s0-s15};\n\t"
		"vldmia %1, {s16-s31};\n\t"
		: : "r" (&regs->fp_volatile), "r" (&regs->fp_non_volatile)
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
		"vstmia %0, {s0-s15};\n\t"
		"vstmia %1, {s16-s31};\n\t"
		: : "r" (&regs->fp_volatile), "r" (&regs->fp_non_volatile)
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

static inline void _load_then_store_all_float_registers(struct fp_register_set
							*regs)
{
	_load_all_float_registers(regs);
	_store_all_float_registers(regs);
}

#endif

#endif /* _FLOAT_REGS_ARM_GCC_H */
