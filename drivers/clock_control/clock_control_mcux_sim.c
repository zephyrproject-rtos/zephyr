/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <soc.h>
#include <clock_control.h>
#include <fsl_clock.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CLOCK_CONTROL_LEVEL
#include <logging/sys_log.h>

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
	clock_name_t clock_name = (clock_name_t) sub_system;

	*rate = CLOCK_GetFreq(clock_name);

	return 0;
}

static int mcux_sim_init(struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api mcux_sim_driver_api = {
	.on = mcux_sim_on,
	.off = mcux_sim_off,
	.get_rate = mcux_sim_get_subsys_rate,
};

DEVICE_AND_API_INIT(mcux_sim, CONFIG_SIM_NAME,
		    &mcux_sim_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_sim_driver_api);
