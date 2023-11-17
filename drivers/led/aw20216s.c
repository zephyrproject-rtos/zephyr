/*
 * Copyright (c) 2023 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw20216s

/**
 * @file
 * @brief LED driver for the AW20216S SPI LED driver
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/led/aw20216s.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aw20216s, LOG_LEVEL);

#include "led_context.h"


#define AW20216S_MIN_BRIGHTNESS 0
#define AW20216S_MAX_BRIGHTNESS 255

#define AW20216S_SPI_SPEC_CONF (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | \
								SPI_OP_MODE_MASTER)


struct aw20216s_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec enable;
	struct gpio_dt_spec sync;
	uint8_t current_limit;
	uint8_t sl_current_limit;
};

struct aw20216s_data {
	struct led_data dev_data;
};

static int aw20216s_write_register(
		const struct device *dev,
		uint8_t reg,
		uint8_t page,
		uint8_t val
)
{
	if (page > AW20216S_PAGE_4) {
		return -EINVAL;
	}

	const struct aw20216s_config *config = dev->config;

	uint8_t cmd[] = {AW20216S_CHIP_ID | (page << 1) | AW20216S_WRITE, reg, val};

	const struct spi_buf tx[] = {
		{
		.buf = &cmd,
		.len = sizeof(cmd),
		},
	};

	struct spi_buf_set tx_buf_set = {
		.buffers = tx,
		.count = 1,
	};

	return spi_write_dt(&config->spi, &tx_buf_set);
}

static int aw20216s_read_register(
		const struct device *dev,
		uint8_t reg,
		uint8_t page,
		uint8_t *rx_buf,
		size_t len
)
{
	if (page > AW20216S_PAGE_4) {
		return -EINVAL;
	}

	const struct aw20216s_config *config = dev->config;

	uint8_t read_cmd[] = {AW20216S_CHIP_ID | (page << 1) | AW20216S_READ, reg};

	struct spi_buf transmit_spi_bufs[] = {
		{
			.buf = &read_cmd,
			.len = sizeof(read_cmd),
		},
		{
			.buf = NULL,
			.len = len,
		},
	};

	struct spi_buf_set transmit_spi_buf_set = {
		.buffers = transmit_spi_bufs,
		.count = ARRAY_SIZE(transmit_spi_bufs),
	};

	struct spi_buf receive_spi_bufs[] = {
		{
			.buf = NULL,
			.len = sizeof(read_cmd),
		},
		{
			.buf = rx_buf,
			.len = len,
		},
	};

	struct spi_buf_set receive_spi_buf_set = {
		.buffers = receive_spi_bufs,
		.count = ARRAY_SIZE(receive_spi_bufs),
	};

	return spi_transceive_dt(&config->spi, &transmit_spi_buf_set, &receive_spi_buf_set);
}

static int aw20216s_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	struct aw20216s_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	if (value < dev_data->min_brightness || value > dev_data->max_brightness) {
		return -EINVAL;
	}

	uint8_t val = (value * 255U) / dev_data->max_brightness;

	int err = aw20216s_write_register(
			dev,
			AW20216S_PWM_CONFIGURATION_REGISTER_BASE + led,
			AW20216S_PAGE_1,
			val
	);

	if (err) {
		LOG_ERR("Failed to set PWM configuration register %d", led);
		return err;
	}

	return 0;
}

static inline int aw20216s_led_on(const struct device *dev, uint32_t led)
{
	struct aw20216s_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	if (led > AW20216S_NUM_PWM_CONFIG_REGISTERS) {
		return -EINVAL;
	}

	LOG_DBG("LED %d on", led);

	int err = aw20216s_write_register(
			dev,
			AW20216S_PWM_CONFIGURATION_REGISTER_BASE + led,
			AW20216S_PAGE_1,
			dev_data->max_brightness
	);

	if (err) {
		LOG_ERR("Failed to enable LED %d", led);
	}

	return 0;
}

static inline int aw20216s_led_off(const struct device *dev, uint32_t led)
{
	if (led > AW20216S_NUM_PWM_CONFIG_REGISTERS) {
		return -EINVAL;
	}

	LOG_DBG("LED %d off", led);

	int err = aw20216s_write_register(
			dev,
			AW20216S_PWM_CONFIGURATION_REGISTER_BASE + led,
			AW20216S_PAGE_1,
			0
	);
	if (err) {
		LOG_ERR("Failed to disable LED %d", led);
	}

	return 0;
}

static int aw20216s_led_init(const struct device *dev)
{
	const struct aw20216s_config *config = dev->config;
	struct aw20216s_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	int err;

#if DT_INST_NODE_HAS_PROP(0, en_gpios)
	if (!device_is_ready(config->enable.port)) {
		LOG_ERR("Enable GPIO port %s not ready", config->enable.port->name);
		return -ENODEV;
	}
	gpio_pin_configure_dt(&config->enable, GPIO_OUTPUT);
	gpio_pin_set_dt(&config->enable, 1);
	k_msleep(2);
#endif

	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	dev_data->min_brightness = AW20216S_MIN_BRIGHTNESS;
	dev_data->max_brightness = AW20216S_MAX_BRIGHTNESS;

	err = aw20216s_write_register(
			dev,
			AW20216S_RESET_REGISTER,
			AW20216S_PAGE_0,
			AW20216S_DEFAULT_RESET_REGISTER_VALUE
	);
	if (err) {
		LOG_ERR("Failed to reset AW20216S");
		return err;
	}
	k_msleep(2);

	err = aw20216s_write_register(
			dev,
			AW20216S_GLOBAL_CONTROL_REGISTER,
			AW20216S_PAGE_0,
			(AW20216S_GLOBAL_CONTROL_REGISTER_VALUE_ALL_SW | \
			AW20216S_GLOBAL_CONTROL_REGISTER_CHIP_ENABLE)
	);
	if (err) {
		LOG_ERR("Failed to enable LED driver");
		return err;
	}

	err = aw20216s_write_register(
			dev,
			AW20216S_GLOBAL_CURRENT_CONTROL_REGISTER,
			AW20216S_PAGE_0,
			config->current_limit
	);

	if (err) {
		LOG_ERR("Failed to set global current limit");
		return err;
	}

	for (int i = 0; i < AW20216S_NUM_SOURCE_LEVEL_CONFIG_REGISTERS; i++) {
		err = aw20216s_write_register(
				dev,
				AW20216S_SOURCE_LEVEL_CONFIGURATION_REGISTER_BASE + i,
				AW20216S_PAGE_2,
				config->sl_current_limit
		);

		if (err) {
			LOG_ERR("Failed to set source level configuration register %d", i);
			return err;
		}
	}

	return 0;
}

static const struct led_driver_api aw20216s_led_api = {
	.set_brightness = aw20216s_led_set_brightness,
	.on = aw20216s_led_on,
	.off = aw20216s_led_off,
};

#define AW20216S_DEVICE(id)                                                                       \
	static const struct aw20216s_config aw20216s_##id##_cfg = {                               \
			.spi = SPI_DT_SPEC_INST_GET(id, AW20216S_SPI_SPEC_CONF, 0),               \
			.enable = GPIO_DT_SPEC_INST_GET_OR(id, en_gpios, {0}),                    \
			.sync = GPIO_DT_SPEC_INST_GET_OR(id, sync_gpios, {0}),                    \
			.current_limit = DT_INST_PROP(id, current_limit),                         \
			.sl_current_limit = DT_INST_PROP(id, sl_current_limit)                    \
	};		                                                                          \
	static struct aw20216s_data aw20216s_##id##_data;                                         \
                                                                                                  \
	DEVICE_DT_INST_DEFINE(id, &aw20216s_led_init, NULL, &aw20216s_##id##_data,                \
			      &aw20216s_##id##_cfg, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,        \
			      &aw20216s_led_api);

DT_INST_FOREACH_STATUS_OKAY(AW20216S_DEVICE)
