/*
 * Copyright (c) 2024 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hpmicro_hpm_pllv2

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#include <hpm_clock_drv.h>

#define HPM_CLOCK_CONTROL_GROUP 0U

static clock_name_t hpm_clock_from_subsys(clock_control_subsys_t sys)
{
	return (clock_name_t)(uintptr_t)sys;
}

static int hpm_clock_control_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	if (sys == NULL) {
		return -EINVAL;
	}

	clock_add_to_group(hpm_clock_from_subsys(sys), HPM_CLOCK_CONTROL_GROUP);

	return 0;
}

static int hpm_clock_control_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	if (sys == NULL) {
		return -EINVAL;
	}

	clock_remove_from_group(hpm_clock_from_subsys(sys), HPM_CLOCK_CONTROL_GROUP);

	return 0;
}

static int hpm_clock_control_get_rate(const struct device *dev, clock_control_subsys_t sys,
				      uint32_t *rate)
{
	ARG_UNUSED(dev);

	if (sys == NULL || rate == NULL) {
		return -EINVAL;
	}

	*rate = clock_get_frequency(hpm_clock_from_subsys(sys));

	return *rate == 0U ? -EINVAL : 0;
}

static enum clock_control_status hpm_clock_control_get_status(const struct device *dev,
							      clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	if (sys == NULL) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return clock_check_in_group(hpm_clock_from_subsys(sys), HPM_CLOCK_CONTROL_GROUP)
		       ? CLOCK_CONTROL_STATUS_ON
		       : CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, hpm_clock_control_api) = {
	.on = hpm_clock_control_on,
	.off = hpm_clock_control_off,
	.get_rate = hpm_clock_control_get_rate,
	.get_status = hpm_clock_control_get_status,
};

#define HPM_CLOCK_CONTROL_INIT(inst)                                                               \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, PRE_KERNEL_1,                          \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &hpm_clock_control_api);

DT_INST_FOREACH_STATUS_OKAY(HPM_CLOCK_CONTROL_INIT)
