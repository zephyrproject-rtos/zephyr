/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <led_strip.h>
#include <spi.h>

struct apa102_data {
	struct device *spi;
	struct spi_config cfg;
};

static int apa102_update(struct device *dev, void *buf, size_t size)
{
	struct apa102_data *data = dev->driver_data;
	static const u8_t zeros[] = {0, 0, 0, 0};
	static const u8_t ones[] = {0xFF, 0xFF, 0xFF, 0xFF};
	const struct spi_buf tx_bufs[] = {
		{
			/* Start frame: at least 32 zeros */
			.buf = (u8_t *)zeros,
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
			.buf = (u8_t *)ones,
			.len = sizeof(ones),
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx)
	};

	return spi_write(data->spi, &data->cfg, &tx);
}

static int apa102_update_rgb(struct device *dev, struct led_rgb *pixels,
			     size_t count)
{
	u8_t *p = (u8_t *)pixels;
	size_t i;
	/* SOF (3 bits) followed by the 0 to 31 global dimming level */
	u8_t prefix = 0xE0 | 31;

	/* Rewrite to the on-wire format */
	for (i = 0; i < count; i++) {
		u8_t r = pixels[i].r;
		u8_t g = pixels[i].g;
		u8_t b = pixels[i].b;

		*p++ = prefix;
		*p++ = b;
		*p++ = g;
		*p++ = r;
	}

	BUILD_ASSERT(sizeof(struct led_rgb) == 4);
	return apa102_update(dev, pixels, sizeof(struct led_rgb) * count);
}

static int apa102_update_channels(struct device *dev, u8_t *channels,
				  size_t num_channels)
{
	/* Not implemented */
	return -EINVAL;
}

static int apa102_init(struct device *dev)
{
	struct apa102_data *data = dev->driver_data;

	data->spi = device_get_binding(CONFIG_APA102_STRIP_BUS_NAME);
	if (!data->spi) {
		return -ENODEV;
	}

	data->cfg.frequency = CONFIG_APA102_STRIP_FREQUENCY;
	data->cfg.operation =
		SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8);

	return 0;
}

static struct apa102_data apa102_data_0;

static const struct led_strip_driver_api apa102_api = {
	.update_rgb = apa102_update_rgb,
	.update_channels = apa102_update_channels,
};

DEVICE_AND_API_INIT(apa102_0, CONFIG_APA102_STRIP_NAME, apa102_init,
		    &apa102_data_0, NULL, POST_KERNEL,
		    CONFIG_LED_STRIP_INIT_PRIORITY, &apa102_api);
