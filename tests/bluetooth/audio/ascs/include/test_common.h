/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/* Initialize connection object for test */
void test_conn_init(struct bt_conn *conn);
const struct bt_gatt_attr *test_ase_control_point_get(void);

/* client-initiated ASE Control Operations */
void test_ase_control_client_config_codec(struct bt_conn *conn, struct bt_bap_stream *stream);
void test_ase_control_client_config_qos(struct bt_conn *conn);
void test_ase_control_client_enable(struct bt_conn *conn);
void test_ase_control_client_disable(struct bt_conn *conn);
void test_ase_control_client_release(struct bt_conn *conn);
void test_ase_control_client_update_metadata(struct bt_conn *conn);
