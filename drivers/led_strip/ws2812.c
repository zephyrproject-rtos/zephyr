/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <led_strip.h>

#include <string.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LED_STRIP_LEVEL
#include <logging/sys_log.h>

#include <zephyr.h>
#include <device.h>
#include <spi.h>
#include <misc/util.h>

/*
 * WS2812-ish SPI master configuration:
 *
 * - mode 0 (the default), 8 bit, MSB first (arbitrary), one-line SPI
 * - no shenanigans (don't hold CS, don't hold the device lock, this
 *   isn't an EEPROM)
 */
#define SPI_OPER (SPI_OP_MODE_MASTER |		\
		  SPI_TRANSFER_MSB |		\
		  SPI_WORD_SET(8) |		\
		  SPI_LINES_SINGLE)

#define SPI_FREQ              CONFIG_WS2812_STRIP_SPI_BAUD_RATE
#define ONE_FRAME             CONFIG_WS2812_STRIP_ONE_FRAME
#define ZERO_FRAME            CONFIG_WS2812_STRIP_ZERO_FRAME
#define RED_OFFSET            (8 * sizeof(u8_t) * CONFIG_WS2812_RED_ORDER)
#define GRN_OFFSET            (8 * sizeof(u8_t) * CONFIG_WS2812_GRN_ORDER)
#define BLU_OFFSET            (8 * sizeof(u8_t) * CONFIG_WS2812_BLU_ORDER)
#ifdef CONFIG_WS2812_HAS_WHITE_CHANNEL
#define WHT_OFFSET            (8 * sizeof(u8_t) * CONFIG_WS2812_WHT_ORDER)
#else
#define WHT_OFFSET            -1
#endif

/*
 * Despite datasheet claims (see blog post link in Kconfig.ws2812), a
 * 6 microsecond pulse is enough to reset the strip. Convert that into
 * a number of 8 bit SPI frames, adding another just to be safe.
 */
#define RESET_NFRAMES ((size_t)ceiling_fraction(3 * SPI_FREQ, 4000000) + 1)

struct ws2812_data {
	struct device *spi;
	struct spi_config config;
};

/*
 * Convert a color channel's bits into a sequence of SPI frames (with
 * the proper pulse and inter-pulse widths) to shift out.
 */
static inline void ws2812_serialize_color(u8_t buf[8], u8_t color)
{
	int i;

	for (i = 0; i < 8; i++) {
		buf[i] = color & BIT(7 - i) ? ONE_FRAME : ZERO_FRAME;
	}
}

/*
 * Convert a pixel into SPI frames, returning the number of bytes used.
 */
static size_t ws2812_serialize_pixel(u8_t px[32], struct led_rgb *pixel)
{
	ws2812_serialize_color(px + RED_OFFSET, pixel->r);
	ws2812_serialize_color(px + GRN_OFFSET, pixel->g);
	ws2812_serialize_color(px + BLU_OFFSET, pixel->b);
	if (IS_ENABLED(CONFIG_WS2812_HAS_WHITE_CHANNEL)) {
		ws2812_serialize_color(px + WHT_OFFSET, 0); /* unused */
		return 32;
	}
	return 24;
}

/*
 * Latch current color values on strip and reset its state machines.
 */
static int ws2812_reset_strip(struct ws2812_data *data)
{
	u8_t reset_buf[RESET_NFRAMES];
	const struct spi_buf reset = {
		.buf = reset_buf,
		.len = sizeof(reset_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &reset,
		.count = 1
	};

	(void)memset(reset_buf, 0x00, sizeof(reset_buf));

	return spi_write(data->spi, &data->config, &tx);
}

static int ws2812_strip_update_rgb(struct device *dev, struct led_rgb *pixels,
				   size_t num_pixels)
{
	struct ws2812_data *drv_data = dev->driver_data;
	struct spi_config *config = &drv_data->config;
	u8_t px_buf[32]; /* 32 are needed when a white channel is present. */
	struct spi_buf buf = {
		.buf = px_buf,
	};
	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1
	};
	size_t i;
	int rc;

	for (i = 0; i < num_pixels; i++) {
		buf.len = ws2812_serialize_pixel(px_buf, &pixels[i]);
		rc = spi_write(drv_data->spi, config, &tx);
		if (rc) {
			/*
			 * Latch anything we've shifted out first, to
			 * call visual attention to the problematic
			 * pixel.
			 */
			(void)ws2812_reset_strip(drv_data);
			SYS_LOG_ERR("can't set pixel %u: %d", i, rc);
			return rc;
		}
	}

	return ws2812_reset_strip(drv_data);
}

static int ws2812_strip_update_channels(struct device *dev, u8_t *channels,
					size_t num_channels)
{
	struct ws2812_data *drv_data = dev->driver_data;
	struct spi_config *config = &drv_data->config;
	u8_t px_buf[8]; /* one byte per bit */
	const struct spi_buf buf = {
		.buf = px_buf,
		.len = sizeof(px_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1
	};
	size_t i;
	int rc;

	for (i = 0; i < num_channels; i++) {
		ws2812_serialize_color(px_buf, channels[i]);
		rc = spi_write(drv_data->spi, config, &tx);
		if (rc) {
			/*
			 * Latch anything we've shifted out first, to
			 * call visual attention to the problematic
			 * pixel.
			 */
			(void)ws2812_reset_strip(drv_data);
			SYS_LOG_ERR("can't set channel %u: %d", i, rc);
			return rc;
		}
	}

	return ws2812_reset_strip(drv_data);
}

static int ws2812_strip_init(struct device *dev)
{
	struct ws2812_data *data = dev->driver_data;
	struct spi_config *config = &data->config;

	data->spi = device_get_binding(CONFIG_WS2812_STRIP_SPI_DEV_NAME);
	if (!data->spi) {
		SYS_LOG_ERR("SPI device %s not found",
			    CONFIG_WS2812_STRIP_SPI_DEV_NAME);
		return -ENODEV;
	}

	config->frequency = SPI_FREQ;
	config->operation = SPI_OPER;
	config->slave = 0;	/* MOSI only. */
	config->cs = NULL;

	return 0;
}

static struct ws2812_data ws2812_strip_data;

static const struct led_strip_driver_api ws2812_strip_api = {
	.update_rgb = ws2812_strip_update_rgb,
	.update_channels = ws2812_strip_update_channels,
};

DEVICE_AND_API_INIT(ws2812_strip, CONFIG_WS2812_STRIP_NAME,
		    ws2812_strip_init, &ws2812_strip_data,
		    NULL, POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,
		    &ws2812_strip_api);
