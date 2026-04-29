/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_bl61x_wo

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/dt-bindings/led/led.h>

#include <zephyr/drivers/gpio/gpio_bl61x_wo.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_bl61x_wo);

/* Map the RGB(W) sequence to corresponding bit offsets */
#define WS2812_BL61X_WO_MAP_COLORS_IMPL_OTHERS(_n, _p, _i) (DT_PROP_BY_IDX(_n, _p, _i) * 8),
#define WS2812_BL61X_WO_MAP_COLORS_IMPL_RGB(_n, _p, _i) ((DT_PROP_BY_IDX(_n, _p, _i) - 1) * 8),
#define WS2812_BL61X_WO_MAP_COLORS_IMPL(_n, _p, _i)						\
	COND_CODE_1(IS_EQ(DT_PROP_LEN(_n, _p), 3),						\
		(WS2812_BL61X_WO_MAP_COLORS_IMPL_RGB(_n, _p, _i)),				\
		(WS2812_BL61X_WO_MAP_COLORS_IMPL_OTHERS(_n, _p, _i)))
#define WS2812_BL61X_WO_MAP_COLORS(_inst, _prop)						\
	(const size_t []){									\
		DT_INST_FOREACH_PROP_ELEM(_inst, _prop, WS2812_BL61X_WO_MAP_COLORS_IMPL) }

#define WS2812_BL61X_WO_VALIDATE_COLORS_IMPL_RGBW(_n, _p, _i)					\
	BUILD_ASSERT(DT_PROP_BY_IDX(_n, _p, _i) <= LED_COLOR_ID_BLUE				\
		     && DT_PROP_BY_IDX(_n, _p, _i) >= LED_COLOR_ID_WHITE,			\
		     "Mapping is invalid for RGBW");
#define WS2812_BL61X_WO_VALIDATE_COLORS_IMPL_RGB(_n, _p, _i)					\
	BUILD_ASSERT(DT_PROP_BY_IDX(_n, _p, _i) <= LED_COLOR_ID_BLUE				\
		     && DT_PROP_BY_IDX(_n, _p, _i) >= LED_COLOR_ID_RED,				\
		     "Mapping is invalid for RGB");
#define WS2812_BL61X_WO_VALIDATE_COLORS_IMPL(_n, _p, _i)					\
	COND_CODE_1(IS_EQ(DT_PROP_LEN(_n, _p), 3),						\
		(WS2812_BL61X_WO_VALIDATE_COLORS_IMPL_RGB(_n, _p, _i)),				\
		(WS2812_BL61X_WO_VALIDATE_COLORS_IMPL_RGBW(_n, _p, _i)))
#define WS2812_BL61X_WO_VALIDATE_COLORS(_inst, _prop)						\
	DT_INST_FOREACH_PROP_ELEM(_inst, _prop, WS2812_BL61X_WO_VALIDATE_COLORS_IMPL)

#define WS2812_BL61X_WO_MAX_FRAME_BITS	32

struct ws2812_bl61x_wo_config {
	const struct gpio_dt_spec gpio_pin;
	size_t length;
	uint32_t t0h;
	uint32_t t1h;
	uint32_t ttotal;
	const size_t *color_map_offsets_rgb;
	const uint8_t *color_mapping;
	uint8_t num_colors;
	uint16_t reset_delay;
};

struct ws2812_bl61x_wo_data {
	k_timepoint_t reset_timeout;
	struct k_mutex lock;
#ifdef CONFIG_WS2812_STRIP_BL61X_WO_ASYNC
	uint16_t *async_buf;
#endif
};

static size_t ws2812_bl61x_wo_length(const struct device *dev)
{
	const struct ws2812_bl61x_wo_config *config = dev->config;

	return config->length;
}

