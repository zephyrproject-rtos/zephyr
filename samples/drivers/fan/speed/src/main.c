/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fan.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define FAN_NODE DT_ALIAS(fan0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(FAN_NODE),
	     "No default fan specified in DT (add `aliases { fan0 = ... };`)");

/* Speed steps, as a percentage of full speed, to cycle through. */
static const uint8_t speed_steps[] = {0U, 25U, 50U, 75U, 100U};

int main(void)
{
	const struct device *fan = DEVICE_DT_GET(FAN_NODE);

	if (!device_is_ready(fan)) {
		printk("Fan device %s not ready\n", fan->name);
		return 0;
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(speed_steps); i++) {
			uint8_t percent = speed_steps[i];
			int ret;

			ret = fan_set_speed(fan, percent);
			if (ret < 0) {
				printk("Failed to set fan speed to %u%% (%d)\n", percent, ret);
				continue;
			}

			printk("Fan speed set to %u%%\n", percent);

			/* Give the fan a moment to reach the new speed. */
			k_sleep(K_SECONDS(2));
		}
	}
	return 0;
}
