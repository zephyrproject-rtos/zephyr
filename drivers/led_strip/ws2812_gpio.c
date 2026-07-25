/*
 * SPDX-FileCopyrightText: Copyright (c) 2018 Intel Corporation
 * SPDX-FileCopyrightText: Copyright (c) 2019 Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright (c) 2021 Seagate Technology LLC
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_gpio

#include <zephyr/drivers/led_strip.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_gpio);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fast.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>

struct ws2812_gpio_cfg {
	struct gpio_dt_spec gpio;
	uint8_t num_colors;
	const uint8_t *color_mapping;
	size_t length;
};

#define WS2812_NODE DT_INST(0, DT_DRV_COMPAT)
#define WS2812_BACKEND GPIO_FAST_COMPAT(WS2812_NODE, gpios)

struct ws2812_gpio_data {
	struct GPIO_FAST_DISPATCH_TYPE(WS2812_BACKEND) fast_spec;
};

/*
 * WS2812 datasheet timing (nanoseconds):
 *
 *   Symbol  Typical  Tolerance    Description
 *   T0H       400    ±150 ns     0-bit high phase
 *   T0L       850    ±150 ns     0-bit low phase
 *   T1H       800    ±150 ns     1-bit high phase
 *   T1L       450    ±150 ns     1-bit low phase
 *
 * NOP delay targets are set BELOW the datasheet center values to leave
 * headroom for per-bit overhead (bit-test branch, pipeline refills, etc.)
 * that is not accounted for by the GPIO_FAST_*_CYCLES subtraction:
 *
 *   T0H target: 350 ns (50 ns headroom within 250–550 ns window)
 *   T0L target: 800 ns (50 ns headroom within 700–1000 ns window)
 *   T1H target: 700 ns (100 ns headroom within 650–950 ns window)
 *   T1L target: 600 ns (150 ns headroom within 300–600 ns window)
 *
 * The NOP count is: target_ns converted to cycles, minus the cost of
 * the gpio_fast operation that follows each delay:
 *   - HIGH phases (T0H, T1H): delay precedes gpio_fast_clear
 *   - LOW phases (T0L, T1L): delay precedes gpio_fast_set (next bit)
 *
 * Boards can override via DT delay-t0h/t1h/t0l/t1l properties on the
 * chosen zephyr,led-strip node if the computed values need tuning.
 */
#define WS2812_CHOSEN DT_CHOSEN(zephyr_led_strip)

#define WS2812_SET_CYCLES \
	GPIO_FAST_DISPATCH_VAL(WS2812_BACKEND, GPIO_FAST_SET_CYCLES)
#define WS2812_CLEAR_CYCLES \
	GPIO_FAST_DISPATCH_VAL(WS2812_BACKEND, GPIO_FAST_CLEAR_CYCLES)

#if DT_NODE_HAS_PROP(WS2812_CHOSEN, delay_t0h)
#define WS2812_T0H_NOPS DT_PROP(WS2812_CHOSEN, delay_t0h)
#else
#define WS2812_T0H_NOPS (GPIO_FAST_NS_TO_CYCLES(350) - WS2812_CLEAR_CYCLES)
#endif

#if DT_NODE_HAS_PROP(WS2812_CHOSEN, delay_t0l)
#define WS2812_T0L_NOPS DT_PROP(WS2812_CHOSEN, delay_t0l)
#else
#define WS2812_T0L_NOPS (GPIO_FAST_NS_TO_CYCLES(800) - WS2812_SET_CYCLES)
#endif

#if DT_NODE_HAS_PROP(WS2812_CHOSEN, delay_t1h)
#define WS2812_T1H_NOPS DT_PROP(WS2812_CHOSEN, delay_t1h)
#else
#define WS2812_T1H_NOPS (GPIO_FAST_NS_TO_CYCLES(700) - WS2812_CLEAR_CYCLES)
#endif

#if DT_NODE_HAS_PROP(WS2812_CHOSEN, delay_t1l)
#define WS2812_T1L_NOPS DT_PROP(WS2812_CHOSEN, delay_t1l)
#else
#define WS2812_T1L_NOPS (GPIO_FAST_NS_TO_CYCLES(600) - WS2812_SET_CYCLES)
#endif

