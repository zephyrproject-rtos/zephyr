/*
 * Copyright (c) 2021 Jimmy Johnson <catch22@fastmail.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tmp108.h>

void temperature_one_shot(const struct device *dev,
			  const struct sensor_trigger *trigger)
{

	struct sensor_value temp_value;
	int result;

	result = sensor_channel_get(dev,
				    SENSOR_CHAN_AMBIENT_TEMP,
				    &temp_value);

	if (result) {
		printf("error: sensor_channel_get failed: %d\n", result);
		return;
	}

	printf("One shot power saving mode enabled, temperature is %gC\n",
	       sensor_value_to_double(&temp_value));
}

void temperature_alert(const struct device *dev,
		       const struct sensor_trigger *trigger)
{

	struct sensor_value temp_flags = { 0 };

	sensor_attr_get(dev,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_CONFIGURATION,
			&temp_flags);

	/* use a mask to pull your specific chip set bits out */

	printf("Temperature alert config register = %x!\n", temp_flags.val1);
}

void enable_temp_alerts(const struct device *tmp108)
{

	struct sensor_trigger sensor_trigger_type_temp_alert = {
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
		.type = SENSOR_TRIG_THRESHOLD
	};

	struct sensor_value alert_upper_thresh = {
		CONFIG_APP_TEMP_ALERT_HIGH_THRESH,
		0
	};

	struct sensor_value alert_lower_thresh = {
		CONFIG_APP_TEMP_ALERT_LOW_THRESH,
		0
	};

	struct sensor_value alert_hysterisis = { 1, 0 };
	struct sensor_value thermostat_mode = { 0, 0 };

	sensor_attr_set(tmp108,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_ALERT,
			&thermostat_mode);

	sensor_attr_set(tmp108,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_HYSTERESIS,
			&alert_hysterisis);

	sensor_attr_set(tmp108,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_UPPER_THRESH,
			&alert_upper_thresh);

	sensor_attr_set(tmp108,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_LOWER_THRESH,
			&alert_lower_thresh);

	sensor_trigger_set(tmp108,
			   &sensor_trigger_type_temp_alert,
			   temperature_alert);
}

void enable_one_shot(const struct device *tmp108)
{

	struct sensor_trigger sensor_trigger_type_temp_one_shot = {
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
		.type = SENSOR_TRIG_DATA_READY
	};

	sensor_attr_set(tmp108,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_TMP108_ONE_SHOT_MODE,
			NULL);

	sensor_trigger_set(tmp108,
			   &sensor_trigger_type_temp_one_shot,
			   temperature_one_shot);
}

void get_temperature_continuous(const struct device *tmp108)
{

	struct sensor_value temp_value;
	int result;

	result = sensor_channel_get(tmp108,
				    SENSOR_CHAN_AMBIENT_TEMP,
				    &temp_value);

	if (result) {
		printf("error: sensor_channel_get failed: %d\n", result);
		return;
	}

	printf("temperature is %gC\n", sensor_value_to_double(&temp_value));
}

void main(void)
{
	const struct device *temp_sensor;
	int result;

	printf("TI TMP108 Example, %s\n", CONFIG_ARCH);

	temp_sensor = DEVICE_DT_GET_ANY(ti_tmp108);

	if (!temp_sensor) {
		printf("warning: tmp108 device not found checking for compatible ams device\n");

		temp_sensor = DEVICE_DT_GET_ANY(ams_as6212);

		if (!temp_sensor) {
			printf("error: tmp108 compatible devices not found\n");
			return;
		}
	}

	if (!device_is_ready(temp_sensor)) {
		printf("error: tmp108 device not ready\n");
		return;
	}

	sensor_attr_set(temp_sensor,
			SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_TMP108_CONTINUOUS_CONVERSION_MODE,
			NULL);

#if CONFIG_APP_ENABLE_ONE_SHOT
	enable_one_shot(temp_sensor);
#endif

#if CONFIG_APP_REPORT_TEMP_ALERTS
	enable_temp_alerts(temp_sensor);
#endif

	while (1) {

		result = sensor_sample_fetch(temp_sensor);

		if (result) {
			printf("error: sensor_sample_fetch failed: %d\n", result);
			break;
		}

#if !CONFIG_APP_ENABLE_ONE_SHOT
		get_temperature_continuous(temp_sensor);
#endif
		k_sleep(K_MSEC(3000));
	}
}
