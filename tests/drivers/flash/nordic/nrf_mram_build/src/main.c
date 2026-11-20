/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_RISCV_CORE_NORDIC_VPR)
BUILD_ASSERT(!IS_ENABLED(CONFIG_SOC_FLASH_NRF_MRAM),
	     "The MRAM flash driver must not be built for the VPR cores");
#endif

/* Build-only test: it just needs to compile and link the MRAM flash driver. */
int main(void)
{
	return 0;
}
