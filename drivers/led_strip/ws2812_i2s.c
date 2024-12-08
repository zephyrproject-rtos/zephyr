/*
 * Copyright (c) 2022 Jonathan Rico
 *
 * Adapted from the SPI driver, using the procedure in this blog post:
 * https://electronut.in/nrf52-i2s-ws2812/
 *
 * Note: the word "word" refers to a 32-bit integer unless otherwise stated.
 *
 * WS/LRCK frequency:
 * This refers to the "I2S word or channel select" clock.
 * The I2S peripheral sends two 16-bit channel values for each clock period.
 * A single LED color (8 data bits) will take up one 32-bit word or one LRCK
 * period. This means a standard RGB led will take 3 LRCK periods to transmit.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_i2s

#include <zephyr/drivers/led_strip.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_i2s);

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define WS2812_I2S_PRE_DELAY_WORDS 1

struct ws2812_i2s_cfg {
	struct device const *dev;
	size_t tx_buf_bytes;
	struct k_mem_slab *mem_slab;
	uint8_t num_colors;
	size_t length;
	const uint8_t *color_mapping;
	uint16_t reset_words;
	uint32_t lrck_period;
	uint32_t extra_wait_time_us;
	bool active_low;
	uint8_t nibble_one;
	uint8_t nibble_zero;
};

/* Serialize an 8-bit color channel value into two 16-bit I2S values (or 1 32-bit
 * word).
 */
static inline uint32_t ws2812_i2s_ser(uint8_t color, const uint8_t sym_one, const uint8_t sym_zero)
{
	uint32_t word = 0;

	for (uint_fast8_t mask = 0x80; mask != 0; mask >>= 1) {
		word <<= 4;
		word |= (color & mask) ? sym_one : sym_zero;
	}

	/* Swap the two I2S values due to the (audio) channel TX order. */
	return (word >> 16) | (word << 16);
}

static int ws2812_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				   size_t num_pixels)
{
	const struct ws2812_i2s_cfg *cfg = dev->config;
	const uint8_t sym_one = cfg->nibble_one;
	const uint8_t sym_zero = cfg->nibble_zero;
	const uint32_t reset_word = cfg->active_low ? ~0 : 0;
	uint32_t *tx_buf;
	uint32_t flush_time_us;
	void *mem_block;
	int ret;

	/* Acquire memory for the I2S payload. */
	ret = k_mem_slab_alloc(cfg->mem_slab, &mem_block, K_SECONDS(10));
	if (ret < 0) {
		LOG_ERR("Unable to allocate mem slab for TX (err %d)", ret);
		return -ENOMEM;
	}
	tx_buf = (uint32_t *)mem_block;

	/* Add a pre-data reset, so the first pixel isn't skipped by the strip. */
	for (uint16_t i = 0; i < WS2812_I2S_PRE_DELAY_WORDS; i++) {
		*tx_buf = reset_word;
		tx_buf++;
	}

	/*
	 * Convert pixel data into I2S frames. Each frame has pixel data
	 * in color mapping on-wire format (e.g. GRB, GRBW, RGB, etc).
	 */
	for (uint16_t i = 0; i < num_pixels; i++) {
		for (uint16_t j = 0; j < cfg->num_colors; j++) {
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
			*tx_buf = ws2812_i2s_ser(pixel, sym_one, sym_zero) ^ reset_word;
			tx_buf++;
		}
	}

	for (uint16_t i = 0; i < cfg->reset_words; i++) {
		*tx_buf = reset_word;
		tx_buf++;
	}

	/* Flush the buffer on the wire. */
	ret = i2s_write(cfg->dev, mem_block, cfg->tx_buf_bytes);
	if (ret < 0) {
		k_mem_slab_free(cfg->mem_slab, mem_block);
		LOG_ERR("Failed to write data: %d", ret);
		return ret;
	}

	ret = i2s_trigger(cfg->dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("Failed to trigger command %d on TX: %d", I2S_TRIGGER_START, ret);
		return ret;
	}

	ret = i2s_trigger(cfg->dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret < 0) {
		LOG_ERR("Failed to trigger command %d on TX: %d", I2S_TRIGGER_DRAIN, ret);
		return ret;
	}

	/* Wait until transaction is over */
	flush_time_us = cfg->lrck_period * cfg->tx_buf_bytes / sizeof(uint32_t);
	k_usleep(flush_time_us + cfg->extra_wait_time_us);

	return ret;
}

