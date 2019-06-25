/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/kinetis_sim.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static int mcux_sim_on(struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_sim_off(struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_sim_get_subsys_rate(struct device *dev,
				    clock_control_subsys_t sub_system,
				    u32_t *rate)
{
	clock_name_t clock_name;

	switch ((u32_t) sub_system) {
	case KINETIS_SIM_LPO_CLK:
		clock_name = kCLOCK_LpoClk;
		break;
	default:
		clock_name = (clock_name_t) sub_system;
		break;
	}

	*rate = CLOCK_GetFreq(clock_name);

	return 0;
}

static int mcux_sim_init(struct device *dev)
{
#ifdef DT_SIM_CLKOUT_DIVIDER
	SIM->CHIPCTL = (SIM->CHIPCTL & ~SIM_CHIPCTL_CLKOUTDIV_MASK)
		| SIM_CHIPCTL_CLKOUTDIV(DT_SIM_CLKOUT_DIVIDER);
#endif
#ifdef DT_SIM_CLKOUT_SOURCE
	SIM->CHIPCTL = (SIM->CHIPCTL & ~SIM_CHIPCTL_CLKOUTSEL_MASK)
		| SIM_CHIPCTL_CLKOUTSEL(DT_SIM_CLKOUT_SOURCE);
#endif

	return 0;
}

static const struct clock_control_driver_api mcux_sim_driver_api = {
	.on = mcux_sim_on,
	.off = mcux_sim_off,
	.get_rate = mcux_sim_get_subsys_rate,
};

DEVICE_AND_API_INIT(mcux_sim, DT_SIM_NAME,
		    &mcux_sim_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_sim_driver_api);
