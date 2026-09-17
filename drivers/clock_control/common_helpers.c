/*
 * Copyright (c) 2026 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common_helpers.h"

int clock_control_always_running_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

int clock_control_always_running_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOTSUP;
}

enum clock_control_status clock_control_always_running_clk_get_status(const struct device *dev,
								      clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return CLOCK_CONTROL_STATUS_ON;
}
