/*
 * SPDX-FileCopyrightText: Copyright (c) 2023-2026 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef CONFIG_CAN_FAKE
#include <zephyr/drivers/can/can_fake.h>
DEFINE_FFF_GLOBALS;
#endif /* CONFIG_CAN_FAKE */

int main(void)
{
	const struct device *can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

	if (!device_is_ready(can)) {
		return -ENODEV;
	}

	return 0;
}
