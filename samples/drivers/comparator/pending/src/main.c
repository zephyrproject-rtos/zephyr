/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <stdio.h>

const struct device *sample_comparator = DEVICE_DT_GET(DT_ALIAS(comparator));

int main(void)
{
	int ret;

	ret = comparator_set_trigger(sample_comparator, COMPARATOR_TRIGGER_BOTH_EDGES);
	if (ret < 0) {
		printf("failed to set comparator trigger (ret = %i)\n", ret);
	}

	while (1) {
		k_msleep(1000);
		ret = comparator_trigger_is_pending(sample_comparator);
		if (ret == 1) {
			printf("comparator trigger is %s\n", "pending");
		} else if (ret == 0) {
			printf("comparator trigger is %s\n", "not pending");
		} else {
			printf("failed to check if trigger is pending (ret = %i)\n", ret);
		}
	}

	return 0;
}
