/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led_strip.h>

#include <errno.h>
#include <string.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, greeled_lpd8806), okay)
#define DT_DRV_COMPAT greeled_lpd8806
#else
#define DT_DRV_COMPAT greeled_lpd8803
#endif

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lpd880x);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

/*
 * LPD880X SPI master configuration:
 *
 * - mode 0 (the default), 8 bit, MSB first, one-line SPI
 * - no shenanigans (no CS hold, release device lock, not an EEPROM)
 */
#define LPD880X_SPI_OPERATION (SPI_OP_MODE_MASTER | \
			       SPI_TRANSFER_MSB |   \
			       SPI_WORD_SET(8))

struct lpd880x_config {
	struct spi_dt_spec bus;
};

static int lpd880x_update(const struct device *dev, void *data, size_t size)
{
	const struct lpd880x_config *config = dev->config;
	/*
	 * Per the AdaFruit reverse engineering notes on the protocol,
	 * a zero byte propagates through at most 32 LED driver ICs.
	 * The LPD8803 is the worst case, at 3 output channels per IC.
	 */
	uint8_t reset_size = ceiling_fraction(ceiling_fraction(size, 3), 32);
	uint8_t reset_buf[reset_size];
	uint8_t last = 0x00;
	const struct spi_buf bufs[3] = {
		{
			/* Prepares the strip to shift in new data values. */
			.buf = reset_buf,
			.len = reset_size
		},
		{
			/* Displays the serialized pixel data. */
			.buf = data,
			.len = size
		},
		{
			/* Ensures the last byte of pixel data is displayed. */
			.buf = &last,
			.len = sizeof(last)
		}

	};
	const struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 3
	};
	size_t rc;

	(void)memset(reset_buf, 0x00, reset_size);

	rc = spi_write_dt(&config->bus, &tx);
	if (rc) {
		LOG_ERR("can't update strip: %d", rc);
	}

	return rc;
}

static int lpd880x_strip_update_rgb(const struct device *dev,
				    struct led_rgb *pixels,
				    size_t num_pixels)
{
	uint8_t *px = (uint8_t *)pixels;
	uint8_t r, g, b;
	size_t i;

	/*
	 * Overwrite a prefix of the pixels array with its on-wire
	 * representation, eliminating padding/scratch garbage, if any.
	 */
	for (i = 0; i < num_pixels; i++) {
		r = 0x80 | (pixels[i].r >> 1);
		g = 0x80 | (pixels[i].g >> 1);
		b = 0x80 | (pixels[i].b >> 1);

		/*
		 * GRB is the ordering used by commonly available
		 * LPD880x strips.
		 */
		*px++ = g;
		*px++ = r;
		*px++ = b;
	}

	return lpd880x_update(dev, pixels, 3 * num_pixels);
}

static int lpd880x_strip_update_channels(const struct device *dev,
					 uint8_t *channels,
					 size_t num_channels)
{
	size_t i;

	for (i = 0; i < num_channels; i++) {
		channels[i] = 0x80 | (channels[i] >> 1);
	}

	return lpd880x_update(dev, channels, num_channels);
}

static int lpd880x_strip_init(const struct device *dev)
{
	const struct lpd880x_config *config = dev->config;

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI device %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

static const struct lpd880x_config lpd880x_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, LPD880X_SPI_OPERATION, 0)
};

static const struct led_strip_driver_api lpd880x_strip_api = {
	.update_rgb = lpd880x_strip_update_rgb,
	.update_channels = lpd880x_strip_update_channels,
};

DEVICE_DT_INST_DEFINE(0, lpd880x_strip_init, NULL,
		      NULL, &lpd880x_config,
		      POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,
		      &lpd880x_strip_api);
