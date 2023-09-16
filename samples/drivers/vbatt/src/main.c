/*
 * Copyright (c) 2023 Mark Kettner <mark@kettner.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>
#include <zephyr/kernel.h>

#define BAT_NODE DT_ALIAS(battery)
static const struct device *bat_dev = DEVICE_DT_GET(BAT_NODE);

int main(void)
{
	int rc = 0;
	int mea;

	for (;;) {
		usbc_vbus_measure(bat_dev, &mea);
		printf("bat=%i.%iV", mea / 100, mea % 100);
		k_sleep(K_SECONDS(5));
	}
}
