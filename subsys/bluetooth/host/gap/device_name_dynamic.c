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

#include "../settings.h"

struct bt_gap_device_name_s {
	uint8_t buf[CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC_MAX];
	size_t size;
};

static K_MUTEX_DEFINE(bt_gap_device_name_lock);
static struct bt_gap_device_name_s bt_gap_device_name;

LOG_MODULE_REGISTER(bt_gap_device_name, CONFIG_BT_GAP_LOG_LEVEL);

int bt_gap_get_device_name(uint8_t *buf, size_t size)
{
	k_mutex_lock(&bt_gap_device_name_lock, K_FOREVER);

	if (size > bt_gap_device_name.size) {
		LOG_DBG("Device name is too big for the provided buffer.");
		return -ENOMEM;
	}

	memcpy(buf, bt_gap_device_name.buf, bt_gap_device_name.size);

	k_mutex_unlock(&bt_gap_device_name_lock);

	return bt_gap_device_name.size;
}

int bt_gap_set_device_name(const uint8_t *buf, size_t size)
{
	int err;

	if (size > BT_GAP_DEVICE_NAME_MAX_SIZE) {
		return -ENOBUFS;
	}

	k_mutex_lock(&bt_gap_device_name_lock, K_FOREVER);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = bt_settings_store_name(buf, size);
		if (err) {
			LOG_ERR("Unable to store name (err %d)", err);
			k_mutex_unlock(&bt_gap_device_name_lock);
			return -EIO;
		}
	}

	memcpy(bt_gap_device_name.buf, buf, size);
	bt_gap_device_name.size = size;

	k_mutex_unlock(&bt_gap_device_name_lock);

	return 0;
}
