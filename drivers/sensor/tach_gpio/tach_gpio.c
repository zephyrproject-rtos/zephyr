/*
 * Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_tach_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tach_gpio, CONFIG_SENSOR_LOG_LEVEL);

static const uint32_t us_per_min = USEC_PER_SEC * SEC_PER_MIN;

struct tach_gpio_config {
	struct gpio_dt_spec gpio;
	k_timeout_t timeout;
};

struct tach_gpio_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_sem data_ready;
	int64_t start_ticks;
	/* RPM or errno */
	int32_t rpm;
};

static void tach_gpio_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct tach_gpio_data *data = CONTAINER_OF(cb, struct tach_gpio_data, gpio_cb);
	const struct tach_gpio_config *config = data->dev->config;
	uint32_t pulse_us;
	int64_t ticks;

	ticks = k_uptime_ticks();

	if (data->start_ticks == -ENODATA) {
		data->start_ticks = ticks;
		return;
	}

	gpio_pin_interrupt_configure(config->gpio.port, config->gpio.pin, GPIO_INT_DISABLE);

	pulse_us = k_ticks_to_us_floor32(ticks - data->start_ticks);
	data->rpm = pulse_us > 0 ? us_per_min / pulse_us : -ERANGE;

	LOG_DBG("rpm: %u, pulse: %d us", data->rpm, pulse_us);

	k_sem_give(&data->data_ready);
}

static int tach_gpio_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tach_gpio_config *config = dev->config;
	struct tach_gpio_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_RPM && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* In case previous fetch timed out: Disable GPIO interrupt and clear semaphore */
	ret = gpio_pin_interrupt_configure(config->gpio.port, config->gpio.pin, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_DBG("Disable GPIO interrupt failed: %d", ret);
		return ret;
	}

	k_sem_reset(&data->data_ready);

	data->start_ticks = -ENODATA;

	ret = gpio_pin_interrupt_configure_dt(&config->gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_DBG("Configure GPIO interrupt failed: %d", ret);
		return ret;
	}

	return k_sem_take(&data->data_ready, config->timeout);
}

static int tach_gpio_get(const struct device *dev, enum sensor_channel chan,
			 struct sensor_value *val)
{
	struct tach_gpio_data *data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	if (data->rpm < 0) {
		return data->rpm;
	}

	val->val1 = data->rpm;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, tach_gpio_api) = {
	.sample_fetch = tach_gpio_fetch,
	.channel_get = tach_gpio_get,
};

static int tach_gpio_init(const struct device *dev)
{
	const struct tach_gpio_config *config = dev->config;
	struct tach_gpio_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->gpio)) {
		LOG_DBG("Gpio is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Configure gpio failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tach_gpio_cb, BIT(config->gpio.pin));

	ret = gpio_add_callback_dt(&config->gpio, &data->gpio_cb);
	if (ret < 0) {
		LOG_DBG("Add gpio callback failed: %d", ret);
		return ret;
	}

	data->dev = dev;

	k_sem_init(&data->data_ready, 0, 1);

	return 0;
}

#define TACH_GPIO_INIT(n)                                                                          \
	static struct tach_gpio_data tach_gpio_data_##n = {                                        \
		.rpm = -ENODATA,                                                                   \
	};                                                                                         \
                                                                                                   \
	static const struct tach_gpio_config tach_gpio_config_##n = {                              \
		.gpio = GPIO_DT_SPEC_INST_GET(n, gpios),                                           \
		.timeout = K_MSEC(DT_INST_PROP(n, timeout_ms)),                                    \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &tach_gpio_init, NULL, &tach_gpio_data_##n,                \
				     &tach_gpio_config_##n, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &tach_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_GPIO_INIT)
