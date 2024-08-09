/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT worldsemi_ws2812_uart

#include <zephyr/drivers/led_strip.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_uart);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/led/led.h>

struct ws2812_uart_cfg {
	const struct device *uart_dev; /* UART device */
	uint8_t *px_buf;
	size_t length;
	uint8_t num_colors;
	const uint8_t *color_mapping;
	const uint8_t *uart_lookup_table;
	uint16_t reset_delay;
	uint8_t repeat_count;
};

/*
 * Serialize a 24-bit color channel value into an equivalent sequence
 * of 8 bytes of UART frames,.
 */
static inline int ws2812_uart_ser(const struct device *dev, uint8_t *buf, uint8_t color[3])
{
	int i, j;
	int buff_indx = 0;
	uint8_t serialized_val = 0;
	const struct ws2812_uart_cfg *cfg = dev->config;

	/* Loop through each of the 3 color components (Red, Green, Blue) */
	for (i = 0; i < 3; i++) {
		/* Loop through each bit of the current color component */
		for (j = 0; j < 8; j++) {
			/* Shift the serialized value left by 1 bit to make room for the next bit,
			 *  then OR it with the current bit of the color component.
			 */
			serialized_val = (serialized_val << 1) | ((color[i] >> (7 - j)) & 0x01);

			/* Check if we have serialized enough bits based on the repeat_count.
			 * If so, use the serialized value to look up the corresponding UART value
			 * from the lookup table and store it in the buffer.
			 */
			if (((i * 8) + j) % cfg->repeat_count == (cfg->repeat_count - 1)) {
				buf[buff_indx++] = cfg->uart_lookup_table[serialized_val];

				/* Now Reset the serialized value for the next set of bits */
				serialized_val = 0;
			}
		}
	}
	/* Return the number of bytes filled in the buffer */
	return buff_indx;
}

static void ws2812_uart_tx(const struct ws2812_uart_cfg *cfg, uint8_t *tx, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uart_poll_out(cfg->uart_dev, tx[i]);
	}
}

static int ws2812_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				   size_t num_pixels)
{
	const struct ws2812_uart_cfg *cfg = dev->config;

	uint8_t *px_buf = cfg->px_buf;
	size_t px_buf_len = cfg->length * cfg->num_colors * 8;
	size_t uart_data_len = 0;
	size_t i;
	uint8_t color[3];

	/*
	 * Convert pixel data into UART frames. Each frame has pixel data
	 * in color mapping on-wire format (e.g. GRB, GRBW, RGB, etc).
	 */

	for (i = 0; i < num_pixels; i++) {
		uint8_t j;

		for (j = 0; j < cfg->num_colors; j++) {
			switch (cfg->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				color[j] = 0;
				break;
			case LED_COLOR_ID_RED:
				color[j] = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				color[j] = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				color[j] = pixels[i].b;
				break;
			default:
				return -EINVAL;
			}
		}
		uart_data_len += ws2812_uart_ser(dev, px_buf, &color[0]);
		if (uart_data_len <= px_buf_len) {
			px_buf = cfg->px_buf + uart_data_len;
		} else {
			return -ENOMEM;
		}
	}
	ws2812_uart_tx(cfg, cfg->px_buf, uart_data_len);
	k_usleep(cfg->reset_delay);

	return 0;
}

