/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on clock_control_mcux_sim.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_mcg

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/kinetis_mcg.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_mcg);

static int mcux_mcg_on(const struct device *dev,
		       clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_mcg_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_mcg_get_rate(const struct device *dev,
			     clock_control_subsys_t sub_system,
			     uint32_t *rate)
{
	clock_name_t clock_name;

	switch ((uint32_t) sub_system) {
#if defined(FSL_FEATURE_MCG_FFCLK_DIV) && (FSL_FEATURE_MCG_FFCLK_DIV)
	case KINETIS_MCG_FIXED_FREQ_CLK:
		clock_name = kCLOCK_McgFixedFreqClk;
		break;
#endif
	case KINETIS_MCG_OUT_CLK:
		*rate = CLOCK_GetOutClkFreq();
		return 0;
	default:
		LOG_ERR("Unsupported clock name");
		return -EINVAL;
		break;
	}

	*rate = CLOCK_GetFreq(clock_name);
	return 0;
}

static const struct clock_control_driver_api mcux_mcg_driver_api = {
	.on = mcux_mcg_on,
	.off = mcux_mcg_off,
	.get_rate = mcux_mcg_get_rate,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &mcux_mcg_driver_api);
