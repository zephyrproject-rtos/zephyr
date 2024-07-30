/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT analog_axis

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_analog_axis.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(analog_axis, CONFIG_INPUT_LOG_LEVEL);

struct analog_axis_channel_config {
	struct adc_dt_spec adc;
	int16_t out_min;
	int16_t out_max;
	uint16_t axis;
	bool invert_input;
	bool invert_output;
};

struct analog_axis_channel_data {
	int last_out;
};

struct analog_axis_config {
	uint32_t poll_period_ms;
	const struct analog_axis_channel_config *channel_cfg;
	struct analog_axis_channel_data *channel_data;
	struct analog_axis_calibration *calibration;
	const uint8_t num_channels;
};

struct analog_axis_data {
	struct k_sem cal_lock;
	analog_axis_raw_data_t raw_data_cb;
	struct k_timer timer;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_INPUT_ANALOG_AXIS_THREAD_STACK_SIZE);

#ifdef CONFIG_PM_DEVICE
	atomic_t suspended;
	struct k_sem wakeup;
#endif
};

int analog_axis_num_axes(const struct device *dev)
{
	const struct analog_axis_config *cfg = dev->config;

	return cfg->num_channels;
}

int analog_axis_calibration_get(const struct device *dev,
				int channel,
				struct analog_axis_calibration *out_cal)
{
	const struct analog_axis_config *cfg = dev->config;
	struct analog_axis_data *data = dev->data;
	struct analog_axis_calibration *cal = &cfg->calibration[channel];

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	k_sem_take(&data->cal_lock, K_FOREVER);
	memcpy(out_cal, cal, sizeof(struct analog_axis_calibration));
	k_sem_give(&data->cal_lock);

	return 0;
}

void analog_axis_set_raw_data_cb(const struct device *dev, analog_axis_raw_data_t cb)
{
	struct analog_axis_data *data = dev->data;

	k_sem_take(&data->cal_lock, K_FOREVER);
	data->raw_data_cb = cb;
	k_sem_give(&data->cal_lock);
}

int analog_axis_calibration_set(const struct device *dev,
				int channel,
				struct analog_axis_calibration *new_cal)
{
	const struct analog_axis_config *cfg = dev->config;
	struct analog_axis_data *data = dev->data;
	struct analog_axis_calibration *cal = &cfg->calibration[channel];

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	k_sem_take(&data->cal_lock, K_FOREVER);
	memcpy(cal, new_cal, sizeof(struct analog_axis_calibration));
	k_sem_give(&data->cal_lock);

	return 0;
}

static int32_t analog_axis_out_deadzone(const struct device *dev,
					int channel,
					int32_t raw_val)
{
	const struct analog_axis_config *cfg = dev->config;
	const struct analog_axis_channel_config *axis_cfg = &cfg->channel_cfg[channel];
	struct analog_axis_calibration *cal = &cfg->calibration[channel];

	int16_t in_range = cal->in_max - cal->in_min;
	int16_t out_range = axis_cfg->out_max - axis_cfg->out_min;
	int16_t in_mid = DIV_ROUND_CLOSEST(cal->in_min + cal->in_max, 2);
	int16_t in_min = cal->in_min;

	if (abs(raw_val - in_mid) < cal->in_deadzone) {
		return DIV_ROUND_CLOSEST(axis_cfg->out_max + axis_cfg->out_min, 2);
	}

	in_range -= cal->in_deadzone * 2;
	in_min += cal->in_deadzone;
	if (raw_val < in_mid) {
		raw_val += cal->in_deadzone;
	} else {
		raw_val -= cal->in_deadzone;
	}

	return DIV_ROUND_CLOSEST((raw_val - in_min) * out_range, in_range) + axis_cfg->out_min;
}