static int ws2812_bl61x_wo_update_rgb(const struct device *dev, struct led_rgb *pixels,
				     size_t num_pixels)
{
	const struct ws2812_bl61x_wo_config *cfg = dev->config;
	struct ws2812_bl61x_wo_data *data = dev->data;
	int ret;
	uint16_t buf[WS2812_BL61X_WO_MAX_FRAME_BITS];
	uint16_t pin_msk = BIT(cfg->gpio_pin.pin % BL61X_WO_PIN_CNT);
	struct bl61x_wo_config wo_cfg = {
		.total_cycles = bl61x_wo_time_to_cycles(cfg->ttotal, false),
		.set_cycles = bl61x_wo_time_to_cycles(cfg->t1h, false),
		.unset_cycles = bl61x_wo_time_to_cycles(cfg->t0h, false),
		.set_invert = false,
		.unset_invert = false,
		.park_high = false,
	};

#ifdef CONFIG_WS2812_STRIP_BL61X_WO_ASYNC
	if (num_pixels > cfg->length) {
		LOG_ERR("Update length is greater than configured length");
		return -EINVAL;
	}
#endif

	k_sleep(sys_timepoint_timeout(data->reset_timeout));

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = bl61x_wo_configure_dt(&wo_cfg, &cfg->gpio_pin, 1);
	if (ret != 0) {
		LOG_ERR("Could not configure Wire Out: %d", ret);
		goto exit_here;
	}

	for (size_t i = 0; i < num_pixels; i++) {
		for (size_t j = 0; j < 8; j++) {
			buf[j + cfg->color_map_offsets_rgb[0]] =
				((pixels[i].r >> (7 - j)) & 0x1) ? pin_msk : 0;
			buf[j + cfg->color_map_offsets_rgb[1]] =
				((pixels[i].g >> (7 - j)) & 0x1) ? pin_msk : 0;
			buf[j + cfg->color_map_offsets_rgb[2]] =
				((pixels[i].b >> (7 - j)) & 0x1) ? pin_msk : 0;
			if (cfg->num_colors > 3) {
				buf[j + cfg->color_map_offsets_rgb[3]] = 0;
			}
		}
#ifdef CONFIG_WS2812_STRIP_BL61X_WO_ASYNC
		memcpy(&data->async_buf[cfg->num_colors * 8 * i], buf,
		       sizeof(uint16_t) * cfg->num_colors * 8);
#else
		ret = bl61x_wo_write(buf, cfg->num_colors * 8);
		if (ret != 0) {
			LOG_ERR("Could not write to FIFO: %d", ret);
			goto exit_here;
		}
#endif
	}

#ifdef CONFIG_WS2812_STRIP_BL61X_WO_ASYNC
	data->reset_timeout = sys_timepoint_calc(K_NSEC(cfg->reset_delay * 1000 +
						 cfg->ttotal * cfg->num_colors * 8 * num_pixels));
	ret = bl61x_wo_write_async(data->async_buf, cfg->num_colors * 8 * num_pixels, NULL, NULL);
#else
	data->reset_timeout = sys_timepoint_calc(K_USEC(cfg->reset_delay));
#endif

exit_here:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int ws2812_bl61x_wo_init(const struct device *dev)
{
	const struct ws2812_bl61x_wo_config *config = dev->config;
	struct ws2812_bl61x_wo_data *data = dev->data;

	if (!gpio_is_ready_dt(&config->gpio_pin)) {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	data->reset_timeout = sys_timepoint_calc(K_USEC(config->reset_delay));

	return 0;
}

static DEVICE_API(led_strip, ws2812_bl61x_wo_api) = {
	.update_rgb = ws2812_bl61x_wo_update_rgb,
	.length = ws2812_bl61x_wo_length,
};

#ifdef CONFIG_WS2812_STRIP_BL61X_WO_ASYNC
#define WS2812_BL61X_WO_DEVICE_BUFFER(_s) .async_buf = (uint16_t [_s]) {},
#else
#define WS2812_BL61X_WO_DEVICE_BUFFER(_s)
#endif

#define WS2812_BL61X_WO_DEVICE(n)								\
	BL61X_WO_VALIDATE_INST(n, worldsemi_ws2812_bl61x_wo);					\
	static const struct ws2812_bl61x_wo_config ws2812_bl61x_wo_##n##_cfg = {		\
		.gpio_pin = GPIO_DT_SPEC_INST_GET(n, gpios),					\
		.t0h = DT_INST_PROP(n, time_t0h),						\
		.t1h = DT_INST_PROP(n, time_t1h),						\
		.ttotal = DT_INST_PROP_OR(n, time_total, DT_INST_PROP(n, Z_HZ_ns / frequency)),	\
		.color_map_offsets_rgb = WS2812_BL61X_WO_MAP_COLORS(n, color_mapping),		\
		.color_mapping = (const uint8_t []) DT_INST_PROP(n, color_mapping),		\
		.num_colors = DT_INST_PROP_LEN(n, color_mapping),				\
		.length = DT_INST_PROP(n, chain_length),					\
		.reset_delay = DT_INST_PROP(n, reset_delay),					\
	};											\
	static struct ws2812_bl61x_wo_data ws2812_bl61x_wo_##n##_data = {			\
		WS2812_BL61X_WO_DEVICE_BUFFER(DT_INST_PROP(n, chain_length)			\
			* DT_INST_PROP_LEN(n, color_mapping) * 8)				\
	};											\
	DEVICE_DT_INST_DEFINE(n, ws2812_bl61x_wo_init, NULL, &ws2812_bl61x_wo_##n##_data,	\
			      &ws2812_bl61x_wo_##n##_cfg, POST_KERNEL,				\
			      CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_bl61x_wo_api);		\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, color_mapping) >= 3					\
		&& DT_INST_PROP_LEN(n, color_mapping) <= 4,					\
	"Mapping is invalid, only RGB and RGBW are supported");					\
	WS2812_BL61X_WO_VALIDATE_COLORS(n, color_mapping);

DT_INST_FOREACH_STATUS_OKAY(WS2812_BL61X_WO_DEVICE)
