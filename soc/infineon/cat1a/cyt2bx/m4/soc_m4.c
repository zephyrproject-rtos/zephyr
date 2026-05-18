/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon CAT1A SOC.
 */

#include <zephyr/init.h>
#include <cy_wdt.h>

void soc_prep_hook(void)
{
	Cy_WDT_Unlock();
	Cy_WDT_Disable();
}

static int early_init() {
	Cy_PDL_Init(CY_DEVICE_CFG);
	SystemCoreClockUpdate();

	return 0;
}
SYS_INIT(early_init, PRE_KERNEL_1, 0);
