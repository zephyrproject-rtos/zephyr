/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_espressif_rmt

#include <zephyr/drivers/led_strip.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_espressif_rmt);

#include <zephyr/device.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_tx.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Each color channel is represented by 8 bits. */
#define BITS_PER_COLOR_CHANNEL 8

/* Calculate the buffer size needed. */
#define WS2812_ESPRESSIF_RMT_CALC_BUFSZ(num_px, num_colors) \
	DIV_ROUND_UP((num_px) * (num_colors) * BITS_PER_COLOR_CHANNEL, 8)

/* increase the block size can make the LED less flickering */
#define WS2812_ESPRESSIF_RMT_MEM_BLOCK_SYMBOL (64)

/* 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution) */
#define WS2812_ESPRESSIF_RMT_RESOLUTION_HZ (10000000)

/* set the number of transactions that can be pending in the background */
#define WS2812_ESPRESSIF_RMT_TRANS_QUEUE_DEPTH (4)

typedef struct {
	uint32_t resolution;
} led_strip_encoder_config_t;

struct ws2812_espressif_rmt_config {
	struct device const *dev;
	size_t regs_count;
	size_t regs[1];
	uint8_t *px_buf;
	uint8_t num_colors;
	size_t length;
	const uint8_t *color_mapping;
	uint16_t reset_delay;
};

struct ws2812_espressif_rmt_data {
	rmt_tx_channel_config_t tx_chan_config;
	rmt_channel_handle_t led_chan;
	led_strip_encoder_config_t encoder_config;
	rmt_encoder_handle_t led_encoder;
};

typedef struct {
	rmt_encoder_t base;
	rmt_encoder_t *bytes_encoder;
	rmt_encoder_t *copy_encoder;
	int state;
	rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

static size_t ws2812_espressif_rmt_encoder_encode(rmt_encoder_t *encoder,
	rmt_channel_handle_t channel, const void *primary_data, size_t data_size,
	rmt_encode_state_t *ret_state)
{
	rmt_led_strip_encoder_t *led_encoder =
		__containerof(encoder, rmt_led_strip_encoder_t, base);
	rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
	rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
	rmt_encode_state_t session_state = RMT_ENCODING_RESET;
	rmt_encode_state_t state = RMT_ENCODING_RESET;
	size_t encoded_symbols = 0;

	switch (led_encoder->state) {
	case 0: /* send RGB data */
		encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data,
			data_size, &session_state);
		if (session_state & RMT_ENCODING_COMPLETE) {
			/* switch to next state when current encoding session finished */
			led_encoder->state = 1;
		}
		if (session_state & RMT_ENCODING_MEM_FULL) {
			state |= RMT_ENCODING_MEM_FULL;
			goto out; /* yield if there's no free space for encoding artifacts */
		}
	/* fall-through */
	case 1: /* send reset code */
		encoded_symbols += copy_encoder->encode(copy_encoder, channel,
			&led_encoder->reset_code, sizeof(led_encoder->reset_code), &session_state);
		if (session_state & RMT_ENCODING_COMPLETE) {
			/* back to the initial encoding session */
			led_encoder->state = RMT_ENCODING_RESET;
			state |= RMT_ENCODING_COMPLETE;
		}
		if (session_state & RMT_ENCODING_MEM_FULL) {
			state |= RMT_ENCODING_MEM_FULL;
			goto out; /* yield if there's no free space for encoding artifacts */
		}
		break;
	default:
		break;
	}
out:
	*ret_state = state;
	return encoded_symbols;
}

static int ws2812_espressif_rmt_encoder_del(rmt_encoder_t *encoder)
{
	rmt_led_strip_encoder_t *led_encoder =
		__containerof(encoder, rmt_led_strip_encoder_t, base);

	rmt_del_encoder(led_encoder->bytes_encoder);
	rmt_del_encoder(led_encoder->copy_encoder);
	k_free(led_encoder);

	return 0;
}

static int ws2812_espressif_rmt_encoder_reset(rmt_encoder_t *encoder)
{
	rmt_led_strip_encoder_t *led_encoder =
		__containerof(encoder, rmt_led_strip_encoder_t, base);

	rmt_encoder_reset(led_encoder->bytes_encoder);
	rmt_encoder_reset(led_encoder->copy_encoder);
	led_encoder->state = RMT_ENCODING_RESET;

	return 0;
}

