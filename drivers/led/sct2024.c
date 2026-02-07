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
#include <zephyr/sys/math_extras.h>

LOG_MODULE_REGISTER(sct2024, CONFIG_LED_LOG_LEVEL);

#define SCT2024_LED_COUNT 16
/* SCT2024 can be chained up to control more LEDs.
 * The driver can be extended to support chaining.
 */
#define SCT2024_MAX_CHAIN_LENGTH 1

struct sct2024_cfg {
	const struct spi_dt_spec spi;
	const struct gpio_dt_spec la_pin;
	const struct gpio_dt_spec oe_pin;
	const struct led_info *leds_info;
};

struct sct2024_data {
	uint16_t led_bitmap[SCT2024_MAX_CHAIN_LENGTH];
};

static bool sct2024_is_oe_pin_defined(const struct device *dev);

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
	uint8_t buffer[SCT2024_MAX_CHAIN_LENGTH * 2];
	const struct spi_buf tx_buf = {
		.buf = buffer,
		.len = sizeof(buffer),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	int ret;

	for (int i = 0; i < SCT2024_MAX_CHAIN_LENGTH; i++) {
		buffer[i * 2] = (data->led_bitmap[i] >> 8) & 0xFF; /* Upper 8 bits */
		buffer[i * 2 + 1] = data->led_bitmap[i] & 0xFF; /* Lower 8 bits */
	}

	ret = spi_write_dt(&cfg->spi, &tx);

	/* Toggle LA pin to latch data */
	gpio_pin_set(cfg->la_pin.port, cfg->la_pin.pin, 1);
	/* The specification mandates that the pin must remain high for a minimum duration of 20ns.
	 * To prevent unnecessary CPU usage through busy waiting, k_sleep is utilized here.
	 * This approach is generally equivalent to k_yield(), allowing other threads to execute.
	 */
	k_sleep(K_NSEC(20));
	gpio_pin_set(cfg->la_pin.port, cfg->la_pin.pin, 0);

	return ret;
}

static int sct2024_write(const struct device *dev)
{
	const struct sct2024_cfg *cfg = dev->config;
	struct sct2024_data *data = dev->data;
	bool all_leds_off = true;
	int ret;

	if (sct2024_is_oe_pin_defined(dev)) {
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

	ret = sct2024_spi_write(dev);

	if (sct2024_is_oe_pin_defined(dev)) {
		gpio_pin_set(cfg->oe_pin.port, cfg->oe_pin.pin, 1);
	}

	return ret;
}

static int sct2024_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct led_info *led_info = sct2024_get_led_info(dev, led);

	if (!led_info) {
		return -EINVAL;
	}

	*info = led_info;

	return 0;
}

static int sct2024_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	struct sct2024_data *data = dev->data;
	int led_index;

	led_index = sct2024_get_led_index(dev, led);
	if (led_index < 0) {
		return led_index;
	}

	if (value > 0) {
		data->led_bitmap[led_index / 16] |= BIT(led_index % 16);
	} else {
		data->led_bitmap[led_index / 16] &= ~BIT(led_index % 16);
	}

	return sct2024_write(dev);
}

static int sct2024_led_write_channels(const struct device *dev, uint32_t start_channel,
				      uint32_t num_channels, const uint8_t *brightness_values)
{
	struct sct2024_data *data = dev->data;
	uint32_t end_channel;
	uint16_t led_bitmap[SCT2024_MAX_CHAIN_LENGTH];
	const size_t led_bitmap_size = MIN(sizeof(led_bitmap), sizeof(data->led_bitmap));
	int ret = 0;

	__ASSERT(brightness_values != NULL, "Passed NULL as brightness_values array.");

	if (u32_add_overflow(start_channel, num_channels, &end_channel)) {
		LOG_ERR("Channel range overflow: start=%u, num=%u", start_channel, num_channels);
		return -EINVAL;
	}

	if (end_channel > SCT2024_LED_COUNT * SCT2024_MAX_CHAIN_LENGTH) {
		LOG_ERR("Channel range out of bounds: end=%u, max=%u", end_channel,
			SCT2024_LED_COUNT * SCT2024_MAX_CHAIN_LENGTH);
		return -EINVAL;
	}

	memcpy(led_bitmap, data->led_bitmap, led_bitmap_size);

	for (uint32_t i = 0; i < num_channels; i++) {
		int led_index = sct2024_get_led_index(dev, start_channel + i);
		uint32_t mask_index;
		uint8_t bit_index;

		if (led_index < 0) {
			LOG_ERR("Invalid LED index for channel %u", start_channel + i);
			ret = led_index;
			continue;
		}

		if (led_index >= SCT2024_MAX_CHAIN_LENGTH * SCT2024_LED_COUNT) {
			LOG_ERR("LED index out of bounds: index=%d, max=%u", led_index,
				SCT2024_MAX_CHAIN_LENGTH * SCT2024_LED_COUNT);
			continue;
		}

		mask_index = led_index / 16;
		bit_index = led_index % 16;

		if (brightness_values[i] > 0) {
			led_bitmap[mask_index] |= BIT(bit_index);
		} else {
			led_bitmap[mask_index] &= ~BIT(bit_index);
		}
	}

	if (ret == 0) {
		memcpy(data->led_bitmap, led_bitmap, led_bitmap_size);
		ret = sct2024_write(dev);
	}

	return ret;
}

static DEVICE_API(led, sct2024_led_api) = {
	.get_info = sct2024_get_info,
	.set_brightness = sct2024_set_brightness,
	.write_channels = sct2024_led_write_channels,
};

static int sct2024_init(const struct device *dev)
{
	const struct sct2024_cfg *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->la_pin)) {
		LOG_ERR("LA GPIO device not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&cfg->la_pin, GPIO_OUTPUT_INACTIVE) < 0) {
		LOG_ERR("Failed to configure LA pin");
		return -EIO;
	}

	if (sct2024_is_oe_pin_defined(dev)) {
		if (!gpio_is_ready_dt(&cfg->oe_pin)) {
			LOG_ERR("OE GPIO device not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&cfg->oe_pin, GPIO_OUTPUT_ACTIVE) < 0) {
			LOG_ERR("Failed to configure OE pin");
			return -EIO;
		}
	}

	return 0;
}

static bool sct2024_is_oe_pin_defined(const struct device *dev)
{
	const struct sct2024_cfg *cfg = dev->config;

	return cfg->oe_pin.port != NULL;
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
		.spi = SPI_DT_SPEC_INST_GET(inst, \
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB), \
		.la_pin = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), la_gpios), \
		.oe_pin = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), oe_gpios, {0}), \
		.leds_info = sct2024_leds_##inst, \
	}; \
	DEVICE_DT_INST_DEFINE(inst, sct2024_init, NULL, \
			      &sct2024_data_##inst, &sct2024_cfg_##inst, \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, \
			      &sct2024_led_api);

DT_INST_FOREACH_STATUS_OKAY(SCT2024_INIT)

BUILD_ASSERT(SCT2024_MAX_CHAIN_LENGTH == 1,
		     "Driver currently supports only a single SCT2024 device in the chain");
