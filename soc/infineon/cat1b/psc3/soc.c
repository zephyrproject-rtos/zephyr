/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ifx_cycfg_init.h>

void soc_early_init_hook(void)
{
	ifx_cycfg_init();
}
