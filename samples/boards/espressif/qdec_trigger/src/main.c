/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/pcnt_esp32.h>

#define PCNT_NODE DT_NODELABEL(pcnt)

static volatile uint32_t unit0_hits;
static volatile uint32_t unit1_hits;

static void unit0_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	unit0_hits++;
}

static void unit1_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	unit1_hits++;
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(PCNT_NODE);
	struct sensor_value v;
	int rc;

	static struct sensor_trigger trig0 = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_ROTATION,
	};
	static struct sensor_trigger trig1 = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_PCNT_ESP32_UNIT(1),
	};

	if (!device_is_ready(dev)) {
		printf("pcnt not ready\n");
		return 0;
	}

	v.val1 = 100;
	v.val2 = 0;
	rc = sensor_attr_set(dev, SENSOR_CHAN_ROTATION, SENSOR_ATTR_UPPER_THRESH, &v);
	printf("unit0 upper_thresh=100: rc=%d\n", rc);

	v.val1 = -100;
	rc = sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(1), SENSOR_ATTR_LOWER_THRESH, &v);
	printf("unit1 lower_thresh=-100: rc=%d\n", rc);

	rc = sensor_trigger_set(dev, &trig0, unit0_handler);
	rc |= sensor_trigger_set(dev, &trig1, unit1_handler);
	printf("triggers armed: rc=%d\n", rc);

	while (1) {
		k_msleep(1000);
		printf("hits unit0=%u unit1=%u\n", unit0_hits, unit1_hits);
	}
	return 0;
}
