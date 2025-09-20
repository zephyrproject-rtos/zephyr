/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief WS2812 LED strip driver using a UART peripheral
 *
 * This driver uses a UART's asynchronous API to generate the precise,
 * high-speed signal required by WS2812 and compatible LEDs.
 *
 * The driver encodes each WS2812 data bit ('1' or '0') into a multi-bit
 * "symbol" (e.g., 110 for '1', 100 for '0'). It then employs a frame-aware
 * packing strategy to transmit these symbols efficiently.
 *
 * Signal Inversion:
 * The WS2812 protocol requires an idle-low signal. This is achieved by
 * inverting the UART's TX output (requiring the "tx-invert" devicetree
 * property).
 *
 * A standard UART frame:
 *                       d0 d1 d2 d3 d4 d5 d6
 *                ___    __ __ __ __ __ __ __ __ ...
 *                   |__|__|__|__|__|__|__|__|
 *    Start Bit (low) ^                       ^ Stop Bit (high)
 *
 * An inverted UART frame:
 *                       d0 d1 d2 d3 d4 d5 d6
 *                    __ __ __ __ __ __ __ __
 *                ___|  |__|__|__|__|__|__|__|__ ...
 *   Start Bit (high) ^                       ^ Stop Bit (low)
 *
 * Frame-Aware Packing:
 * The driver reuses the UART's hardware-generated start and stop bits as part
 * of the on-wire symbol.
 *  - The symbol's MSB ('1') maps to the inverted UART start bit.
 *  - The inner bits are packed into the UART data payload.
 *  - The symbol's LSB ('0') maps to the inverted UART stop bit.
 *
 * Configuration Constraint:
 * This packing scheme imposes a constraint: the total number of bits in a
 * UART frame (1 start + N data + 1 stop) must be an integer multiple of the
 * symbol's length (`bits-per-symbol`). For example, if `data-bits` is set to 7,
 * the 9-bit total frame size (1 + 7 + 1) is compatible with a `bits-per-symbol`
 * of 3.
 *
 * Example: The WS2812 data stream `101` sent as symbols `110`, `100`, `110`
 * and packed into one 9-bit UART frame (1 start + 7 data + 1 stop):
 *                       d0 d1 d2 d3 d4 d5 d6
 *                    __ __    __       __ __
 *                ___|     |__|  |__ __|     |__ ...
 *   Start Bit (high) ^                       ^ Stop Bit (low)
 */

#define DT_DRV_COMPAT worldsemi_ws2812_uart

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_uart);

/* Each color channel is represented by 8 bits. */
#define BITS_PER_COLOR_CHANNEL 8

/*
 * Helper macros to get UART frame configuration from the parent UART's devicetree node.
 */
#define DT_UART_NODE(inst)          DT_INST_PARENT(inst)
#define DT_UART_DATA_BITS(inst)     DT_PROP_OR(DT_UART_NODE(inst), data_bits, 8)
/* Only UART_CFG_STOP_BITS_1 or 1 is supported. Other values will fail the BUILD_ASSERT below. */
#define DT_UART_STOP_BITS(inst)     DT_ENUM_IDX_OR(DT_UART_NODE(inst), stop_bits, 1)
#define DT_UART_HAS_PARITY(inst)    (DT_ENUM_IDX(DT_UART_NODE(inst), parity) != \
				     UART_CFG_PARITY_NONE)
#define DT_UART_HAS_TX_INVERT(inst) (DT_PROP_OR(DT_UART_NODE(inst), tx_invert, 0) != 0)

/* The total number of bits for one UART frame transmission (start + data + parity + stop). */
#define UART_FRAME_BITS_FROM_DT(inst) \
	(1 + DT_UART_DATA_BITS(inst) + DT_UART_HAS_PARITY(inst) + DT_UART_STOP_BITS(inst))

/* Calculate the buffer size needed. */
#define WS2812_UART_CALC_BUFSZ(num_px, num_colors, bits_symbol, bits_frame)                        \
	DIV_ROUND_UP((num_px) * (num_colors) * BITS_PER_COLOR_CHANNEL * (bits_symbol),             \
		     (bits_frame))

struct ws2812_uart_cfg {
	const struct device *uart_dev;
	uint8_t *px_buf;
	uint16_t one_symbol;
	uint16_t zero_symbol;
	uint8_t bits_per_symbol;
	uint8_t num_colors;
	const uint8_t *color_mapping;
	size_t length;
	uint16_t reset_delay;
	uint8_t uart_frame_bits;
};

struct ws2812_uart_data {
	struct k_mutex lock;
	struct k_sem tx_done_sem;
};

