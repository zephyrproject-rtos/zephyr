/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <kernel.h>
#include <sensor.h>

struct battery_data {
	struct device *device;
	struct k_delayed_work trigger_work;
	sensor_trigger_handler_t trigger_callback;
	u8_t battery_lvl;
};

static void trigger_fire(struct k_work *work)
{
	struct k_delayed_work *dwork =
	    CONTAINER_OF(work, struct k_delayed_work, work);

	struct battery_data *data =
	    CONTAINER_OF(dwork, struct battery_data, trigger_work);

	struct sensor_trigger trigger = {
		.chan = SENSOR_CHAN_STATE_OF_CHARGE,
		.type = SENSOR_TRIG_DATA_READY
	};

	/* decrease battery level and reset to max upon reaching the minimum */
	data->battery_lvl = data->battery_lvl > CONFIG_BATTERY_SENSOR_LVL_MIN
				? data->battery_lvl - 1
				: CONFIG_BATTERY_SENSOR_LVL_MAX;

	data->trigger_callback(data->device, &trigger);

	/* resubmit this work to fire the trigger periodically */
	k_delayed_work_submit(dwork,
			      K_MSEC(CONFIG_BATTERY_SENSOR_TRIG_PERIOD));
}

static int battery_init(struct device *dev)
{
	struct battery_data *data = dev->driver_data;

	data->device = dev;
	k_delayed_work_init(&data->trigger_work, trigger_fire);

	return 0;
}

static int battery_trigger_set(struct device *dev,
			       const struct sensor_trigger *trig,
			       sensor_trigger_handler_t handler)
{
	struct battery_data *data;

	if (trig->type != SENSOR_TRIG_DATA_READY ||
	    trig->chan != SENSOR_CHAN_STATE_OF_CHARGE) {
		return -ENOTSUP;
	}

	data = dev->driver_data;
	data->trigger_callback = handler;

	k_delayed_work_submit(&data->trigger_work,
			      K_MSEC(CONFIG_BATTERY_SENSOR_TRIG_PERIOD));

	return 0;
}

static int battery_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_STATE_OF_CHARGE) {
		return -ENOTSUP;
	}

	/* nothing do to, battery level is updated periodically */

	return 0;
}

static int battery_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct battery_data *data;

	if (chan != SENSOR_CHAN_STATE_OF_CHARGE) {
		return -ENOTSUP;
	}

	data = dev->driver_data;

	val->val1 = data->battery_lvl;
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api battery_driver_api = {
	.trigger_set = battery_trigger_set,
	.sample_fetch = battery_sample_fetch,
	.channel_get = battery_channel_get,
};

static struct battery_data battery_driver = {
	.battery_lvl = CONFIG_BATTERY_SENSOR_LVL_MAX
};

DEVICE_AND_API_INIT(battery, CONFIG_BATTERY_SENSOR_DEV_NAME, battery_init,
		    &battery_driver, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &battery_driver_api);
