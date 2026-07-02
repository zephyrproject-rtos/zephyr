/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_wdt

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/npm10xx.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

#include "mfd_npm10xx.h"

LOG_MODULE_REGISTER(wdt_npm10xx, CONFIG_WDT_LOG_LEVEL);

/* TIMER registers */
#define NPM10_TIMER_TASKS  0xD6U
#define TIMER_TASKS_KICK   BIT(2)
#define NPM10_TIMER_CONFIG 0xD7U
#define TIMER_CONFIG_GEN   0x00U
#define TIMER_CONFIG_RST   0x01U
#define TIMER_CONFIG_PWRC  0x02U

struct wdt_npm10xx_data {
	bool timeout_valid;
};

struct wdt_npm10xx_gpio_cfg {
	uint8_t pin;
	uint8_t usage;
	gpio_flags_t flags;
};

struct wdt_npm10xx_config {
	const struct device *mfd;
	const struct i2c_dt_spec i2c;
	const struct wdt_npm10xx_gpio_cfg gpio_configs[3]; /* reset, pre-warn, timeout */
};

static int wdt_npm10xx_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_npm10xx_config *config = dev->config;
	struct wdt_npm10xx_data *data = dev->data;

	if (!data->timeout_valid) {
		LOG_ERR("No timeout installed");
		return -EINVAL;
	}

	if (options) {
		LOG_ERR("No extra options are supported");
		return -ENOTSUP;
	}

	return mfd_npm10xx_timer_start(config->mfd);
}

static int wdt_npm10xx_disable(const struct device *dev)
{
	const struct wdt_npm10xx_config *config = dev->config;
	struct wdt_npm10xx_data *data = dev->data;
	int ret;

	ret = mfd_npm10xx_timer_stop(config->mfd);
	if (ret == 0) {
		data->timeout_valid = false;
	}

	return ret;
}

static int wdt_npm10xx_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_npm10xx_config *config = dev->config;
	struct wdt_npm10xx_data *data = dev->data;
	int ret;
	uint8_t mode;

	if (data->timeout_valid) {
		LOG_ERR("Timeout already installed");
		return -ENOMEM;
	}

	if (cfg->window.min != 0) {
		LOG_ERR("Window timeouts are not supported");
		return -EINVAL;
	}

	switch (cfg->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_NONE:
		mode = TIMER_CONFIG_GEN;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		mode = TIMER_CONFIG_RST;
		break;
	case WDT_FLAG_RESET_SOC:
		mode = TIMER_CONFIG_PWRC;
		break;
	default:
		LOG_ERR("Invalid configuration flags 0x%02X", cfg->flags);
		return -ENOTSUP;
	}

	ret = mfd_npm10xx_timer_set(config->mfd, K_MSEC(cfg->window.max));
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_TIMER_CONFIG, mode);
	if (ret == 0) {
		data->timeout_valid = true;
	}

	return ret;
}

static int wdt_npm10xx_feed(const struct device *dev, int channel_id)
{
	const struct wdt_npm10xx_config *config = dev->config;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel ID %d", channel_id);
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_TIMER_TASKS, TIMER_TASKS_KICK);
}

static DEVICE_API(wdt, wdt_npm10xx_api) = {
	.setup = wdt_npm10xx_setup,
	.disable = wdt_npm10xx_disable,
	.install_timeout = wdt_npm10xx_install_timeout,
	.feed = wdt_npm10xx_feed,
};

static int wdt_npm10xx_init(const struct device *dev)
{
	const struct wdt_npm10xx_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR_DEVICE_NOT_READY(config->mfd);
	}

	ARRAY_FOR_EACH(config->gpio_configs, idx) {
		if (config->gpio_configs[idx].pin == UINT8_MAX) {
			continue;
		}

		ret = mfd_npm10xx_pin_configure(config->mfd, config->gpio_configs[idx].pin,
						config->gpio_configs[idx].usage,
						config->gpio_configs[idx].flags | GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure pin %u for Watchdog usage",
				config->gpio_configs[idx].pin);
			return ret;
		}
	}

	return 0;
}

/* clang-format off */
#define GPIO_CONFIG(n, prop, usage)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop),                                                \
		    ({DT_INST_PROP_BY_IDX(n, prop, 0), usage, DT_INST_PROP_BY_IDX(n, prop, 1)}),   \
		    ({UINT8_MAX}))

#define WDT_NPM10XX_DEFINE(n)                                                                      \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reset_gpio_config),                      \
				 (DT_INST_PROP_BY_IDX(n, reset_gpio_config, 0)),                   \
				 (0)) < NPM10_PIN_NUM,                                             \
		     "PMIC wdt reset output pin index out of range");                              \
                                                                                                   \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prewarn_gpio_config),                    \
				 (DT_INST_PROP_BY_IDX(n, prewarn_gpio_config, 0)),                 \
				 (0)) < NPM10_PIN_NUM,                                             \
		     "PMIC wdt pre-warn output pin index out of range");                           \
                                                                                                   \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, timeout_gpio_config),                    \
				 (DT_INST_PROP_BY_IDX(n, timeout_gpio_config, 0)),                 \
				 (0)) < NPM10_PIN_NUM,                                             \
		     "PMIC wdt timeout output pin index out of range");                            \
                                                                                                   \
	static struct wdt_npm10xx_data wdt_npm10xx_data##n;                                        \
                                                                                                   \
	static const struct wdt_npm10xx_config wdt_npm10xx_config##n = {                           \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.gpio_configs = {                                                                  \
			GPIO_CONFIG(n, reset_gpio_config, NPM10_PIN_WD_RST),                       \
			GPIO_CONFIG(n, prewarn_gpio_config, NPM10_PIN_WD_WARN),                    \
			GPIO_CONFIG(n, timeout_gpio_config, NPM10_PIN_TIMER_END),                  \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &wdt_npm10xx_init, NULL, &wdt_npm10xx_data##n,                    \
			      &wdt_npm10xx_config##n, POST_KERNEL,                                 \
			      CONFIG_WDT_NPM10XX_INIT_PRIORITY, &wdt_npm10xx_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(WDT_NPM10XX_DEFINE)