/*
 * Serializes an 8-bit color value into the UART buffer. This function takes
 * an 8-bit color value, expands each of its 8 bits into the appropriate symbol
 * pattern, and packs the resulting stream into UART data payloads.
 */
static inline void ws2812_uart_ser(uint8_t color, const struct ws2812_uart_cfg *cfg,
				   uint8_t *frame_bit_pos, uint8_t **buf)
{
	for (int i = BITS_PER_COLOR_CHANNEL - 1; i >= 0; i--) {
		uint16_t pattern = (color & BIT(i)) ? cfg->one_symbol : cfg->zero_symbol;

		for (int p = cfg->bits_per_symbol - 1; p >= 0; p--) {
			uint8_t pos = *frame_bit_pos;
			/* Start and stop bits are always handled by hardware and skipped. */
			bool is_hw_bit = (pos == 0) || (pos == (cfg->uart_frame_bits - 1));

			/*
			 * With an inverted signal, a high pulse ('1') is made by sending
			 * a low level ('0'). We clear the bit as the buffer is pre-filled.
			 */
			if (!is_hw_bit && (pattern & BIT(p))) {
				/* Map frame position to data position (no start bit). */
				**buf &= ~BIT(pos - 1);
			}

			(*frame_bit_pos)++;
			if (*frame_bit_pos >= cfg->uart_frame_bits) {
				(*buf)++;
				*frame_bit_pos = 0;
			}
		}
	}
}

/*
 * Callback for UART ASYNC API events.
 */
static void ws2812_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct k_sem *tx_done_sem = user_data;

	if (evt->type == UART_TX_DONE) {
		k_sem_give(tx_done_sem);
	}
}

/*
 * Latch current color values on strip and reset its state machines.
 */
static inline void ws2812_reset_delay(uint16_t delay)
{
	k_usleep(delay);
}

static int ws2812_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				   size_t num_pixels)
{
	const struct ws2812_uart_cfg *cfg = dev->config;
	struct ws2812_uart_data *data = dev->data;
	const size_t buf_len = WS2812_UART_CALC_BUFSZ(num_pixels, cfg->num_colors,
						      cfg->bits_per_symbol, cfg->uart_frame_bits);
	uint8_t *px_buf = cfg->px_buf;
	uint8_t *current_buf = px_buf;
	uint8_t frame_bit_pos = 0;
	int ret;

	/* Lock the driver to ensure thread-safe access to the buffer and UART */
	k_mutex_lock(&data->lock, K_FOREVER);

	/* memset to 0xFF is correct for inverted signal logic */
	memset(px_buf, 0xFF, buf_len);

	/*
	 * Convert pixel data into a packed bitstream for the UART.
	 * Each color bit is expanded into a pattern of `bits_per_symbol`.
	 */
	for (size_t i = 0; i < num_pixels; i++) {
		for (uint8_t j = 0; j < cfg->num_colors; j++) {
			uint8_t pixel_val;

			switch (cfg->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				pixel_val = 0;
				break;
			case LED_COLOR_ID_RED:
				pixel_val = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				pixel_val = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				pixel_val = pixels[i].b;
				break;
			default:
				LOG_ERR("Invalid color mapping");
				k_mutex_unlock(&data->lock);
				return -EINVAL;
			}

			ws2812_uart_ser(pixel_val, cfg, &frame_bit_pos, &current_buf);
		}
	}

	/*
	 * Start the non-blocking transfer. The uart_tx function will return
	 * immediately. The callback will signal completion via the semaphore.
	 */
	ret = uart_tx(cfg->uart_dev, px_buf, buf_len, SYS_FOREVER_US);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Wait for the transfer to complete. */
	k_sem_take(&data->tx_done_sem, K_FOREVER);

	/* Latch the data and reset the strip */
	ws2812_reset_delay(cfg->reset_delay);
	k_mutex_unlock(&data->lock);

	return 0;
}

static size_t ws2812_strip_length(const struct device *dev)
{
	const struct ws2812_uart_cfg *cfg = dev->config;

	return cfg->length;
}

static int ws2812_uart_init(const struct device *dev)
{
	const struct ws2812_uart_cfg *cfg = dev->config;
	struct ws2812_uart_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("%s: UART device %s not ready", dev->name, cfg->uart_dev->name);
		return -ENODEV;
	}

	for (int i = 0; i < cfg->num_colors; i++) {
		switch (cfg->color_mapping[i]) {
		case LED_COLOR_ID_WHITE:
		case LED_COLOR_ID_RED:
		case LED_COLOR_ID_GREEN:
		case LED_COLOR_ID_BLUE:
			break;
		default:
			LOG_ERR("%s: invalid channel to color mapping.", dev->name);
			return -EINVAL;
		}
	}

	k_mutex_init(&data->lock);
	k_sem_init(&data->tx_done_sem, 0, 1);

	ret = uart_callback_set(cfg->uart_dev, ws2812_uart_callback, &data->tx_done_sem);
	if (ret) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	return 0;
}

