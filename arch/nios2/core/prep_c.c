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
 * call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <zephyr/types.h>
#include <toolchain.h>
#include <linker/linker-defs.h>
#include <kernel_structs.h>
#include <kernel_internal.h>

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
	z_bss_zero();
#ifdef CONFIG_XIP
	z_data_copy();
	/* In most XIP scenarios we copy the exception code into RAM, so need
	 * to flush instruction cache.
	 */
	z_nios2_icache_flush_all();
#if ALT_CPU_ICACHE_SIZE > 0
	/* Only need to flush the data cache here if there actually is an
	 * instruction cache, so that the cached instruction data written is
	 * actually committed.
	 */
	z_nios2_dcache_flush_all();
#endif
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
