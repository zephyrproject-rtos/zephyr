/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_bflb_wo

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/dt-bindings/led/led.h>

#include <zephyr/drivers/misc/bflb_wo/bflb_wo.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_bflb_wo);

#define WS2812_BFLB_WO_MAP_COLORS_IMPL_OTHERS(_n, _p, _i) (DT_PROP_BY_IDX(_n, _p, _i) * 8),
#define WS2812_BFLB_WO_MAP_COLORS_IMPL_RGB(_n, _p, _i) ((DT_PROP_BY_IDX(_n, _p, _i) - 1) * 8),
#define WS2812_BFLB_WO_MAP_COLORS_IMPL(_n, _p, _i)						   \
	COND_CODE_1(IS_EQ(DT_PROP_LEN(_n, _p), 3),						   \
		(WS2812_BFLB_WO_MAP_COLORS_IMPL_RGB(_n, _p, _i)),				   \
		(WS2812_BFLB_WO_MAP_COLORS_IMPL_OTHERS(_n, _p, _i)))
#define WS2812_BFLB_WO_MAP_COLORS(_inst, _prop)							   \
	(const size_t []){ DT_INST_FOREACH_PROP_ELEM(_inst, _prop, WS2812_BFLB_WO_MAP_COLORS_IMPL) }

#define WS2812_BFLB_WO_VALIDATE_COLORS_IMPL_RGBW(_n, _p, _i)					   \
	BUILD_ASSERT(DT_PROP_BY_IDX(_n, _p, _i) <= LED_COLOR_ID_BLUE				   \
		     && DT_PROP_BY_IDX(_n, _p, _i) >= LED_COLOR_ID_WHITE,			   \
		     "Mapping is invalid for RGBW");
#define WS2812_BFLB_WO_VALIDATE_COLORS_IMPL_RGB(_n, _p, _i)					   \
	BUILD_ASSERT(DT_PROP_BY_IDX(_n, _p, _i) <= LED_COLOR_ID_BLUE				   \
		     && DT_PROP_BY_IDX(_n, _p, _i) >= LED_COLOR_ID_RED,				   \
		     "Mapping is invalid for RGB");
#define WS2812_BFLB_WO_VALIDATE_COLORS_IMPL(_n, _p, _i)						   \
	COND_CODE_1(IS_EQ(DT_PROP_LEN(_n, _p), 3),						   \
		(WS2812_BFLB_WO_VALIDATE_COLORS_IMPL_RGB(_n, _p, _i)),				   \
		(WS2812_BFLB_WO_VALIDATE_COLORS_IMPL_RGBW(_n, _p, _i)))
#define WS2812_BFLB_WO_VALIDATE_COLORS(_inst, _prop)						   \
	DT_INST_FOREACH_PROP_ELEM(_inst, _prop, WS2812_BFLB_WO_VALIDATE_COLORS_IMPL)

struct ws2812_bflb_wo_config {
	const struct gpio_dt_spec gpio_pin;
	size_t length;
	uint32_t t0h;
	uint32_t t1h;
	uint32_t ttotal;
	const size_t *color_map_offsets_RGB;
	const uint8_t *color_mapping;
	uint8_t num_colors;
	uint16_t reset_delay;
};

static size_t ws2812_bflb_wo_length(const struct device *dev)
{
	const struct ws2812_bflb_wo_config *config = dev->config;

	return config->length;
}

static int ws2812_bflb_wo_update_rgb(const struct device *dev, struct led_rgb *pixels,
				     size_t num_pixels)
{
	const struct ws2812_bflb_wo_config *cfg = dev->config;
	int ret;
	uint16_t buf[cfg->num_colors * 8];
	uint16_t pin_msk = 1U << (cfg->gpio_pin.pin % BFLB_WO_PIN_CNT);
	struct bflb_wo_config wo_cfg = {
		.total_cycles = bflb_wo_time_to_cycles(cfg->ttotal, false),
		.set_cycles = bflb_wo_time_to_cycles(cfg->t1h, false),
		.unset_cycles = bflb_wo_time_to_cycles(cfg->t0h, false),
		.set_invert = false,
		.unset_invert = false,
		.park_high = false,
	};

	ret = bflb_wo_configure_dt(&wo_cfg, &cfg->gpio_pin, 1);
	if (ret != 0) {
		LOG_ERR("Could not configure Wire Out: %d", ret);
		return ret;
	}

	for (size_t i = 0; i < num_pixels; i++) {
		for (size_t j = 0; j < 8; j++) {
			buf[j + cfg->color_map_offsets_RGB[0]] =
				((pixels[i].r >> (7 - j)) & 0x1) ? pin_msk : 0;
			buf[j + cfg->color_map_offsets_RGB[1]] =
				((pixels[i].g >> (7 - j)) & 0x1) ? pin_msk : 0;
			buf[j + cfg->color_map_offsets_RGB[2]] =
				((pixels[i].b >> (7 - j)) & 0x1) ? pin_msk : 0;
			if (cfg->num_colors > 3) {
				buf[j + cfg->color_map_offsets_RGB[3]] = 0;
			}
		}
		ret = bflb_wo_write(buf, cfg->num_colors * 8);
		if (ret != 0) {
			LOG_ERR("Could not write to FIFO: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int ws2812_bflb_wo_init(const struct device *dev)
{
	const struct ws2812_bflb_wo_config *config = dev->config;

	if (!device_is_ready(config->gpio_pin.port)) {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(led_strip, ws2812_bflb_wo_api) = {
	.update_rgb = ws2812_bflb_wo_update_rgb,
	.length = ws2812_bflb_wo_length,
};

#define WS2812_BFLB_WO_DEVICE(n)								\
	BFLB_WO_VALIDATE_INST(n, worldsemi_ws2812_bflb_wo);					\
	static const struct ws2812_bflb_wo_config ws2812_bflb_wo_##n##_cfg = {			\
		.gpio_pin = GPIO_DT_SPEC_INST_GET(n, gpios),					\
		.t0h = DT_INST_PROP(n, time_t0h),						\
		.t1h = DT_INST_PROP(n, time_t1h),						\
		.ttotal = DT_INST_PROP_OR(n, time_total, DT_INST_PROP(n, Z_HZ_ns / frequency)),	\
		.color_map_offsets_RGB = WS2812_BFLB_WO_MAP_COLORS(n, color_mapping),		\
		.color_mapping = (const uint8_t []) DT_INST_PROP(n, color_mapping),		\
		.num_colors = DT_INST_PROP_LEN(n, color_mapping),				\
		.length = DT_INST_PROP(n, chain_length),					\
		.reset_delay = DT_INST_PROP(n, reset_delay),					\
	};											\
	DEVICE_DT_INST_DEFINE(n, ws2812_bflb_wo_init, NULL, NULL,				\
			      &ws2812_bflb_wo_##n##_cfg, POST_KERNEL,				\
			      CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_bflb_wo_api);		\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, color_mapping) >= 3					\
		&& DT_INST_PROP_LEN(n, color_mapping) <= 4,					\
	"Mapping is invalid, only RGB and RGBW are supported");					\
	WS2812_BFLB_WO_VALIDATE_COLORS(n, color_mapping);

DT_INST_FOREACH_STATUS_OKAY(WS2812_BFLB_WO_DEVICE)
