/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pwm_ir_transmitter

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/sys/util.h>

/* A carrier period of at least this many PWM cycles gives a 1 % duty step */
#define RC_PWM_IR_MIN_PERIOD_CYCLES 100U
#define RC_PWM_IR_TX_SLACK_MS       100U

struct rc_pwm_ir_config {
	struct pwm_dt_spec pwm;
	const struct device *counter;
	uint32_t carrier_hz;
	uint8_t duty_pct;
	uint8_t channel;
};

struct rc_pwm_ir_data {
	struct rc_dev_data rc;
	struct k_spinlock lock;
	struct k_sem done;
	uint64_t cycles_per_sec;
	uint32_t period;
	uint32_t pulse;
	uint32_t guard;
	const uint32_t *words;
	size_t count;
	size_t index;
	uint64_t wrap;
	uint32_t start;
	uint32_t target;
	uint64_t elapsed_us;
	bool active;
	int result;
};

static uint32_t rc_pwm_ir_diff(uint32_t later, uint32_t earlier, uint64_t wrap)
{
	return (uint32_t)(((uint64_t)later + wrap - earlier) % wrap);
}

static int rc_pwm_ir_carrier(const struct device *dev, bool on)
{
	const struct rc_pwm_ir_config *config = dev->config;
	struct rc_pwm_ir_data *data = dev->data;

	return pwm_set_cycles(config->pwm.dev, config->pwm.channel, data->period,
			      on ? data->pulse : 0U, config->pwm.flags);
}

static void rc_pwm_ir_finish(const struct device *dev, int result)
{
	struct rc_pwm_ir_data *data = dev->data;

	data->active = false;
	data->result = result;
	(void)rc_pwm_ir_carrier(dev, false);
	k_sem_give(&data->done);
}

static void rc_pwm_ir_alarm(const struct device *counter, uint8_t chan, uint32_t ticks,
			    void *user_data);

/* Arm the alarm at the end of the current word, absolute from the frame start */
static int rc_pwm_ir_arm(const struct device *dev)
{
	const struct rc_pwm_ir_config *config = dev->config;
	struct rc_pwm_ir_data *data = dev->data;
	struct counter_alarm_cfg alarm = {
		.callback = rc_pwm_ir_alarm,
		.user_data = (void *)dev,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
	};
	uint32_t prev = data->target;
	uint32_t now;
	int ret;

	data->elapsed_us += rc_raw_duration_us(data->words[data->index]);
	data->target = (uint32_t)(((uint64_t)data->start +
				   counter_us_to_ticks(config->counter, data->elapsed_us)) %
				  data->wrap);

	ret = counter_get_value(config->counter, &now);
	if (ret != 0) {
		return ret;
	}

	if ((uint64_t)rc_pwm_ir_diff(now, prev, data->wrap) + data->guard >
	    rc_pwm_ir_diff(data->target, prev, data->wrap)) {
		return -ETIME;
	}

	alarm.ticks = data->target;

	return counter_set_channel_alarm(config->counter, config->channel, &alarm);
}

static void rc_pwm_ir_alarm(const struct device *counter, uint8_t chan, uint32_t ticks,
			    void *user_data)
{
	const struct device *dev = user_data;
	struct rc_pwm_ir_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	ARG_UNUSED(counter);
	ARG_UNUSED(chan);
	ARG_UNUSED(ticks);

	if (data->active) {
		data->index++;
		if (data->index == data->count) {
			rc_pwm_ir_finish(dev, 0);
		} else {
			ret = rc_pwm_ir_carrier(dev, rc_raw_is_pulse(data->words[data->index]));
			if (ret == 0) {
				ret = rc_pwm_ir_arm(dev);
			}
			if (ret != 0) {
				rc_pwm_ir_finish(dev, ret);
			}
		}
	}

	k_spin_unlock(&data->lock, key);
}

static int rc_pwm_ir_get_caps(const struct device *dev, struct rc_caps *caps)
{
	struct rc_pwm_ir_data *data = dev->data;

	*caps = (struct rc_caps){
		.roles = RC_ROLE_RAW_TX,
		.carrier_min_hz = (uint32_t)MAX(1U, DIV_ROUND_UP(data->cycles_per_sec, UINT32_MAX)),
		.carrier_max_hz = (uint32_t)MIN(data->cycles_per_sec / RC_PWM_IR_MIN_PERIOD_CYCLES,
						UINT32_MAX),
	};

	return 0;
}

static int rc_pwm_ir_set_carrier(const struct device *dev, uint32_t hz, uint8_t duty_pct)
{
	const struct rc_pwm_ir_config *config = dev->config;
	struct rc_pwm_ir_data *data = dev->data;
	uint64_t period;
	int ret;

	if ((hz == 0U) || (duty_pct == 0U) || (duty_pct > 99U)) {
		return -EINVAL;
	}

	period = DIV_ROUND_CLOSEST(data->cycles_per_sec, hz);
	if ((period < RC_PWM_IR_MIN_PERIOD_CYCLES) || (period > UINT32_MAX)) {
		return -EINVAL;
	}

	/* A period change may wait on the PWM, so it is applied here with the carrier off */
	ret = pwm_set_cycles(config->pwm.dev, config->pwm.channel, (uint32_t)period, 0U,
			     config->pwm.flags);
	if (ret != 0) {
		return ret;
	}

	data->period = (uint32_t)period;
	data->pulse = (uint32_t)DIV_ROUND_CLOSEST(period * duty_pct, 100U);

	return 0;
}

