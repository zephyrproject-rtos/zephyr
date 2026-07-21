/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/conn.h>

void disconnect(struct bt_conn *conn);
void wait_disconnected(void);
struct bt_conn *get_conn(void);
struct bt_conn *connect_as_central(void);
struct bt_conn *connect_as_peripheral(void);

void set_security(struct bt_conn *conn, bt_security_t sec);
void wait_secured(void);
void bond(struct bt_conn *conn);
void wait_bonded(void);
