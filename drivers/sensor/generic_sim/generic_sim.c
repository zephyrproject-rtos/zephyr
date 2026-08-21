/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_generic_sim_sensor

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/pm/device.h>

#include <zephyr/drivers/sensor/generic_sim.h>

struct generic_sim_data {
	struct sensor_value channel_values[SENSOR_CHAN_ALL];
	ATOMIC_DEFINE(channels_set, SENSOR_CHAN_ALL);
	int resume_rc;
	int suspend_rc;
	int fetch_rc;
};

struct generic_sim_cfg {
	int init_rc;
};

LOG_MODULE_DECLARE(GENERIC_SIM, CONFIG_SENSOR_LOG_LEVEL);

void generic_sim_reset(const struct device *dev, bool reset_rc)
{
	struct generic_sim_data *data = dev->data;

	memset(data->channels_set, 0, sizeof(data->channels_set));
	if (reset_rc) {
		data->resume_rc = 0;
		data->suspend_rc = 0;
		data->fetch_rc = 0;
	}
}

void generic_sim_func_rc(const struct device *dev, int resume_rc, int suspend_rc, int fetch_rc)
{
	struct generic_sim_data *data = dev->data;

	data->resume_rc = resume_rc;
	data->suspend_rc = suspend_rc;
	data->fetch_rc = fetch_rc;
}

int generic_sim_channel_set(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value val)
{
	struct generic_sim_data *data = dev->data;

	if (chan >= SENSOR_CHAN_ALL) {
		return -EINVAL;
	}

	atomic_set_bit(data->channels_set, chan);
	data->channel_values[chan] = val;
	return 0;
}

static int generic_sim_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct generic_sim_data *data = dev->data;

	if (chan > SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	return data->fetch_rc;
}

static int generic_sim_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct generic_sim_data *data = dev->data;

	if (chan >= SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (!atomic_test_bit(data->channels_set, chan)) {
		/* Channel that hasn't been configured */
		return -ENOTSUP;
	}

	*val = data->channel_values[chan];
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int generic_sim_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct generic_sim_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return data->suspend_rc;
	case PM_DEVICE_ACTION_RESUME:
		return data->resume_rc;
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, generic_sim_driver_api) = {
	.sample_fetch = generic_sim_sample_fetch,
	.channel_get = generic_sim_channel_get,
};

int generic_sim_init(const struct device *dev)
{
	const struct generic_sim_cfg *cfg = dev->config;

	return cfg->init_rc;
}

#define GENERIC_SIM_DEFINE(inst)                                                                   \
	static const struct generic_sim_cfg generic_sim_cfg_##inst = {                             \
		.init_rc = -DT_INST_PROP(inst, negated_init_rc),                                   \
	};                                                                                         \
	static struct generic_sim_data generic_sim_data_##inst;                                    \
	PM_DEVICE_DT_INST_DEFINE(inst, generic_sim_pm_control);                                    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, generic_sim_init, PM_DEVICE_DT_INST_GET(inst),          \
				     &generic_sim_data_##inst, &generic_sim_cfg_##inst,            \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &generic_sim_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GENERIC_SIM_DEFINE)
