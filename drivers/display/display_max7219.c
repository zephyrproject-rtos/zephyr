/*
 * Copyright (c) 2022 Jimmy Ou <yanagiis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max7219

/**
 * @file
 * @brief MAX7219 LED display driver
 *
 * This driver map the segment as x, digit as y.
 *
 * A MAX7219 has 8x8 pixels.
 * Two MAX7219s (with cascading) have 8x16 pixels.
 * So on and so forth.
 *
 * Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
 *
 * Limitations:
 *  1. This driver only implements no-decode mode.
 */

#include <stddef.h>

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(max7219, CONFIG_DISPLAY_LOG_LEVEL);

#define MAX7219_SEGMENTS_PER_DIGIT 8
#define MAX7219_DIGITS_PER_DEVICE  8

/* clang-format off */

/* MAX7219 registers and fields */
#define MAX7219_REG_NOOP		0x00
#define MAX7219_NOOP			0x00

#define MAX7219_REG_DECODE_MODE		0x09
#define MAX7219_NO_DECODE		0x00

#define MAX7219_REG_INTENSITY		0x0A

#define MAX7219_REG_SCAN_LIMIT		0x0B

#define MAX7219_REG_SHUTDOWN		0x0C
#define MAX7219_SHUTDOWN_MODE		0x00
#define MAX7219_LEAVE_SHUTDOWN_MODE	0x01

#define MAX7219_REG_DISPLAY_TEST	0x0F
#define MAX7219_LEAVE_DISPLAY_TEST_MODE	0x00
#define MAX7219_DISPLAY_TEST_MODE	0x01

/* clang-format on */

struct max7219_config {
	struct spi_dt_spec spi;
	uint32_t num_cascading;
	uint8_t intensity;
	uint8_t scan_limit;
};

struct max7219_data {
	/* Keep all digit_buf for all cascading MAX7219 */
	uint8_t *digit_buf;
	uint8_t *tx_buf;
};

static int max7219_transmit_all(const struct device *dev, const uint8_t addr, const uint8_t value)
{
	const struct max7219_config *dev_config = dev->config;
	struct max7219_data *dev_data = dev->data;

	const struct spi_buf tx_buf = {
		.buf = dev_data->tx_buf,
		.len = dev_config->num_cascading * 2,
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1U,
	};

	for (int i = 0; i < dev_config->num_cascading; i++) {
		dev_data->tx_buf[i * 2] = addr;
		dev_data->tx_buf[i * 2 + 1] = value;
	}

	return spi_write_dt(&dev_config->spi, &tx_bufs);
}

static int max7219_transmit_one(const struct device *dev, const uint8_t max7219_idx,
				const uint8_t addr, const uint8_t value)
{
	const struct max7219_config *dev_config = dev->config;
	struct max7219_data *dev_data = dev->data;

	const struct spi_buf tx_buf = {
		.buf = dev_data->tx_buf,
		.len = dev_config->num_cascading * 2,
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1U,
	};

	for (int i = 0; i < dev_config->num_cascading; i++) {
		if (i != (dev_config->num_cascading - 1 - max7219_idx)) {
			dev_data->tx_buf[i * 2] = MAX7219_REG_NOOP;
			dev_data->tx_buf[i * 2 + 1] = MAX7219_NOOP;
			continue;
		}

		dev_data->tx_buf[i * 2] = addr;
		dev_data->tx_buf[i * 2 + 1] = value;
	}

	return spi_write_dt(&dev_config->spi, &tx_bufs);
}

static inline uint8_t next_pixel(uint8_t *mask, uint8_t *data, const uint8_t **buf)
{
	*mask <<= 1;
	if (!*mask) {
		*mask = 0x01;
		*data = *(*buf)++;
	}
	return *data & *mask;
}

static inline void skip_pixel(uint8_t *mask, uint8_t *data, const uint8_t **buf, uint16_t count)
{
	while (count--) {
		next_pixel(mask, data, buf);
	}
}

static int max7219_blanking_on(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int max7219_blanking_off(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int max7219_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct max7219_config *dev_config = dev->config;
	struct max7219_data *dev_data = dev->data;

	const uint16_t max_width = MAX7219_SEGMENTS_PER_DIGIT;
	const uint16_t max_height = dev_config->num_cascading * MAX7219_DIGITS_PER_DEVICE;

	/*
	 * MAX7219 only supports PIXEL_FORMAT_MONO01. 1 bit stands for 1 pixel.
	 */
	__ASSERT((desc->pitch * desc->height) <= (desc->buf_size * 8U), "Input buffer to small");
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT(desc->pitch <= max_width, "Pitch in descriptor is larger than screen size");
	__ASSERT(desc->height <= max_height, "Height in descriptor is larger than screen size");
	__ASSERT(x + desc->pitch <= max_width,
		 "Writing outside screen boundaries in horizontal direction");
	__ASSERT(y + desc->height <= max_height,
		 "Writing outside screen boundaries in vertical direction");

	if (desc->width > desc->pitch || (desc->pitch * desc->height) > (desc->buf_size * 8U)) {
		return -EINVAL;
	}

	if ((x + desc->pitch) > max_width || (y + desc->height) > max_height) {
		return -EINVAL;
	}

	const uint16_t end_x = x + desc->width;
	const uint16_t end_y = y + desc->height;
	const uint8_t *byte_buf = buf;
	const uint16_t to_skip = desc->pitch - desc->width;
	uint8_t mask = 0;
	uint8_t data = 0;

	for (uint16_t py = y; py < end_y; ++py) {
		const uint8_t max7219_idx = py / MAX7219_DIGITS_PER_DEVICE;
		const uint8_t digit_idx = py % MAX7219_DIGITS_PER_DEVICE;
		uint8_t segment = dev_data->digit_buf[py];
		int ret;

		for (uint16_t px = x; px < end_x; ++px) {
			WRITE_BIT(segment, px, next_pixel(&mask, &data, &byte_buf));
		}

		skip_pixel(&mask, &data, &byte_buf, to_skip);

		/* led register address begins from 1 */
		ret = max7219_transmit_one(dev, max7219_idx, digit_idx + 1, segment);
		if (ret < 0) {
			return ret;
		}

		dev_data->digit_buf[y] = segment;
	}

	return 0;
}

static int max7219_read(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, void *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(x);
	ARG_UNUSED(y);
	ARG_UNUSED(desc);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}

static void *max7219_get_framebuffer(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NULL;
}

static int max7219_set_brightness(const struct device *dev, const uint8_t brightness)
{
	int ret;

	/*
	 * max7219 supports intensity value from 0x0 to 0xF.
	 * map the brightness from [0, 255] to [0, 15]
	 */
	ret = max7219_transmit_all(dev, MAX7219_REG_INTENSITY, brightness >> 4);
	if (ret < 0) {
		LOG_ERR("Failed to set brightness");
		return ret;
	}

	return 0;
}

static int max7219_set_contrast(const struct device *dev, const uint8_t contrast)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(contrast);

	return -ENOTSUP;
}

