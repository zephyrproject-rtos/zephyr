/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys_clock.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/drivers/sensor/veml6031.h>

static void read_with_attr(const struct device *dev, int it, int div4, int gain)
{
	int ret;
	struct sensor_value light;
	struct sensor_value als_raw;
	struct sensor_value ir_raw;
	struct sensor_value sen;

	sen.val2 = 0;

	sen.val1 = it;
	ret = sensor_attr_set(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML6031_IT, &sen);
	if (ret) {
		printf("Failed to set it attribute ret: %d\n", ret);
	}
	sen.val1 = div4;
	ret = sensor_attr_set(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML6031_DIV4, &sen);
	if (ret) {
		printf("Failed to set div4 attribute ret: %d\n", ret);
	}
	sen.val1 = gain;
	ret = sensor_attr_set(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML6031_GAIN, &sen);
	if (ret) {
		printf("Failed to set gain attribute ret: %d\n", ret);
	}

	ret = sensor_sample_fetch(dev);
	if ((ret < 0) && (ret != -E2BIG)) {
		printf("sample update error. ret: %d\n", ret);
	}

	sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &light);

	sensor_channel_get(dev, SENSOR_CHAN_VEML6031_ALS_RAW_COUNTS, &als_raw);

	sensor_channel_get(dev, SENSOR_CHAN_VEML6031_IR_RAW_COUNTS, &ir_raw);

	printf("Light (lux): %6d ALS (raw): %6d IR (raw): %6d "
	       "  it: %d div4: %d gain: %d  --  %s\n",
	       light.val1, als_raw.val1, ir_raw.val1, it, div4, gain,
	       ret == -E2BIG ? "OVERFLOW"
	       : ret         ? "ERROR"
			     : "");
}

static void read_with_all_attr(const struct device *dev)
{
	int it, div4, gain;

	for (it = VEML6031_IT_3_125; it <= VEML6031_IT_400; it++) {
		for (div4 = VEML6031_SIZE_4_4; div4 <= VEML6031_SIZE_1_4; div4++) {
			for (gain = VEML6031_GAIN_1; gain <= VEML6031_GAIN_0_5; gain++) {
				read_with_attr(dev, it, div4, gain);
			}
		}
	}
}

int main(void)
{
	const struct device *const veml = DEVICE_DT_GET(DT_NODELABEL(light));

	if (!device_is_ready(veml)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	printf("Test all attributes for a good guess of attribute usage away of saturation.\n");
	read_with_all_attr(veml);
	printf("Test finished.\n");

	return 0;
}
