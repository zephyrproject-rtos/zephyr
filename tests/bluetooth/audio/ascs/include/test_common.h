/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#define test_ase_snk_get(_num_ase, ...) test_ase_get(BT_UUID_ASCS_ASE_SNK, _num_ase, __VA_ARGS__)
#define test_ase_src_get(_num_ase, ...) test_ase_get(BT_UUID_ASCS_ASE_SRC, _num_ase, __VA_ARGS__)

struct test_ase_chrc_value_hdr {
	uint8_t ase_id;
	uint8_t ase_state;
	uint8_t params[0];
} __packed;

void test_mocks_init(void);
void test_mocks_cleanup(void);
void test_mocks_reset(void);

/* Initialize connection object for test */
void test_conn_init(struct bt_conn *conn);
uint8_t test_ase_get(const struct bt_uuid *uuid, int num_ase, ...);
uint8_t test_ase_id_get(const struct bt_gatt_attr *ase);
const struct bt_gatt_attr *test_ase_control_point_get(void);

/* client-initiated ASE Control Operations */
void test_ase_control_client_config_codec(struct bt_conn *conn, uint8_t ase_id,
					  struct bt_bap_stream *stream);
void test_ase_control_client_config_qos(struct bt_conn *conn, uint8_t ase_id);
void test_ase_control_client_enable(struct bt_conn *conn, uint8_t ase_id);
void test_ase_control_client_disable(struct bt_conn *conn, uint8_t ase_id);
void test_ase_control_client_release(struct bt_conn *conn, uint8_t ase_id);
void test_ase_control_client_update_metadata(struct bt_conn *conn, uint8_t ase_id);
