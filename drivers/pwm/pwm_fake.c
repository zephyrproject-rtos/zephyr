/*
 * Copyright (c) 2024 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm/pwm_fake.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_pwm

/** Fake PWM config structure */
struct fake_pwm_config {
	/** Frequency of the (fake) underlying timer */
	uint64_t frequency_hz;
};

DEFINE_FAKE_VALUE_FUNC(int, fake_pwm_set_cycles, const struct device *, uint32_t, uint32_t,
		       uint32_t, pwm_flags_t);

#ifdef CONFIG_ZTEST
static void fake_pwm_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_pwm_set_cycles);
}

ZTEST_RULE(fake_pwm_reset_rule, fake_pwm_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(channel);
	const struct fake_pwm_config *config = dev->config;

	*cycles = config->frequency_hz;

	return 0;
}

static DEVICE_API(pwm, fake_pwm_driver_api) = {
	.set_cycles = fake_pwm_set_cycles,
	.get_cycles_per_sec = fake_pwm_get_cycles_per_sec,
};

#define FAKE_PWM_INIT(inst)                                                                        \
	static const struct fake_pwm_config fake_pwm_config_##inst = {                             \
		.frequency_hz = DT_INST_PROP(inst, frequency),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &fake_pwm_config_##inst, POST_KERNEL,        \
			      CONFIG_PWM_INIT_PRIORITY, &fake_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_PWM_INIT)
