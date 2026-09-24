/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Display driver for a monochrome / grayscale LED matrix driven by
 * charlieplexed GPIOs.
 *
 * Charlieplexing drives up to N*(N-1) LEDs from N GPIO lines: each LED sits
 * between an ordered pair of lines, and is lit by driving one line high and
 * one low while every other line is high-impedance (configured as input /
 * disconnected). Only one LED is physically lit at any instant; a timer ISR
 * scans the matrix fast enough for persistence of vision. Lines are driven at
 * physical levels, so the per-line devicetree GPIO polarity flag is ignored
 * (each line is driven both ways across a scan).
 *
 * Grayscale is produced by binary-coded-modulation-free duty scanning: the
 * full pixel list is scanned 2^grayscale_bits times per refresh ("sub-frames");
 * a pixel with brightness B is driven during the first B sub-frames and dark in
 * the rest. brightness == max => always on.
 *
 * The driver uses only the generic GPIO and counter APIs, so it is portable
 * across any Zephyr SoC (no direct register access).
 */

#define DT_DRV_COMPAT charlieplex_led_matrix

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(display_charlieplex_led_matrix, CONFIG_DISPLAY_LOG_LEVEL);

struct cplx_config {
	const struct device *counter;
	const struct gpio_dt_spec *gpios;
	uint8_t num_gpios;
	const uint16_t *pixel_pairs; /* (high<<8)|low per pixel, fb order */
	uint16_t num_pixels;
	uint16_t width;
	uint16_t height;
	uint8_t grayscale_bits;
	uint32_t refresh_hz;
};

struct cplx_data {
	const struct device *dev; /* back-pointer for the ISR */
	uint8_t *framebuf;        /* per-pixel brightness 0..(2^bits - 1) */
	uint16_t scan_idx;        /* pixel currently being serviced */
	uint8_t subframe;         /* 0..(2^bits - 1) duty phase */
	int8_t cur_high;          /* gpio index currently driven, -1 = none */
	int8_t cur_low;
	bool blanking;
};

/* Release whichever pair was driven on the previous tick back to Hi-Z. */
static void cplx_all_off(const struct device *dev)
{
	const struct cplx_config *cfg = dev->config;
	struct cplx_data *data = dev->data;

	if (data->cur_high >= 0) {
		gpio_pin_configure_dt(&cfg->gpios[data->cur_high], GPIO_DISCONNECTED);
		data->cur_high = -1;
	}
	if (data->cur_low >= 0) {
		gpio_pin_configure_dt(&cfg->gpios[data->cur_low], GPIO_DISCONNECTED);
		data->cur_low = -1;
	}
}

/* Light a single pixel by driving its high/low pair, rest already Hi-Z. */
static void cplx_light(const struct device *dev, uint16_t pixel)
{
	const struct cplx_config *cfg = dev->config;
	struct cplx_data *data = dev->data;
	uint16_t entry = cfg->pixel_pairs[pixel];
	uint8_t hi = entry >> 8;
	uint8_t lo = entry & 0xFF;

	/*
	 * Drive physical levels (GPIO_OUTPUT_LOW/HIGH), not logical levels
	 * (GPIO_OUTPUT_INACTIVE/ACTIVE): every line is driven both high and low
	 * across a scan depending on the pixel, so a per-line GPIO_ACTIVE_LOW
	 * polarity flag is meaningless here and is intentionally ignored. Drive
	 * low first then high to avoid a transient on the wrong pair.
	 */
	gpio_pin_configure_dt(&cfg->gpios[lo], GPIO_OUTPUT_LOW);
	gpio_pin_configure_dt(&cfg->gpios[hi], GPIO_OUTPUT_HIGH);
	data->cur_low = lo;
	data->cur_high = hi;
}

