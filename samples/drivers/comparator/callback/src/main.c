/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <stdio.h>

const struct device *sample_comparator = DEVICE_DT_GET(DT_ALIAS(comparator));

static void sample_get_output_handler(struct k_work *work)
{
	int ret;

	ARG_UNUSED(work);

	ret = comparator_get_output(sample_comparator);
	if (ret == 1) {
		printf("comparator output is %s\n", "high");
	} else if (ret == 0) {
		printf("comparator output is %s\n", "low");
	} else {
		printf("failed to get comparator output (ret = %i)\n", ret);
	}
}

K_WORK_DEFINE(sample_get_output_work, sample_get_output_handler);

static void sample_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	k_work_submit(&sample_get_output_work);
}

int main(void)
{
	int ret;

	ret = comparator_set_trigger(sample_comparator, COMPARATOR_TRIGGER_BOTH_EDGES);
	if (ret < 0) {
		printf("failed to set comparator trigger (ret = %i)\n", ret);
	}

	ret = comparator_set_trigger_callback(sample_comparator,
					      sample_callback,
					      NULL);
	if (ret < 0) {
		printf("failed to set comparator callback (ret = %i)\n", ret);
	}

	k_work_submit(&sample_get_output_work);
	return 0;
}