static int ws2812_uart_init(const struct device *dev)
{
	const struct ws2812_uart_cfg *cfg = dev->config;
	uint8_t i;

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
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

static size_t ws2812_strip_length(const struct device *dev)
{
	const struct ws2812_uart_cfg *cfg = dev->config;

	return cfg->length;
}

static const struct led_strip_driver_api ws2812_uart_api = {
	.update_rgb = ws2812_strip_update_rgb,
	.length = ws2812_strip_length,
};

/* Lookup table formation macro definitions starts here*/

/*
 * Reverses the bits in an 8-bit octet.
 * For example, if the input octet is 0b10110010, the output will be 0b01001101.
 */
#define WS2812_UART_BYTE_BITREVERSE(octet)                                                         \
	((((octet >> 0) & 1) << 7) | (((octet >> 1) & 1) << 6) | (((octet >> 2) & 1) << 5) |       \
	 (((octet >> 3) & 1) << 4) | (((octet >> 4) & 1) << 3) | (((octet >> 5) & 1) << 2) |       \
	 (((octet >> 6) & 1) << 1) | (((octet >> 7) & 1) << 0))

/*
 * Extracts data bits from an inverted frame, removing the start and stop bits.
 * Shifts the input frame right by 1 bit, and masks to keep only the relevant data bits.
 */
#define WS2812_PACKED_INVERTED_UART_WITHOUT_START_STOP(inv_pack_frame, data_bits)                  \
	(((inv_pack_frame) >> 1) & ((1 << (data_bits)) - 1))

/*
 * Produces a UART byte for WS2812 LEDs by bit-reversing(because in UART transactions LSB is sent
 * first) and packing the frame. Inverts the input frame(because we are inverting the tx-lines),
 * extracts the relevant data bits, and right shifts the result.
 */
#define WS2812_PACKED_UART_BYTE(packed_frame, packed_frame_len)                                    \
	(WS2812_UART_BYTE_BITREVERSE(WS2812_PACKED_INVERTED_UART_WITHOUT_START_STOP(               \
		 ~(packed_frame), packed_frame_len - 2)) >>                                        \
	 (8 - (packed_frame_len - 2)))

/*
 * Creates a 1x lookup table entry for WS2812 LEDs.
 * The lookup table contains packed UART bytes for zero and one frames.
 */
#define WS2812_1X_LOOK_UP_TABLE_PREPARE(zero_frame, one_frame, frame_len)                          \
	{                                                                                          \
		WS2812_PACKED_UART_BYTE((zero_frame), (frame_len)),                                \
			WS2812_PACKED_UART_BYTE((one_frame), (frame_len)),                         \
	}

/*
 * Creates a 2x lookup table entry for WS2812 LEDs.
 * The lookup table contains packed UART bytes for all combinations of zero and one frames.
 */
#define WS2812_2X_LOOK_UP_TABLE_PREPARE(zero_frame, one_frame, frame_len)                          \
	{                                                                                          \
		WS2812_PACKED_UART_BYTE(((zero_frame) << frame_len) | (zero_frame),                \
					2 * frame_len),                                            \
			WS2812_PACKED_UART_BYTE(((zero_frame) << frame_len) | (one_frame),         \
						2 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((one_frame) << frame_len) | (zero_frame),         \
						2 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((one_frame) << frame_len) | (one_frame),          \
						2 * frame_len),                                    \
	}

/*
 * Creates a 3x lookup table entry for WS2812 LEDs.
 * The lookup table contains packed UART bytes for all combinations of zero and one frames.
 */
#define WS2812_3X_LOOK_UP_TABLE_PREPARE(zero_frame, one_frame, frame_len)                          \
	{                                                                                          \
		WS2812_PACKED_UART_BYTE(((zero_frame) << 2 * frame_len) |                          \
						((zero_frame) << frame_len) | (zero_frame),        \
					3 * frame_len),                                            \
			WS2812_PACKED_UART_BYTE(((zero_frame) << 2 * frame_len) |                  \
							((zero_frame) << frame_len) | (one_frame), \
						3 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((zero_frame) << 2 * frame_len) |                  \
							((one_frame) << frame_len) | (zero_frame), \
						3 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((zero_frame) << 2 * frame_len) |                  \
							((one_frame) << frame_len) | (one_frame),  \
						3 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((one_frame) << 2 * frame_len) |                   \
							((zero_frame) << frame_len) |              \
							(zero_frame),                              \
						3 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((one_frame) << 2 * frame_len) |                   \
							((zero_frame) << frame_len) | (one_frame), \
						3 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((one_frame) << 2 * frame_len) |                   \
							((one_frame) << frame_len) | (zero_frame), \
						3 * frame_len),                                    \
			WS2812_PACKED_UART_BYTE(((one_frame) << 2 * frame_len) |                   \
							((one_frame) << frame_len) | (one_frame),  \
						3 * frame_len),                                    \
	}

/*
 * Macro to create a lookup table with repeated frames for WS2812 LEDs.
 * This generates the appropriate lookup table based on the repeat count.
 */
#define WS2812_LOOK_UP_TABLE_LOOP(repeat_count, zero_frame, one_frame, frame_len)                  \
	WS2812_##repeat_count##X_LOOK_UP_TABLE_PREPARE((zero_frame) & ((1 << frame_len) - 1),      \
						       (one_frame) & ((1 << frame_len) - 1),       \
						       frame_len)