static const struct led_strip_driver_api ws2812_uart_api = {
	.update_rgb = ws2812_strip_update_rgb,
	.length = ws2812_strip_length,
};

#define WS2812_NUM_PIXELS(idx)           (DT_INST_PROP(idx, chain_length))
#define WS2812_NUM_COLORS(idx)           (DT_INST_PROP_LEN(idx, color_mapping))
#define WS2812_UART_BITS_PER_SYMBOL(idx) (DT_INST_PROP(idx, bits_per_symbol))
#define WS2812_UART_BUFSZ(idx)                                                                     \
	WS2812_UART_CALC_BUFSZ(WS2812_NUM_PIXELS(idx), WS2812_NUM_COLORS(idx),                     \
			       WS2812_UART_BITS_PER_SYMBOL(idx), UART_FRAME_BITS_FROM_DT(idx))

#define WS2812_UART_CHECK(idx)                                                                     \
	BUILD_ASSERT(!DT_UART_HAS_PARITY(idx),                                                     \
		     "The UART peripheral must be configured with parity disabled.");              \
	BUILD_ASSERT(DT_UART_STOP_BITS(idx) == 1,                                                  \
		     "The UART peripheral's stop-bits property must be set to 1.");                \
	BUILD_ASSERT(DT_UART_HAS_TX_INVERT(idx),                                                   \
		     "The UART peripheral must be configured with tx-invert.");                    \
	BUILD_ASSERT((UART_FRAME_BITS_FROM_DT(idx) % WS2812_UART_BITS_PER_SYMBOL(idx)) == 0,       \
		     "Total UART frame bits must be a multiple of bits-per-symbol.");              \
	BUILD_ASSERT(WS2812_UART_BITS_PER_SYMBOL(idx) <= 10,                                       \
		     "bits-per-symbol cannot be greater than 10.");                                \
	BUILD_ASSERT(WS2812_UART_BITS_PER_SYMBOL(idx) >= 3,                                        \
		     "bits-per-symbol must be at least 3.");                                       \
	BUILD_ASSERT(                                                                              \
		(DT_INST_PROP(idx, one_symbol) & BIT(WS2812_UART_BITS_PER_SYMBOL(idx) - 1)) &&     \
			(DT_INST_PROP(idx, zero_symbol) &                                          \
			 BIT(WS2812_UART_BITS_PER_SYMBOL(idx) - 1)),                               \
		"Symbol's MSB must be 1 (the start bit may be reused).");                          \
	BUILD_ASSERT(!((DT_INST_PROP(idx, one_symbol) & BIT(0)) ||                                 \
		       (DT_INST_PROP(idx, zero_symbol) & BIT(0))),                                 \
		     "Symbol's LSB must be 0 (the stop bit may be reused).")

#define WS2812_UART_DEVICE(idx)                                                                    \
	WS2812_UART_CHECK(idx);                                                                    \
	static uint8_t ws2812_uart_##idx##_px_buf[WS2812_UART_BUFSZ(idx)];                         \
	static struct ws2812_uart_data ws2812_uart_##idx##_data;                                   \
	static const uint8_t ws2812_uart_##idx##_color_mapping[] =                                 \
		DT_INST_PROP(idx, color_mapping);                                                  \
	static const struct ws2812_uart_cfg ws2812_uart_##idx##_cfg = {                            \
		.uart_dev = DEVICE_DT_GET(DT_INST_PARENT(idx)),                                    \
		.px_buf = ws2812_uart_##idx##_px_buf,                                              \
		.one_symbol = DT_INST_PROP(idx, one_symbol),                                       \
		.zero_symbol = DT_INST_PROP(idx, zero_symbol),                                     \
		.bits_per_symbol = WS2812_UART_BITS_PER_SYMBOL(idx),                               \
		.num_colors = WS2812_NUM_COLORS(idx),                                              \
		.color_mapping = ws2812_uart_##idx##_color_mapping,                                \
		.length = WS2812_NUM_PIXELS(idx),                                                  \
		.reset_delay = DT_INST_PROP(idx, reset_delay),                                     \
		.uart_frame_bits = UART_FRAME_BITS_FROM_DT(idx),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, ws2812_uart_init, NULL, &ws2812_uart_##idx##_data,              \
			      &ws2812_uart_##idx##_cfg, POST_KERNEL,                               \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_uart_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_UART_DEVICE)
