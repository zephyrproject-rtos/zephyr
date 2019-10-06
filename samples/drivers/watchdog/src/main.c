/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>
#include <sys/printk.h>
#include <stdbool.h>

#define WDT_FEED_TRIES 5


#ifdef CONFIG_WDT_0_NAME
#define WDT_DEV_NAME CONFIG_WDT_0_NAME
#else
#ifdef CONFIG_WWDG_STM32
#define WDT_DEV_NAME DT_WWDT_0_NAME
#else
#define WDT_DEV_NAME DT_WDT_0_NAME
#endif
#endif

static void wdt_callback(struct device *wdt_dev, int channel_id)
{
	static bool handled_event;

	if (handled_event) {
		return;
	}

	wdt_feed(wdt_dev, channel_id);

	printk("Handled things..ready to reset\n");
	handled_event = true;
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

	/* Expire watchdog after 1000 milliseconds. */
	wdt_config.window.min = 0U;
	wdt_config.window.max = 1000U;

	/* Set up watchdog callback. Jump into it when watchdog expired. */
	wdt_config.callback = wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP) {
		/* IWDG driver for STM32 doesn't support callback */
		wdt_config.callback = NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
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
		k_sleep(K_MSEC(50));
	}

	/* Waiting for the SoC reset. */
	printk("Waiting for reset...\n");
	while (1) {
		k_yield();
	}
}
