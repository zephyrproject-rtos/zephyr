/*
 * Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/math_extras.h>

struct led_dac_leds {
	const struct device *dac;
	struct dac_channel_cfg chan_cfg;
	uint32_t dac_max;
	uint32_t dac_min;
};

struct led_dac_config {
	const struct led_dac_leds *leds;
	uint8_t num_leds;
};

static int led_dac_set_raw(const struct device *dev, uint32_t led, uint32_t value)
{
	const struct led_dac_config *config = dev->config;

	return dac_write_value(config->leds[led].dac, config->leds[led].chan_cfg.channel_id, value);
}

static int led_dac_set_brightness(const struct device *dev, uint32_t led, uint8_t pct)
{
	const struct led_dac_config *config = dev->config;
	uint32_t value;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	value = (uint64_t)(config->leds[led].dac_max - config->leds[led].dac_min) * pct / 100;
	value += config->leds[led].dac_min;

	return led_dac_set_raw(dev, led, value);
}

static inline int led_dac_on(const struct device *dev, uint32_t led)
{
	const struct led_dac_config *config = dev->config;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	return led_dac_set_raw(dev, led, config->leds[led].dac_max);
}

static inline int led_dac_off(const struct device *dev, uint32_t led)
{
	const struct led_dac_config *config = dev->config;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	return led_dac_set_raw(dev, led, 0);
}

static DEVICE_API(led, led_dac_api) = {
	.on = led_dac_on,
	.off = led_dac_off,
	.set_brightness = led_dac_set_brightness,
};

static int led_dac_init(const struct device *dev)
{
	const struct led_dac_config *config = dev->config;
	int ret;

	for (uint8_t i = 0; i < config->num_leds; ++i) {
		const struct led_dac_leds *led = &config->leds[i];

		if (!device_is_ready(led->dac)) {
			return -ENODEV;
		}

		ret = dac_channel_setup(led->dac, &led->chan_cfg);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#define LED_DAC_MAX(n) (BIT(DT_PROP(n, resolution)) - 1)

#define LED_DAC_DT_GET(n)                                                                          \
	{                                                                                          \
		.dac = DEVICE_DT_GET(DT_PHANDLE(n, dac)),                                          \
		.chan_cfg = {.channel_id = DT_PROP(n, channel),                                    \
			     .resolution = DT_PROP(n, resolution),                                 \
			     .buffered = false,                                                    \
			     .internal = false},                                                   \
		.dac_max = LED_DAC_MAX(n), .dac_min = LED_DAC_MIN(n)                               \
	}

#define LED_DAC_DEFINE(n, compat)                                                                  \
	static const struct led_dac_leds led_##compat##_##n[] = {                                  \
		DT_INST_FOREACH_CHILD_SEP(n, LED_DAC_DT_GET, (,))};                                \
                                                                                                   \
	static const struct led_dac_config led_##compat##_config_##n = {                           \
		.leds = led_##compat##_##n,                                                        \
		.num_leds = ARRAY_SIZE(led_##compat##_##n),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &led_dac_init, NULL, NULL, &led_##compat##_config_##n,            \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &led_dac_api);

#define DT_DRV_COMPAT dac_leds
#define LED_DAC_MIN(n)                                                                             \
	((uint64_t)LED_DAC_MAX(n) *                                                                \
	 ((uint64_t)DT_PROP(n, forward_voltage_mv) + DT_PROP(n, resistor_forward_voltage_mv)) /    \
	 DT_PROP(n, max_voltage_mv))
DT_INST_FOREACH_STATUS_OKAY_VARGS(LED_DAC_DEFINE, DT_DRV_COMPAT);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT dac_leds_vccs
#undef LED_DAC_MIN
#define LED_DAC_MIN(n) 0

DT_INST_FOREACH_STATUS_OKAY_VARGS(LED_DAC_DEFINE, DT_DRV_COMPAT);
