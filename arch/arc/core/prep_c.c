/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call _Cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <zephyr/types.h>
#include <toolchain.h>
#include <linker/linker-defs.h>
#include <arch/arc/v2/aux_regs.h>
#include <kernel_structs.h>
#include <nano_internal.h>


/* XXX - keep for future use in full-featured cache APIs */
#if 0
/**
 *
 * @brief Disable the i-cache if present
 *
 * For those ARC CPUs that have a i-cache present,
 * invalidate the i-cache and then disable it.
 *
 * @return N/A
 */

static void disable_icache(void)
{
	unsigned int val;

	val = _arc_v2_aux_reg_read(_ARC_V2_I_CACHE_BUILD);
	val &= 0xff; /* version field */
	if (val == 0) {
		return; /* skip if i-cache is not present */
	}
	_arc_v2_aux_reg_write(_ARC_V2_IC_IVIC, 0);
	__asm__ __volatile__ ("nop");
	_arc_v2_aux_reg_write(_ARC_V2_IC_CTRL, 1);
}

/**
 *
 * @brief Invalidate the data cache if present
 *
 * For those ARC CPUs that have a data cache present,
 * invalidate the data cache.
 *
 * @return N/A
 */

static void invalidate_dcache(void)
{
	unsigned int val;

	val = _arc_v2_aux_reg_read(_ARC_V2_D_CACHE_BUILD);
	val &= 0xff; /* version field */
	if (val == 0) {
		return; /* skip if d-cache is not present */
	}
	_arc_v2_aux_reg_write(_ARC_V2_DC_IVDC, 1);
}
#endif

/**
 *
 * @brief Adjust the vector table base
 *
 * Set the vector table base if the value found in the
 * _ARC_V2_IRQ_VECT_BASE auxiliary register is different from the
 * _VectorTable known by software. It is important to do this very early
 * so that exception vectors can be handled.
 *
 * @return N/A
 */

static void adjust_vector_table_base(void)
{
#ifdef CONFIG_ARC_HAS_SECURE
#undef _ARC_V2_IRQ_VECT_BASE
#define _ARC_V2_IRQ_VECT_BASE _ARC_V2_IRQ_VECT_BASE_S
#endif
	extern struct vector_table _VectorTable;
	unsigned int vbr;
	/* if the compiled-in vector table is different
	 * from the base address known by the ARC CPU,
	 * set the vector base to the compiled-in address.
	 */
	vbr = _arc_v2_aux_reg_read(_ARC_V2_IRQ_VECT_BASE);
	vbr &= 0xfffffc00;
	if (vbr != (unsigned int)&_VectorTable) {
		_arc_v2_aux_reg_write(_ARC_V2_IRQ_VECT_BASE,
					(unsigned int)&_VectorTable);
	}
}

extern FUNC_NORETURN void _Cstart(void);
/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void _PrepC(void)
{
	_icache_setup();
	adjust_vector_table_base();
	_bss_zero();
	_data_copy();
	_Cstart();
	CODE_UNREACHABLE;
}