static int rc_pwm_ir_tx(const struct device *dev, const uint32_t *words, size_t count)
{
	const struct rc_pwm_ir_config *config = dev->config;
	struct rc_pwm_ir_data *data = dev->data;
	uint32_t total_us = 0U;
	uint32_t max_us = 0U;
	k_spinlock_key_t key;
	int ret;

	for (size_t i = 0; i < count; i++) {
		uint32_t us = rc_raw_duration_us(words[i]);

		total_us += us;
		max_us = MAX(max_us, us);
	}

	data->wrap = (uint64_t)counter_get_top_value(config->counter) + 1U;
	if ((uint64_t)counter_us_to_ticks(config->counter, max_us) + data->guard >= data->wrap) {
		return -EINVAL;
	}

	k_sem_reset(&data->done);

	key = k_spin_lock(&data->lock);
	data->words = words;
	data->count = count;
	data->index = 0U;
	data->elapsed_us = 0U;
	data->result = 0;
	data->active = true;

	ret = rc_pwm_ir_carrier(dev, true);
	if (ret == 0) {
		ret = counter_get_value(config->counter, &data->start);
	}
	if (ret == 0) {
		data->target = data->start;
		ret = rc_pwm_ir_arm(dev);
	}
	if (ret != 0) {
		rc_pwm_ir_finish(dev, ret);
	}
	k_spin_unlock(&data->lock, key);

	ret = k_sem_take(&data->done, K_MSEC((total_us / USEC_PER_MSEC) + RC_PWM_IR_TX_SLACK_MS));
	if (ret != 0) {
		key = k_spin_lock(&data->lock);
		if (data->active) {
			(void)counter_cancel_channel_alarm(config->counter, config->channel);
			rc_pwm_ir_finish(dev, -ETIMEDOUT);
		}
		k_spin_unlock(&data->lock, key);
	}

	return data->result;
}

static int rc_pwm_ir_init(const struct device *dev)
{
	const struct rc_pwm_ir_config *config = dev->config;
	struct rc_pwm_ir_data *data = dev->data;
	int ret;

	if (!pwm_is_ready_dt(&config->pwm) || !device_is_ready(config->counter)) {
		return -ENODEV;
	}

	if (config->channel >= counter_get_num_of_channels(config->counter)) {
		return -EINVAL;
	}

	ret = pwm_get_cycles_per_sec(config->pwm.dev, config->pwm.channel, &data->cycles_per_sec);
	if (ret != 0) {
		return ret;
	}

	if ((data->cycles_per_sec / RC_PWM_IR_MIN_PERIOD_CYCLES) == 0U) {
		return -ENOTSUP;
	}

	k_sem_init(&data->done, 0, 1);
	data->guard = counter_us_to_ticks(config->counter, CONFIG_RC_PWM_IR_TRANSMITTER_GUARD_US);
	data->guard = MAX(1U, data->guard);

	ret = counter_start(config->counter);
	if ((ret != 0) && (ret != -EALREADY)) {
		return ret;
	}

	return rc_pwm_ir_set_carrier(dev, config->carrier_hz, config->duty_pct);
}

static DEVICE_API(rc, rc_pwm_ir_api) = {
	.get_caps = rc_pwm_ir_get_caps,
	.set_carrier = rc_pwm_ir_set_carrier,
	.tx = rc_pwm_ir_tx,
};

#define RC_PWM_IR_DEFINE(inst)                                                                     \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, pwms) == 1, "pwm-ir-transmitter takes one PWM");       \
	static const struct rc_pwm_ir_config rc_pwm_ir_config_##inst = {                           \
		.pwm = PWM_DT_SPEC_INST_GET(inst),                                                 \
		.counter = DEVICE_DT_GET(DT_INST_PHANDLE(inst, counter)),                          \
		.carrier_hz = DT_INST_PROP(inst, carrier_hz),                                      \
		.duty_pct = DT_INST_PROP(inst, duty_cycle_percent),                                \
		.channel = DT_INST_PROP(inst, counter_channel),                                    \
	};                                                                                         \
	static struct rc_pwm_ir_data rc_pwm_ir_data_##inst;                                        \
	RC_DEVICE_DT_INST_DEFINE(inst, rc_pwm_ir_init, NULL, &rc_pwm_ir_data_##inst,               \
				 &rc_pwm_ir_config_##inst, POST_KERNEL, CONFIG_RC_INIT_PRIORITY,   \
				 &rc_pwm_ir_api);

DT_INST_FOREACH_STATUS_OKAY(RC_PWM_IR_DEFINE)