static int max7219_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format format)
{
	ARG_UNUSED(dev);

	switch (format) {
	case PIXEL_FORMAT_MONO01:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max7219_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void max7219_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct max7219_config *dev_config = dev->config;

	caps->x_resolution = MAX7219_SEGMENTS_PER_DIGIT;
	caps->y_resolution = MAX7219_DIGITS_PER_DEVICE * dev_config->num_cascading;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->screen_info = 0;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static const struct display_driver_api max7219_api = {
	.blanking_on = max7219_blanking_on,
	.blanking_off = max7219_blanking_off,
	.write = max7219_write,
	.read = max7219_read,
	.get_framebuffer = max7219_get_framebuffer,
	.set_brightness = max7219_set_brightness,
	.set_contrast = max7219_set_contrast,
	.get_capabilities = max7219_get_capabilities,
	.set_pixel_format = max7219_set_pixel_format,
	.set_orientation = max7219_set_orientation,
};

static int max7219_init(const struct device *dev)
{
	const struct max7219_config *dev_config = dev->config;
	struct max7219_data *dev_data = dev->data;
	int ret;

	if (!spi_is_ready(&dev_config->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	/* turn off all leds */
	memset(dev_data->digit_buf, 0,
	       dev_config->num_cascading * MAX7219_DIGITS_PER_DEVICE * sizeof(uint8_t));

	ret = max7219_transmit_all(dev, MAX7219_REG_DISPLAY_TEST, MAX7219_LEAVE_DISPLAY_TEST_MODE);
	if (ret < 0) {
		LOG_ERR("Failed to disable display test");
		return ret;
	}

	ret = max7219_transmit_all(dev, MAX7219_REG_DECODE_MODE, MAX7219_NO_DECODE);
	if (ret < 0) {
		LOG_ERR("Failed to set decode mode");
		return ret;
	}

	ret = max7219_transmit_all(dev, MAX7219_REG_INTENSITY, dev_config->intensity);
	if (ret < 0) {
		LOG_ERR("Failed to set global brightness");
		return ret;
	}

	ret = max7219_transmit_all(dev, MAX7219_REG_SCAN_LIMIT, dev_config->scan_limit);
	if (ret < 0) {
		LOG_ERR("Failed to set scan limit");
		return ret;
	}

	ret = max7219_transmit_all(dev, MAX7219_REG_SHUTDOWN, MAX7219_LEAVE_SHUTDOWN_MODE);
	if (ret < 0) {
		LOG_ERR("Failed to leave shutdown state");
		return ret;
	}

	const struct display_buffer_descriptor desc = {
		.buf_size = dev_config->num_cascading * MAX7219_DIGITS_PER_DEVICE,
		.height = dev_config->num_cascading * MAX7219_DIGITS_PER_DEVICE,
		.width = MAX7219_DIGITS_PER_DEVICE,
		.pitch = MAX7219_DIGITS_PER_DEVICE,
	};

	ret = max7219_write(dev, 0, 0, &desc, dev_data->digit_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define DISPLAY_MAX7219_INIT(n)                                                \
	static uint8_t max7219_digit_data_##n[DT_INST_PROP(n, num_cascading) * \
					      MAX7219_DIGITS_PER_DEVICE];      \
	static uint8_t max7219_tx_buf##n[DT_INST_PROP(n, num_cascading) * 2];  \
	static struct max7219_data max7219_data_##n = {                        \
		.digit_buf = max7219_digit_data_##n,                           \
		.tx_buf = max7219_tx_buf##n,                                   \
	};                                                                     \
	static const struct max7219_config max7219_config_##n = {              \
		.spi = SPI_DT_SPEC_INST_GET(                                   \
			n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),         \
		.num_cascading = DT_INST_PROP(n, num_cascading),               \
		.intensity = DT_INST_PROP(n, intensity),                       \
		.scan_limit = DT_INST_PROP(n, scan_limit),                     \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(n, max7219_init, NULL, &max7219_data_##n,        \
			      &max7219_config_##n, POST_KERNEL,                \
			      CONFIG_DISPLAY_INIT_PRIORITY, &max7219_api);

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_MAX7219_INIT)
