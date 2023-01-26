/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT apa_apa102

#include <errno.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

struct apa102_config {
	struct spi_dt_spec bus;
};

static int apa102_update(const struct device *dev, void *buf, size_t size)
{
	const struct apa102_config *config = dev->config;
	static const uint8_t zeros[] = { 0, 0, 0, 0 };
	static const uint8_t ones[] = { 0xFF, 0xFF, 0xFF, 0xFF };
	const struct spi_buf tx_bufs[] = {
		{
			/* Start frame: at least 32 zeros */
			.buf = (uint8_t *)zeros,
			.len = sizeof(zeros),
		},
		{
			/* LED data itself */
			.buf = buf,
			.len = size,
		},
		{
			/* End frame: at least 32 ones to clock the
			 * remaining bits to the LEDs at the end of
			 * the strip.
			 */
			.buf = (uint8_t *)ones,
			.len = sizeof(ones),
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	return spi_write_dt(&config->bus, &tx);
}

static int apa102_update_rgb(const struct device *dev, struct led_rgb *pixels,
			     size_t count)
{
	uint8_t *p = (uint8_t *)pixels;
	size_t i;
	/* SOF (3 bits) followed by the 0 to 31 global dimming level */
	uint8_t prefix = 0xE0 | 31;

	/* Rewrite to the on-wire format */
	for (i = 0; i < count; i++) {
		uint8_t r = pixels[i].r;
		uint8_t g = pixels[i].g;
		uint8_t b = pixels[i].b;

		*p++ = prefix;
		*p++ = b;
		*p++ = g;
		*p++ = r;
	}

	BUILD_ASSERT(sizeof(struct led_rgb) == 4);
	return apa102_update(dev, pixels, sizeof(struct led_rgb) * count);
}

static int apa102_update_channels(const struct device *dev, uint8_t *channels,
				  size_t num_channels)
{
	/* Not implemented */
	return -EINVAL;
}

static int apa102_init(const struct device *dev)
{
	const struct apa102_config *config = dev->config;

	if (!spi_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	return 0;
}

static const struct apa102_config apa102_config = {
	.bus = SPI_DT_SPEC_INST_GET(
		0, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0)
};

static const struct led_strip_driver_api apa102_api = {
	.update_rgb = apa102_update_rgb,
	.update_channels = apa102_update_channels,
};

DEVICE_DT_INST_DEFINE(0, apa102_init, NULL,
		      NULL, &apa102_config, POST_KERNEL,
		      CONFIG_LED_STRIP_INIT_PRIORITY, &apa102_api);
