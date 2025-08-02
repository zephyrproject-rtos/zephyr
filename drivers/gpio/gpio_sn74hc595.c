/*
 * Copyright (c) 2022 Matthias Freese
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_sn74hc595

/**
 * @file Driver for 74 HC shift register
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_sn74hc595, CONFIG_GPIO_LOG_LEVEL);

#if CONFIG_SPI_INIT_PRIORITY >= CONFIG_GPIO_SN74HC595_INIT_PRIORITY
#error SPI_INIT_PRIORITY must be lower than SN74HC595_INIT_PRIORITY
#endif

struct gpio_sn74hc595_config {
	struct gpio_driver_config config;

	struct spi_dt_spec bus;
	struct gpio_dt_spec enable_gpio;
	struct gpio_dt_spec reset_gpio;

	uint8_t num_registers;
};

struct gpio_sn74hc595_drv_data {
	struct gpio_driver_data data;

	struct k_mutex lock;
	uint32_t output;
};

static int sn74hc595_write(const struct device *dev, uint32_t value)
{
	int ret;
	uint32_t adjusted_value;
	const struct gpio_sn74hc595_config *config = dev->config;
	struct gpio_sn74hc595_drv_data *drv_data = dev->data;

	__ASSERT(!k_is_in_isr(), "attempt to access SPI from ISR");

	adjusted_value = sys_cpu_to_be32(
		value << (BITS_PER_BYTE * (sizeof(uint32_t) - config->num_registers)));

	struct spi_buf tx_buf[] = {
		{.buf = (uint8_t *)&adjusted_value, .len = config->num_registers}};
	const struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};

	ret = spi_write_dt(&config->bus, &tx);

	if (ret == 0) {
		drv_data->output = value;
	}

	return ret;
}

static int gpio_sn74hc595_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(flags);
	return 0;
}

static int gpio_sn74hc595_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_sn74hc595_drv_data *drv_data = dev->data;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	*value = drv_data->output;

	k_mutex_unlock(&drv_data->lock);

	return 0;
}

static int gpio_sn74hc595_port_set_masked_raw(const struct device *dev, uint32_t mask,
					      uint32_t value)
{
	struct gpio_sn74hc595_drv_data *drv_data = dev->data;
	int ret = 0;
	uint32_t output;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	/* check if we need to do something at all      */
	/* current output differs from new masked value */
	if ((drv_data->output & mask) != (mask & value)) {
		output = (drv_data->output & ~mask) | (mask & value);

		ret = sn74hc595_write(dev, output);
	}

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int gpio_sn74hc595_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return gpio_sn74hc595_port_set_masked_raw(dev, mask, mask);
}

static int gpio_sn74hc595_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return gpio_sn74hc595_port_set_masked_raw(dev, mask, 0U);
}

static int gpio_sn74hc595_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	struct gpio_sn74hc595_drv_data *drv_data = dev->data;
	int ret;
	uint32_t toggled_output;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	toggled_output = drv_data->output ^ mask;

	ret = sn74hc595_write(dev, toggled_output);

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static DEVICE_API(gpio, gpio_sn74hc595_drv_api_funcs) = {
	.pin_configure = gpio_sn74hc595_config,
	.port_get_raw = gpio_sn74hc595_port_get_raw,
	.port_set_masked_raw = gpio_sn74hc595_port_set_masked_raw,
	.port_set_bits_raw = gpio_sn74hc595_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sn74hc595_port_clear_bits_raw,
	.port_toggle_bits = gpio_sn74hc595_port_toggle_bits,
};

/**
 * @brief Initialization function of sn74hc595
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_sn74hc595_init(const struct device *dev)
{
	int ret;
	struct gpio_sn74hc595_drv_data *drv_data = dev->data;
	const struct gpio_sn74hc595_config *config = dev->config;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->enable_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->enable_gpio)) {
			LOG_ERR("GPIO port %s not ready", config->enable_gpio.port->name);
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_INACTIVE) < 0) {
			LOG_ERR("Unable to configure ENABLE GPIO pin %u", config->enable_gpio.pin);
			return -EINVAL;
		}
	}

	if (config->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			LOG_ERR("GPIO port %s not ready", config->reset_gpio.port->name);
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE) < 0) {
			LOG_ERR("Unable to configure RST GPIO pin %u", config->reset_gpio.pin);
			return -EINVAL;
		}

		/* The reset signal must be stable for at least 120 ns */
		k_busy_wait(1);

		gpio_pin_set_dt(&config->reset_gpio, 0);

		/* The reset signal must be stable for at least 75 ns before
		 * clocking the SRCLK pin
		 */
		k_busy_wait(1);
	}

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	/* `drv_data->output` is initialized with the reset_value property from the device tree */
	ret = sn74hc595_write(dev, drv_data->output);

	k_mutex_unlock(&drv_data->lock);

	if (config->enable_gpio.port != NULL) {
		gpio_pin_set_dt(&config->enable_gpio, 1);
	}

	return ret;
}

#define SN74HC595_SPI_OPERATION                                                                    \
	((uint16_t)(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)))

#define SN74HC595_INIT(n)                                                                          \
	static struct gpio_sn74hc595_drv_data sn74hc595_data_##n = {                               \
		.output = DT_INST_PROP(n, reset_value),                                            \
		.lock = Z_MUTEX_INITIALIZER(sn74hc595_data_##n.lock),                              \
	};                                                                                         \
                                                                                                   \
	static const struct gpio_sn74hc595_config sn74hc595_config_##n = {                         \
		.config =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.bus = SPI_DT_SPEC_INST_GET(n, SN74HC595_SPI_OPERATION, 0),                        \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpio, {0}),                        \
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(n, enable_gpios, {0}),                     \
		.num_registers = DT_INST_PROP(n, ngpios) / BITS_PER_BYTE,                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(DT_DRV_INST(n), &gpio_sn74hc595_init, NULL, &sn74hc595_data_##n,          \
			 &sn74hc595_config_##n, POST_KERNEL, CONFIG_GPIO_SN74HC595_INIT_PRIORITY,  \
			 &gpio_sn74hc595_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(SN74HC595_INIT)
