/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor/grow_r502a.h>

static bool enroll;
static struct sensor_value fid, val;

static void finger_match(const struct device *dev)
{
	struct sensor_value input;
	int ret;

	ret = sensor_attr_get(dev, SENSOR_CHAN_FINGERPRINT,
		SENSOR_ATTR_R502A_RECORD_FIND, &input);
	if (ret != 0) {
		printk("Sensor attr get failed %d\n", ret);
		return;
	}
	printk("Matched ID : %d\n", input.val1);
	printk("confidence : %d\n", input.val2);
}

static void finger_enroll(const struct device *dev)
{
	int ret;

	ret = sensor_attr_set(dev, SENSOR_CHAN_FINGERPRINT, SENSOR_ATTR_R502A_RECORD_ADD, &fid);

	if (ret == 0) {
		printk("Fingerprint successfully stored at #%d\n", fid.val1);
		enroll = false;
	}
}

static void template_count_get(const struct device *dev)
{
	int ret;

	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		printk("Sample Fetch Error %d\n", ret);
		return;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_FINGERPRINT, &val);
	if (ret < 0) {
		printk("Channel Get Error %d\n", ret);
		return;
	}
	printk("template count : %d\n", val.val1);
}

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	if (enroll) {
		finger_enroll(dev);
	} else {
		template_count_get(dev);
		finger_match(dev);
	}
}

int main(void)
{
	static struct sensor_value del, fid_get;
	int ret;

	const struct device *dev =  DEVICE_DT_GET_ONE(hzgrow_r502a);

	if (dev ==  NULL) {
		printk("Error: no device found\n");
		return 0;
	}

	if (!device_is_ready(dev)) {
		printk("Error: Device %s is not ready\n", dev->name);
		return 0;
	}

	template_count_get(dev);

	del.val1 = 3;
	ret = sensor_attr_set(dev, SENSOR_CHAN_FINGERPRINT, SENSOR_ATTR_R502A_RECORD_DEL, &del);
	if (ret != 0) {
		printk("Sensor attr set failed %d\n", ret);
		return 0;
	}
	printk("Fingerprint Deleted at ID #%d\n", del.val1);

	ret = sensor_attr_get(dev, SENSOR_CHAN_FINGERPRINT,
					SENSOR_ATTR_R502A_RECORD_FREE_IDX, &fid_get);
	if (ret != 0) {
		printk("Sensor attr get failed %d\n", ret);
		return 0;
	}
	printk("Fingerprint template free idx at ID #%d\n", fid_get.val1);

	fid.val1 = fid_get.val1;
	printk("Waiting for valid finger to enroll as ID #%d\n"
		"Place your finger\n", fid.val1);
	enroll = true;

	if (IS_ENABLED(CONFIG_GROW_R502A_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_TOUCH,
			.chan = SENSOR_CHAN_FINGERPRINT,
		};
		ret = sensor_trigger_set(dev, &trig, trigger_handler);
		if (ret != 0) {
			printk("Could not set trigger\n");
			return 0;
		}
	}

	return 0;
}
