/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/gap/device_name.h>

LOG_MODULE_REGISTER(bt_gap_device_name, CONFIG_BT_GAP_LOG_LEVEL);

int bt_gap_get_device_name(uint8_t *buf, size_t size)
{
	size_t name_size = sizeof(CONFIG_BT_GAP_DEVICE_NAME);

	if (size < name_size) {
		LOG_DBG("Device name is too big for the provided buffer.");
		return -ENOMEM;
	}

	memcpy(buf, CONFIG_BT_GAP_DEVICE_NAME, name_size);

	return name_size;
}

int bt_gap_set_device_name(const uint8_t *buf, size_t size)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(size);

	return 0;
}
