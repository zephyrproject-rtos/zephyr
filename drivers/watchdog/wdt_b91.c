/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_watchdog

#include <clock.h>
#include <watchdog.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_MODULE_NAME watchdog_b91
#if defined(CONFIG_WDT_LOG_LEVEL)
#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);


static int wdt_b91_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(options);

	wd_start();

	LOG_INF("HW watchdog started");

	return 0;
}

static int wdt_b91_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	wd_stop();

	LOG_INF("HW watchdog stopped");

	return 0;
}

static int wdt_b91_install_timeout(const struct device *dev,
				    const struct wdt_timeout_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (cfg->window.min != 0) {
		LOG_WRN("Window watchdog not supported");
		return -EINVAL;
	}

	const uint32_t max_timeout_ms = UINT32_MAX / (sys_clk.pclk * 1000);

	if (cfg->window.max > max_timeout_ms) {
		LOG_WRN("Timeout overflows %umS max is %umS", cfg->window.max, max_timeout_ms);
		return -EINVAL;
	}

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		LOG_WRN("Not supported flag");
		return -EINVAL;
	}

	wd_set_interval_ms(cfg->window.max);

	/* HW restriction */
	LOG_INF("HW watchdog set to %u mS", cfg->window.max & 0xffffff00);

	return 0;
}

static int wdt_b91_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	wd_clear_cnt();
	return 0;
}


static const struct wdt_driver_api wdt_b91_api = {
	.setup = wdt_b91_setup,
	.disable = wdt_b91_disable,
	.install_timeout = wdt_b91_install_timeout,
	.feed = wdt_b91_feed,
};


static int wdt_b91_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}


DEVICE_DT_INST_DEFINE(0, wdt_b91_init, NULL,
	NULL, NULL, PRE_KERNEL_1,
	CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_b91_api);
