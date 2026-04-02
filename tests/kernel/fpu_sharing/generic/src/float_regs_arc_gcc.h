/**
 * @file
 * @brief ARC GCC specific floating point register macros
 */

/*
 * Copyright (c) 2019, Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_ARC_GCC_H
#define _FLOAT_REGS_ARC_GCC_H

#if !defined(__GNUC__) || !defined(CONFIG_ISA_ARCV2)
#error __FILE__ goes only with ARC GCC
#endif

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
#ifdef CONFIG_FP_FPU_DA
	uint32_t temp = 0;

	__asm__ volatile (
		"ld.ab %1, [%0, 4];\n\t"
		"sr %1, [%2];\n\t"
		"ld.ab %1, [%0, 4];\n\t"
		"sr %1, [%3];\n\t"
		"ld.ab %1, [%0, 4];\n\t"
		"sr %1, [%4];\n\t"
		"ld.ab %1, [%0, 4];\n\t"
		"sr %1, [%5];\n\t"
		: : "r" (regs), "r" (temp),
		"i"(_ARC_V2_FPU_DPFP1L), "i"(_ARC_V2_FPU_DPFP1H),
		"i"(_ARC_V2_FPU_DPFP2L), "i"(_ARC_V2_FPU_DPFP2H)
		: "memory"
		);
#endif
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
#ifdef CONFIG_FP_FPU_DA
	uint32_t temp = 0;

	__asm__ volatile (
		"lr %1, [%2];\n\t"
		"st.ab %1, [%0, 4];\n\t"
		"lr %1, [%3];\n\t"
		"st.ab %1, [%0, 4];\n\t"
		"lr %1, [%4];\n\t"
		"st.ab %1, [%0, 4];\n\t"
		"lr %1, [%5];\n\t"
		"st.ab %1, [%0, 4];\n\t"
		: : "r" (regs), "r" (temp),
		"i"(_ARC_V2_FPU_DPFP1L), "i"(_ARC_V2_FPU_DPFP1H),
		"i"(_ARC_V2_FPU_DPFP2L), "i"(_ARC_V2_FPU_DPFP2H)
		);
#endif
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
