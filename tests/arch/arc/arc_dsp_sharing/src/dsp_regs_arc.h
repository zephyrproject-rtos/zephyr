/*
 * Copyright (c) 2022, Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC specific dsp register macros
 */

#ifndef _DSP_REGS_ARC_H
#define _DSP_REGS_ARC_H

#include <zephyr/toolchain.h>
#include "dsp_context.h"

/**
 *
 * @brief Load all dsp registers
 *
 * This function loads all DSP and AGU registers pointed to by @a regs.
 * It is expected that a subsequent call to _store_all_dsp_registers()
 * will be issued to dump the dsp registers to memory.
 *
 * The format/organization of 'struct dsp_register_set'; the generic C test
 * code (main.c) merely treat the register set as an array of bytes.
 *
 * The only requirement is that the arch specific implementations of
 * _load_all_dsp_registers() and _store_all_dsp_registers() agree
 * on the format.
 *
 */
void _load_all_dsp_registers(struct dsp_register_set *regs)
{
	uint32_t *temp = (uint32_t *)regs;

	z_arc_v2_aux_reg_write(_ARC_V2_DSP_BFLY0, *temp++);
	z_arc_v2_aux_reg_write(_ARC_V2_AGU_AP0, *temp++);
	z_arc_v2_aux_reg_write(_ARC_V2_AGU_OS0, *temp++);
}

/**
 *
 * @brief Dump all dsp registers to memory
 *
 * This function stores all DSP and AGU registers to the memory buffer
 * specified by @a regs. It is expected that a previous invocation of
 * _load_all_dsp_registers() occurred to load all the dsp
 * registers from a memory buffer.
 *
 */

void _store_all_dsp_registers(struct dsp_register_set *regs)
{
	uint32_t *temp = (uint32_t *)regs;

	*temp++ = z_arc_v2_aux_reg_read(_ARC_V2_DSP_BFLY0);
	*temp++ = z_arc_v2_aux_reg_read(_ARC_V2_AGU_AP0);
	*temp++ = z_arc_v2_aux_reg_read(_ARC_V2_AGU_OS0);
}

/**
 *
 * @brief Load then dump all dsp registers to memory
 *
 * This function loads all DSP and AGU registers from the memory buffer
 * specified by @a regs, and then stores them back to that buffer.
 *
 * This routine is called by a high priority thread prior to calling a primitive
 * that pends and triggers a co-operative context switch to a low priority
 * thread.
 *
 */

void _load_then_store_all_dsp_registers(struct dsp_register_set *regs)
{
	_load_all_dsp_registers(regs);
	_store_all_dsp_registers(regs);
}
#endif /* _DSP_REGS_ARC_H */
