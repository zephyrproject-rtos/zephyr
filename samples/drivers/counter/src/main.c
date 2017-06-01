/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <counter.h>
#include <misc/printk.h>

#define ALARM (32768)
#define ALARM_2 (5*32768)

uint32_t some_user_data;

void counter_cb_1(struct device *counter_dev, void *user_data)
{
	u32_t now = counter_read(counter_dev);

	(*((uint32_t *)user_data))++;
	printk("Alarm 1, user data = %d\n", (*(uint32_t *)user_data));
	counter_set_alarm(counter_dev,
			  counter_cb_1,
			  now + ALARM,
			  user_data);
}

void counter_cb_2(struct device *counter_dev, void *user_data)
{
	u32_t now = counter_read(counter_dev);

	printk("Alarm 2\n");
	counter_set_alarm(counter_dev, counter_cb_2, now + ALARM_2, NULL);
}

void main(void)
{
	int err;
	int ad; /*Alarm descriptor */
	struct counter_config config;
	struct device *counter_dev;

	printk("Counter driver test\n");

#if defined(CONFIG_SOC_FAMILY_NRF5)
	counter_dev = device_get_binding(CONFIG_RTC_0_NAME);
#endif

	config.divider = 1;
	config.init_val = 0;

	err = counter_config(counter_dev, &config);
	if (!err) {
		printk("Counter configured\n");
	} else {
		printk("Error %d while configuring counter\n", err);
	}

	err = counter_start(counter_dev);
	if (!err) {
		printk("Counter started\n");
	} else {
		printk("Error %d while starting counter\n", err);
	}

	ad = counter_set_alarm(counter_dev,
			  counter_cb_1,
			  counter_read(counter_dev) + ALARM,
			  (void *)&some_user_data);
	if (ad >= 0) {
		printk("Alarm set, alarm descriptor = %d\n", ad);
	} else {
		printk("Error %d while setting the alarm\n", err);
	}

	ad = counter_set_alarm(counter_dev,
			  counter_cb_2,
			  counter_read(counter_dev) + ALARM_2,
			  NULL);
	if (ad >= 0) {
		printk("Alarm set, alarm descriptor = %d\n", ad);
	} else {
		printk("Error %d while setting the alarm\n", err);
	}

	while (1) {
		/* do nothing */
	}
}