/*
 * Helper macro to handle concatenation for lookup table creation.
 */
#define WS2812_LOOK_UP_TABLE_LOOP_PASTER(repeat_count, zero_frame, one_frame, frame_len)           \
	WS2812_LOOK_UP_TABLE_LOOP(repeat_count, zero_frame, one_frame, frame_len)

/*
 * Macro to prepare the lookup table for WS2812 LEDs.
 * Generates the lookup table based on the repeat count, zero frame, one frame, and frame length.
 */
#define WS2812_LOOK_UP_TABLE_PREPARE(repeat_count, zero_frame, one_frame, frame_len)               \
	WS2812_LOOK_UP_TABLE_LOOP_PASTER(repeat_count, zero_frame, one_frame, frame_len)

/* Lookup table formation macro definitions ends here*/

#define WS2812_DEFAULT_RESET_DELAY_MS (50)
#define WS2812_UART_NUM_PIXELS(idx)   (DT_INST_PROP(idx, chain_length))
#define WS2812_UART_BUFSZ(idx, repeat)                                                             \
	((WS2812_NUM_COLORS(idx) * 8 * WS2812_UART_NUM_PIXELS(idx)) / repeat)

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
#define WS2812_COLOR_MAPPING(idx)                                                                  \
	static const uint8_t ws2812_uart_##idx##_color_mapping[] = DT_INST_PROP(idx, color_mapping)

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

/* Get the latch/reset delay from the "reset-delay" DT property. */
#define WS2812_RESET_DELAY(idx) DT_INST_PROP_OR(idx, reset_delay, WS2812_DEFAULT_RESET_DELAY_MS)

#define UART_NODE(idx) DT_PARENT(DT_INST(idx, worldsemi_ws2812_uart))

#define WS2812_UART_DEVICE(idx)                                                                    \
                                                                                                   \
	BUILD_ASSERT(DT_PROP(UART_NODE(idx), tx_invert),                                           \
		     "This driver depends on tx-invert UART property and "                         \
		     "is not set. Please check your device-tree settings");                        \
                                                                                                   \
	BUILD_ASSERT(DT_PROP(UART_NODE(idx), data_bits), "data-bits property missing");            \
                                                                                                   \
	BUILD_ASSERT(DT_PROP(UART_NODE(idx), data_bits) <= 8, "data-bits > 8 is not supported");   \
                                                                                                   \
	BUILD_ASSERT((2 + DT_PROP(UART_NODE(idx), data_bits)) % DT_INST_PROP(idx, frame_len) == 0, \
		     "data-bits+2 should be a multiple of frame-len");                             \
                                                                                                   \
	static uint8_t ws2812_uart_##idx##_px_buf[WS2812_UART_BUFSZ(                               \
		idx, DT_INST_PROP(idx, rgb_frame_per_uart_frame))];                                \
                                                                                                   \
	WS2812_COLOR_MAPPING(idx);                                                                 \
	static const uint8_t ws2812_uart_##idx##_lut[] = WS2812_LOOK_UP_TABLE_PREPARE(             \
		DT_INST_PROP(idx, rgb_frame_per_uart_frame), DT_INST_PROP(idx, zero_frame),        \
		DT_INST_PROP(idx, one_frame), DT_INST_PROP(idx, frame_len));                       \
                                                                                                   \
	static const struct ws2812_uart_cfg ws2812_uart_##idx##_cfg = {                            \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(idx)),                                       \
		.px_buf = ws2812_uart_##idx##_px_buf,                                              \
		.num_colors = WS2812_NUM_COLORS(idx),                                              \
		.color_mapping = ws2812_uart_##idx##_color_mapping,                                \
		.reset_delay = MAX(WS2812_RESET_DELAY(idx), 8),                                    \
		.length = WS2812_UART_NUM_PIXELS(idx),                                             \
		.uart_lookup_table = ws2812_uart_##idx##_lut,                                      \
		.repeat_count = DT_INST_PROP(idx, rgb_frame_per_uart_frame),                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, ws2812_uart_init, NULL, NULL, &ws2812_uart_##idx##_cfg,         \
			      POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_uart_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_UART_DEVICE)
