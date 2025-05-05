/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "cy_pdl.h"

#undef CY_CORTEX_M7_0_APPL_ADDR
#undef CY_CORTEX_M7_1_APPL_ADDR
#define CY_CORTEX_M7_0_APPL_ADDR 0x10080000
#define CY_CORTEX_M7_1_APPL_ADDR 0x10280000

#define ENABLE_HEART_BEAT 0

int main(void)
{
	/* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
	printf("\x1b[2J\x1b[;H");

	printf("CM0 - %s\n", CONFIG_BOARD_TARGET);

#if CONFIG_ENABLE_CM7_0
	k_msleep(1000);
	Cy_SysEnableCM7(CORE_CM7_0, CY_CORTEX_M7_0_APPL_ADDR);
#endif

#if CONFIG_ENABLE_CM7_1
	k_msleep(1000);
	Cy_SysEnableCM7(CORE_CM7_1, CY_CORTEX_M7_1_APPL_ADDR);
#endif

	for (;;) {
		/* TODO: Add support for deep sleep */
		/* Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT); */
#if ENABLE_HEART_BEAT
		k_msleep(1000);
		printf(".");
#endif
	}
}