/*
 * Compile-time timing validation.
 *
 * Check that each gpio_fast operation alone doesn't exceed the shortest
 * WS2812 phase (using datasheet values, not the conservative targets).
 */
BUILD_ASSERT(WS2812_SET_CYCLES < GPIO_FAST_NS_TO_CYCLES(400),
	     "gpio_fast_set is too slow for WS2812 T0H (datasheet: 400 ns)");
BUILD_ASSERT(WS2812_CLEAR_CYCLES < GPIO_FAST_NS_TO_CYCLES(450),
	     "gpio_fast_clear is too slow for WS2812 T1L (datasheet: 450 ns)");
BUILD_ASSERT(WS2812_T0H_NOPS > 0,
	     "T0H NOP count must be > 0 (clock too slow or overhead too high)");
BUILD_ASSERT(WS2812_T1H_NOPS > 0,
	     "T1H NOP count must be > 0 (clock too slow or overhead too high)");
BUILD_ASSERT(WS2812_T0L_NOPS > 0,
	     "T0L NOP count must be > 0 (clock too slow or overhead too high)");
BUILD_ASSERT(WS2812_T1L_NOPS > 0,
	     "T1L NOP count must be > 0 (clock too slow or overhead too high)");
BUILD_ASSERT(WS2812_T0H_NOPS < WS2812_T1H_NOPS,
	     "T0H must be shorter than T1H for WS2812 to distinguish bits");

/*
 * Transmit buffer using gpio_fast_set/clear with NOP delays.
 *
 * GPIO_FAST_SEND_STREAM_ATTR places this in RAM on platforms without
 * instruction cache (e.g. SAM0) to avoid flash wait-state jitter.
 */
static GPIO_FAST_DISPATCH_VAL(WS2812_BACKEND, GPIO_FAST_SEND_STREAM_ATTR)
int ws2812_send_buf(
	const struct GPIO_FAST_DISPATCH_TYPE(WS2812_BACKEND) *spec,
	const uint8_t *buf, size_t len)
{
	unsigned int key;
	int rc;

	rc = GPIO_FAST_DISPATCH_CALL(WS2812_BACKEND, gpio_fast_pre_stream, spec);
	if (rc < 0) {
		return rc;
	}

	key = irq_lock();

	while (len--) {
		uint8_t byte = *buf++;

		for (int i = 7; i >= 0; i--) {
			if (byte & BIT(i)) {
				GPIO_FAST_DISPATCH_CALL(WS2812_BACKEND,
					gpio_fast_set, spec);
				GPIO_FAST_DELAY_NOPS(WS2812_T1H_NOPS);
				GPIO_FAST_DISPATCH_CALL(WS2812_BACKEND,
					gpio_fast_clear, spec);
				GPIO_FAST_DELAY_NOPS(WS2812_T1L_NOPS);
			} else {
				GPIO_FAST_DISPATCH_CALL(WS2812_BACKEND,
					gpio_fast_set, spec);
				GPIO_FAST_DELAY_NOPS(WS2812_T0H_NOPS);
				GPIO_FAST_DISPATCH_CALL(WS2812_BACKEND,
					gpio_fast_clear, spec);
				GPIO_FAST_DELAY_NOPS(WS2812_T0L_NOPS);
			}
		}
	}

	irq_unlock(key);

	rc = GPIO_FAST_DISPATCH_CALL(WS2812_BACKEND, gpio_fast_post_stream, spec);

	return rc;
}

