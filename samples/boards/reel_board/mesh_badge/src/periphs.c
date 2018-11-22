/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <gpio.h>
#include <sensor.h>
#include "board.h"

struct device_info {
	struct device *dev;
	char *name;
};

struct led_device_info {
	struct device *dev;
	char *name;
	u32_t pin;
};

static struct led_device_info led_dev_info[] = {
	{ NULL, LED0_GPIO_CONTROLLER, LED0_GPIO_PIN }, /* green back LED */
	{ NULL, LED1_GPIO_CONTROLLER, LED1_GPIO_PIN }, /* red front LED */
	{ NULL, LED2_GPIO_CONTROLLER, LED2_GPIO_PIN }, /* green front LED */
	{ NULL, LED3_GPIO_CONTROLLER, LED3_GPIO_PIN }, /* blue front LED */
};

static struct device_info dev_info[] = {
	{ NULL, SW0_GPIO_CONTROLLER },
	{ NULL, DT_HDC1008_NAME },
	{ NULL, DT_FXOS8700_NAME },
	{ NULL, DT_APDS9960_DRV_NAME },
	{ NULL, DT_SSD1673_DEV_NAME },
};

static void configure_gpios(void)
{
	gpio_pin_configure(led_dev_info[DEV_IDX_LED0].dev,
			   led_dev_info[DEV_IDX_LED0].pin, GPIO_DIR_OUT);
	gpio_pin_write(led_dev_info[DEV_IDX_LED0].dev,
		       led_dev_info[DEV_IDX_LED0].pin, 1);

	gpio_pin_configure(led_dev_info[DEV_IDX_LED1].dev,
			   led_dev_info[DEV_IDX_LED1].pin, GPIO_DIR_OUT);
	gpio_pin_write(led_dev_info[DEV_IDX_LED1].dev,
		       led_dev_info[DEV_IDX_LED1].pin, 1);

	gpio_pin_configure(led_dev_info[DEV_IDX_LED2].dev,
			   led_dev_info[DEV_IDX_LED2].pin, GPIO_DIR_OUT);
	gpio_pin_write(led_dev_info[DEV_IDX_LED2].dev,
		       led_dev_info[DEV_IDX_LED2].pin, 1);

	gpio_pin_configure(led_dev_info[DEV_IDX_LED3].dev,
			   led_dev_info[DEV_IDX_LED3].pin, GPIO_DIR_OUT);
	gpio_pin_write(led_dev_info[DEV_IDX_LED3].dev,
		       led_dev_info[DEV_IDX_LED3].pin, 1);
}

int set_led_state(u8_t id, bool state)
{
	/* Invert state because of active low state for GPIO LED pins */
	return gpio_pin_write(led_dev_info[id].dev, led_dev_info[id].pin,
			      !state);
}

int get_hdc1010_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_HDC1010].dev)) {
		printk("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_HDC1010].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_HDC1010].dev,
			       SENSOR_CHAN_AMBIENT_TEMP, &val[0])) {
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_HDC1010].dev,
			       SENSOR_CHAN_HUMIDITY, &val[1])) {
		return -1;
	}

	return 0;
}

int get_mma8652_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_MMA8652].dev)) {
		printk("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_MMA8652].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_MMA8652].dev,
			       SENSOR_CHAN_ACCEL_XYZ, &val[0])) {
		return -1;
	}

	return 0;
}

int get_apds9960_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_APDS9960].dev)) {
		printk("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_APDS9960].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_APDS9960].dev,
			       SENSOR_CHAN_LIGHT, &val[0])) {
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_APDS9960].dev,
			       SENSOR_CHAN_PROX, &val[1])) {
		return -1;
	}

	return 0;
}

int periphs_init(void)
{
	unsigned int i;

	/* Bind sensors */
	for (i = 0; i < ARRAY_SIZE(dev_info); i++) {
		dev_info[i].dev = device_get_binding(dev_info[i].name);
		if (dev_info[i].dev == NULL) {
			printk("Failed to get %s device\n", dev_info[i].name);
			return -EBUSY;
		}
	}

	/* Bind leds */
	for (i = 0; i < ARRAY_SIZE(led_dev_info); i++) {
		led_dev_info[i].dev = device_get_binding(led_dev_info[i].name);
		if (led_dev_info[i].dev == NULL) {
			printk("Failed to get %s led device\n",
			       led_dev_info[i].name);
			return -EBUSY;
		}
	}

	configure_gpios();
	return 0;
}
