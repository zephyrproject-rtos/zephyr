/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on clock_control_mcux_sim.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_mcg

#include <drivers/clock_control.h>
#include <dt-bindings/clock/kinetis_mcg.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
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
	case KINETIS_MCG_FIXED_FREQ_CLK:
		clock_name = kCLOCK_McgFixedFreqClk;
		break;
	default:
		LOG_ERR("Unsupported clock name");
		return -EINVAL;
		break;
	}

	*rate = CLOCK_GetFreq(clock_name);
	return 0;
}

static int mcux_mcg_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api mcux_mcg_driver_api = {
	.on = mcux_mcg_on,
	.off = mcux_mcg_off,
	.get_rate = mcux_mcg_get_rate,
};

DEVICE_AND_API_INIT(mcux_mcg, DT_INST_LABEL(0),
		    &mcux_mcg_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_mcg_driver_api);