static int32_t analog_axis_out_linear(const struct device *dev,
				      int channel,
				      int32_t raw_val)
{
	const struct analog_axis_config *cfg = dev->config;
	const struct analog_axis_channel_config *axis_cfg = &cfg->channel_cfg[channel];
	struct analog_axis_calibration *cal = &cfg->calibration[channel];

	int16_t in_range = cal->in_max - cal->in_min;
	int16_t out_range = axis_cfg->out_max - axis_cfg->out_min;

	return DIV_ROUND_CLOSEST((raw_val - cal->in_min) * out_range, in_range) + axis_cfg->out_min;
}

static void analog_axis_loop(const struct device *dev)
{
	const struct analog_axis_config *cfg = dev->config;
	struct analog_axis_data *data = dev->data;
	int16_t bufs[cfg->num_channels];
	int32_t out;
	struct adc_sequence sequence = {
		.buffer = bufs,
		.buffer_size = sizeof(bufs),
	};
	const struct analog_axis_channel_config *axis_cfg_0 = &cfg->channel_cfg[0];
	int err;
	int i;

	adc_sequence_init_dt(&axis_cfg_0->adc, &sequence);

	for (i = 0; i < cfg->num_channels; i++) {
		const struct analog_axis_channel_config *axis_cfg = &cfg->channel_cfg[i];

		sequence.channels |= BIT(axis_cfg->adc.channel_id);
	}

	err = adc_read(axis_cfg_0->adc.dev, &sequence);
	if (err < 0) {
		LOG_ERR("Could not read (%d)", err);
		return;
	}

	k_sem_take(&data->cal_lock, K_FOREVER);

	for (i = 0; i < cfg->num_channels; i++) {
		const struct analog_axis_channel_config *axis_cfg = &cfg->channel_cfg[i];
		struct analog_axis_channel_data *axis_data = &cfg->channel_data[i];
		struct analog_axis_calibration *cal = &cfg->calibration[i];
		int32_t raw_val = bufs[i];

		if (axis_cfg->invert_input) {
			raw_val *= -1;
		}

		if (data->raw_data_cb != NULL) {
			data->raw_data_cb(dev, i, raw_val);
		}

		LOG_DBG("%s: ch %d: raw_val: %d", dev->name, i, raw_val);

		if (cal->in_deadzone > 0) {
			out = analog_axis_out_deadzone(dev, i, raw_val);
		} else {
			out = analog_axis_out_linear(dev, i, raw_val);
		}

		out = CLAMP(out, axis_cfg->out_min, axis_cfg->out_max);

		if (axis_cfg->invert_output) {
			out = axis_cfg->out_max - out;
		}

		if (axis_data->last_out != out) {
			input_report_abs(dev, axis_cfg->axis, out, true, K_FOREVER);
		}
		axis_data->last_out = out;
	}

	k_sem_give(&data->cal_lock);
}

static void analog_axis_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	const struct analog_axis_config *cfg = dev->config;
	struct analog_axis_data *data = dev->data;
	int err;
	int i;

	for (i = 0; i < cfg->num_channels; i++) {
		const struct analog_axis_channel_config *axis_cfg = &cfg->channel_cfg[i];

		if (!adc_is_ready_dt(&axis_cfg->adc)) {
			LOG_ERR("ADC controller device not ready");
			return;
		}

		err = adc_channel_setup_dt(&axis_cfg->adc);
		if (err < 0) {
			LOG_ERR("Could not setup channel #%d (%d)", i, err);
			return;
		}
	}

	while (true) {
#ifdef CONFIG_PM_DEVICE
		if (atomic_get(&data->suspended) == 1) {
			k_sem_take(&data->wakeup, K_FOREVER);
		}
#endif

		analog_axis_loop(dev);
		k_timer_status_sync(&data->timer);
	}
}

