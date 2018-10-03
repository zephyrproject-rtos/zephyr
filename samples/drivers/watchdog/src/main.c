/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <watchdog.h>
#include <misc/printk.h>

#define WDT_DEV_NAME   CONFIG_WDT_0_NAME
#define WDT_FEED_TRIES 5

static void wdt_callback(struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	/* Watchdog timer expired. Handle it here.
	 * Remember that SoC reset will be done soon.
	 */
}

void main(void)
{
	int err;
	int wdt_channel_id;
	struct device *wdt;
	struct wdt_timeout_cfg wdt_config;

	printk("Watchdog sample application\n");
	wdt = device_get_binding(WDT_DEV_NAME);

	if (!wdt) {
		printk("Cannot get WDT device\n");
		return;
	}

	/* Reset SoC when watchdog timer expires. */
	wdt_config.flags = WDT_FLAG_RESET_SOC;

	/* Expire watchdog after 5000 milliseconds. */
	wdt_config.window.min = 0;
	wdt_config.window.max = 5000;

	/* Set up watchdog callback. Jump into it when watchdog expired. */
	wdt_config.callback = wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return;
	}

	err = wdt_setup(wdt, 0);
	if (err < 0) {
		printk("Watchdog setup error\n");
		return;
	}

	/* Feeding watchdog. */
	printk("Feeding watchdog %d times\n", WDT_FEED_TRIES);
	for (int i = 0; i < WDT_FEED_TRIES; ++i) {
		printk("Feeding watchdog...\n");
		wdt_feed(wdt, wdt_channel_id);
		k_sleep(1000);
	}

	/* Waiting for the SoC reset. */
	printk("Waiting for reset...\n");
	while (1) {
		k_yield();
	};
}
