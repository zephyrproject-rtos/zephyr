/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(watchdog_sample, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
#define WDT_NODE DT_ALIAS(watchdog0)

#if !DT_NODE_EXISTS(WDT_NODE)
#error "Board does not have a watchdog device"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);

static struct wdt_timeout_cfg wdt_config;
static int wdt_channel_id;

/*
 * Watchdog callback - this is called when the watchdog is about to expire
 * Note: This callback runs in interrupt context
 */
static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);

	LOG_WRN("Watchdog callback triggered! System will reset soon...");
	
	/* In a real application, you might:
	 * - Save critical data
	 * - Send a distress signal
	 * - Try to recover the system
	 */
}

int main(void)
{
	int ret;
	int counter = 0;
	bool feed_dog = true;

	LOG_INF("Watchdog Sample Application");
	LOG_INF("Press button or wait 10 seconds to stop feeding watchdog");

	/* Configure LED */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED pin");
		return ret;
	}

	/* Check if watchdog is ready */
	if (!device_is_ready(wdt)) {
		LOG_ERR("Watchdog device not ready");
		return -ENODEV;
	}

	/* Configure watchdog */
	wdt_config.flags = WDT_FLAG_RESET_SOC;
	wdt_config.window.min = 0U;
	wdt_config.window.max = 5000U; /* 5 seconds timeout */
	wdt_config.callback = wdt_callback;

	/* Install timeout */
	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id < 0) {
		LOG_ERR("Watchdog install timeout error: %d", wdt_channel_id);
		return wdt_channel_id;
	}

	/* Setup and start watchdog */
	ret = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		LOG_ERR("Watchdog setup error: %d", ret);
		return ret;
	}

	LOG_INF("Watchdog configured with 5 second timeout");
	LOG_INF("Watchdog will be fed every second for the first 10 seconds");

	/* Main loop */
	while (1) {
		/* Toggle LED to show we're alive */
		gpio_pin_toggle_dt(&led);

		/* Feed the watchdog for the first 10 seconds */
		if (feed_dog && counter < 10) {
			ret = wdt_feed(wdt, wdt_channel_id);
			if (ret < 0) {
				LOG_ERR("Failed to feed watchdog: %d", ret);
			} else {
				LOG_INF("Fed watchdog (%d/10)", counter + 1);
			}
		} else if (feed_dog) {
			feed_dog = false;
			LOG_WRN("Stopped feeding watchdog - system will reset in ~5 seconds");
			LOG_WRN("Starting countdown...");
		} else {
			/* After we stop feeding, count down to reset */
			LOG_WRN("Reset in %d seconds...", 15 - counter);
		}

		counter++;
		k_sleep(K_SECONDS(1));
	}

	return 0;
}