/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <gpio.h>
#include <sensor.h>
#include "board.h"
#include "mesh.h"

#include <bluetooth/mesh.h>

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
	{ NULL, DT_TI_HDC1010_0_LABEL },
	{ NULL, DT_NXP_MMA8652FC_0_LABEL },
	{ NULL, DT_AVAGO_APDS9960_0_LABEL },
	{ NULL, DT_SOLOMON_SSD1673FB_0_LABEL },
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

#define MOTION_TIMEOUT K_MINUTES(30)

static struct k_delayed_work motion_work;

static void motion_timeout(struct k_work *work)
{
	int err;

	printk("power save\n");

	if (!mesh_is_initialized()) {
		k_delayed_work_submit(&motion_work, MOTION_TIMEOUT);
		return;
	}

	err = bt_mesh_suspend();
	if (err && err != -EALREADY) {
		printk("failed to suspend mesh (err %d)\n", err);
	}
}

static void motion_handler(struct device *dev, struct sensor_trigger *trig)
{
	int err;

	printk("motion!\n");

	if (!mesh_is_initialized()) {
		return;
	}

	err = bt_mesh_resume();
	if (err && err != -EALREADY) {
		printk("failed to resume mesh (err %d)\n", err);
	}

	k_delayed_work_submit(&motion_work, MOTION_TIMEOUT);
}

static void configure_accel(void)
{
	struct device_info *accel = &dev_info[DEV_IDX_MMA8652];
	struct sensor_trigger trig_motion = {
		.type = SENSOR_TRIG_DELTA,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};
	int err;

	err = sensor_trigger_set(accel->dev, &trig_motion, motion_handler);
	if (err) {
		printk("setting motion trigger failed, err %d\n", err);
		return;
	}


	k_delayed_work_init(&motion_work, motion_timeout);
	k_delayed_work_submit(&motion_work, MOTION_TIMEOUT);
}

int periphs_init(void)
{
	unsigned int i;

	/* Bind sensors */
	for (i = 0U; i < ARRAY_SIZE(dev_info); i++) {
		dev_info[i].dev = device_get_binding(dev_info[i].name);
		if (dev_info[i].dev == NULL) {
			printk("Failed to get %s device\n", dev_info[i].name);
			return -EBUSY;
		}
	}

	/* Bind leds */
	for (i = 0U; i < ARRAY_SIZE(led_dev_info); i++) {
		led_dev_info[i].dev = device_get_binding(led_dev_info[i].name);
		if (led_dev_info[i].dev == NULL) {
			printk("Failed to get %s led device\n",
			       led_dev_info[i].name);
			return -EBUSY;
		}
	}

	configure_gpios();

	configure_accel();

	return 0;
}
