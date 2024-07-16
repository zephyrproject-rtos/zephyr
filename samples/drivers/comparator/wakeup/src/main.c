/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <stdio.h>

const struct device *sample_comparator = DEVICE_DT_GET(DT_ALIAS(comparator));

int main(void)
{
	int ret;

	ret = comparator_set_trigger(sample_comparator, COMPARATOR_TRIGGER_BOTH_EDGES);
	if (ret < 0) {
		printf("failed to set comparator trigger (ret = %i)\n", ret);
	}

	printf("delay before poweroff\n");
	k_msleep(5000);

	printf("check if trigger is pending to clear it\n");
	ret = comparator_trigger_is_pending(sample_comparator);
	if (ret == 1) {
		printf("comparator trigger was %s\n", "pending");
	} else if (ret == 0) {
		printf("comparator trigger was %s\n", "not pending");
	} else {
		printf("failed to check if comparator trigger was pending (ret = %i)\n", ret);
	}

	printf("poweroff\n");
	sys_poweroff();
	return 0;
}
