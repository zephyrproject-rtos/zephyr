/*
 * Copyright (c) 2023, Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tps382x

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_ti_tps382x, CONFIG_WDT_LOG_LEVEL);

struct ti_tps382x_config {
	struct gpio_dt_spec wdi_gpio;
	int timeout;
};

static int ti_tps382x_init(const struct device *dev)
{
	const struct ti_tps382x_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->wdi_gpio)) {
		LOG_ERR("WDI gpio not ready");
		return -ENODEV;
	}

	return 0;
}

static int ti_tps382x_setup(const struct device *dev, uint8_t options)
{
	const struct ti_tps382x_config *config = dev->config;

	return gpio_pin_configure_dt(&config->wdi_gpio, GPIO_OUTPUT);
}

static int ti_tps382x_disable(const struct device *dev)
{
	const struct ti_tps382x_config *config = dev->config;

	/* The watchdog timer can be disabled by disconnecting the WDI pin from
	 * the system. Do this by changing the gpio to an input (tri-state).
	 */
	return gpio_pin_configure_dt(&config->wdi_gpio, GPIO_INPUT);
}

static int ti_tps382x_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	const struct ti_tps382x_config *config = dev->config;

	if (cfg->window.max != config->timeout) {
		LOG_ERR("Upper limit of watchdog timeout must be %d not %u",
			config->timeout, cfg->window.max);
		return -EINVAL;
	} else if (cfg->window.min != 0) {
		LOG_ERR("Window timeouts not supported");
		return -EINVAL;
	} else if (cfg->callback != NULL) {
		LOG_ERR("Callbacks not supported");
		return -EINVAL;
	}

	return 0;
}

static int ti_tps382x_feed(const struct device *dev, int channel_id)
{
	const struct ti_tps382x_config *config = dev->config;

	return gpio_pin_toggle_dt(&config->wdi_gpio);
}

static DEVICE_API(wdt, ti_tps382x_api) = {
	.setup = ti_tps382x_setup,
	.disable = ti_tps382x_disable,
	.install_timeout = ti_tps382x_install_timeout,
	.feed = ti_tps382x_feed,
};

#define WDT_TI_TPS382X_INIT(n)                                                \
	static const struct ti_tps382x_config ti_tps382x_##n##config = {      \
		.wdi_gpio = GPIO_DT_SPEC_INST_GET(n, wdi_gpios),              \
		.timeout = DT_INST_PROP(n, timeout_period),                   \
	};                                                                    \
                                                                              \
	DEVICE_DT_INST_DEFINE(                                                \
		n, ti_tps382x_init, NULL, NULL, &ti_tps382x_##n##config,      \
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,              \
		&ti_tps382x_api                                               \
	);

DT_INST_FOREACH_STATUS_OKAY(WDT_TI_TPS382X_INIT);
