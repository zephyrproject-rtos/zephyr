/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/led_fake.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_leds

struct fake_led_config {
	uint8_t num_leds;
	const struct led_info *info;
};

DEFINE_FAKE_VALUE_FUNC(int, fake_led_on, const struct device *, uint32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_led_off, const struct device *, uint32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_led_set_brightness, const struct device *, uint32_t, uint8_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_led_blink, const struct device *, uint32_t, uint32_t, uint32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_led_get_info, const struct device *, uint32_t,
		       const struct led_info **);

DEFINE_FAKE_VALUE_FUNC(int, fake_led_set_color, const struct device *, uint32_t, uint8_t,
		       const uint8_t *);

DEFINE_FAKE_VALUE_FUNC(int, fake_led_write_channels, const struct device *, uint32_t, uint32_t,
		       const uint8_t *);

static int fake_led_get_info_delegate(const struct device *dev, uint32_t led,
				      const struct led_info **info)
{
	const struct fake_led_config *config = dev->config;

	if (info == NULL) {
		return -EINVAL;
	}

	if (led < config->num_leds) {
		*info = &config->info[led];
	} else {
		*info = NULL;
	}

	return 0;
}

static int fake_led_set_color_delegate(const struct device *dev, uint32_t led, uint8_t num_colors,
				       const uint8_t *color)
{
	const struct fake_led_config *config = dev->config;

	ARG_UNUSED(color);

	if (led >= config->num_leds) {
		return -ENODEV;
	}

	if (num_colors != config->info[led].num_colors) {
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_ZTEST
static void fake_led_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_led_on);
	RESET_FAKE(fake_led_off);
	RESET_FAKE(fake_led_set_brightness);
	RESET_FAKE(fake_led_blink);
	RESET_FAKE(fake_led_get_info);
	RESET_FAKE(fake_led_set_color);
	RESET_FAKE(fake_led_write_channels);

	/* Re-install default delegates */
	fake_led_get_info_fake.custom_fake = fake_led_get_info_delegate;
	fake_led_set_color_fake.custom_fake = fake_led_set_color_delegate;
}

ZTEST_RULE(fake_led_reset_rule, fake_led_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_led_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Install default delegates */
	fake_led_get_info_fake.custom_fake = fake_led_get_info_delegate;
	fake_led_set_color_fake.custom_fake = fake_led_set_color_delegate;

	return 0;
}

static DEVICE_API(led, fake_led_driver_api) = {
	.on = fake_led_on,
	.off = fake_led_off,
	.set_brightness = fake_led_set_brightness,
	.blink = fake_led_blink,
	.get_info = fake_led_get_info,
	.set_color = fake_led_set_color,
	.write_channels = fake_led_write_channels,
};

#define FAKE_LED_COLOR_MAPPING_DEFINE(led_node_id)                                                 \
	static const uint8_t fake_led_color_mapping_##led_node_id[] =                              \
		DT_PROP_OR(led_node_id, color_mapping, {});

#define FAKE_LED_INFO_INIT(led_node_id)                                                            \
	{                                                                                          \
		.label = DT_PROP_OR(led_node_id, label, NULL),                                     \
		.index = DT_PROP_OR(led_node_id, index, 0U),                                       \
		.num_colors = ARRAY_SIZE(fake_led_color_mapping_##led_node_id),                    \
		.color_mapping = fake_led_color_mapping_##led_node_id,                             \
	},

#define FAKE_LED_INIT(inst)                                                                        \
	DT_INST_FOREACH_CHILD(inst, FAKE_LED_COLOR_MAPPING_DEFINE)                                 \
                                                                                                   \
	static const struct led_info fake_led_info_##inst[] = {                                    \
		DT_INST_FOREACH_CHILD(inst, FAKE_LED_INFO_INIT)};                                  \
                                                                                                   \
	static const struct fake_led_config fake_led_config_##inst = {                             \
		.num_leds = ARRAY_SIZE(fake_led_info_##inst),                                      \
		.info = fake_led_info_##inst,                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fake_led_init, NULL, NULL, &fake_led_config_##inst,            \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &fake_led_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_LED_INIT)