static int analog_axis_init(const struct device *dev)
{
	struct analog_axis_data *data = dev->data;
	k_tid_t tid;

	k_sem_init(&data->cal_lock, 1, 1);
	k_timer_init(&data->timer, NULL, NULL);

#ifdef CONFIG_PM_DEVICE
	k_sem_init(&data->wakeup, 0, 1);
#endif

	tid = k_thread_create(&data->thread, data->thread_stack,
			      K_KERNEL_STACK_SIZEOF(data->thread_stack),
			      analog_axis_thread, (void *)dev, NULL, NULL,
			      CONFIG_INPUT_ANALOG_AXIS_THREAD_PRIORITY,
			      0, K_NO_WAIT);
	if (!tid) {
		LOG_ERR("thread creation failed");
		return -ENODEV;
	}

	k_thread_name_set(&data->thread, dev->name);

#ifndef CONFIG_PM_DEVICE_RUNTIME
	const struct analog_axis_config *cfg = dev->config;

	k_timer_start(&data->timer,
		      K_MSEC(cfg->poll_period_ms), K_MSEC(cfg->poll_period_ms));
#else
	int ret;

	atomic_set(&data->suspended, 1);

	pm_device_init_suspended(dev);
	ret = pm_device_runtime_enable(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable runtime power management");
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int analog_axis_pm_action(const struct device *dev,
				 enum pm_device_action action)
{
	const struct analog_axis_config *cfg = dev->config;
	struct analog_axis_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		atomic_set(&data->suspended, 1);
		k_timer_stop(&data->timer);
		break;
	case PM_DEVICE_ACTION_RESUME:
		k_timer_start(&data->timer,
			      K_MSEC(cfg->poll_period_ms),
			      K_MSEC(cfg->poll_period_ms));
		atomic_set(&data->suspended, 0);
		k_sem_give(&data->wakeup);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define ANALOG_AXIS_CHANNEL_CFG_DEF(node_id) \
	{ \
		.adc = ADC_DT_SPEC_GET(node_id), \
		.out_min = (int16_t)DT_PROP(node_id, out_min), \
		.out_max = (int16_t)DT_PROP(node_id, out_max), \
		.axis = DT_PROP(node_id, zephyr_axis), \
		.invert_input = DT_PROP(node_id, invert_input), \
		.invert_output = DT_PROP(node_id, invert_output), \
	}

#define ANALOG_AXIS_CHANNEL_CAL_DEF(node_id) \
	{ \
		.in_min = (int16_t)DT_PROP(node_id, in_min), \
		.in_max = (int16_t)DT_PROP(node_id, in_max), \
		.in_deadzone = DT_PROP(node_id, in_deadzone), \
	}

#define ANALOG_AXIS_INIT(inst)									\
	static const struct analog_axis_channel_config analog_axis_channel_cfg_##inst[] = {	\
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, ANALOG_AXIS_CHANNEL_CFG_DEF, (,))	\
	};											\
												\
	static struct analog_axis_channel_data							\
		analog_axis_channel_data_##inst[ARRAY_SIZE(analog_axis_channel_cfg_##inst)];	\
												\
	static struct analog_axis_calibration							\
		analog_axis_calibration_##inst[ARRAY_SIZE(analog_axis_channel_cfg_##inst)] = {	\
			DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(					\
				inst, ANALOG_AXIS_CHANNEL_CAL_DEF, (,))				\
		};										\
												\
	static const struct analog_axis_config analog_axis_cfg_##inst = {			\
		.poll_period_ms = DT_INST_PROP(inst, poll_period_ms),				\
		.channel_cfg = analog_axis_channel_cfg_##inst,					\
		.channel_data = analog_axis_channel_data_##inst,				\
		.calibration = analog_axis_calibration_##inst,					\
		.num_channels = ARRAY_SIZE(analog_axis_channel_cfg_##inst),			\
	};											\
												\
	static struct analog_axis_data analog_axis_data_##inst;					\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, analog_axis_pm_action);					\
												\
	DEVICE_DT_INST_DEFINE(inst, analog_axis_init, PM_DEVICE_DT_INST_GET(inst),		\
			      &analog_axis_data_##inst, &analog_axis_cfg_##inst,		\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ANALOG_AXIS_INIT)