static int ws2812_espressif_rmt_encoder_new(const led_strip_encoder_config_t *config,
	rmt_encoder_handle_t *ret_encoder)
{
	rmt_led_strip_encoder_t *led_encoder = NULL;
	rmt_copy_encoder_config_t copy_encoder_config;
	uint32_t reset_ticks;
	int rc;

	if (!(config && ret_encoder)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	led_encoder = rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
	if (!led_encoder) {
		LOG_ERR("Unable to allocate memory for LED strip encoder");
		return -ENOMEM;
	}
	led_encoder->base.encode = ws2812_espressif_rmt_encoder_encode;
	led_encoder->base.del = ws2812_espressif_rmt_encoder_del;
	led_encoder->base.reset = ws2812_espressif_rmt_encoder_reset;
	/* timing requirements for WS2812 led strip */
	rmt_bytes_encoder_config_t bytes_encoder_config = {
		.bit0 = {
			.level0 = 1,
			.duration0 = 0.3 * config->resolution / 1000000, /* T0H=0.3us */
			.level1 = 0,
			.duration1 = 0.9 * config->resolution / 1000000, /* T0L=0.9us */
		},
		.bit1 = {
			.level0 = 1,
			.duration0 = 0.9 * config->resolution / 1000000, /* T1H=0.9us */
			.level1 = 0,
			.duration1 = 0.3 * config->resolution / 1000000, /* T1L=0.3us */
		},
		.flags.msb_first = 1 /* WS2812 transfer bit order: G7...G0R7...R0B7...B0 */
	};
	rc = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
	if (rc) {
		LOG_ERR("Create bytes encoder failed");
		goto err;
	}
	rc = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
	if (rc) {
		LOG_ERR("Create copy encoder failed");
		goto err;
	}

	/* reset code duration defaults to 50us */
	reset_ticks = config->resolution / 1000000 * 50 / 2;
	led_encoder->reset_code = (rmt_symbol_word_t) {
		.level0 = 0,
		.duration0 = reset_ticks,
		.level1 = 0,
		.duration1 = reset_ticks,
	};

	*ret_encoder = &led_encoder->base;
	return 0;

err:
	if (led_encoder) {
		if (led_encoder->bytes_encoder) {
			rmt_del_encoder(led_encoder->bytes_encoder);
		}
		if (led_encoder->copy_encoder) {
			rmt_del_encoder(led_encoder->copy_encoder);
		}
		k_free(led_encoder);
	}

	return rc;
}

static int ws2812_espressif_rmt_udate_rbg(const struct device *dev,
	struct led_rgb *pixels, size_t num_pixels)
{
	const struct ws2812_espressif_rmt_config *cfg = dev->config;
	struct ws2812_espressif_rmt_data *data = dev->data;
	const size_t buf_len = WS2812_ESPRESSIF_RMT_CALC_BUFSZ(num_pixels, cfg->num_colors);
	uint8_t *px_buf = cfg->px_buf;
	uint8_t pixel;
	int rc;

	/*
	 * Convert pixel data into RMT frames. Each frame has pixel data
	 * in color mapping on-wire format (e.g. GRB, GRBW, RGB, etc).
	 */
	for (uint16_t i = 0; i < num_pixels; i++) {
		for (uint16_t j = 0; j < cfg->num_colors; j++) {
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
			*px_buf = pixel;
			px_buf++;
		}
	}

	/* Flush RGB values to LEDs */
	rmt_transmit_config_t tx_config = {
		.loop_count = 0, /* no transfer loop */
	};
	rc = rmt_transmit(data->led_chan, data->led_encoder, cfg->px_buf, buf_len, &tx_config);
	if (rc) {
		LOG_ERR("Unable to transmit data over TX channel");
		return rc;
	}
	rc = rmt_tx_wait_all_done(data->led_chan, K_FOREVER);
	if (rc) {
		LOG_ERR("Waiting until all done failed");
		return rc;
	}

	return 0;
}

static size_t ws2812_espressif_rmt_length(const struct device *dev)
{
	const struct ws2812_espressif_rmt_config *cfg = dev->config;

	return cfg->length;
}

static int ws2812_espressif_rmt_init(const struct device *dev)
{
	const struct ws2812_espressif_rmt_config *cfg = dev->config;
	const struct espressif_rmt_config *rmt_cfg = cfg->dev->config;
	struct ws2812_espressif_rmt_data *data = dev->data;
	int rc;

	/* Ensure RMT device is ready */
	if (!device_is_ready(cfg->dev)) {
		LOG_ERR("%s is not ready", cfg->dev->name);
		return -ENODEV;
	}

	/* Retrieve pinmux from RMT device */
	if (cfg->regs_count != 1) {
		LOG_ERR("Invalid reg size");
		return -EINVAL;
	}
	if (cfg->regs[0] >= rmt_cfg->pcfg->states[0].pin_cnt) {
		LOG_ERR("Invalid reg value");
		return -EINVAL;
	}
	data->tx_chan_config.gpio_pinmux = rmt_cfg->pcfg->states[0].pins[cfg->regs[0]].pinmux;

	/* Create channel */
	rc = rmt_new_tx_channel(cfg->dev, &data->tx_chan_config, &data->led_chan);
	if (rc) {
		LOG_ERR("RMT TX channel creation failed");
		return rc;
	}

	/* Create encoder */
	rc = ws2812_espressif_rmt_encoder_new(&data->encoder_config, &data->led_encoder);
	if (rc) {
		LOG_ERR("Unable to create encoder");
		return rc;
	}

	/* Enable channel */
	LOG_INF("Enable RMT TX channel");
	rc = rmt_enable(data->led_chan);
	if (rc) {
		LOG_ERR("Unable to enable RMT TX channel");
		return rc;
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

static DEVICE_API(led_strip, ws2812_espressif_rmt_api) = {
	.update_rgb = ws2812_espressif_rmt_udate_rbg,
	.length = ws2812_espressif_rmt_length,
};

#define WS2812_NUM_PIXELS(idx) (DT_INST_PROP(idx, chain_length))
#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))
#define WS2812_ESPRESSIF_RMT_BUFSZ(idx)                                             \
	WS2812_ESPRESSIF_RMT_CALC_BUFSZ(WS2812_NUM_PIXELS(idx), WS2812_NUM_COLORS(idx))

#define WS2812_ESPRESSIF_RMT_DEVICE(idx)                                            \
	static const uint8_t ws2812_espressif_rmt_##idx##_color_mapping[] =             \
		DT_INST_PROP(idx, color_mapping);                                           \
	static uint8_t ws2812_espressif_rmt_##idx##_px_buf[WS2812_ESPRESSIF_RMT_BUFSZ(idx)]; \
	static const struct ws2812_espressif_rmt_config ws2812_espressif_rmt_##idx##_cfg = { \
		.dev = DEVICE_DT_GET(DT_INST_BUS(idx)),                                     \
		.regs_count = DT_NUM_REGS(DT_DRV_INST(idx)),                                \
		.regs = {                                                                   \
			DT_REG_ADDR_BY_IDX(DT_DRV_INST(idx), 0)                                 \
		},                                                                          \
		.px_buf = ws2812_espressif_rmt_##idx##_px_buf,                              \
		.num_colors = WS2812_NUM_COLORS(idx),                                       \
		.length = DT_INST_PROP(idx, chain_length),                                  \
		.color_mapping = ws2812_espressif_rmt_##idx##_color_mapping,                \
		.reset_delay = DT_INST_PROP(idx, reset_delay),                              \
	};                                                                              \
	static struct ws2812_espressif_rmt_data ws2812_espressif_rmt_##idx_data = {     \
		.tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT,                              \
		.tx_chan_config.mem_block_symbols = WS2812_ESPRESSIF_RMT_MEM_BLOCK_SYMBOL,  \
		.tx_chan_config.resolution_hz = WS2812_ESPRESSIF_RMT_RESOLUTION_HZ,         \
		.tx_chan_config.trans_queue_depth = WS2812_ESPRESSIF_RMT_TRANS_QUEUE_DEPTH, \
		.encoder_config.resolution = WS2812_ESPRESSIF_RMT_RESOLUTION_HZ,            \
	};                                                                              \
	DEVICE_DT_INST_DEFINE(idx, ws2812_espressif_rmt_init, NULL,                     \
		&ws2812_espressif_rmt_##idx_data, &ws2812_espressif_rmt_##idx##_cfg,        \
		POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_espressif_rmt_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_ESPRESSIF_RMT_DEVICE)
