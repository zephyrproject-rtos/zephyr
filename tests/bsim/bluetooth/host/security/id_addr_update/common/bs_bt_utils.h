/**
 * Common functions and helpers for this test
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/types.h>

void bs_bt_utils_setup(void);

void clear_conn(struct bt_conn *conn);
void wait_connected(struct bt_conn **conn);
void wait_disconnected(void);
void disconnect(struct bt_conn *conn);
void scan_connect_to_first_result(void);
void advertise_connectable(int id);

void set_security(struct bt_conn *conn, bt_security_t sec);
void wait_pairing_completed(void);

void bas_subscribe(struct bt_conn *conn);
void wait_bas_ccc_subscription(void);
void bas_notify(struct bt_conn *conn);
void wait_bas_notification(void);
