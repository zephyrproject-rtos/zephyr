/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_GATT_EXPECTS_H_
#define MOCKS_GATT_EXPECTS_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest_assert.h>

#include <sys/types.h>

#include "gatt.h"
#include "expects_util.h"

static inline void expect_bt_gatt_notify_cb_called_with(const struct bt_conn *conn,
							const struct bt_uuid *uuid,
							const struct bt_gatt_attr *attr,
							const uint8_t *data, ssize_t data_len)
{
	const char *func_name = "bt_gatt_notify_cb";

	for (size_t i = 0U; i < mock_bt_gatt_notify_cb_fake.call_count; i++) {
		const struct bt_gatt_notify_params *params =
			mock_bt_gatt_notify_cb_fake.arg1_history[i];

		if (conn != NULL && conn != mock_bt_gatt_notify_cb_fake.arg0_history[i]) {
			continue;
		}

		if (uuid != NULL) {
			if (params->uuid == NULL || bt_uuid_cmp(uuid, params->uuid) != 0) {
				continue;
			}
		} else if (attr != NULL) {
			if (params->attr != attr) {
				continue;
			}
		} else {
			zassert_unreachable("Either uuid or attr shall be provided");
		}

		if (data != NULL && !util_memeq(data, params->data, data_len)) {
			continue;
		}

		if (data_len != -1 && data_len != params->len) {
			continue;
		}

		/* All specified criteria matched for call i */
		return;
	}

	zassert_unreachable("'%s()' was never called with the expected arguments", func_name);
}

static inline void expect_bt_gatt_notify_cb_not_called(void)
{
	const char *func_name = "bt_gatt_notify_cb";

	zassert_equal(0, mock_bt_gatt_notify_cb_fake.call_count, "'%s()' was called unexpectedly",
		      func_name);
}

#endif /* MOCKS_GATT_EXPECTS_H_ */
