/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <counter.h>
#include <misc/printk.h>

#define DELAY 2000000
#define ALARM_CHANNEL_ID 0

struct counter_alarm_cfg alarm_cfg;

static void test_counter_interrupt_fn(struct device *counter_dev,
				u8_t chan_id, u32_t ticks, void *user_data)
{
	u32_t now_ticks = counter_read(counter_dev);
	u64_t now_usec = counter_ticks_to_us(counter_dev, now_ticks);
	int now_sec = (int)(now_usec / USEC_PER_SEC);
	struct counter_alarm_cfg *config = user_data;

	printk("!!! Alarm !!!\n");
	printk("Now: %d\n", now_sec);

	/* Set a new alarm with a double lenght duration */
	config->ticks = 2 * config->ticks;

	printk("Set alarm in %d sec\n", config->ticks);
	counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, user_data);
}

void main(void)
{
	struct device *counter_dev;
	int err = 0;

	printk("Counter alarm sample\n\n");
	counter_dev = device_get_binding(DT_RTC_0_NAME);

	counter_start(counter_dev);

	alarm_cfg.absolute = false;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
	alarm_cfg.callback = test_counter_interrupt_fn;
	alarm_cfg.user_data = &alarm_cfg;

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
					&alarm_cfg);
	printk("Set alarm in %d sec\n", alarm_cfg.ticks);

	if (-EINVAL == err) {
		printk("Alarm settings invalid\n");
	} else if (-ENOTSUP == err) {
		printk("Alarm setting request not supported\n");
	} else if (err != 0) {
		printk("Error\n");
	}

	while (1) {
		k_sleep(K_FOREVER);
	}
}
