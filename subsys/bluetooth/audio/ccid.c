/*  Bluetooth Audio Content Control */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include "ccid_internal.h"

uint8_t bt_ccid_get_value(void)
{
	static uint8_t ccid_value;

	/* By spec, the CCID can take all values up to and including 0xFF.
	 * But since this is a value we provide, we do not have to use all of
	 * them.  254 CCID values on a device should be plenty, the last one
	 * can be used to prevent wraparound.
	 */
	__ASSERT(ccid_value != UINT8_MAX,
		 "Cannot allocate any more control control IDs");

	return ccid_value++;
}

struct ccid_search_param {
	const struct bt_gatt_attr *attr;
	uint8_t ccid;
};

static uint8_t ccid_attr_cb(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
	struct ccid_search_param *search_param = user_data;

	if (attr->read != NULL) {
		uint8_t ccid = 0U;
		ssize_t res;

		res = attr->read(NULL, attr, &ccid, sizeof(ccid), 0);

		if (res == sizeof(ccid) && search_param->ccid == ccid) {
			search_param->attr = attr;

			return BT_GATT_ITER_STOP;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

const struct bt_gatt_attr *bt_ccid_find_attr(uint8_t ccid)
{
	struct ccid_search_param search_param = {
		.attr = NULL,
		.ccid = ccid,
	};

	bt_gatt_foreach_attr_type(BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
				  BT_UUID_CCID, NULL, 0, ccid_attr_cb, &search_param);

	return search_param.attr;
}
