/*
 * Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT dac_leds

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/math_extras.h>

struct led_dac_leds {
	const struct device *dac;
	struct dac_channel_cfg chan_cfg;
	uint32_t dac_max_brightness;
	uint32_t dac_min_brightness;
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

	value = pct > 0 ? config->leds[led].dac_min_brightness +
				  (uint64_t)(config->leds[led].dac_max_brightness -
					     config->leds[led].dac_min_brightness) *
					  pct / LED_BRIGHTNESS_MAX
			: 0;

	return led_dac_set_raw(dev, led, value);
}

static DEVICE_API(led, led_dac_api) = {
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

#define LED_DAC_MAX_MV(n)  DT_PROP(n, voltage_max_dac_mv)
#define LED_DAC_MAX_VAL(n) (BIT(DT_PROP(n, resolution)) - 1)

#define LED_DAC_MAX_BRIGHTNESS(n)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(n, voltage_max_brightness_mv),                                \
	     (DT_PROP(n, voltage_max_brightness_mv) * LED_DAC_MAX_VAL(n) / LED_DAC_MAX_MV(n)),     \
	     (LED_DAC_MAX_VAL(n)))

#define LED_DAC_MIN_BRIGHTNESS(n)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(n, voltage_min_brightness_mv),                                \
	     (DT_PROP(n, voltage_min_brightness_mv) * LED_DAC_MAX_VAL(n) / LED_DAC_MAX_MV(n)),     \
	     (0))

#define LED_DAC_DT_GET(n)                                                                          \
	{                                                                                          \
		.dac = DEVICE_DT_GET(DT_PHANDLE(n, dac_dev)),                                      \
		.chan_cfg =                                                                        \
			{                                                                          \
				.channel_id = DT_PROP(n, channel),                                 \
				.resolution = DT_PROP(n, resolution),                              \
				.buffered = DT_PROP(n, output_buffer),                             \
				.internal = false,                                                 \
			},                                                                         \
		.dac_max_brightness = LED_DAC_MAX_BRIGHTNESS(n),                                   \
		.dac_min_brightness = LED_DAC_MIN_BRIGHTNESS(n),                                   \
	}

#define LED_DAC_DEFINE(n)                                                                          \
	BUILD_ASSERT((DT_NODE_HAS_PROP(n, voltage_max_brightness_mv) ||                            \
		      DT_NODE_HAS_PROP(n, voltage_min_brightness_mv)) ==                           \
			     DT_NODE_HAS_PROP(n, voltage_max_dac_mv),                              \
		     "'voltage-max-dac-mv' must be set when 'voltage-max-brightness-mv' or "       \
		     "'voltage-max-brightness-mv' is set");                                        \
                                                                                                   \
	static const struct led_dac_leds led_dac_##n[] = {                                         \
		DT_INST_FOREACH_CHILD_SEP(n, LED_DAC_DT_GET, (,))};                               \
                                                                                                   \
	static const struct led_dac_config led_config_##n = {                                      \
		.leds = led_dac_##n,                                                               \
		.num_leds = ARRAY_SIZE(led_dac_##n),                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &led_dac_init, NULL, NULL, &led_config_##n, POST_KERNEL,          \
			      CONFIG_LED_INIT_PRIORITY, &led_dac_api);

DT_INST_FOREACH_STATUS_OKAY(LED_DAC_DEFINE)
