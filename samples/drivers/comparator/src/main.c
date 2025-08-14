/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/comparator.h>

void comp_callback(const struct device *dev, void *user_data)
{
	int val;

	val = comparator_get_output(dev);
	printk("Comparator triggered! Output: %d\n", val);
}

int main(void)
{
	const struct device *comp = DEVICE_DT_GET(DT_NODELABEL(comp0));
	int ret;

	if (!device_is_ready(comp)) {
		printk("Comparator not ready\n");
		return 0;
	}

	comparator_set_trigger_callback(comp, comp_callback, NULL);

	ret = comparator_set_trigger(comp, COMPARATOR_TRIGGER_RISING_EDGE);
	if (ret < 0) {
		printk("failed to set trigger\n");
		return 0;
	}

	return 0;
}
