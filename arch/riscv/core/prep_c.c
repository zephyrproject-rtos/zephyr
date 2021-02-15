/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss and call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <stddef.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <core_pmp.h>

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
#endif
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
#ifdef CONFIG_PMP_STACK_GUARD
	z_riscv_configure_interrupt_stack_guard();
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
