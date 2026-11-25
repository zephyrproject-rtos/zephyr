/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(brownout));

	if (device_is_ready(dev) == false) {
		return 1;
	}

	return 0;
}
