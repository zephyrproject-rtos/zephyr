/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
LOG_MODULE_REGISTER(xlnx_clk_wiz_sample, LOG_LEVEL_INF);

#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_clkx5_wiz_1_0)
#define CLK_WIZ_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(xlnx_clkx5_wiz_1_0)
#define CLK_WIZ_NAME DT_PROP_OR(CLK_WIZ_NODE, label, DT_NODE_FULL_NAME(CLK_WIZ_NODE))

#define TEST_CLKOUT_ID  0U
static const uint32_t test_freqs_hz[] = {
	100000000U,
	200000000U,
	300000000U,
};
#endif

int main(void)
{
#if !DT_HAS_COMPAT_STATUS_OKAY(xlnx_clkx5_wiz_1_0)
	printk("No enabled xlnx,clkx5-wiz-1.0 node found in devicetree\n");
	printk("Add an enabled Clocking Wizard node to the active board DTS\n");
	return -ENODEV;
#else
	const struct device *dev = DEVICE_DT_GET(CLK_WIZ_NODE);
	clock_control_subsys_t subsys =
		(clock_control_subsys_t)(uintptr_t)TEST_CLKOUT_ID;
	uint32_t rate_hz = 0U;
	int ret;
	int i;

	if (!device_is_ready(dev)) {
		printk("CLK_WIZ device %s not ready\n", dev->name);
		return -ENODEV;
	}

	printk("\n Clocking Wizard sample application\n");
	printk("Device: %s, testing CLKOUT%u\n", CLK_WIZ_NAME, TEST_CLKOUT_ID);

	ret = clock_control_get_rate(dev, subsys, &rate_hz);
	if (ret == 0) {
		printk(" Initial CLKOUT%u rate: %u Hz\n", TEST_CLKOUT_ID,
		       rate_hz);
	} else {
		printk(" get_rate failed (ret=%d) - clock may not be programmed yet\n",
		       ret);
	}

	ret = clock_control_on(dev, subsys);
	if (ret == 0) {
		printk(" CLKOUT%u enabled (clock_control_on OK)\n", TEST_CLKOUT_ID);
	} else {
		printk(" clock_control_on failed (ret=%d)\n", ret);
	}

	for (i = 0; i < ARRAY_SIZE(test_freqs_hz); i++) {
		uint32_t target = test_freqs_hz[i];

		printk("Requesting CLKOUT%u = %u Hz ...\n", TEST_CLKOUT_ID,
		       target);
		ret = clock_control_set_rate(dev, subsys,
					     (clock_control_subsys_rate_t)&target);
		if (ret != 0) {
			printk(" set_rate(%u Hz) failed (ret=%d)\n", target, ret);
		} else {
			rate_hz = 0U;
			ret = clock_control_get_rate(dev, subsys, &rate_hz);
			if (ret == 0) {
				printk("Actual  CLKOUT%u = %u Hz\n", TEST_CLKOUT_ID,
				       rate_hz);
			} else {
				printk(" get_rate failed after set_rate (ret=%d)\n", ret);
			}
		}
	}

	ret = clock_control_off(dev, subsys);
	if (ret == 0) {
		printk("CLKOUT%u disabled (clock_control_off OK)\n", TEST_CLKOUT_ID);
	} else {
		printk("clock_control_off failed (ret=%d)\n", ret);
	}

	return 0;
#endif
}
