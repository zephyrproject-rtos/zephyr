/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon CAT1A M0+ SoC.
 */

#include "cy_device.h"
#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_sysint.h>
#include <cy_wdt.h>
#include <cy_sysclk.h>

#if (CONFIG_SOC_DIE_CYT2B7)
#include "tviibe1m_config.h"
#elif (CONFIG_SOC_DIE_CYT2BL)
#include "tviibe4m_config.h"
#endif

#ifdef CONFIG_SOC_DIE_CYT2B7
#define M4_FREQ DT_PROP(DT_NODELABEL(m4), clock_frequency)
#define IFX_FAST_CLOCK_DOMAIN_FREQ (M4_FREQ / 1000000)
#endif

void soc_prep_hook(void)
{
	Cy_WDT_Unlock();
	Cy_WDT_Disable();
}

static int early_init() {
	Cy_PDL_Init(CY_DEVICE_CFG);
	SystemCoreClockUpdate();
#ifdef CONFIG_SOC_DIE_CYT2B7
	Cy_SysLib_SetWaitStates(false, IFX_FAST_CLOCK_DOMAIN_FREQ);
#endif
	return 0;
}

static int soc_start_cm4()
{
#if defined(CONFIG_SOC_CYT2BX_START_M4)
	Cy_SysEnableCM4(DT_REG_ADDR(DT_NODELABEL(m4_partition)));
#endif
	return 0;
}

SYS_INIT(early_init, PRE_KERNEL_1, 0);
SYS_INIT(soc_start_cm4, PRE_KERNEL_2, 0);
