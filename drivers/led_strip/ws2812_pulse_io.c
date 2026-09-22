/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_pulse_io

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/pulse_io.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ws2812_pulse_io, CONFIG_LED_STRIP_LOG_LEVEL);

/* One cell tick is 100 ns; the WS2812 bit period is nominally 1.25 us */
#define WS2812_RESOLUTION_HZ 10000000
#define WS2812_CELL_PERIOD   13
#define WS2812_DUTY_ZERO     4
#define WS2812_DUTY_ONE      8

struct ws2812_pio_config {
	const struct device *pio_dev;
	uint8_t channel;
	uint8_t num_colors;
	const uint8_t *color_mapping;
	uint16_t length;
	uint16_t reset_delay;
	struct pulse_cell *cells;
};

struct ws2812_pio_data {
	struct pulse_io_channel *chan;
};

static void ws2812_encode_byte(struct pulse_cell *cells, uint8_t byte)
{
	for (int i = 0; i < 8; i++) {
		cells[i] = (struct pulse_cell){
			.duty = (byte & BIT(7 - i)) ? WS2812_DUTY_ONE : WS2812_DUTY_ZERO,
		};
	}
}

static int ws2812_pio_update_rgb(const struct device *dev, struct led_rgb *pixels,
				 size_t num_pixels)
{
	const struct ws2812_pio_config *cfg = dev->config;
	struct ws2812_pio_data *data = dev->data;
	struct pulse_cell *cells = cfg->cells;
	struct pulse_io_tx_req req;
	int ret;

	if (num_pixels > cfg->length) {
		return -EINVAL;
	}

	for (size_t i = 0; i < num_pixels; i++) {
		for (uint8_t j = 0; j < cfg->num_colors; j++) {
			uint8_t component;

			switch (cfg->color_mapping[j]) {
			case LED_COLOR_ID_RED:
				component = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				component = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				component = pixels[i].b;
				break;
			default:
				return -EINVAL;
			}
			ws2812_encode_byte(&cells[(i * cfg->num_colors + j) * 8], component);
		}
	}

	req = (struct pulse_io_tx_req){
		.cells = cells,
		.count = (size_t)num_pixels * cfg->num_colors * 8,
	};
	ret = pulse_io_transmit_sync(cfg->pio_dev, data->chan, &req, K_MSEC(100));
	if (ret) {
		return ret;
	}

	/* latch the shifted-out colors */
	k_usleep(cfg->reset_delay);

	return 0;
}

static size_t ws2812_pio_length(const struct device *dev)
{
	const struct ws2812_pio_config *cfg = dev->config;

	return cfg->length;
}

static int ws2812_pio_init(const struct device *dev)
{
	const struct ws2812_pio_config *cfg = dev->config;
	struct ws2812_pio_data *data = dev->data;
	struct pulse_io_config pio_cfg = {
		.mode = PULSE_IO_MODE_CELL,
		.dir = PULSE_IO_DIR_TX,
		.resolution_hz = WS2812_RESOLUTION_HZ,
		.cell_period_ticks = WS2812_CELL_PERIOD,
	};
	int ret;

	if (!device_is_ready(cfg->pio_dev)) {
		LOG_ERR("pulse_io device not ready");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < cfg->num_colors; i++) {
		switch (cfg->color_mapping[i]) {
		case LED_COLOR_ID_RED:
		case LED_COLOR_ID_GREEN:
		case LED_COLOR_ID_BLUE:
			break;
		default:
			LOG_ERR("unsupported color mapping");
			return -EINVAL;
		}
	}

	ret = pulse_io_channel_get(cfg->pio_dev, cfg->channel, &data->chan);
	if (ret) {
		LOG_ERR("failed to reserve pulse_io channel %u (%d)", cfg->channel, ret);
		return ret;
	}

	ret = pulse_io_channel_configure(cfg->pio_dev, data->chan, &pio_cfg);
	if (ret) {
		LOG_ERR("failed to configure pulse_io channel (%d)", ret);
		pulse_io_channel_release(cfg->pio_dev, data->chan);
		return ret;
	}

	return 0;
}

static DEVICE_API(led_strip, ws2812_pio_api) = {
	.update_rgb = ws2812_pio_update_rgb,
	.length = ws2812_pio_length,
};

#define WS2812_PIO_DEVICE(idx)                                                                     \
	static const uint8_t ws2812_pio_##idx##_color_mapping[] =                                  \
		DT_INST_PROP(idx, color_mapping);                                                  \
	static struct pulse_cell ws2812_pio_##idx##_cells[DT_INST_PROP(idx, chain_length) *        \
							  DT_INST_PROP_LEN(idx, color_mapping) *   \
							  8];                                      \
	static const struct ws2812_pio_config ws2812_pio_##idx##_config = {                        \
		.pio_dev = DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(idx, pulse_ios, 0)),               \
		.channel = DT_INST_PHA_BY_IDX(idx, pulse_ios, 0, channel),                         \
		.num_colors = DT_INST_PROP_LEN(idx, color_mapping),                                \
		.color_mapping = ws2812_pio_##idx##_color_mapping,                                 \
		.length = DT_INST_PROP(idx, chain_length),                                         \
		.reset_delay = DT_INST_PROP(idx, reset_delay),                                     \
		.cells = ws2812_pio_##idx##_cells,                                                 \
	};                                                                                         \
	static struct ws2812_pio_data ws2812_pio_##idx##_data;                                     \
	DEVICE_DT_INST_DEFINE(idx, ws2812_pio_init, NULL, &ws2812_pio_##idx##_data,                \
			      &ws2812_pio_##idx##_config, POST_KERNEL,                             \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_pio_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_PIO_DEVICE)
