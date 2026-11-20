/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

int main(void)
{
	const struct device *uhc = DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0));

	printk("DWC3 xHCI host sample\n");

	if (!device_is_ready(uhc)) {
		printk("UHC device not ready\n");
		return 1;
	}

	printk("DWC3 UHC device ready\n");

	return 0;
}
