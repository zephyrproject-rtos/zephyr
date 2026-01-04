/*
 * Copyright (c) 2017, NXP
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hr, LOG_LEVEL_INF);

#include <zephyr/drivers/gpio.h>
const struct gpio_dt_spec sensor_ctrl = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), ctrl_gpios);

#define MAX30101_SENSOR_CHANNEL SENSOR_CHAN_GREEN

static void print_sample_fetch(const struct device *dev)
{
	static struct sensor_value red, ir, green, temp;

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_RED, &red);
	sensor_channel_get(dev, SENSOR_CHAN_IR, &ir);
	sensor_channel_get(dev, SENSOR_CHAN_GREEN, &green);
	sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temp);

	/* Print LED data*/
	LOG_INF("RED = %d, IR = %d, GREEN = %d", red.val1, ir.val1, green.val1);
	/* Print temperature data */
	LOG_INF("Temperature = %d.%d", temp.val1, temp.val2);
}

#if CONFIG_MAX30101_TRIGGER
static struct sensor_trigger trig_drdy;

void sensor_data_ready(const struct device *dev, const struct sensor_trigger *trigger)
{
	print_sample_fetch(dev);
}
#endif /* CONFIG_MAX30101_TRIGGER */

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(heart_rate_sensor));

	if (dev == NULL) {
		printf("Could not get heart_rate_sensor\n");
		return 0;
	}
	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	/* Setup CTLR */
	if (!gpio_is_ready_dt(&sensor_ctrl)) {
		printf("ERROR: PPG CTRL device is not ready\n");
		return 0;
	}

	int result = gpio_pin_configure_dt(&sensor_ctrl, GPIO_OUTPUT_ACTIVE);

	if (result < 0) {
		printf("ERROR [%d]: Failed to configure PPG CTRL gpio\n", result);
		return 0;
	}

#if CONFIG_MAX30101_TRIGGER
	trig_drdy.type = SENSOR_TRIG_DATA_READY;
	trig_drdy.chan = MAX30101_SENSOR_CHANNEL;
	sensor_trigger_set(dev, &trig_drdy, sensor_data_ready);
#endif /* CONFIG_MAX30101_TRIGGER */

	while (1) {
#if !CONFIG_MAX30101_TRIGGER
		print_sample_fetch(dev);
#endif /* !CONFIG_MAX30101_TRIGGER */

		k_sleep(K_MSEC(20));
	}
	return 0;
}