static size_t ws2812_strip_length(const struct device *dev)
{
	const struct ws2812_i2s_cfg *cfg = dev->config;

	return cfg->length;
}

static int ws2812_i2s_init(const struct device *dev)
{
	const struct ws2812_i2s_cfg *cfg = dev->config;
	struct i2s_config config;
	uint32_t lrck_hz;
	int ret;

	lrck_hz = USEC_PER_SEC / cfg->lrck_period;
	LOG_DBG("Word clock: freq %u Hz period %u us",
		lrck_hz, cfg->lrck_period);

	/* 16-bit stereo, 100kHz LCLK */
	config.word_size = 16;
	config.channels = 2;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = lrck_hz; /* WS (or LRCK) */
	config.mem_slab = cfg->mem_slab;
	config.block_size = cfg->tx_buf_bytes;
	config.timeout = 1000;

	ret = i2s_configure(cfg->dev, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S device: %d\n", ret);
		return ret;
	}

	for (uint16_t i = 0; i < cfg->num_colors; i++) {
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

static DEVICE_API(led_strip, ws2812_i2s_api) = {
	.update_rgb = ws2812_strip_update_rgb,
	.length = ws2812_strip_length,
};

#define WS2812_I2S_LRCK_PERIOD_US(idx) DT_INST_PROP(idx, lrck_period)

#define WS2812_RESET_DELAY_US(idx)    DT_INST_PROP(idx, reset_delay)
/* Rounds up to the next 20us. */
#define WS2812_RESET_DELAY_WORDS(idx)                                                              \
	DIV_ROUND_UP(WS2812_RESET_DELAY_US(idx), WS2812_I2S_LRCK_PERIOD_US(idx))

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

#define WS2812_I2S_NUM_PIXELS(idx) (DT_INST_PROP(idx, chain_length))

#define WS2812_I2S_BUFSIZE(idx)                                                                    \
	(((WS2812_NUM_COLORS(idx) * WS2812_I2S_NUM_PIXELS(idx)) +	                           \
	  WS2812_I2S_PRE_DELAY_WORDS + WS2812_RESET_DELAY_WORDS(idx)) * 4)

#define WS2812_I2S_DEVICE(idx)                                                                     \
                                                                                                   \
	K_MEM_SLAB_DEFINE_STATIC(ws2812_i2s_##idx##_slab, WS2812_I2S_BUFSIZE(idx), 2, 4);          \
                                                                                                   \
	static const uint8_t ws2812_i2s_##idx##_color_mapping[] =                                  \
		DT_INST_PROP(idx, color_mapping);                                                  \
                                                                                                   \
	static const struct ws2812_i2s_cfg ws2812_i2s_##idx##_cfg = {                              \
		.dev = DEVICE_DT_GET(DT_INST_PROP(idx, i2s_dev)),                                  \
		.tx_buf_bytes = WS2812_I2S_BUFSIZE(idx),                                           \
		.mem_slab = &ws2812_i2s_##idx##_slab,                                              \
		.num_colors = WS2812_NUM_COLORS(idx),                                              \
		.length = DT_INST_PROP(idx, chain_length),                                         \
		.color_mapping = ws2812_i2s_##idx##_color_mapping,                                 \
		.lrck_period = WS2812_I2S_LRCK_PERIOD_US(idx),                                     \
		.extra_wait_time_us = DT_INST_PROP(idx, extra_wait_time),                          \
		.reset_words = WS2812_RESET_DELAY_WORDS(idx),                                      \
		.active_low = DT_INST_PROP(idx, out_active_low),                                   \
		.nibble_one = DT_INST_PROP(idx, nibble_one),                                       \
		.nibble_zero = DT_INST_PROP(idx, nibble_zero),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, ws2812_i2s_init, NULL, NULL, &ws2812_i2s_##idx##_cfg,           \
			      POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_i2s_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_I2S_DEVICE)