/* Timer top-value callback: advance the scan one pixel per tick. */
static void cplx_tick(const struct device *counter_dev, void *user_data)
{
	const struct device *dev = user_data;
	const struct cplx_config *cfg = dev->config;
	struct cplx_data *data = dev->data;

	ARG_UNUSED(counter_dev);

	cplx_all_off(dev);

	if (data->blanking) {
		return;
	}

	uint16_t pixel = data->scan_idx;
	uint8_t bright = data->framebuf[pixel];

	/* Pixel is lit during the first `bright` sub-frames of the duty cycle. */
	if (bright > data->subframe) {
		cplx_light(dev, pixel);
	}

	/* Advance scan position; wrap advances the sub-frame duty phase. */
	data->scan_idx++;
	if (data->scan_idx >= cfg->num_pixels) {
		data->scan_idx = 0;
		uint8_t levels = BIT(cfg->grayscale_bits);

		data->subframe++;
		if (data->subframe >= (levels - 1)) {
			data->subframe = 0;
		}
	}
}

static int cplx_start_timer(const struct device *dev)
{
	const struct cplx_config *cfg = dev->config;
	uint8_t levels = BIT(cfg->grayscale_bits);
	/* sub-steps per refresh = (levels - 1), one scan of all pixels each. */
	uint32_t sub_steps = (levels > 1) ? (levels - 1) : 1;
	uint32_t tick_hz = cfg->refresh_hz * sub_steps * cfg->num_pixels;
	uint32_t freq = counter_get_frequency(cfg->counter);
	uint32_t ticks;

	if (tick_hz == 0) {
		return -EINVAL;
	}
	ticks = freq / tick_hz;
	if (ticks == 0) {
		/* Timer too slow to hit the requested refresh rate. */
		LOG_WRN("counter freq %u Hz too low for %u Hz scan; clamping", freq, tick_hz);
		ticks = 1;
	}

	struct counter_top_cfg top_cfg = {
		.ticks = ticks,
		.callback = cplx_tick,
		.user_data = (void *)dev,
		.flags = 0,
	};

	int err = counter_set_top_value(cfg->counter, &top_cfg);

	if (err) {
		LOG_ERR("counter_set_top_value failed: %d", err);
		return err;
	}
	return counter_start(cfg->counter);
}

/* display API */

static int cplx_blanking_on(const struct device *dev)
{
	struct cplx_data *data = dev->data;

	data->blanking = true;
	cplx_all_off(dev);
	return 0;
}

static int cplx_blanking_off(const struct device *dev)
{
	struct cplx_data *data = dev->data;

	data->blanking = false;
	return 0;
}

static int cplx_write(const struct device *dev, uint16_t x, uint16_t y,
		      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct cplx_config *cfg = dev->config;
	struct cplx_data *data = dev->data;
	const uint8_t *src = buf;
	uint8_t max = BIT(cfg->grayscale_bits) - 1;

	if (desc->pitch < desc->width) {
		return -EINVAL;
	}
	if (x + desc->width > cfg->width || y + desc->height > cfg->height) {
		return -EINVAL;
	}

	/* MONO01: one bit per pixel, rows padded to whole bytes per `pitch`.
	 * Bit set => full brightness, clear => off.
	 */
	for (uint16_t row = 0; row < desc->height; row++) {
		const uint8_t *line = src + row * (desc->pitch / 8);

		for (uint16_t col = 0; col < desc->width; col++) {
			uint8_t bit = (line[col / 8] >> (col % 8)) & 0x1;
			uint16_t px = (y + row) * cfg->width + (x + col);

			data->framebuf[px] = bit ? max : 0;
		}
	}
	return 0;
}

static void *cplx_get_framebuffer(const struct device *dev)
{
	struct cplx_data *data = dev->data;

	/* Exposes the per-pixel brightness buffer (1 byte/pixel, 0..max). */
	return data->framebuf;
}

