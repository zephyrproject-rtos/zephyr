/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_sysint.h>
#include <system_cat2.h>/* PSoC4 system init header from PDL */
#include "cy_pdl.h"

/* Minimal early initialization for PSoC4100tp (CAT2) */
void soc_early_init_hook(void)
{
	/* Initializes the system */
	SystemInit();
}
