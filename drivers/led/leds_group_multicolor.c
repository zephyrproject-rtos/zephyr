/*
 * Copyright (c) 2025 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT leds_group_multicolor

/**
 * @file
 * @brief Driver for multi-color LED built from monochromatic LEDs.
 */

#include <zephyr/drivers/led.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(leds_group_multicolor, CONFIG_LED_LOG_LEVEL);

struct leds_group_multicolor_config {
	uint8_t num_leds;
	const struct led_dt_spec *led;
};

static int leds_group_multicolor_set_color(const struct device *dev, uint32_t led,
					   uint8_t num_colors, const uint8_t *color)
{
	const struct leds_group_multicolor_config *config = dev->config;

	if (led != 0) {
		return -EINVAL;
	}
	if (num_colors != config->num_leds) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < num_colors; i++) {
		int err;

		err = led_set_brightness_dt(&config->led[i], color[i]);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int leds_group_multicolor_init(const struct device *dev)
{
	const struct leds_group_multicolor_config *config = dev->config;

	for (uint8_t i = 0; i < config->num_leds; i++) {
		const struct led_dt_spec *led = &config->led[i];

		if (!led_is_ready_dt(led)) {
			LOG_ERR("%s: LED device %s is not ready", dev->name, led->dev->name);
			return -ENODEV;
		}
	}

	return 0;
}

static DEVICE_API(led, leds_group_multicolor_api) = {
	.set_color = leds_group_multicolor_set_color,
};

#define LED_DT_SPEC_GET_BY_PHANDLE_IDX(node_id, prop, idx)			\
	LED_DT_SPEC_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define LEDS_GROUP_MULTICOLOR_DEVICE(inst)					\
										\
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, leds) > 0,				\
			"at least one LED phandle must be present");		\
										\
	static const struct led_dt_spec led_group_multicolor_##inst[] = {	\
		DT_INST_FOREACH_PROP_ELEM_SEP(					\
			inst, leds, LED_DT_SPEC_GET_BY_PHANDLE_IDX, (,))	\
	};									\
										\
	static const struct leds_group_multicolor_config			\
				leds_group_multicolor_config_##inst = {		\
		.num_leds	= ARRAY_SIZE(led_group_multicolor_##inst),	\
		.led		= led_group_multicolor_##inst,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &leds_group_multicolor_init, NULL,		\
			      NULL, &leds_group_multicolor_config_##inst,	\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,		\
			      &leds_group_multicolor_api);

DT_INST_FOREACH_STATUS_OKAY(LEDS_GROUP_MULTICOLOR_DEVICE)
