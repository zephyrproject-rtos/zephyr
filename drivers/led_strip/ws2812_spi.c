/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019, Nordic Semiconductor ASA
 * Copyright (c) 2021 Seagate Technology LLC
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_spi

#include <zephyr/drivers/led_strip.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_spi);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/led/led.h>

/*
 * The N-bit symbols for a '1' and '0' bit (spi-one-frame, spi-zero-frame)
 * are packed into the 8-bit SPI frames sent on the bus. The symbol width
 * is defined by the 'spi-bits-per-symbol' DT property.
 */
#define SPI_FRAME_BITS 8

/* Each color channel is represented by 8 bits. */
#define BITS_PER_COLOR_CHANNEL 8

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
	uint8_t bits_per_symbol;
	uint8_t num_colors;
	const uint8_t *color_mapping;
	size_t length;
	uint16_t reset_delay;
};

static const struct ws2812_spi_cfg *dev_cfg(const struct device *dev)
{
	return dev->config;
}

/*
 * Serialize an 8-bit color value into the SPI buffer, MSbit first. Each of
 * the 8 data bits is represented by a N-bit symbol (`one_frame` for a '1' or
 * `zero_frame` for a '0'), which is then packed into the SPI buffer.
 */
static inline void ws2812_spi_ser(uint8_t color, uint8_t one, uint8_t zero,
				  uint8_t bits_per_symbol, uint8_t **buf,
				  uint8_t *bit_mask)
{
	for (int i = BITS_PER_COLOR_CHANNEL - 1; i >= 0; i--) {
		uint8_t pattern = (color & BIT(i)) ? one : zero;

		if (bits_per_symbol == SPI_FRAME_BITS) {
			/* Fast path for the common 8-bit symbol case */
			*(*buf)++ = pattern;
		} else {
			/* Generic path for N-bit symbols */
			for (int p = bits_per_symbol - 1; p >= 0; p--) {
				if (pattern & BIT(p)) {
					**buf |= *bit_mask;
				} else {
					**buf &= ~(*bit_mask);
				}

				*bit_mask >>= 1;
				if (!*bit_mask) {
					*bit_mask = BIT(SPI_FRAME_BITS - 1);
					(*buf)++;
				}
			}
		}
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
	const uint8_t bits_per_symbol = cfg->bits_per_symbol;
	const size_t total_bits = num_pixels * cfg->num_colors *
				  BITS_PER_COLOR_CHANNEL * bits_per_symbol;
	const size_t buf_len = DIV_ROUND_UP(total_bits, SPI_FRAME_BITS);
	struct spi_buf buf = {
		.buf = cfg->px_buf,
		.len = buf_len,
	};
	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1
	};
	uint8_t *px_buf = cfg->px_buf;
	uint8_t bit_mask = BIT(SPI_FRAME_BITS - 1);
	size_t i;
	int rc;

	/*
	 * Convert pixel data into an SPI bitstream. The bitstream contains
	 * pixel data in color mapping on-wire format (e.g. GRB, GRBW, RGB,
	 * etc).
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

			ws2812_spi_ser(pixel, one, zero, bits_per_symbol,
				       &px_buf, &bit_mask);
		}
	}

	/* Clear unused padding bits in the final SPI byte. */
	if (bit_mask != BIT(SPI_FRAME_BITS - 1)) {
		*px_buf &= ~((bit_mask << 1) - 1);
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

static DEVICE_API(led_strip, ws2812_spi_api) = {
	.update_rgb = ws2812_strip_update_rgb,
	.length = ws2812_strip_length,
};

#define WS2812_SPI_NUM_PIXELS(idx) \
	(DT_INST_PROP(idx, chain_length))
#define WS2812_SPI_ONE_FRAME(idx) \
	(DT_INST_PROP(idx, spi_one_frame))
#define WS2812_SPI_ZERO_FRAME(idx) \
	(DT_INST_PROP(idx, spi_zero_frame))
#define WS2812_SPI_BITS_PER_SYMBOL(idx) \
	(DT_INST_PROP(idx, bits_per_symbol))
#define WS2812_NUM_COLORS(idx) \
	(DT_INST_PROP_LEN(idx, color_mapping))
#define WS2812_SPI_BUFSZ(idx) \
	DIV_ROUND_UP(WS2812_NUM_COLORS(idx) * BITS_PER_COLOR_CHANNEL * \
		     WS2812_SPI_NUM_PIXELS(idx) * \
		     WS2812_SPI_BITS_PER_SYMBOL(idx), SPI_FRAME_BITS)

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
#define WS2812_COLOR_MAPPING(idx)				  \
	static const uint8_t ws2812_spi_##idx##_color_mapping[] = \
		DT_INST_PROP(idx, color_mapping)

/* Get the latch/reset delay from the "reset-delay" DT property. */
#define WS2812_RESET_DELAY(idx) DT_INST_PROP(idx, reset_delay)

#define WS2812_SPI_DEVICE(idx)							\
	BUILD_ASSERT(								\
		(WS2812_SPI_BITS_PER_SYMBOL(idx) >= 3) &&			\
		(WS2812_SPI_BITS_PER_SYMBOL(idx) <= 8),				\
		"bits-per-symbol property must be between 3 and 8");		\
										\
	static uint8_t ws2812_spi_##idx##_px_buf[WS2812_SPI_BUFSZ(idx)]		\
		IF_ENABLED(CONFIG_WS2812_STRIP_SPI_FORCE_NOCACHE, (__nocache)); \
										\
	WS2812_COLOR_MAPPING(idx);						\
										\
	static const struct ws2812_spi_cfg ws2812_spi_##idx##_cfg = {		\
		.bus = SPI_DT_SPEC_INST_GET(idx, SPI_OPER(idx), 0),		\
		.px_buf = ws2812_spi_##idx##_px_buf,				\
		.one_frame = WS2812_SPI_ONE_FRAME(idx),				\
		.zero_frame = WS2812_SPI_ZERO_FRAME(idx),			\
		.bits_per_symbol = WS2812_SPI_BITS_PER_SYMBOL(idx),             \
		.num_colors = WS2812_NUM_COLORS(idx),				\
		.color_mapping = ws2812_spi_##idx##_color_mapping,		\
		.length = WS2812_SPI_NUM_PIXELS(idx),                           \
		.reset_delay = WS2812_RESET_DELAY(idx),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(idx,						\
			      ws2812_spi_init,					\
			      NULL,						\
			      NULL,						\
			      &ws2812_spi_##idx##_cfg,				\
			      POST_KERNEL,					\
			      CONFIG_LED_STRIP_INIT_PRIORITY,			\
			      &ws2812_spi_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_SPI_DEVICE)
