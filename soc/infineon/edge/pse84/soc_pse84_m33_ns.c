/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 soc.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#define PSE84_CPU_FREQ_HZ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)


void soc_early_init_hook(void)
{
	/* Initialize SystemCoreClock variable. */
	SystemCoreClockSetup(PSE84_CPU_FREQ_HZ, PSE84_CPU_FREQ_HZ);
}

void soc_late_init_hook(void)
{
#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
	ifx_pse84_cm55_startup();
#endif
}
