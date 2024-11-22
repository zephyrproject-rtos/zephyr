/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <stdint.h>

#if defined(CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUNET) || defined(CONFIG_SOC_SERIES_BSIM_NRF52X) ||  \
	defined(CONFIG_SOC_SERIES_BSIM_NRF54LX)

#define __SYSTEM_CLOCK_DEFAULT (64000000UL)

uint32_t SystemCoreClock = __SYSTEM_CLOCK_DEFAULT;

__weak void SystemCoreClockUpdate(void)
{
	SystemCoreClock = __SYSTEM_CLOCK_DEFAULT;
}

#elif defined(CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP)

#include "nrfx.h"

/* NRF5340 application core uses a variable System Clock Frequency that starts at 64MHz */
#define __SYSTEM_CLOCK_MAX     (128000000UL)
#define __SYSTEM_CLOCK_DEFAULT (64000000UL)

uint32_t SystemCoreClock = __SYSTEM_CLOCK_DEFAULT;

__weak void SystemCoreClockUpdate(void)
{
	SystemCoreClock =
		__SYSTEM_CLOCK_MAX >> (NRF_CLOCK_S->HFCLKCTRL & (CLOCK_HFCLKCTRL_HCLK_Msk));
}

#endif
