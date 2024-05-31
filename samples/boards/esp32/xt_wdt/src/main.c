/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(xt_wdt_sample, CONFIG_WDT_LOG_LEVEL);

K_SEM_DEFINE(wdt_sem, 0, 1);

static const struct device *const clk_dev = DEVICE_DT_GET_ONE(espressif_esp32_rtc);
const struct device *const wdt = DEVICE_DT_GET_ONE(espressif_esp32_xt_wdt);

static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	printk("XT WDT callback\n");
	k_sem_give(&wdt_sem);
}

int main(void)
{
	uint32_t clk_rate = 0;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("Clock device is not ready");
		return -EIO;
	}

	if (!device_is_ready(wdt)) {
		LOG_ERR("XT WDT device is not ready");
		return -EIO;
	}

	LOG_INF("XT WDT Sample on %s", CONFIG_BOARD_TARGET);

	clock_control_get_rate(clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
			       &clk_rate);

	LOG_INF("Current RTC SLOW clock rate: %d Hz", clk_rate);

	/* Set up the watchdog */
	struct wdt_timeout_cfg wdt_config = {
		.window.max = 200,
		.callback = wdt_callback,
	};

	wdt_install_timeout(wdt, &wdt_config);

	wdt_setup(wdt, 0);

	LOG_INF("Remove the external 32K crystal to trigger the watchdog");

	k_sem_take(&wdt_sem, K_FOREVER);

	clock_control_get_rate(clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
			       &clk_rate);

	LOG_INF("Current RTC SLOW clock rate: %d Hz", clk_rate);

	return 0;
}
