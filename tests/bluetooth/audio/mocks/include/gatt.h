/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_GATT_H_
#define MOCKS_GATT_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/gatt.h>

void mock_bt_gatt_init(void);
void mock_bt_gatt_cleanup(void);

DECLARE_FAKE_VALUE_FUNC(int, mock_bt_gatt_notify_cb, struct bt_conn *,
			struct bt_gatt_notify_params *);
DECLARE_FAKE_VALUE_FUNC(ssize_t, bt_gatt_attr_read, struct bt_conn *,
			const struct bt_gatt_attr *, void *, uint16_t, uint16_t, const void *,
			uint16_t);

void bt_gatt_notify_cb_reset(void);

#endif /* MOCKS_GATT_H_ */
