/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	struct sensor_value temperature;
	const char *overrun = "";
	static unsigned int fifo_size = 32;
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		while (fifo_size--)
		{
			rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
			printf("#%u @ %u ms: %sx %f , y %f , z %f\r\n",
				count, k_uptime_get_32(), overrun,
				sensor_value_to_double(&accel[0]),
				sensor_value_to_double(&accel[1]),
				sensor_value_to_double(&accel[2]));
		}

	}
	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("Done fetching 32 samples from fifo\r\n");
	}

	if (IS_ENABLED(CONFIG_LIS2DH_MEASURE_TEMPERATURE)) {
		if (rc == 0) {
			rc = sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temperature);
			if (rc < 0) {
				printf("\nERROR: Unable to read temperature:%d\n", rc);
			} else {
				printf(", t %f\n", sensor_value_to_double(&temperature));
			}
		}

	} else {
		printf("\n");
	}
}

// #ifdef CONFIG_LIS2DH_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
// #endif

int main(void)
{
	int rc = 0;
	struct device *const sensor = DEVICE_DT_GET(DT_ALIAS(accelerometer));
	static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

    if (!gpio_is_ready_dt(&status_led)) {
        printk("Status LED not in devicetree\r\n");
    }

    rc = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_ACTIVE);
    if (rc < 0){
        printk("Status LED could not be configured\r\n");
    }
    printk("Status LED configured\r\n");

	if (sensor == NULL) {
		printf("No device found\r\n");
		return 0;
	}
	if (!device_is_ready(sensor)) {
		printf("Device %s is not ready\r\n", sensor->name);
		// return 0;
	}
	else {

		struct sensor_trigger trig;
		int rc;

		trig.type = SENSOR_TRIG_FIFO_FULL;
		trig.chan = SENSOR_CHAN_ACCEL_XYZ;

		// if (IS_ENABLED(CONFIG_LIS2DH_ODR_RUNTIME)) {
			struct sensor_value odr = {
				.val1 = 1,
			};

			rc = sensor_attr_set(sensor, trig.chan,
							SENSOR_ATTR_SAMPLING_FREQUENCY,
							&odr);
			if (rc != 0) {
				printf("Failed to set odr: %d\n", rc);
				return 0;
			}
			printf("Sampling at %u Hz\n", odr.val1);
		// }

		rc = sensor_trigger_set(sensor, &trig, trigger_handler);
		if (rc != 0) {
			printf("Failed to set trigger: %d\n", rc);
			return 0;
		}
		printf("Waiting for triggers\n");
	}
	
	while (true) {
		gpio_pin_toggle_dt(&status_led);
		k_sleep(K_MSEC(2000));
	}
}
