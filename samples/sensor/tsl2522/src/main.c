/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tsl2522.h>

#define SLEEP_TIME K_MSEC(1000)

static bool overflow_detected;
static K_SEM_DEFINE(data_ready_sem, 0, 1);

static void handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	k_sem_give(&data_ready_sem);
}

static void overflow_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	overflow_detected = true;
}

static const char *map_gain_to_string(enum sensor_gain_tsl2522 gain)
{
	switch (gain) {
	case TSL2522_GAIN_MOD_HALF:
		return "0.5x";
	case TSL2522_GAIN_MOD_1X:
		return "1.0x";
	case TSL2522_GAIN_MOD_2X:
		return "2.0x";
	case TSL2522_GAIN_MOD_4X:
		return "4.0x";
	case TSL2522_GAIN_MOD_8X:
		return "8.0x";
	case TSL2522_GAIN_MOD_16X:
		return "16.0x";
	case TSL2522_GAIN_MOD_32X:
		return "32.0x";
	case TSL2522_GAIN_MOD_64X:
		return "64.0x";
	case TSL2522_GAIN_MOD_128X:
		return "128.0x";
	case TSL2522_GAIN_MOD_256X:
		return "256.0x";
	case TSL2522_GAIN_MOD_512X:
		return "512.0x";
	case TSL2522_GAIN_MOD_1024X:
		return "1024.0x";
	case TSL2522_GAIN_MOD_2048X:
		return "2048.0x";
	case TSL2522_GAIN_MOD_4096X:
		return "4096.0x";
	default:
		return "Error, invalid gain!";
	}
}

static int change_tls2522_gain_attribute(const struct device *dev, enum sensor_gain_tsl2522 gain)
{
	struct sensor_value attribute = {.val1 = gain, .val2 = 0};

	printf("change SENSOR_ATTR_GAIN to %s.\n", map_gain_to_string(gain));
	return sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_GAIN, &attribute);
}

static int change_tls2522_num_samples_attribute(const struct device *dev, int32_t num_samples)
{
	struct sensor_value attribute = {.val1 = num_samples, .val2 = 0};

	printf("change SENSOR_ATTR_NUMBER_OF_SAMPLES to %d.\n", num_samples);
	return sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_NUMBER_OF_SAMPLES, &attribute);
}

static int change_tls2522_measurement_time_attribute(const struct device *dev,
						     int32_t measurement_time_steps)
{
	struct sensor_value attribute = {.val1 = measurement_time_steps, .val2 = 0};

	printf("change SENSOR_ATTR_MEASUREMENT_TIME_STEPS to %d.\n", measurement_time_steps);
	return sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_MEASUREMENT_TIME_STEPS,
			       &attribute);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(light_sensor));
	const struct sensor_trigger trigger = {.type = SENSOR_TRIG_DATA_READY,
					       .chan = SENSOR_CHAN_LIGHT};
	const struct sensor_trigger overflow_trigger = {.type = SENSOR_TRIG_OVERFLOW,
							.chan = SENSOR_CHAN_LIGHT};
	int rc;
	int count = 0;
	struct sensor_value lux = {0};

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	while (1) {
		k_sem_take(&data_ready_sem, SLEEP_TIME);

		rc = sensor_sample_fetch(dev);
		if (rc) {
			printf("sample fetch error %d\n", rc);
			continue;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &lux);
		if (rc) {
			printf("sample get error %d\n", rc);
		}

		printf("lux: %f - overflow_handler %d\n", sensor_value_to_double(&lux),
		       overflow_detected);
		overflow_detected = false;

		if (count == 5) {
			rc = change_tls2522_gain_attribute(dev, TSL2522_GAIN_MOD_16X);
			if (rc) {
				printf("cannot change gain!(%d)\n", rc);
				return 0;
			}
		} else if (count == 8) {
			rc = change_tls2522_gain_attribute(dev, TSL2522_GAIN_MOD_8X);
			if (rc) {
				printf("cannot change gain!(%d)\n", rc);
				return 0;
			}
		} else if (count == 10) {
			printf("register overflow handler.\n");
			rc = sensor_trigger_set(dev, &overflow_trigger, overflow_handler);
			if (rc) {
				printf("cannot register overflow handler! (%d)\n", rc);
			}
		} else if (count == 15) {
			rc = change_tls2522_num_samples_attribute(dev, 64);
			if (rc) {
				printf("cannot change number of samples!(%d)\n", rc);
				return 0;
			}
		} else if (count == 20) {
			printf("register data ready handler.\n");
			rc = sensor_trigger_set(dev, &trigger, handler);
			if (rc) {
				printf("cannot register data ready handler!(%d)\n", rc);
			}
		} else if (count == 25) {
			rc = change_tls2522_measurement_time_attribute(dev, 2048);
			if (rc) {
				printf("cannot change attribute!(%d)\n", rc);
				return 0;
			}
		} else if (count == 30) {
			printf("remove overflow handler.\n");
			rc = sensor_trigger_set(dev, &overflow_trigger, NULL);
			if (rc) {
				printf("cannot remove overflow handler!(%d)\n", rc);
				return 0;
			}
		} else if (count == 32) {
			rc = change_tls2522_measurement_time_attribute(dev, 1024);
			if (rc) {
				printf("cannot change attribute!(%d)\n", rc);
				return 0;
			}
		} else if (count == 35) {
			rc = change_tls2522_num_samples_attribute(dev, 128);
			if (rc) {
				printf("cannot change number of samples!(%d)\n", rc);
				return 0;
			}
		} else if (count == 40) {
			count = 0;
			printf("remove data ready handler.\n");
			rc = sensor_trigger_set(dev, &trigger, NULL);
			if (rc) {
				printf("cannot remove data ready handler!(%d)\n", rc);
			}
		}

		count++;
	}

	return 0;
}
