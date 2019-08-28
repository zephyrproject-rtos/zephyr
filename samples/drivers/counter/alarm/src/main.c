/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <drivers/counter.h>
#include <sys/printk.h>

#define DELAY 2000000
#define ALARM_CHANNEL_ID 0

struct counter_alarm_cfg alarm_cfg;

static void test_counter_interrupt_fn(struct device *counter_dev,
				      u8_t chan_id, u32_t ticks,
				      void *user_data)
{
	u32_t now_ticks = counter_read(counter_dev);
	u64_t now_usec = counter_ticks_to_us(counter_dev, now_ticks);
	int now_sec = (int)(now_usec / USEC_PER_SEC);
	struct counter_alarm_cfg *config = user_data;
	int err;

	printk("!!! Alarm !!!\n");
	printk("Now: %u\n", now_sec);

	/* Set a new alarm with a double length duration */
	config->ticks = config->ticks * 2U;

	printk("Set alarm in %u sec (%u ticks)\n",
	       (u32_t)(counter_ticks_to_us(counter_dev,
					   config->ticks) / USEC_PER_SEC),
	       config->ticks);

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
					user_data);
	if (err != 0) {
		printk("Alarm could not be set\n");
	}
}

void main(void)
{
	struct device *counter_dev;
	int err;

	printk("Counter alarm sample\n\n");
	counter_dev = device_get_binding(DT_RTC_0_NAME);
	if (counter_dev == NULL) {
		printk("Device not found\n");
		return;
	}

	counter_start(counter_dev);

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
	alarm_cfg.callback = test_counter_interrupt_fn;
	alarm_cfg.user_data = &alarm_cfg;

	err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
					&alarm_cfg);
	printk("Set alarm in %u sec (%u ticks)\n",
	       (u32_t)(counter_ticks_to_us(counter_dev,
					   alarm_cfg.ticks) / USEC_PER_SEC),
	       alarm_cfg.ticks);

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
