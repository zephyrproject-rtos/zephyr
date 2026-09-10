/*
 * Copyright (c) 2023 Andriy Gelman <andriy.gelman@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pwm_clock

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_pwm);

BUILD_ASSERT(CONFIG_CLOCK_CONTROL_PWM_INIT_PRIORITY > CONFIG_PWM_INIT_PRIORITY,
	     "PWM must have a higher priority than PWM clock control");

#define NUM_PWM_CLOCKS 1

struct clock_control_pwm_config {
	const struct pwm_dt_spec pwm_dt;
	const uint16_t pwm_on_delay;
};

struct clock_control_pwm_data {
	uint32_t clock_frequency;
	bool is_enabled;
};

static int clock_control_pwm_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_pwm_data *data = dev->data;
	const struct clock_control_pwm_config *config = dev->config;
	const struct pwm_dt_spec *spec;
	int id = (int)sys;
	int ret;

	if (id >= NUM_PWM_CLOCKS) {
		return -EINVAL;
	}

	spec = &config->pwm_dt;
	if (data->clock_frequency == 0) {
		ret = pwm_set_dt(spec, spec->period, spec->period / 2);
	} else {
		uint64_t cycles_per_sec;
		uint32_t period_cycles;

		ret = pwm_get_cycles_per_sec(spec->dev, spec->channel, &cycles_per_sec);
		if (ret) {
			return ret;
		}

		if (cycles_per_sec % data->clock_frequency > 0) {
			LOG_WRN("Target clock frequency cannot be expressed in PWM clock ticks");
		}

		period_cycles = cycles_per_sec / data->clock_frequency;
		ret = pwm_set_cycles(spec->dev, spec->channel, period_cycles, period_cycles / 2,
				     spec->flags);
	}

	if (ret) {
		return ret;
	}

	k_busy_wait(config->pwm_on_delay);

	data->is_enabled = true;

	return 0;
}

static int clock_control_pwm_get_rate(const struct device *dev, clock_control_subsys_t sys,
				      uint32_t *rate)
{
	struct clock_control_pwm_data *data = dev->data;
	const struct clock_control_pwm_config *config = dev->config;
	int id = (int)sys;

	if (id >= NUM_PWM_CLOCKS) {
		return -EINVAL;
	}

	if (data->clock_frequency > 0) {
		*rate = data->clock_frequency;
	} else {
		*rate = NSEC_PER_SEC / config->pwm_dt.period;
	}

	return 0;
}

static int clock_control_pwm_set_rate(const struct device *dev, clock_control_subsys_rate_t sys,
				      clock_control_subsys_rate_t rate)
{
	struct clock_control_pwm_data *data = dev->data;
	uint32_t rate_hz = (uint32_t)rate;
	int id = (int)sys;

	if (id >= NUM_PWM_CLOCKS) {
		return -EINVAL;
	}

	if (data->clock_frequency == rate_hz && data->is_enabled) {
		return -EALREADY;
	}

	data->clock_frequency = rate_hz;

	return clock_control_pwm_on(dev, sys);
}

static int clock_control_pwm_init(const struct device *dev)
{
	const struct clock_control_pwm_config *config = dev->config;

	if (!device_is_ready(config->pwm_dt.dev)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(clock_control, clock_control_pwm_api) = {
	.on = clock_control_pwm_on,
	.get_rate = clock_control_pwm_get_rate,
	.set_rate = clock_control_pwm_set_rate,
};

#define PWM_CLOCK_INIT(i)                                                                          \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP_LEN(i, pwms) <= 1,                                               \
		     "One PWM per clock control node is supported");                               \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(i, pwm_on_delay) <= UINT16_MAX,                                  \
		     "Maximum pwm-on-delay is 65535 usec");                                        \
                                                                                                   \
	static const struct clock_control_pwm_config clock_control_pwm_config_##i = {              \
		.pwm_dt = PWM_DT_SPEC_INST_GET(i),                                                 \
		.pwm_on_delay = DT_INST_PROP(i, pwm_on_delay),                                     \
	};                                                                                         \
                                                                                                   \
	static struct clock_control_pwm_data clock_control_pwm_data_##i = {                        \
		.clock_frequency = DT_INST_PROP_OR(i, clock_frequency, 0),                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, clock_control_pwm_init, NULL, &clock_control_pwm_data_##i,        \
			      &clock_control_pwm_config_##i, POST_KERNEL,                          \
			      CONFIG_CLOCK_CONTROL_PWM_INIT_PRIORITY, &clock_control_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_CLOCK_INIT)
