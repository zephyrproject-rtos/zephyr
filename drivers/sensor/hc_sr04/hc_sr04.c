/*
 * Copyright (c) 2024 Adrien Leravat
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hc_sr04

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(HC_SR04, CONFIG_SENSOR_LOG_LEVEL);

#define HC_SR04_MM_PER_MS     171

static const uint32_t hw_cycles_per_ms = sys_clock_hw_cycles_per_sec() / 1000;

struct hcsr04_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_sem sem;
	uint32_t start_cycles;
	atomic_t echo_high_cycles;
};

struct hcsr04_config {
	struct gpio_dt_spec trigger_gpios;
	struct gpio_dt_spec echo_gpios;
};

static void hcsr04_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

static int hcsr04_configure_gpios(const struct hcsr04_config *cfg)
{
	int ret;

	if (!gpio_is_ready_dt(&cfg->trigger_gpios)) {
		LOG_ERR("GPIO '%s' not ready", cfg->trigger_gpios.port->name);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->trigger_gpios, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Failed to configure '%s' as output: %d", cfg->trigger_gpios.port->name,
			ret);
		return ret;
	}

	if (!gpio_is_ready_dt(&cfg->echo_gpios)) {
		LOG_ERR("GPIO '%s' not ready", cfg->echo_gpios.port->name);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->echo_gpios, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure '%s' as output: %d", cfg->echo_gpios.port->name, ret);
		return ret;
	}

	return 0;
}

static int hcsr04_configure_interrupt(const struct hcsr04_config *cfg, struct hcsr04_data *data)
{
	int ret;

	/* Disable initially to avoid spurious interrupts. */
	ret = gpio_pin_interrupt_configure(cfg->echo_gpios.port, cfg->echo_gpios.pin,
		GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to configure '%s' as interrupt: %d", cfg->echo_gpios.port->name,
			ret);
		return -EIO;
	}
	gpio_init_callback(&data->gpio_cb, &hcsr04_gpio_callback, BIT(cfg->echo_gpios.pin));
	ret = gpio_add_callback(cfg->echo_gpios.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add callback on '%s': %d", cfg->echo_gpios.port->name, ret);
		return -EIO;
	}
	return 0;
}

static int hcsr04_init(const struct device *dev)
{
	const struct hcsr04_config *cfg = dev->config;
	struct hcsr04_data *data = dev->data;
	int ret;

	k_sem_init(&data->sem, 0, 1);

	ret = hcsr04_configure_gpios(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = hcsr04_configure_interrupt(cfg, data);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void hcsr04_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct hcsr04_data *data = CONTAINER_OF(cb, struct hcsr04_data, gpio_cb);
	const struct hcsr04_config *cfg = data->dev->config;

	if (gpio_pin_get(dev, cfg->echo_gpios.pin) == 1) {
		data->start_cycles = k_cycle_get_32();
	} else {
		atomic_set(&data->echo_high_cycles, k_cycle_get_32() - data->start_cycles);
		gpio_pin_interrupt_configure_dt(&cfg->echo_gpios, GPIO_INT_DISABLE);
		k_sem_give(&data->sem);
	}
}

static int hcsr04_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct hcsr04_config *cfg = dev->config;
	struct hcsr04_data *data = dev->data;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->echo_gpios, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to set configure echo pin as interrupt: %d", ret);
		return ret;
	}

	/* Generate 10us trigger */
	ret = gpio_pin_set_dt(&cfg->trigger_gpios, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set trigger pin: %d", ret);
		return ret;
	}

	k_busy_wait(10);

	ret = gpio_pin_set_dt(&cfg->trigger_gpios, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set trigger pin: %d", ret);
		return ret;
	}

	if (k_sem_take(&data->sem, K_MSEC(10)) != 0) {
		LOG_ERR("Echo signal was not received");
		return -EIO;
	}
	return 0;
}

static int hcsr04_channel_get(const struct device *dev, enum sensor_channel chan,
	struct sensor_value *val)
{
	const struct hcsr04_data *data = dev->data;
	uint32_t distance_mm;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	distance_mm = HC_SR04_MM_PER_MS * atomic_get(&data->echo_high_cycles) /
			hw_cycles_per_ms;
	return sensor_value_from_milli(val, distance_mm);
}

static const struct sensor_driver_api hcsr04_driver_api = {
	.sample_fetch = hcsr04_sample_fetch,
	.channel_get = hcsr04_channel_get
};


#define HC_SR04_INIT(index)                                                           \
	static struct hcsr04_data hcsr04_data_##index = {                             \
		.dev = DEVICE_DT_INST_GET(index),                                     \
		.start_cycles = 0,                                                    \
		.echo_high_cycles = ATOMIC_INIT(0),                                   \
	};                                                                            \
	static struct hcsr04_config hcsr04_config_##index = {                         \
		.trigger_gpios = GPIO_DT_SPEC_INST_GET(index, trigger_gpios),         \
		.echo_gpios = GPIO_DT_SPEC_INST_GET(index, echo_gpios),               \
	};                                                                            \
                                                                                      \
	SENSOR_DEVICE_DT_INST_DEFINE(index, &hcsr04_init, NULL, &hcsr04_data_##index, \
				&hcsr04_config_##index, POST_KERNEL,                  \
				CONFIG_SENSOR_INIT_PRIORITY, &hcsr04_driver_api);     \

DT_INST_FOREACH_STATUS_OKAY(HC_SR04_INIT)
