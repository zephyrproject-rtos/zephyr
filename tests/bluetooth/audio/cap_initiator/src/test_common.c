/* test_common.c - common procedures for unit test of CAP initiator */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>

#include "bap_endpoint.h"
#include "cap_initiator.h"
#include "conn.h"
#include "test_common.h"
#include "ztest_assert.h"

void test_mocks_init(void)
{
	mock_cap_initiator_init();
}

void test_mocks_cleanup(void)
{
	mock_cap_initiator_cleanup();
}

void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_CENTRAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}

void test_unicast_set_state(struct bt_cap_stream *cap_stream, struct bt_conn *conn,
			    struct bt_bap_ep *ep, struct bt_bap_lc3_preset *preset,
			    enum bt_bap_ep_state state)
{
	struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;

	printk("Setting stream %p to state %d\n", bap_stream, state);

	if (state == BT_BAP_EP_STATE_IDLE) {
		return;
	}

	zassert_not_null(cap_stream);
	zassert_not_null(conn);
	zassert_not_null(ep);
	zassert_not_null(preset);

	bap_stream->conn = conn;
	bap_stream->ep = ep;
	bap_stream->qos = &preset->qos;
	bap_stream->codec_cfg = &preset->codec_cfg;
	bap_stream->ep->status.state = state;
}
