/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_GATT_H_
#define MOCKS_GATT_H_
#include <stdint.h>

#include <zephyr/fff.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

void mock_bt_gatt_init(void);
void mock_bt_gatt_cleanup(void);

DECLARE_FAKE_VALUE_FUNC(int, mock_bt_gatt_notify_cb, struct bt_conn *,
			struct bt_gatt_notify_params *);
DECLARE_FAKE_VALUE_FUNC(bool, mock_bt_gatt_is_subscribed, struct bt_conn *,
			const struct bt_gatt_attr *, uint16_t);

void bt_gatt_notify_cb_reset(void);
uint16_t bt_gatt_get_mtu(struct bt_conn *conn);
int bt_gatt_service_register(struct bt_gatt_service *svc);
int bt_gatt_service_unregister(struct bt_gatt_service *svc);

#endif /* MOCKS_GATT_H_ */
