/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>

#define DELAY 20000000
#define COUNTER DT_ALIAS(counter)

struct counter_top_cfg top_cfg;
static void test_counter_interrupt_fn(const struct device *counter_dev,
				      void *user_data)
{
	int err;
	uint32_t current_top = counter_get_top_value(counter_dev);
	uint32_t ticks_per_sec = counter_us_to_ticks(counter_dev,
						     USEC_PER_SEC);

	printk("!!! Counter Top reached %u !!!\n", current_top);

	top_cfg.flags = COUNTER_TOP_CFG_RESET_WHEN_LATE;
	top_cfg.ticks = current_top - (ticks_per_sec * 2);
	top_cfg.callback = test_counter_interrupt_fn;
	top_cfg.user_data = &top_cfg;

	err = counter_set_top_value(counter_dev, &top_cfg);
	printk("Set New top in %u sec (%u ticks)\n",
		(uint32_t)(counter_ticks_to_us(counter_dev,
			   top_cfg.ticks) / USEC_PER_SEC),
		top_cfg.ticks);

	if (-EINVAL == err) {
		printk("top settings invalid\n");
	} else if (-ENOTSUP == err) {
		printk("top setting request not supported\n");
	} else if (-EBUSY == err) {
		printk("alarm is active\n");
	} else if (err != 0) {
		printk("Error %d\n", err);
	}
}

int main(void)
{
	const struct device *const counter_dev = DEVICE_DT_GET(COUNTER);
	int err;

	printk("Counter sample\n\n");

	if (!device_is_ready(counter_dev)) {
		printk("device not ready.\n");
		return 0;
	}

	top_cfg.flags = 0;
	top_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
	top_cfg.callback = test_counter_interrupt_fn;
	top_cfg.user_data = &top_cfg;

	err = counter_set_top_value(counter_dev, &top_cfg);

	if (-EINVAL == err) {
		printk("top settings invalid\n");
	} else if (-ENOTSUP == err) {
		printk("top setting request not supported\n");
	} else if (-EBUSY == err) {
		printk("alarm is active\n");
	} else if (err != 0) {
		printk("Error %d\n", err);
	}

	printk("Set counter top in %u sec (%u ticks)\n",
		(uint32_t)(counter_ticks_to_us(counter_dev,
			   top_cfg.ticks) / USEC_PER_SEC),
		top_cfg.ticks);

	counter_start(counter_dev);

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
