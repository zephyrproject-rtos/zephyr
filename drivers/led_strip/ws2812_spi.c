/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019, Nordic Semiconductor ASA
 * Copyright (c) 2021 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_spi

#include <zephyr/drivers/led_strip.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_spi);
#define CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT 1
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/led/led.h>

/* spi-one-frame and spi-zero-frame in DT are for 8-bit frames. */
#define SPI_FRAME_BITS 8

/*
 * SPI master configuration:
 *
 * - mode 0 (the default), 8 bit, MSB first (arbitrary), one-line SPI
 * - no shenanigans (don't hold CS, don't hold the device lock, this
 *   isn't an EEPROM)
 */
#define SPI_OPER(idx) (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
		  SPI_WORD_SET(SPI_FRAME_BITS))

struct ws2812_spi_cfg {
	struct spi_dt_spec bus;
	uint8_t *px_buf;
	uint8_t one_frame;
	uint8_t zero_frame;
	uint8_t num_colors;
	const uint8_t *color_mapping;
	size_t length;
	uint16_t reset_delay;
#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_PREAMBLE_SUPPORT) || IS_ENABLED(CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT)
	uint16_t empty_padding_frames;
#endif
};

static const struct ws2812_spi_cfg *dev_cfg(const struct device *dev)
{
	return dev->config;
}

/*
 * Serialize an 8-bit color channel value into an equivalent sequence
 * of SPI frames, MSbit first, where a one bit becomes SPI frame
 * one_frame, and zero bit becomes zero_frame.
 */
static inline void ws2812_spi_ser(uint8_t buf[8], uint8_t color,
				  const uint8_t one_frame, const uint8_t zero_frame)
{
	int i;

	for (i = 0; i < 8; i++) {
		buf[i] = color & BIT(7 - i) ? one_frame : zero_frame;
	}
}

/*
 * Latch current color values on strip and reset its state machines.
 */
static inline void ws2812_reset_delay(uint16_t delay)
{
	k_usleep(delay);
}

static int ws2812_strip_update_rgb(const struct device *dev,
				   struct led_rgb *pixels,
				   size_t num_pixels)
{
	const struct ws2812_spi_cfg *cfg = dev_cfg(dev);
	const uint8_t one = cfg->one_frame, zero = cfg->zero_frame;
	struct spi_buf buf = {
		.buf = cfg->px_buf,
		.len = (cfg->length * 8 * cfg->num_colors),
	};
	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1
	};
	size_t i;
	int rc;
#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT)
    buf.len += cfg->empty_padding_frames;
#endif
#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_PREAMBLE_SUPPORT)
    buf.len += cfg->empty_padding_frames;
	uint8_t *px_buf = &cfg->px_buf[cfg->empty_padding_frames];
	/* Ensure zero fill on preamble (if present) */
#else
	uint8_t *px_buf = cfg->px_buf;
#endif
#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_PREAMBLE_SUPPORT) || IS_ENABLED(CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT)
    /* If zero values are padded on either end we need to ensure the buffer is all zeros before filling */
	memset(cfg->px_buf, 0, buf.len);
#endif
	/*
	 * Convert pixel data into SPI frames. Each frame has pixel data
	 * in color mapping on-wire format (e.g. GRB, GRBW, RGB, etc).
	 */
	for (i = 0; i < num_pixels; i++) {
		uint8_t j;

		for (j = 0; j < cfg->num_colors; j++) {
			uint8_t pixel;

			switch (cfg->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				pixel = 0;
				break;
			case LED_COLOR_ID_RED:
				pixel = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				pixel = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				pixel = pixels[i].b;
				break;
			default:
				return -EINVAL;
			}
			ws2812_spi_ser(px_buf, pixel, one, zero);
			px_buf += 8;
		}
	}

	/*
	 * Display the pixel data.
	 */
	rc = spi_write_dt(&cfg->bus, &tx);
	ws2812_reset_delay(cfg->reset_delay);

	return rc;
}

static size_t ws2812_strip_length(const struct device *dev)
{
	const struct ws2812_spi_cfg *cfg = dev_cfg(dev);

	return cfg->length;
}

static int ws2812_spi_init(const struct device *dev)
{
	const struct ws2812_spi_cfg *cfg = dev_cfg(dev);
	uint8_t i;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI device %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	for (i = 0; i < cfg->num_colors; i++) {
		switch (cfg->color_mapping[i]) {
		case LED_COLOR_ID_WHITE:
		case LED_COLOR_ID_RED:
		case LED_COLOR_ID_GREEN:
		case LED_COLOR_ID_BLUE:
			break;
		default:
			LOG_ERR("%s: invalid channel to color mapping."
				"Check the color-mapping DT property",
				dev->name);
			return -EINVAL;
		}
	}

	return 0;
}

static const struct led_strip_driver_api ws2812_spi_api = {
	.update_rgb = ws2812_strip_update_rgb,
	.length = ws2812_strip_length,
};

#define WS2812_SPI_NUM_PIXELS(idx) \
	(DT_INST_PROP(idx, chain_length))
