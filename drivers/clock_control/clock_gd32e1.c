/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/gd32_clock_control.h>


int gd32_clock_control_init(const struct device *dev)
{
	return 0;
}

static inline int gd32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	return 0;
}

static inline int gd32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	return 0;
}

static int gd32_clock_control_get_subsys_rate(const struct device *clock,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	return 0;
}

static struct clock_control_driver_api gd32_clock_control_api = {
	.on = gd32_clock_control_on,
	.off = gd32_clock_control_off,
	.get_rate = gd32_clock_control_get_subsys_rate,
};

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    &gd32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_GD32_DEVICE_INIT_PRIORITY,
		    &gd32_clock_control_api);
