/* test_common.h */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>

#include "conn.h"

void test_mocks_init(void);
void test_mocks_cleanup(void);

void test_conn_init(struct bt_conn *conn, uint8_t index);

void test_unicast_set_state(struct bt_cap_stream *cap_stream, struct bt_conn *conn,
			    struct bt_bap_ep *ep, struct bt_bap_lc3_preset *preset,
			    enum bt_bap_ep_state state);
void mock_discover(
	struct bt_conn conns[CONFIG_BT_MAX_CONN],
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT],
	struct bt_bap_ep *src_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT]);
