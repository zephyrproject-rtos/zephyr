/*
 * Copyright (c) 2024 Joakim Andersson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

int main(void)
{
	const struct device *dev;

	/* This sample demonstrates MCO usage via Device Tree.
	 * MCO configuration is performed in the Device Tree overlay files.
	 * Each MCO will be enabled automatically by the driver during device
	 * initialization. This sample checks that all MCOs are ready - if so,
	 * the selected clock should be visible on the chosen GPIO pin.
	 */
	dev = DEVICE_DT_GET(DT_NODELABEL(mco1));
	if (device_is_ready(dev)) {
		printk("MCO1 device successfully configured\n");
	} else {
		printk("MCO1 device not ready\n");
		return -1;
	}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mco2), okay)
	dev = DEVICE_DT_GET(DT_NODELABEL(mco2));
	if (device_is_ready(dev)) {
		printk("MCO2 device successfully configured\n");
	} else {
		printk("MCO2 device not ready\n");
		return -1;
	}
#endif

	printk("\nDisplayed the status of all MCO devices - end of example.\n");
	return 0;
}
