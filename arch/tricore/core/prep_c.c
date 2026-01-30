/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy .data and call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel_internal.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/common/xip.h>

extern void z_tricore_mpu_init(void);

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

void z_prep_c(void)
{
	arch_bss_zero();
	arch_data_copy();

	z_cstart();
	CODE_UNREACHABLE;
}
