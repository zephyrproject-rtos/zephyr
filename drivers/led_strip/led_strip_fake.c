/*
 * Copyright (c) 2025 Gielen De Maesschalck <gielen.de.maesschalck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/led_strip/led_strip_fake.h>
#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_led_strip

struct fake_led_strip_config {
	size_t length; // Number of LEDs in the strip
};

DEFINE_FAKE_VALUE_FUNC(int, fake_led_strip_update_rgb, const struct device *, struct led_rgb *, size_t);

DEFINE_FAKE_VALUE_FUNC(size_t, fake_led_strip_length, const struct device *);

size_t fake_led_strip_length_delegate(const struct device *dev)
{
	const struct fake_led_strip_config *config = dev->config;

	return config->length;
}

#ifdef CONFIG_ZTEST
static void fake_led_strip_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_led_strip_update_rgb);
	RESET_FAKE(fake_led_strip_length);

	fake_led_strip_length_fake.custom_fake = fake_led_strip_length_delegate;
}
ZTEST_RULE(fake_led_strip_reset_rule, fake_led_strip_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static DEVICE_API(led_strip, fake_led_strip_api) = {
	.update_rgb = fake_led_strip_update_rgb,
	.length = fake_led_strip_length,
};

static int fake_led_strip_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	fake_led_strip_length_fake.custom_fake = fake_led_strip_length_delegate;

	return 0;
}

#define FAKE_LED_STRIP_INIT(inst)                                                                  \
	static const struct fake_led_strip_config fake_led_strip_config_##inst = {                 \
		.length = DT_INST_PROP(inst, chain_length),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &fake_led_strip_init, NULL, NULL,                              \
			      &fake_led_strip_config_##inst, POST_KERNEL,                           \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &fake_led_strip_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_LED_STRIP_INIT)