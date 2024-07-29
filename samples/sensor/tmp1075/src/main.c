/*
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tmp1075_sample, LOG_LEVEL_DBG);
#define DT_TMP1075 DT_NODELABEL(tmp1075_temperature)

static void trigger_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	LOG_WRN("Detected tmp1075 interrupt");
}

int tmp1075_setup(void)
{
	const struct device *const sensor = DEVICE_DT_GET(DT_TMP1075);

	if (!device_is_ready(sensor)) {
		LOG_ERR("Device %s is not ready!", sensor->name);
	}
	static const struct sensor_trigger trig = {.chan = SENSOR_CHAN_AMBIENT_TEMP,
						   .type = SENSOR_TRIG_THRESHOLD};
	static const struct sensor_value lower = {.val1 = DT_PROP(DT_TMP1075, lower_threshold)};
	static const struct sensor_value higher = {.val1 = DT_PROP(DT_TMP1075, upper_threshold)};

	int status = sensor_trigger_set(sensor, &trig, trigger_handler);

	if (status != 0) {
		LOG_ERR("Couldn't set the trigger to: %s!", sensor->name);
		return status;
	}
	LOG_INF("setting thresholds ---- lower: %d higher: %d", lower.val1, higher.val1);
	status =
		sensor_attr_set(sensor, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH, &lower);
	status = sensor_attr_set(sensor, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
				 &higher);
	return status;
}

int main(void)
{
	const struct device *temp_sensor = DEVICE_DT_GET(DT_TMP1075);
	struct sensor_value temp;

	printf("TI TMP1075 Example, %s\n", CONFIG_ARCH);

#ifdef CONFIG_TMP1075_ALERT_INTERRUPTS
	if (0 != tmp1075_setup()) {
		LOG_ERR("Error: Alert GPIO device is not ready\n");
		return 1;
	}
#endif

	while (1) {
		if (sensor_sample_fetch(temp_sensor) < 0) {
			LOG_ERR("Sensor sample update error\n");
			return 1;
		}

		if (sensor_channel_get(temp_sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
			LOG_ERR("Cannot read temperature channel\n");
			return 1;
		}

		LOG_INF("Temperature: %d.%04d°C", temp.val1, temp.val2);
		k_sleep(K_SECONDS(1));
	}
}
