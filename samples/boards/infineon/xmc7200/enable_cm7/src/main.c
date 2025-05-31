/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "cy_pdl.h"

#define ENABLE_HEART_BEAT 0

int main(void)
{
	/* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
	printf("\x1b[2J\x1b[;H");

	k_msleep(1000);
	printf("=== CM0 - %s\n", CONFIG_BOARD_TARGET);

#if !defined(ENABLE_CM7_0) || ENABLE_CM7_0
	printf("=== CM0: Enable CM7_0\n");
	k_msleep(1000);
	Cy_SysEnableCM7(CORE_CM7_0, CY_CORTEX_M7_0_APPL_ADDR);
#endif

#if !defined(ENABLE_CM7_1) || ENABLE_CM7_1
	k_msleep(1000);
	printf("=== CM0: Enable CM7_1\n");
	k_msleep(1000);
	Cy_SysEnableCM7(CORE_CM7_1, CY_CORTEX_M7_1_APPL_ADDR);
#endif

	for (;;) {
#if ENABLE_HEART_BEAT
		k_msleep(1000);
		printf(".");
#endif
	}
}
