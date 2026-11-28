/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_aic3104_test_clock

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#include "fake_clock.h"

static unsigned int s_on_count;
static uintptr_t s_last_subsys;
static int s_on_result;

void fake_clock_reset(void)
{
	s_on_count = 0U;
	s_last_subsys = 0U;
	s_on_result = 0;
}

unsigned int fake_clock_on_count(void)
{
	return s_on_count;
}

uintptr_t fake_clock_last_subsys(void)
{
	return s_last_subsys;
}

void fake_clock_set_on_result(int ret)
{
	s_on_result = ret;
}

static int fake_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	s_on_count++;
	s_last_subsys = (uintptr_t)sys;

	return s_on_result;
}

static int fake_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
			       uint32_t *rate)
{
	ARG_UNUSED(sys);

	*rate = *(const uint32_t *)dev->config;

	return 0;
}

static DEVICE_API(clock_control, fake_clock_api) = {
	.on = fake_clock_on,
	.get_rate = fake_clock_get_rate,
};

#define FAKE_CLOCK_INIT(inst)                                                                      \
	static const uint32_t fake_clock_rate_##inst = DT_INST_PROP(inst, output_rate);            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &fake_clock_rate_##inst, PRE_KERNEL_1,       \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &fake_clock_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_CLOCK_INIT)
