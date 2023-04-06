/* test_ase_state_transition.c - ASE state transition tests */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/util_macro.h>

#include "bap_unicast_server.h"
#include "bap_unicast_server_expects.h"
#include "bap_stream.h"
#include "bap_stream_expects.h"
#include "conn.h"
#include "gatt.h"
#include "gatt_expects.h"
#include "iso.h"

#include "test_common.h"

struct test_sink_ase_state_transition_fixture {
	struct bt_conn conn;
	struct bt_bap_stream stream;
	struct {
		uint8_t id;
		const struct bt_gatt_attr *attr;
	} ase;
};

static void *test_sink_ase_state_transition_setup(void)
{
	struct test_sink_ase_state_transition_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);
	test_ase_snk_get(1, &fixture->ase.attr);
	if (fixture->ase.attr != NULL) {
		fixture->ase.id = test_ase_id_get(fixture->ase.attr);
	}

	return fixture;
}

static void test_ase_state_transition_before(void *f)
{
	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
}

static void test_ase_state_transition_after(void *f)
{
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

static void test_ase_state_transition_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(test_sink_ase_state_transition, NULL, test_sink_ase_state_transition_setup,
	    test_ase_state_transition_before, test_ase_state_transition_after,
	    test_ase_state_transition_teardown);

ZTEST_F(test_sink_ase_state_transition, test_client_idle_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_ase_control_client_config_codec(conn, ase_id, stream);
	expect_bt_bap_unicast_server_cb_config_called_once(conn, EMPTY, BT_AUDIO_DIR_SINK, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_sink_ase_state_transition, test_client_codec_configured_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_config_qos(conn, ase_id);
	expect_bt_bap_unicast_server_cb_qos_called_once(stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_enable(conn, ase_id);
	expect_bt_bap_unicast_server_cb_enable_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_enabled_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_enabling_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_disable(conn, ase_id);
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_enabling_to_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	err = bt_bap_stream_start(stream);
	zassert_equal(0, err, "Failed to start stream: err %d", err);

	/* XXX: unicast_server_cb->start is not called for Sink ASE */
	expect_bt_bap_stream_ops_started_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_codec_configured_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_config_codec(conn, ase_id, stream);
	expect_bt_bap_unicast_server_cb_config_not_called();
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SINK, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_config_qos(conn, ase_id);
	expect_bt_bap_unicast_server_cb_qos_called_once(stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_config_codec(conn, ase_id, stream);
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SINK, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_sink_ase_state_transition, test_client_codec_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_enabling_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_enabling_to_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_update_metadata(conn, ase_id);
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_streaming_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_ase_control_client_release(conn, ase_id);

	/* Client disconnects the ISO */
	mock_bt_iso_disconnect(chan);

	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_streaming_to_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_ase_control_client_update_metadata(conn, ase_id);
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_streaming_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_ase_control_client_disable(conn, ase_id);
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
}
