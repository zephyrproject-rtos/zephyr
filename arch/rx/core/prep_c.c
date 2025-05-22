/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
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

#include <kernel_internal.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_initialization_process_stacks, CONFIG_MP_MAX_NUM_CPUS,
				   CONFIG_INITIALIZATION_STACK_SIZE);
/**
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */
void z_prep_c(void)
{
	z_bss_zero();

	z_data_copy();

	z_cstart();
	CODE_UNREACHABLE;
}
