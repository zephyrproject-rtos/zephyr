/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_relay

#include <zephyr/drivers/relay/relay.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <errno.h>

LOG_MODULE_REGISTER(relay_gpio, CONFIG_RELAY_LOG_LEVEL);

struct relay_gpio_config {
	struct gpio_dt_spec gpio;
};

struct relay_gpio_data {
	struct k_mutex lock;
	enum relay_state state;
};

static int relay_gpio_set_state(const struct device *dev, enum relay_state state)
{
	const struct relay_gpio_config *cfg = (const struct relay_gpio_config *)dev->config;
	struct relay_gpio_data *data = (struct relay_gpio_data *)dev->data;
	int err;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	err = gpio_pin_set_dt(&cfg->gpio, state == RELAY_STATE_ON ? 1 : 0);
	if (err != 0) {
		LOG_ERR("%s: gpio_pin_set failed (err %d)", dev->name, err);
	} else {
		data->state = state;
	}
	k_mutex_unlock(&data->lock);
	return err;
}

static int relay_gpio_get_state(const struct device *dev, enum relay_state *state)
{
	struct relay_gpio_data *data = (struct relay_gpio_data *)dev->data;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	*state = data->state;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int relay_gpio_init(const struct device *dev)
{
	const struct relay_gpio_config *cfg = (const struct relay_gpio_config *)dev->config;
	struct relay_gpio_data *data = (struct relay_gpio_data *)dev->data;

	k_mutex_init(&data->lock);
	if (!gpio_is_ready_dt(&cfg->gpio)) {
		LOG_ERR("%s: gpio not ready", dev->name);
		return -ENODEV;
	}
	/* Released state; the coil's active level comes from the DT flags. */
	return gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT_INACTIVE);
}

static DEVICE_API(relay, relay_gpio_api) = {
	.set_state = relay_gpio_set_state,
	.get_state = relay_gpio_get_state,
};

#define RELAY_GPIO_DEFINE(inst)                                                                    \
	static const struct relay_gpio_config relay_gpio_config_##inst = {                         \
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                                        \
	};                                                                                         \
	static struct relay_gpio_data relay_gpio_data_##inst = {                                   \
		.state = RELAY_STATE_OFF,                                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, relay_gpio_init, NULL, &relay_gpio_data_##inst,                \
			      &relay_gpio_config_##inst, POST_KERNEL,                              \
			      CONFIG_RELAY_GPIO_INIT_PRIORITY, &relay_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(RELAY_GPIO_DEFINE)
