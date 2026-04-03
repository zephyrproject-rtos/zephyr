/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_XTENSA_H
#define _FLOAT_REGS_XTENSA_H

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
	__asm__ volatile("wfr f0, %0\n" :: "r"(regs->fp_non_volatile.reg[0]));
	__asm__ volatile("wfr f1, %0\n" :: "r"(regs->fp_non_volatile.reg[1]));
	__asm__ volatile("wfr f2, %0\n" :: "r"(regs->fp_non_volatile.reg[2]));
	__asm__ volatile("wfr f3, %0\n" :: "r"(regs->fp_non_volatile.reg[3]));
	__asm__ volatile("wfr f4, %0\n" :: "r"(regs->fp_non_volatile.reg[4]));
	__asm__ volatile("wfr f5, %0\n" :: "r"(regs->fp_non_volatile.reg[5]));
	__asm__ volatile("wfr f6, %0\n" :: "r"(regs->fp_non_volatile.reg[6]));
	__asm__ volatile("wfr f7, %0\n" :: "r"(regs->fp_non_volatile.reg[7]));
	__asm__ volatile("wfr f8, %0\n" :: "r"(regs->fp_non_volatile.reg[8]));
	__asm__ volatile("wfr f9, %0\n" :: "r"(regs->fp_non_volatile.reg[9]));
	__asm__ volatile("wfr f10, %0\n" :: "r"(regs->fp_non_volatile.reg[10]));
	__asm__ volatile("wfr f11, %0\n" :: "r"(regs->fp_non_volatile.reg[11]));
	__asm__ volatile("wfr f12, %0\n" :: "r"(regs->fp_non_volatile.reg[12]));
	__asm__ volatile("wfr f13, %0\n" :: "r"(regs->fp_non_volatile.reg[13]));
	__asm__ volatile("wfr f14, %0\n" :: "r"(regs->fp_non_volatile.reg[14]));
	__asm__ volatile("wfr f15, %0\n" :: "r"(regs->fp_non_volatile.reg[15]));
	__asm__ volatile("wur.fsr %0\n" :: "r"(regs->fp_non_volatile.reg[16]));
	__asm__ volatile("wur.fcr %0\n" :: "r"(regs->fp_non_volatile.reg[17]));
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
	__asm__ volatile("rfr %0, f0\n" : "=r"(regs->fp_non_volatile.reg[0]));
	__asm__ volatile("rfr %0, f1\n" : "=r"(regs->fp_non_volatile.reg[1]));
	__asm__ volatile("rfr %0, f2\n" : "=r"(regs->fp_non_volatile.reg[2]));
	__asm__ volatile("rfr %0, f3\n" : "=r"(regs->fp_non_volatile.reg[3]));
	__asm__ volatile("rfr %0, f4\n" : "=r"(regs->fp_non_volatile.reg[4]));
	__asm__ volatile("rfr %0, f5\n" : "=r"(regs->fp_non_volatile.reg[5]));
	__asm__ volatile("rfr %0, f6\n" : "=r"(regs->fp_non_volatile.reg[6]));
	__asm__ volatile("rfr %0, f7\n" : "=r"(regs->fp_non_volatile.reg[7]));
	__asm__ volatile("rfr %0, f8\n" : "=r"(regs->fp_non_volatile.reg[8]));
	__asm__ volatile("rfr %0, f9\n" : "=r"(regs->fp_non_volatile.reg[9]));
	__asm__ volatile("rfr %0, f10\n" : "=r"(regs->fp_non_volatile.reg[10]));
	__asm__ volatile("rfr %0, f11\n" : "=r"(regs->fp_non_volatile.reg[11]));
	__asm__ volatile("rfr %0, f12\n" : "=r"(regs->fp_non_volatile.reg[12]));
	__asm__ volatile("rfr %0, f13\n" : "=r"(regs->fp_non_volatile.reg[13]));
	__asm__ volatile("rfr %0, f14\n" : "=r"(regs->fp_non_volatile.reg[14]));
	__asm__ volatile("rfr %0, f15\n" : "=r"(regs->fp_non_volatile.reg[15]));
	__asm__ volatile("rur.fsr %0\n" : "=r"(regs->fp_non_volatile.reg[16]));
	__asm__ volatile("rur.fcr %0\n" : "=r"(regs->fp_non_volatile.reg[17]));
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

#endif /* _FLOAT_REGS_XTENSA_H */
