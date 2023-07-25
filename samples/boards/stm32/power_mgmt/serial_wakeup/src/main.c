/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

int main(void)
{
	const struct device *const dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(dev)) {
		printk("Console device not ready");
		return 0;
	}

#if CONFIG_PM_DEVICE
	/* In PM_DEVICE modes, enable device as a wakeup source will prevent
	 * system to switch it off (clock off, set pins to sleep configuration, ...)
	 * It is not requested in PM mode only since this mode will not
	 */

	bool ret;

	ret = pm_device_wakeup_is_capable(dev);
	if (!ret) {
		printk("Device is not wakeup capable\n");
	} else {
		printk("Device is wakeup capable\n");

		ret = pm_device_wakeup_enable(dev, true);
		if (!ret) {
			printk("Could not enable wakeup source\n");
		} else {
			printk("Wakeup source enable ok\n");
		}

		ret = pm_device_wakeup_is_enabled(dev);
		if (!ret) {
			printk("Wakeup source not enabled\n");
		} else {
			printk("Wakeup source enabled\n");
		}
	}
#endif

	return 0;
}
