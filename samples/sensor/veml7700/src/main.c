/*
 * Copyright (c) 2022 innblue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main);

const struct device *als_dev;

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value lux;

	int err = sensor_sample_fetch(sensor);

	++count;

	if (err < 0) {
		LOG_WRN("Sample #%u fetch failed (%d)", count, err);
		return;
	}

	err = sensor_channel_get(als_dev, SENSOR_CHAN_LIGHT, &lux);

	if (err < 0) {
		printf("ALS data update failed (%d)\n", err);
	} else {
		printf("#%u @ %u ms: Ambient light level: %d.%06d (lux)\n", count,
		       k_uptime_get_32(), lux.val1, lux.val2);
	}
}

#ifdef CONFIG_VEML7700_TRIGGER
static void trigger_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
#endif

void main(void)
{
	als_dev = device_get_binding(DT_LABEL(DT_INST(0, vishay_veml7700)));

	if (NULL == als_dev) {
		LOG_ERR("Could not find VEML7700 device, aborting test.");
		return;
	}
	if (!device_is_ready(als_dev)) {
		LOG_ERR("VEML7700 device %s is not ready, aborting test.", als_dev->name);
		return;
	}

#ifdef CONFIG_VEML7700_TRIGGER
	int err;

	struct sensor_trigger trig;
	struct sensor_value attr_thrs = { .val1 = 0, .val2 = 0 };

	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_LIGHT;

	attr_thrs.val1 = CONFIG_VEML7700_LOW_THRS;
	err = sensor_attr_set(als_dev, trig.chan, SENSOR_ATTR_LOWER_THRESH, &attr_thrs);
	if (err < 0) {
		LOG_ERR("VEML7700 lower threshold level set failed (%d)", err);
		return;
	}

	attr_thrs.val1 = CONFIG_VEML7700_HIGH_THRS;
	err = sensor_attr_set(als_dev, trig.chan, SENSOR_ATTR_UPPER_THRESH, &attr_thrs);
	if (err < 0) {
		LOG_ERR("VEML7700 upper threshold level set failed (%d)", err);
		return;
	}

	err = sensor_trigger_set(al_dev, &trig, trigger_handler);
	if (err != 0) {
		LOG_ERR("VEML7700 trigger set failed (%d)", err);
		return;
	}
#endif

	while (1) {
		fetch_and_display(als_dev);
		k_sleep(K_MSEC(5000));
	}
}
