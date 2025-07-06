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
	size_t length;
	uint8_t *const end_frame;
	const size_t end_frame_size;
};

static int apa102_update(const struct device *dev, void *buf, size_t size)
{
	const struct apa102_config *config = dev->config;
	static const uint8_t zeros[] = { 0, 0, 0, 0 };

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
			.buf = (uint8_t *)config->end_frame,
			.len = config->end_frame_size,
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

static size_t apa102_length(const struct device *dev)
{
	const struct apa102_config *config = dev->config;

	return config->length;
}

static int apa102_init(const struct device *dev)
{
	const struct apa102_config *config = dev->config;

	if (!spi_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	memset(config->end_frame, 0xFF, config->end_frame_size);

	return 0;
}

static DEVICE_API(led_strip, apa102_api) = {
	.update_rgb = apa102_update_rgb,
	.length = apa102_length,
};

/*
 * The "End frame" is statically allocated, as a sequence of 0xFF bytes
 * The only function of the “End frame” is to supply more clock pulses
 * to the string until the data has permeated to the last LED. The
 * number of clock pulses required is exactly half the total number
 * of LEDs in the string. See below `end_frame`.
 */
#define APA102_DEVICE(idx)						 \
	static uint8_t apa102_end_frame_##idx				 \
		[(DT_INST_PROP(idx, chain_length) /			 \
			sizeof(struct led_rgb) / 2) + 1];		 \
	static const struct apa102_config apa102_##idx##_config = {	 \
		.bus = SPI_DT_SPEC_INST_GET(				 \
			idx,						 \
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), \
			0),						 \
		.length = DT_INST_PROP(idx, chain_length),		 \
		.end_frame = apa102_end_frame_##idx,			 \
		.end_frame_size = (DT_INST_PROP(idx, chain_length) /	 \
				sizeof(struct led_rgb) / 2) + 1,	 \
	};								 \
									 \
	DEVICE_DT_INST_DEFINE(idx,					 \
			      apa102_init,				 \
			      NULL,					 \
			      NULL,					 \
			      &apa102_##idx##_config,			 \
			      POST_KERNEL,				 \
			      CONFIG_LED_STRIP_INIT_PRIORITY,		 \
			      &apa102_api);

DT_INST_FOREACH_STATUS_OKAY(APA102_DEVICE)
