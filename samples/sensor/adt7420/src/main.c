/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/__assert.h>

#define DELAY_WITH_TRIGGER K_SECONDS(5)
#define DELAY_WITHOUT_TRIGGER K_SECONDS(1)

#define UCEL_PER_CEL 1000000
#define UCEL_PER_MCEL 1000
#define TEMP_INITIAL_CEL 21
#define TEMP_WINDOW_HALF_UCEL 500000

K_SEM_DEFINE(sem, 0, 1);
static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	k_sem_give(&sem);
}

static int low_ucel;
static int high_ucel;

static int sensor_set_attribute(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr, int value)
{
	struct sensor_value sensor_val;
	int ret;

	sensor_val.val1 = value / UCEL_PER_CEL;
	sensor_val.val2 = value % UCEL_PER_CEL;

	ret = sensor_attr_set(dev, chan, attr, &sensor_val);
	if (ret) {
		printf("sensor_attr_set failed ret %d\n", ret);
	}

	return ret;
}

static bool temp_in_window(const struct sensor_value *val)
{
	int temp_ucel = val->val1 * UCEL_PER_CEL + val->val2;

	return (temp_ucel >= low_ucel) && (temp_ucel <= high_ucel);
}

static int sensor_set_window(const struct device *dev,
			     const struct sensor_value *val)
{
	int temp_ucel = val->val1 * UCEL_PER_CEL + val->val2;

	low_ucel = temp_ucel - TEMP_WINDOW_HALF_UCEL;
	high_ucel = temp_ucel + TEMP_WINDOW_HALF_UCEL;

	int rc = sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_UPPER_THRESH, high_ucel);

	if (rc == 0) {
		sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
				     SENSOR_ATTR_LOWER_THRESH, low_ucel);
	}

	if (rc == 0) {
		printk("Alert on temp outside [%d, %d] mCel\n",
		       low_ucel / UCEL_PER_MCEL,
		       high_ucel / UCEL_PER_MCEL);
	}

	return rc;
}

static void process(const struct device *dev)
{
	struct sensor_value temp_val;
	int ret;
	bool reset_window = false;

	/* Set update rate to 240 mHz */
	sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
			     SENSOR_ATTR_SAMPLING_FREQUENCY, 240 * 1000);

	if (IS_ENABLED(CONFIG_ADT7420_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_THRESHOLD,
			.chan = SENSOR_CHAN_AMBIENT_TEMP,
		};
		struct sensor_value val = {
			.val1 = TEMP_INITIAL_CEL,
		};

		ret = sensor_set_window(dev, &val);
		if (ret == 0) {
			ret = sensor_trigger_set(dev, &trig, trigger_handler);
		}
		if (ret != 0) {
			printf("Could not set trigger\n");
			return;
		}
	}

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printf("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
					 &temp_val);
		if (ret) {
			printf("sensor_channel_get failed ret %d\n", ret);
			return;
		}

		if (IS_ENABLED(CONFIG_ADT7420_TRIGGER)) {
			reset_window |= !temp_in_window(&temp_val);
		}

		printf("[%s]: temperature %.6f Cel%s\n", now_str(),
		       sensor_value_to_double(&temp_val),
		       reset_window ? ": NEED RESET" : "");


		if (IS_ENABLED(CONFIG_ADT7420_TRIGGER)) {
			if (reset_window) {
				ret = sensor_set_window(dev, &temp_val);
			}
			if (ret) {
				printf("Window update failed ret %d\n", ret);
				return;
			}

			printk("Wait for trigger...");
			ret = k_sem_take(&sem, DELAY_WITH_TRIGGER);
			reset_window = (ret == 0);
			printk("%s\n", reset_window ? "ALERTED!" : "timed-out");
		} else {
			k_sleep(DELAY_WITHOUT_TRIGGER);
		}
	}
}

void main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(adi_adt7420);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

	printf("device is %p, name is %s\n", dev, dev->name);

	process(dev);
}
