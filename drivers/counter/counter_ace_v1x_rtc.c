/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <soc.h>
#include <counter/counter_ace_v1x_rtc_regs.h>

static int counter_ace_v1x_rtc_get_value(const struct device *dev,
		int64_t *value)
{
	ARG_UNUSED(dev);

		uint32_t hi0, lo, hi1;
	do {
		hi0 = sys_read32(ACE_RTCWC_HI);
		lo = sys_read32(ACE_RTCWC_LO);
		hi1 = sys_read32(ACE_RTCWC_HI);
	} while (hi0 != hi1);

	*value = (((uint64_t)hi0) << 32) | lo;
}

int counter_ace_v1x_rtc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct counter_driver_api ace_v1x_rtc_counter_apis = {
	.get_value_64 = counter_ace_v1x_rtc_get_value
};

DEVICE_DT_DEFINE(DT_NODELABEL(ace_rtc_counter), counter_ace_v1x_rtc_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 &ace_v1x_rtc_counter_apis);
