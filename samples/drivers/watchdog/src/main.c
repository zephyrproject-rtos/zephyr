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

/*
 * To use this sample, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 */
#if DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_window_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_window_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_watchdog)
#define WDT_NODE DT_INST(0, nordic_nrf_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_watchdog)
#define WDT_NODE DT_INST(0, espressif_esp32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(silabs_gecko_wdog)
#define WDT_NODE DT_INST(0, silabs_gecko_wdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_wdog32)
#define WDT_NODE DT_INST(0, nxp_kinetis_wdog32)
#elif DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_watchdog)
#define WDT_NODE DT_INST(0, microchip_xec_watchdog)
#endif

/*
 * If the devicetree has a watchdog node, get its label property.
 */
#ifdef WDT_NODE
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#else
#define WDT_DEV_NAME ""
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif

static void wdt_callback(const struct device *wdt_dev, int channel_id)
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
	const struct device *wdt;
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
