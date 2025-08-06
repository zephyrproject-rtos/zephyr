/* test_common.h */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>

#include "conn.h"

#define TEST_COMMON_ADV_TYPE     BT_ADDR_LE_PUBLIC
#define TEST_COMMON_ADV_SID      0x01U
#define TEST_COMMON_BROADCAST_ID 0x123456U
#define TEST_COMMON_SRC_ID       0x00U

void test_mocks_init(void);
void test_mocks_cleanup(void);
void mock_bt_csip_cleanup(void);

void test_conn_init(struct bt_conn *conn, uint8_t index);

void test_unicast_set_state(struct bt_cap_stream *cap_stream, struct bt_conn *conn,
			    struct bt_bap_ep *ep, struct bt_bap_lc3_preset *preset,
			    enum bt_bap_ep_state state);

void mock_unicast_client_discover(
	struct bt_conn *conn, struct bt_bap_ep *snk_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT],
	struct bt_bap_ep *src_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT]);