static int ws2812_gpio_update_rgb(const struct device *dev,
				  struct led_rgb *pixels,
				  size_t num_pixels)
{
	const struct ws2812_gpio_cfg *config = dev->config;
	struct ws2812_gpio_data *data = dev->data;
	uint8_t *ptr = (uint8_t *)pixels;
	size_t i;

	/* Convert from RGB to on-wire format (e.g. GRB, GRBW, RGB, etc) */
	for (i = 0; i < num_pixels; i++) {
		uint8_t j;
		const struct led_rgb current_pixel = pixels[i];

		for (j = 0; j < config->num_colors; j++) {
			switch (config->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				*ptr++ = 0;
				break;
			case LED_COLOR_ID_RED:
				*ptr++ = current_pixel.r;
				break;
			case LED_COLOR_ID_GREEN:
				*ptr++ = current_pixel.g;
				break;
			case LED_COLOR_ID_BLUE:
				*ptr++ = current_pixel.b;
				break;
			default:
				return -EINVAL;
			}
		}
	}

	return ws2812_send_buf(&data->fast_spec, (uint8_t *)pixels,
			       num_pixels * config->num_colors);
}

static size_t ws2812_gpio_length(const struct device *dev)
{
	const struct ws2812_gpio_cfg *config = dev->config;

	return config->length;
}

static DEVICE_API(led_strip, ws2812_gpio_api) = {
	.update_rgb = ws2812_gpio_update_rgb,
	.length = ws2812_gpio_length,
};

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
#define WS2812_COLOR_MAPPING(idx)					\
static const uint8_t ws2812_gpio_##idx##_color_mapping[] =		\
	DT_INST_PROP(idx, color_mapping)

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

#define WS2812_GPIO_DEVICE(idx)					\
								\
	static int ws2812_gpio_##idx##_init(const struct device *dev)	\
	{								\
		const struct ws2812_gpio_cfg *cfg = dev->config;	\
		struct ws2812_gpio_data *data = dev->data;		\
		uint8_t i;						\
									\
		if (!gpio_is_ready_dt(&cfg->gpio)) {			\
			LOG_ERR("GPIO device not ready");		\
			return -ENODEV;					\
		}							\
									\
		for (i = 0; i < cfg->num_colors; i++) {			\
			switch (cfg->color_mapping[i]) {		\
			case LED_COLOR_ID_WHITE:			\
			case LED_COLOR_ID_RED:				\
			case LED_COLOR_ID_GREEN:			\
			case LED_COLOR_ID_BLUE:				\
				break;					\
			default:					\
				LOG_ERR("%s: invalid channel to color mapping." \
					" Check the color-mapping DT property",	\
					dev->name);			\
				return -EINVAL;				\
			}						\
		}							\
									\
		int ret = GPIO_FAST_CONFIGURE_DT(WS2812_BACKEND,	\
				&data->fast_spec,		\
				&cfg->gpio, GPIO_OUTPUT_LOW);	\
		if (ret < 0) {					\
			return ret;				\
		}						\
									\
		/* Flush boot noise: send a black frame */	\
		{						\
			size_t _sz = cfg->length *		\
				     cfg->num_colors;		\
			uint8_t _zeros[_sz];			\
			memset(_zeros, 0, _sz);			\
			ws2812_send_buf(&data->fast_spec,	\
					_zeros, _sz);		\
		}						\
									\
		return 0;					\
	}								\
									\
	BUILD_ASSERT(WS2812_NUM_COLORS(idx) <= sizeof(struct led_rgb),  \
		"Too many channels in color-mapping; "			\
		"currently not supported by the ws2812_gpio driver");	\
									\
	WS2812_COLOR_MAPPING(idx);					\
									\
	static struct ws2812_gpio_data ws2812_gpio_##idx##_data;	\
									\
	static const struct ws2812_gpio_cfg ws2812_gpio_##idx##_cfg = { \
		.gpio = GPIO_DT_SPEC_INST_GET(idx, gpios),		\
		.num_colors = WS2812_NUM_COLORS(idx),			\
		.color_mapping = ws2812_gpio_##idx##_color_mapping,	\
		.length = DT_INST_PROP(idx, chain_length),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(idx,					\
			    ws2812_gpio_##idx##_init,			\
			    NULL,					\
			    &ws2812_gpio_##idx##_data,			\
			    &ws2812_gpio_##idx##_cfg, POST_KERNEL,	\
			    CONFIG_LED_STRIP_INIT_PRIORITY,		\
			    &ws2812_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_GPIO_DEVICE)
