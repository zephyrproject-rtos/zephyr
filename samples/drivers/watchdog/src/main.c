/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
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
#define WDT_MAX_WINDOW  100U
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
/* Nordic supports a callback, but it has 61.2 us to complete before
 * the reset occurs, which is too short for this sample to do anything
 * useful.  Explicitly disallow use of the callback.
 */
#define WDT_ALLOW_CALLBACK 0
#define WDT_NODE DT_INST(0, nordic_nrf_wdt)
#elif DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_watchdog)
#define WDT_NODE DT_INST(0, espressif_esp32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(silabs_gecko_wdog)
#define WDT_NODE DT_INST(0, silabs_gecko_wdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_wdog32)
#define WDT_NODE DT_INST(0, nxp_kinetis_wdog32)
#elif DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_watchdog)
#define WDT_NODE DT_INST(0, microchip_xec_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(ti_cc32xx_watchdog)
#define WDT_NODE DT_INST(0, ti_cc32xx_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_imx_wdog)
#define WDT_NODE DT_INST(0, nxp_imx_wdog)
#else
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_watchdog)
#define WDT_MAX_WINDOW  600000U
#define WDT_ALLOW_CALLBACK 0
#endif

#ifndef WDT_ALLOW_CALLBACK
#define WDT_ALLOW_CALLBACK 1
#endif

#ifndef WDT_MAX_WINDOW
#define WDT_MAX_WINDOW  1000U
#endif

#if WDT_ALLOW_CALLBACK
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
#endif /* WDT_ALLOW_CALLBACK */

void main(void)
{
	int err;
	int wdt_channel_id;
	const struct device *wdt = DEVICE_DT_GET(WDT_NODE);

	printk("Watchdog sample application\n");

	if (!device_is_ready(wdt)) {
		printk("%s: device not ready.\n", wdt->name);
		return;
	}

	struct wdt_timeout_cfg wdt_config = {
		/* Reset SoC when watchdog timer expires. */
		.flags = WDT_FLAG_RESET_SOC,

		/* Expire watchdog after max window */
		.window.min = 0U,
		.window.max = WDT_MAX_WINDOW,
	};

#if WDT_ALLOW_CALLBACK
	/* Set up watchdog callback. */
	wdt_config.callback = wdt_callback;

	printk("Attempting to test pre-reset callback\n");
#else /* WDT_ALLOW_CALLBACK */
	printk("Callback in RESET_SOC disabled for this platform\n");
#endif /* WDT_ALLOW_CALLBACK */

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP) {
		/* IWDG driver for STM32 doesn't support callback */
		printk("Callback support rejected, continuing anyway\n");
		wdt_config.callback = NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
	if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
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
