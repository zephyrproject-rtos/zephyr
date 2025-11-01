/*
 * Copyright (c) 2025 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sct_sct2024

/**
 * @file
 * @brief LED driver for the SCT2024 LED driver
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sct2024, CONFIG_LED_LOG_LEVEL);

#define SCT2024_LED_COUNT 16
/* SCT2024 can be chained up to control more LEDs.
 * The driver can be extended to support chaining.
 */
#define SCT2024_MAX_CHAIN_LENGTH 1

struct sct2024_cfg {
	const struct device *spi_dev;
	struct spi_config spi_cfg;
	const struct gpio_dt_spec la_pin;
	const struct gpio_dt_spec oe_pin;
	const struct led_info *leds_info;
};

struct sct2024_data {
	uint16_t led_bitmap[SCT2024_MAX_CHAIN_LENGTH]; /* Bitmap to track LED states */
};

static const struct led_info *sct2024_get_led_info(const struct device *dev, uint32_t led)
{
	const struct sct2024_cfg *cfg = dev->config;

	if (led >= SCT2024_LED_COUNT * SCT2024_MAX_CHAIN_LENGTH) {
		return NULL;
	}

	return &cfg->leds_info[led];
}

static int sct2024_get_led_index(const struct device *dev, uint32_t led)
{
	const struct led_info *led_info = sct2024_get_led_info(dev, led);

	if (led_info == NULL) {
		return -EINVAL;
	}

	return led_info->index;
}

static int sct2024_spi_write(const struct device *dev)
{
	const struct sct2024_cfg *cfg = dev->config;
	struct sct2024_data *data = dev->data;
	uint8_t buffer[SCT2024_MAX_CHAIN_LENGTH * 2] = {0};

	/* Feature proposal: When SCT2024_MAX_CHAIN_LENGTH is greater than 1, limit the number of
	 * iterations based on the greatest led index for the chain - detect the actual chain length
	 */
	BUILD_ASSERT(SCT2024_MAX_CHAIN_LENGTH == 1,
		     "Driver currently supports only a single SCT2024 device in the chain");
	for (int i = 0; i < SCT2024_MAX_CHAIN_LENGTH; i++) {
		buffer[i * 2] = (data->led_bitmap[i] >> 8) & 0xFF; /* Upper 8 bits */
		buffer[i * 2 + 1] = data->led_bitmap[i] & 0xFF; /* Lower 8 bits */
	}

	const struct spi_buf tx_buf = {
		.buf = buffer,
		.len = sizeof(buffer),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	int ret = spi_write(cfg->spi_dev, &cfg->spi_cfg, &tx);

	/* Toggle LA pin to latch data */
	gpio_pin_set(cfg->la_pin.port, cfg->la_pin.pin, 1);
	k_sleep(K_NSEC(20));
	gpio_pin_set(cfg->la_pin.port, cfg->la_pin.pin, 0);

	return ret;
}

static int sct2024_write(const struct device *dev)
{
	const struct sct2024_cfg *cfg = dev->config;
	struct sct2024_data *data = dev->data;

	if (cfg->oe_pin.port) {
		bool all_leds_off = true;

		for (int i = 0; i < SCT2024_MAX_CHAIN_LENGTH; i++) {
			if (data->led_bitmap[i] != 0) {
				all_leds_off = false;
				break;
			}
		}

		if (all_leds_off) {
			gpio_pin_set(cfg->oe_pin.port, cfg->oe_pin.pin, 0);
			return 0;
		}
	}

	int ret = sct2024_spi_write(dev);

	if (cfg->oe_pin.port) {
		gpio_pin_set(cfg->oe_pin.port, cfg->oe_pin.pin, 1);
	}

	return ret;
}
static int sct2024_led_on(const struct device *dev, uint32_t led)
{
	struct sct2024_data *data = dev->data;
	int led_index;

	led_index = sct2024_get_led_index(dev, led);
	if (led_index < 0) {
		return led_index;
	}

	data->led_bitmap[led_index / 16] |= (1 << (led_index % 16));

	return sct2024_write(dev);
}

static int sct2024_led_off(const struct device *dev, uint32_t led)
{
	struct sct2024_data *data = dev->data;
	int led_index;

	led_index = sct2024_get_led_index(dev, led);
	if (led_index < 0) {
		return led_index;
	}

	data->led_bitmap[led_index / 16] &= ~(1 << (led_index % 16));

	return sct2024_write(dev);
}

static const struct led_driver_api sct2024_led_api = {
	.on = sct2024_led_on,
	.off = sct2024_led_off,
};

static int sct2024_init(const struct device *dev)
{
	const struct sct2024_cfg *cfg = dev->config;

	if (!device_is_ready(cfg->spi_dev)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->la_pin.port)) {
		LOG_ERR("LA GPIO device not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&cfg->la_pin, GPIO_OUTPUT_INACTIVE) < 0) {
		LOG_ERR("Failed to configure LA pin");
		return -EIO;
	}

	if (cfg->oe_pin.port && !device_is_ready(cfg->oe_pin.port)) {
		LOG_ERR("OE GPIO device not ready");
		return -ENODEV;
	}

	if (cfg->oe_pin.port && gpio_pin_configure_dt(&cfg->oe_pin, GPIO_OUTPUT_ACTIVE) < 0) {
		LOG_ERR("Failed to configure OE pin");
		return -EIO;
	}

	return 0;
}

#define SCT2024_LED_INFO_INIT(child) \
	{ \
		.index = DT_PROP(child, index), \
		.label = DT_PROP_OR(child, label, NULL), \
	},

#define SCT2024_INIT(inst) \
	static struct sct2024_data sct2024_data_##inst; \
	static const struct led_info sct2024_leds_##inst[] = { \
		DT_INST_FOREACH_CHILD(inst, SCT2024_LED_INFO_INIT) \
	}; \
	static const struct sct2024_cfg sct2024_cfg_##inst = { \
		.spi_dev = DEVICE_DT_GET(DT_INST_BUS(inst)), \
		.spi_cfg = { \
			.frequency = DT_INST_PROP(inst, spi_max_frequency), \
			.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB, \
			.slave = DT_INST_REG_ADDR(inst), \
		}, \
		.la_pin = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), la_gpios), \
		.oe_pin = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), oe_gpios, {0}), \
		.leds_info = sct2024_leds_##inst, \
	}; \
	DEVICE_DT_INST_DEFINE(inst, sct2024_init, NULL, \
			      &sct2024_data_##inst, &sct2024_cfg_##inst, \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, \
			      &sct2024_led_api);

DT_INST_FOREACH_STATUS_OKAY(SCT2024_INIT)
