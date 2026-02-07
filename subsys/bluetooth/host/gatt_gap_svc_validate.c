/* gatt_gap_svc_validate.c - GAP service inside GATT validation functions */

/*
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>


#define MODULE gatt_gap_validate

LOG_MODULE_REGISTER(MODULE, CONFIG_GATT_GAP_SVC_VALIDATE_LOG_LEVEL);


static uint8_t gap_service_check_cb(const struct bt_gatt_attr *attr,
				    uint16_t handle,
				    void *user_data)
{
	size_t *cnt = user_data;
	const struct bt_uuid *svc_uuid = attr->user_data;

	if (svc_uuid && bt_uuid_cmp(svc_uuid, BT_UUID_GAP) == 0) {
		(*cnt)++;
		LOG_DBG("GAP service found at handle: %u", handle);
		if (*cnt > 1) {
			LOG_ERR("Multiple (%zu) GAP services found at handle: %u",
				*cnt, handle);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

int gatt_gap_svc_validate(void)
{
	size_t gap_svc_count = 0;

	bt_gatt_foreach_attr_type(
		BT_ATT_FIRST_ATTRIBUTE_HANDLE,
		BT_ATT_LAST_ATTRIBUTE_HANDLE,
		BT_UUID_GATT_PRIMARY,
		NULL, 0,
		gap_service_check_cb,
		&gap_svc_count);

	if (gap_svc_count != 1) {
		LOG_ERR("GAP service count invalid: %zu", gap_svc_count);
		return -EINVAL;
	}
	return 0;
}
