/* gatt_gap_svc_validate.c - GAP service inside GATT validation functions */

/*
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_BT_GATT_LOG_LEVEL
LOG_MODULE_DECLARE(bt_gatt);


static uint8_t gap_service_check_cb(const struct bt_gatt_attr *attr,
				    uint16_t handle,
				    void *user_data)
{
	size_t *cnt = user_data;
	const struct bt_uuid *svc_uuid = attr->user_data;

	if (svc_uuid != NULL && bt_uuid_cmp(svc_uuid, BT_UUID_GAP) == 0) {
		(*cnt)++;
		LOG_DBG("GAP service found at handle: %u", handle);
	}

	return BT_GATT_ITER_CONTINUE;
}

int gatt_gap_svc_validate(void)
{
	size_t gap_svc_count = 0U;

	bt_gatt_foreach_attr_type(
		BT_ATT_FIRST_ATTRIBUTE_HANDLE,
		BT_ATT_LAST_ATTRIBUTE_HANDLE,
		BT_UUID_GATT_PRIMARY,
		NULL, 0,
		gap_service_check_cb,
		&gap_svc_count);

	if (gap_svc_count != 1U) {
		LOG_DBG("GAP service count invalid: %zu", gap_svc_count);
		return -EINVAL;
	}
	return 0;
}
