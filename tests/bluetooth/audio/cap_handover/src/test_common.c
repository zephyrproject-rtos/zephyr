/* test_common.c - common procedures for unit test of CAP handover */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

#include "audio/bap_endpoint.h"
#include "cap_initiator.h"
#include "cap_handover.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

DEFINE_FFF_GLOBALS;

void test_mocks_init(void)
{
	mock_cap_initiator_init();
	mock_cap_handover_init();
}

void test_mocks_cleanup(void)
{
	mock_bt_csip_cleanup();
}

void test_conn_init(struct bt_conn *conn, uint8_t index)
{
	conn->index = index;
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
	bap_stream->ep->state = state;
}

DECLARE_FAKE_VOID_FUNC(mock_bap_discover_endpoint, struct bt_conn *, enum bt_audio_dir,
		       struct bt_bap_ep *);
DEFINE_FAKE_VOID_FUNC(mock_bap_discover_endpoint, struct bt_conn *, enum bt_audio_dir,
		      struct bt_bap_ep *);

void mock_unicast_client_discover(
	struct bt_conn *conn, struct bt_bap_ep *snk_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT],
	struct bt_bap_ep *src_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT])
{
	struct bt_bap_unicast_client_cb unicast_client_cb = {
		.endpoint = mock_bap_discover_endpoint,
	};
	int err;

	err = bt_bap_unicast_client_register_cb(&unicast_client_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	RESET_FAKE(mock_bap_discover_endpoint);

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("unicast_client_cb.bap_discover_endpoint",
			   CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
			   mock_bap_discover_endpoint_fake.call_count);
	for (size_t i = 0U; i < mock_bap_discover_endpoint_fake.call_count; i++) {
		/* Verify conn */
		zassert_equal(mock_bap_discover_endpoint_fake.arg0_history[i], conn, "%p",
			      mock_bap_discover_endpoint_fake.arg0_history[i]);

		/* Verify dir */
		zassert_equal(mock_bap_discover_endpoint_fake.arg1_history[i], BT_AUDIO_DIR_SINK,
			      "%d", mock_bap_discover_endpoint_fake.arg1_history[i]);

		/* Verify and store ep */
		zassert_not_equal(mock_bap_discover_endpoint_fake.arg2_history[i], NULL, "%p",
				  mock_bap_discover_endpoint_fake.arg2_history[i]);

		snk_eps[i] = mock_bap_discover_endpoint_fake.arg2_history[i];

		zassert_equal(bt_bap_ep_get_conn(snk_eps[i]), conn, "Unexpected conn %p != %p",
			      bt_bap_ep_get_conn(snk_eps[i]), conn);
	}

	RESET_FAKE(mock_bap_discover_endpoint);
	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("unicast_client_cb.bap_discover_endpoint",
			   CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT,
			   mock_bap_discover_endpoint_fake.call_count);
	for (size_t i = 0U; i < mock_bap_discover_endpoint_fake.call_count; i++) {
		/* Verify conn */
		zassert_equal(mock_bap_discover_endpoint_fake.arg0_history[i], conn, "%p",
			      mock_bap_discover_endpoint_fake.arg0_history[i]);

		/* Verify dir */
		zassert_equal(mock_bap_discover_endpoint_fake.arg1_history[i], BT_AUDIO_DIR_SOURCE,
			      "%d", mock_bap_discover_endpoint_fake.arg1_history[i]);

		/* Verify and store ep */
		zassert_not_equal(mock_bap_discover_endpoint_fake.arg2_history[i], NULL, "%p",
				  mock_bap_discover_endpoint_fake.arg2_history[i]);

		src_eps[i] = mock_bap_discover_endpoint_fake.arg2_history[i];

		zassert_equal(bt_bap_ep_get_conn(src_eps[i]), conn, "Unexpected conn %p != %p",
			      bt_bap_ep_get_conn(src_eps[i]), conn);
	}

	/* We don't need the callbacks anymore */
	err = bt_bap_unicast_client_unregister_cb(&unicast_client_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}
