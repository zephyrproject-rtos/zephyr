/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/irq.h>
#include <zephyr/platform/hooks.h>

/**
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */
void z_prep_c(void)
{
	soc_prep_hook();
	z_cstart();
	CODE_UNREACHABLE;
}