static int cplx_set_brightness(const struct device *dev, uint8_t brightness)
{
	const struct cplx_config *cfg = dev->config;
	struct cplx_data *data = dev->data;
	uint8_t max = BIT(cfg->grayscale_bits) - 1;
	/* Scale the 0..255 API brightness onto our 0..max duty levels and apply
	 * it to every currently-lit pixel.
	 */
	uint8_t level = (uint8_t)(((uint32_t)brightness * max) / 255U);

	for (uint16_t i = 0; i < cfg->num_pixels; i++) {
		if (data->framebuf[i]) {
			data->framebuf[i] = (level == 0) ? 1 : level;
		}
	}
	return 0;
}

static void cplx_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct cplx_config *cfg = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = cfg->width;
	caps->y_resolution = cfg->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int cplx_set_pixel_format(const struct device *dev, enum display_pixel_format format)
{
	ARG_UNUSED(dev);
	return (format == PIXEL_FORMAT_MONO01) ? 0 : -ENOTSUP;
}

static DEVICE_API(display, cplx_api) = {
	.blanking_on = cplx_blanking_on,
	.blanking_off = cplx_blanking_off,
	.write = cplx_write,
	.get_framebuffer = cplx_get_framebuffer,
	.set_brightness = cplx_set_brightness,
	.get_capabilities = cplx_get_capabilities,
	.set_pixel_format = cplx_set_pixel_format,
};

static int cplx_init(const struct device *dev)
{
	const struct cplx_config *cfg = dev->config;
	struct cplx_data *data = dev->data;

	data->dev = dev;
	data->cur_high = -1;
	data->cur_low = -1;
	data->scan_idx = 0;
	data->subframe = 0;
	data->blanking = false;

	if (!device_is_ready(cfg->counter)) {
		LOG_ERR("counter device not ready");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < cfg->num_gpios; i++) {
		if (!gpio_is_ready_dt(&cfg->gpios[i])) {
			LOG_ERR("gpio %u not ready", i);
			return -ENODEV;
		}
		/* Start every line high-impedance. */
		gpio_pin_configure_dt(&cfg->gpios[i], GPIO_DISCONNECTED);
	}

	return cplx_start_timer(dev);
}

/* per-instance definition */

#define CPLX_GPIO_SPEC(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

#define CPLX_INIT(inst)                                                                            \
	static const struct gpio_dt_spec cplx_gpios_##inst[] = {                                   \
		DT_INST_FOREACH_PROP_ELEM(inst, gpios, CPLX_GPIO_SPEC)};                           \
	static const uint16_t cplx_pixel_map_##inst[] = DT_INST_PROP(inst, pixel_pairs);           \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, pixel_pairs) ==                                        \
			     DT_INST_PROP(inst, width) * DT_INST_PROP(inst, height),               \
		     "pixel-map length must equal width * height");                                \
	static uint8_t cplx_framebuf_##inst[DT_INST_PROP_LEN(inst, pixel_pairs)];                  \
	static const struct cplx_config cplx_config_##inst = {                                     \
		.counter = DEVICE_DT_GET(DT_INST_PHANDLE(inst, counter)),                          \
		.gpios = cplx_gpios_##inst,                                                        \
		.num_gpios = ARRAY_SIZE(cplx_gpios_##inst),                                        \
		.pixel_pairs = cplx_pixel_map_##inst,                                              \
		.num_pixels = ARRAY_SIZE(cplx_pixel_map_##inst),                                   \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.grayscale_bits = DT_INST_PROP(inst, grayscale_bits),                              \
		.refresh_hz = DT_INST_PROP(inst, refresh_frequency),                               \
	};                                                                                         \
	static struct cplx_data cplx_data_##inst = {                                               \
		.framebuf = cplx_framebuf_##inst,                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, cplx_init, NULL, &cplx_data_##inst, &cplx_config_##inst,       \
			      POST_KERNEL, CONFIG_DISPLAY_CHARLIEPLEX_LED_MATRIX_INIT_PRIORITY,    \
			      &cplx_api);

DT_INST_FOREACH_STATUS_OKAY(CPLX_INIT)
