/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <drivers/gpio.h>

#define LED_PORT DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED DT_ALIAS_LED0_GPIOS_PIN
#define LED_TRIG DT_ALIAS_LED1_GPIOS_PIN

#define TH_LOW_VAL 1100
#define TH_HI_VAL 2200

struct channel_info {
	int chan;
	char *chan_name;
};

static struct channel_info channels[] = {
	{ SENSOR_CHAN_RED, "Red" },
	{ SENSOR_CHAN_GREEN, "Green" },
	{ SENSOR_CHAN_BLUE, "Blue" },
};
struct device *led_device;
struct device *sensor_device;

void init_subsystems(void)
{
	led_device = device_get_binding(LED_PORT);

	if (led_device == NULL) {
		printk("Failed to access LED driver\n");
		return;
	}
	gpio_pin_configure(led_device, LED, GPIO_DIR_OUT);
	gpio_pin_configure(led_device, LED_TRIG, GPIO_DIR_OUT);
	gpio_pin_write(led_device, LED_TRIG, 1);

	sensor_device = device_get_binding("COLOR_SENSOR");
	if (sensor_device == NULL) {
		printk("Failed to access ILS29125 driver\n");
		return;
	}
}

void plain_read_example(void)
{
	init_subsystems();
	u32_t led_state = 0;

	while (true) {
		/* fetch sensor samples */
		int rc = sensor_sample_fetch(sensor_device);

		if (rc) {
			printk("Failed to fetch sample from sensor, err=%d\n",
			       rc);
		}

		printk("Detected color:");
		struct sensor_value val;

		for (int i = 0; i < ARRAY_SIZE(channels); i++) {
			rc = sensor_channel_get(sensor_device, channels[i].chan,
						&val);
			if (rc) {
				printk("Failed to get data for chan %s(%d)\n",
				       channels[i].chan_name, rc);
				continue;
			}
			printk("  %s=%d\n", channels[i].chan_name, val.val1);
		}
		gpio_pin_write(led_device, LED, led_state % 2);
		led_state++;
		k_sleep(K_MSEC(2000));
	}
}

static void trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	struct sensor_value val;
	int rc = sensor_sample_fetch(sensor_device);

	rc = sensor_channel_get(sensor_device, SENSOR_CHAN_RED, &val);
	gpio_pin_write(led_device, LED_TRIG, 0);
	printk("Triggered, red value is %d not in window (%d, %d), st:%d\n",
	       val.val1, TH_LOW_VAL, TH_HI_VAL, rc);
}

void threshold_example(void)
{
	init_subsystems();
	int rc;

	while (true) {
		rc = sensor_sample_fetch(sensor_device);
		if (rc) {
			printk("Failed to fetch sample from sensor, err=%d\n",
			       rc);
			k_sleep(K_MSEC(2000));
			continue;
		}
		struct sensor_value val;

		sensor_channel_get(sensor_device, SENSOR_CHAN_RED, &val);
		if (val.val1 != 0) {
			break;
		}
		k_sleep(K_MSEC(2000));
	}

	u32_t led_state = 0;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_ALL,
	};
	struct sensor_value lo_thr = { TH_LOW_VAL, 0 };
	struct sensor_value hi_thr = { TH_HI_VAL, 0 };

	rc = sensor_attr_set(sensor_device, SENSOR_CHAN_RED,
			     SENSOR_ATTR_LOWER_THRESH, &lo_thr);
	printk("Set lower red threshold to %d, result: %d\n", lo_thr.val1, rc);
	if (rc == 0) {
		rc = sensor_attr_set(sensor_device, SENSOR_CHAN_RED,
				     SENSOR_ATTR_UPPER_THRESH, &hi_thr);
		printk("Set upper red threshold to %d, result: %d\n",
		       hi_thr.val1, rc);
	}
	if (rc == 0) {
		rc = sensor_trigger_set(sensor_device, &trig, trigger_handler);
		printk("Setup trigger, result: %d\n", rc);
	}
	if (rc != 0) {
		printk("Alert configuration failed: %d\n", rc);
	}

	printk("Waiting for value out of threshold window (%d, %d)\n",
	       TH_LOW_VAL, TH_HI_VAL);
	while (true) {
		gpio_pin_write(led_device, LED, led_state % 2);
		led_state++;
		k_sleep(K_MSEC(500));
	}
}

void main(void)
{
	printk("Main start\n");
	if (IS_ENABLED(CONFIG_ISL29125_TRIGGER)) {
		threshold_example();
	} else {
		plain_read_example();
	}
}
