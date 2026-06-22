/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_subsys.h"
#if defined CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X
#include <zephyr/drivers/clock_control/mchp_clock_sam_d5x_e5x.h>
#include <zephyr/dt-bindings/clock/mchp_sam_d5x_e5x_clock.h>
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X */

#if defined CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH
#include <zephyr/drivers/clock_control/mchp_clock_pic32cm_jh.h>
#include <zephyr/dt-bindings/clock/mchp_pic32cm_jh_clock.h>
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH */

#define XOSC_STARTUP_US 500

static const struct device_subsys_data subsys_data[] = {
#if defined CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X
	{.subsys = (void *)CLOCK_MCHP_MCLKPERIPH_ID_APBB_SERCOM3},
	{.subsys = (void *)CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_CORE},
	{
		.subsys = (void *)CLOCK_MCHP_XOSC_ID_XOSC1,
		.startup_us = XOSC_STARTUP_US,
	},
	{.subsys = (void *)CLOCK_MCHP_XOSC32K_ID_XOSC32K}
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X */

#if defined CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH
	{.subsys = (void *)CLOCK_MCHP_MCLKPERIPH_ID_APBC_SERCOM1},
	{.subsys = (void *)CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_CORE},
	{
		.subsys = (void *)CLOCK_MCHP_XOSC_ID,
		.startup_us = XOSC_STARTUP_US,
	},
	{.subsys = (void *)CLOCK_MCHP_XOSC32K_ID_XOSC1K}
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH */

};

static const struct device_data devices[] = {{.dev = DEVICE_DT_GET(DT_NODELABEL(clock)),
					      .subsys_data = subsys_data,
					      .subsys_cnt = ARRAY_SIZE(subsys_data)}};
