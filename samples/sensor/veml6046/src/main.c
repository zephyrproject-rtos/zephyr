/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/sensor/veml6046.h>

static void read_with_attr(const struct device *dev, int it, int pdd, int gain)
{
	int ret;
	struct sensor_value red, green, blue, ir;
	struct sensor_value red_raw, green_raw, blue_raw, ir_raw;
	struct sensor_value sen;
	char result[10];

	sen.val2 = 0;

	sen.val1 = it;
	ret = sensor_attr_set(dev, SENSOR_CHAN_LIGHT,
			     (enum sensor_attribute)SENSOR_ATTR_VEML6046_IT, &sen);
	if (ret) {
		printf("Failed to set it attribute ret: %d\n", ret);
	}
	sen.val1 = pdd;
	ret = sensor_attr_set(dev, SENSOR_CHAN_LIGHT,
			     (enum sensor_attribute)SENSOR_ATTR_VEML6046_PDD, &sen);
	if (ret) {
		printf("Failed to set pdd attribute ret: %d\n", ret);
	}
	sen.val1 = gain;
	ret = sensor_attr_set(dev, SENSOR_CHAN_LIGHT,
			     (enum sensor_attribute)SENSOR_ATTR_VEML6046_GAIN, &sen);
	if (ret) {
		printf("Failed to set gain attribute ret: %d\n", ret);
	}

	ret = sensor_sample_fetch(dev);
	if ((ret < 0) && (ret != -E2BIG)) {
		printf("sample update error. ret: %d\n", ret);
	}

	sensor_channel_get(dev, SENSOR_CHAN_RED, &red);
	sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_VEML6046_RED_RAW_COUNTS,
										&red_raw);

	sensor_channel_get(dev, SENSOR_CHAN_GREEN, &green);
	sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_VEML6046_GREEN_RAW_COUNTS,
										&green_raw);

	sensor_channel_get(dev, SENSOR_CHAN_BLUE, &blue);
	sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_VEML6046_BLUE_RAW_COUNTS,
										&blue_raw);

	sensor_channel_get(dev, SENSOR_CHAN_IR, &ir);
	sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_VEML6046_IR_RAW_COUNTS,
										&ir_raw);

	if (ret == -E2BIG) {
		snprintf(result, sizeof(result), "OVERFLOW");
	} else if (ret) {
		snprintf(result, sizeof(result), "ERROR");
	} else {
		snprintf(result, sizeof(result), "");
	}

	printf("Red: %6d lx (%6d) green:  %6d lx (%6d) "
	       "blue: %6d lx (%6d) IR:  %6d lx (%6d) "
	       "  it: %d pdd: %d gain: %d  --  %s\n",
	       red.val1, red_raw.val1,
	       green.val1, green_raw.val1,
	       blue.val1, blue_raw.val1,
	       ir.val1, ir_raw.val1,
	       it, pdd, gain,
	       result);
}

static void read_with_all_attr(const struct device *dev)
{
	for (int it = VEML60XX_IT_3_125; it <= VEML60XX_IT_400; it++) {
		for (int pdd = VEML6046_SIZE_2_2; pdd <= VEML6046_SIZE_1_2; pdd++) {
			for (int gain = VEML60XX_GAIN_1; gain <= VEML60XX_GAIN_0_5; gain++) {
				read_with_attr(dev, it, pdd, gain);
			}
		}
	}
}

int main(void)
{
	const struct device *const veml = DEVICE_DT_GET(DT_NODELABEL(rgbir));

	if (!device_is_ready(veml)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	printf("Test all attributes for a good guess of attribute usage away of saturation.\n");
	read_with_all_attr(veml);
	printf("Test finished.\n");

	return 0;
}