#define WS2812_SPI_HAS_WHITE(idx) \
	(DT_INST_PROP(idx, has_white_channel) == 1)
#define WS2812_SPI_ONE_FRAME(idx) \
	(DT_INST_PROP(idx, spi_one_frame))
#define WS2812_SPI_ZERO_FRAME(idx) \
	(DT_INST_PROP(idx, spi_zero_frame))
/* Get the latch/reset delay from the "reset-delay" DT property. */
#define WS2812_RESET_DELAY(idx) \
	(DT_INST_PROP(idx, reset_delay))
#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_PREAMBLE_SUPPORT) || IS_ENABLED(CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT)
#define WS2812_PADDING_FRAMES(idx) \
	((DT_INST_PROP(idx, spi_max_frequency) * WS2812_RESET_DELAY(idx))/8000000)
#else
#define WS2812_PADDING_FRAMES(idx) \
	(0)
#endif
#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT)
#define WS2812_POSTAMBLE_FRAMES(idx) \
	((DT_INST_PROP(idx, spi_max_frequency) * WS2812_RESET_DELAY(idx))/8000000)
#else
#define WS2812_POSTAMBLE_FRAMES(idx) \
	(0)
#endif
/* size of buffer is number of pixel bits + pre/postamble padding if enabled */
#define WS2812_SPI_BUFSZ(idx) \
	((WS2812_PADDING_FRAMES(idx) \
    * (IS_ENABLED(CONFIG_WS2812_STRIP_SPI_PREAMBLE_SUPPORT) + IS_ENABLED(CONFIG_WS2812_STRIP_SPI_POSTAMBLE_SUPPORT))) \
    + (WS2812_NUM_COLORS(idx) * 8 * WS2812_SPI_NUM_PIXELS(idx)))

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
#define WS2812_COLOR_MAPPING(idx)				  \
	static const uint8_t ws2812_spi_##idx##_color_mapping[] = \
		DT_INST_PROP(idx, color_mapping)

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

#if IS_ENABLED(CONFIG_WS2812_STRIP_SPI_PREAMBLE_SUPPORT)
#define WS2812_SPI_DEVICE(idx)						 \
									 \
	static uint8_t ws2812_spi_##idx##_px_buf[WS2812_SPI_BUFSZ(idx)]; \
									 \
	WS2812_COLOR_MAPPING(idx);					 \
									 \
	static const struct ws2812_spi_cfg ws2812_spi_##idx##_cfg = {	 \
		.bus = SPI_DT_SPEC_INST_GET(idx, SPI_OPER(idx), 0),	 \
		.px_buf = ws2812_spi_##idx##_px_buf,			 \
		.one_frame = WS2812_SPI_ONE_FRAME(idx),			 \
		.zero_frame = WS2812_SPI_ZERO_FRAME(idx),		 \
		.num_colors = WS2812_NUM_COLORS(idx),			 \
		.color_mapping = ws2812_spi_##idx##_color_mapping,	 \
		.length = DT_INST_PROP(idx, chain_length),               \
		.reset_delay = WS2812_RESET_DELAY(idx),			 \
		.empty_padding_frames = WS2812_PADDING_FRAMES(idx),	 \
	};								 \
									 \
	DEVICE_DT_INST_DEFINE(idx,					 \
			      ws2812_spi_init,				 \
			      NULL,					 \
			      NULL,					 \
			      &ws2812_spi_##idx##_cfg,			 \
			      POST_KERNEL,				 \
			      CONFIG_LED_STRIP_INIT_PRIORITY,		 \
			      &ws2812_spi_api);
#else
#define WS2812_SPI_DEVICE(idx)						 \
									 \
	static uint8_t ws2812_spi_##idx##_px_buf[WS2812_SPI_BUFSZ(idx)]; \
									 \
	WS2812_COLOR_MAPPING(idx);					 \
									 \
	static const struct ws2812_spi_cfg ws2812_spi_##idx##_cfg = {	 \
		.bus = SPI_DT_SPEC_INST_GET(idx, SPI_OPER(idx), 0),	 \
		.px_buf = ws2812_spi_##idx##_px_buf,			 \
		.one_frame = WS2812_SPI_ONE_FRAME(idx),			 \
		.zero_frame = WS2812_SPI_ZERO_FRAME(idx),		 \
		.num_colors = WS2812_NUM_COLORS(idx),			 \
		.color_mapping = ws2812_spi_##idx##_color_mapping,	 \
		.length = DT_INST_PROP(idx, chain_length),               \
		.reset_delay = WS2812_RESET_DELAY(idx),			 \
	};								 \
									 \
	DEVICE_DT_INST_DEFINE(idx,					 \
			      ws2812_spi_init,				 \
			      NULL,					 \
			      NULL,					 \
			      &ws2812_spi_##idx##_cfg,			 \
			      POST_KERNEL,				 \
			      CONFIG_LED_STRIP_INIT_PRIORITY,		 \
			      &ws2812_spi_api);
#endif
DT_INST_FOREACH_STATUS_OKAY(WS2812_SPI_DEVICE)
