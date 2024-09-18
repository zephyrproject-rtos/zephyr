/* test_ase_state_transition.c - ASE state transition tests */

/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Demant A/S
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

#define test_sink_ase_state_transition_fixture test_ase_state_transition_fixture
#define test_source_ase_state_transition_fixture test_ase_state_transition_fixture

static const struct bt_audio_codec_qos_pref qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000);

struct test_ase_state_transition_fixture {
	struct bt_conn conn;
	struct bt_bap_stream stream;
	struct {
		uint8_t id;
		const struct bt_gatt_attr *attr;
	} ase;
};

static void *test_sink_ase_state_transition_setup(void)
{
	struct test_ase_state_transition_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void test_ase_snk_state_transition_before(void *f)
{
	struct test_ase_state_transition_fixture *fixture =
		(struct test_ase_state_transition_fixture *) f;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
	};
	int err;

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, 0, "unexpected err response %d", err);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	zassert_equal(err, 0, "unexpected err response %d", err);

	memset(fixture, 0, sizeof(struct test_ase_state_transition_fixture));
	test_conn_init(&fixture->conn);
	test_ase_snk_get(1, &fixture->ase.attr);
	if (fixture->ase.attr != NULL) {
		fixture->ase.id = test_ase_id_get(fixture->ase.attr);
	}

	bt_bap_stream_cb_register(&fixture->stream, &mock_bap_stream_ops);
}

static void test_ase_src_state_transition_before(void *f)
{
	struct test_ase_state_transition_fixture *fixture =
		(struct test_ase_state_transition_fixture *) f;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
	};
	int err;

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, 0, "unexpected err response %d", err);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	zassert_equal(err, 0, "unexpected err response %d", err);

	memset(fixture, 0, sizeof(struct test_ase_state_transition_fixture));
	test_conn_init(&fixture->conn);
	test_ase_src_get(CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT, &fixture->ase.attr);
	if (fixture->ase.attr != NULL) {
		fixture->ase.id = test_ase_id_get(fixture->ase.attr);
	}

	bt_bap_stream_cb_register(&fixture->stream, &mock_bap_stream_ops);
}

static void test_ase_state_transition_after(void *f)
{
	int err;

	err = bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
	zassert_equal(err, 0, "unexpected err response %d", err);

	err = bt_bap_unicast_server_unregister();
	while (err != 0) {
		zassert_equal(err, -EBUSY, "unexpected err response %d", err);
		k_sleep(K_MSEC(10));
		err = bt_bap_unicast_server_unregister();
	}
}

static void test_ase_state_transition_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(test_sink_ase_state_transition, NULL, test_sink_ase_state_transition_setup,
	    test_ase_snk_state_transition_before, test_ase_state_transition_after,
	    test_ase_state_transition_teardown);

ZTEST_F(test_sink_ase_state_transition, test_client_idle_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Verification */
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

	/* Verification */
	expect_bt_bap_unicast_server_cb_qos_called_once(stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_enable(conn, ase_id);

	/* Verification */
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

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_codec_configured_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Verification */
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

	/* Verification */
	expect_bt_bap_unicast_server_cb_qos_called_once(stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_client_qos_configured_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Verification */
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

	/* Verification */
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

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_client_enabling_to_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_update_metadata(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_client_streaming_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_ase_control_client_release(conn, ase_id);

	/* Client disconnects the ISO */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_client_streaming_to_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_ase_control_client_update_metadata(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_client_streaming_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_ase_control_client_disable(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_server_idle_to_codec_configured)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	err = bt_bap_unicast_server_config_ase(conn, stream, &codec_cfg, &qos_pref);
	zassert_false(err < 0, "bt_bap_unicast_server_config_ase returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_config_not_called();
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_sink_ase_state_transition, test_server_codec_configured_to_codec_configured)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	err = bt_bap_stream_reconfig(stream, &codec_cfg);
	zassert_false(err < 0, "bt_bap_stream_reconfig returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SINK, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_sink_ase_state_transition, test_server_codec_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_server_qos_configured_to_codec_configured)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	err = bt_bap_stream_reconfig(stream, &codec_cfg);
	zassert_false(err < 0, "bt_bap_stream_reconfig returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SINK, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_sink_ase_state_transition, test_server_qos_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_server_enabling_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_server_enabling_to_enabling)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_MEDIA)),
	};
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = bt_bap_stream_metadata(stream, meta, ARRAY_SIZE(meta));
	zassert_false(err < 0, "bt_bap_stream_metadata returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_server_enabling_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = bt_bap_stream_disable(stream);
	zassert_false(err < 0, "bt_bap_stream_disable returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_server_enabling_to_streaming)
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
	zassert_false(err < 0, "bt_bap_stream_start returned err %d", err);

	/* Verification */
	expect_bt_bap_stream_ops_connected_called_once(stream);
	expect_bt_bap_stream_ops_started_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
	/* XXX: unicast_server_cb->start is not called for Sink ASE */
}

ZTEST_F(test_sink_ase_state_transition, test_server_streaming_to_streaming)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_MEDIA)),
	};
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	err = bt_bap_stream_metadata(stream, meta, ARRAY_SIZE(meta));
	zassert_false(err < 0, "bt_bap_stream_metadata returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_sink_ase_state_transition, test_server_streaming_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	err = bt_bap_stream_disable(stream);
	zassert_false(err < 0, "bt_bap_stream_disable returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_sink_ase_state_transition, test_server_streaming_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Client disconnects the ISO */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);
}

static void *test_source_ase_state_transition_setup(void)
{
	struct test_ase_state_transition_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);
	test_ase_src_get(CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT, &fixture->ase.attr);
	if (fixture->ase.attr != NULL) {
		fixture->ase.id = test_ase_id_get(fixture->ase.attr);
	}

	return fixture;
}

ZTEST_SUITE(test_source_ase_state_transition, NULL, test_source_ase_state_transition_setup,
	    test_ase_src_state_transition_before, test_ase_state_transition_after,
	    test_ase_state_transition_teardown);

ZTEST_F(test_source_ase_state_transition, test_client_idle_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Verification */
	expect_bt_bap_unicast_server_cb_config_called_once(conn, EMPTY, BT_AUDIO_DIR_SOURCE, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_source_ase_state_transition, test_client_codec_configured_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_config_qos(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_qos_called_once(stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_qos_configured_to_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_enable(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_enable_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_enabled_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_client_enabling_to_disabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_disable(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_client_qos_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_client_enabling_to_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	test_ase_control_client_receiver_start_ready(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_start_called_once(stream);
	expect_bt_bap_stream_ops_connected_called_once(stream);
	expect_bt_bap_stream_ops_started_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_codec_configured_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Verification */
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SOURCE, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_source_ase_state_transition, test_client_qos_configured_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_config_qos(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_qos_called_once(stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_qos_configured_to_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Verification */
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SOURCE, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_source_ase_state_transition, test_client_codec_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_client_enabling_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_release(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_enabling_to_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	test_ase_control_client_update_metadata(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_streaming_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_ase_control_client_release(conn, ase_id);

	/* Client disconnects the ISO */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_client_streaming_to_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_ase_control_client_update_metadata(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_streaming_to_disabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_ase_control_client_disable(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_client_enabling_to_disabling_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);
	test_ase_control_client_disable(conn, ase_id);
	expect_bt_bap_stream_ops_disabled_called_once(stream);

	test_mocks_reset();

	test_ase_control_client_receiver_stop_ready(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_stop_called_once(stream);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_client_streaming_to_disabling_to_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);
	test_ase_control_client_disable(conn, ase_id);

	/* Verify that stopped callback was called by disable */
	expect_bt_bap_stream_ops_disabled_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_REMOTE_USER_TERM_CONN);


	test_mocks_reset();

	test_ase_control_client_receiver_stop_ready(conn, ase_id);

	/* Verification */
	expect_bt_bap_unicast_server_cb_stop_called_once(stream);
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_server_idle_to_codec_configured)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	err = bt_bap_unicast_server_config_ase(conn, stream, &codec_cfg, &qos_pref);
	zassert_false(err < 0, "bt_bap_unicast_server_config_ase returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_config_not_called();
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_source_ase_state_transition, test_server_codec_configured_to_codec_configured)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	err = bt_bap_stream_reconfig(stream, &codec_cfg);
	zassert_false(err < 0, "bt_bap_stream_reconfig returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SOURCE, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_source_ase_state_transition, test_server_codec_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_codec_configured(conn, ase_id, stream);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_server_qos_configured_to_codec_configured)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	err = bt_bap_stream_reconfig(stream, &codec_cfg);
	zassert_false(err < 0, "bt_bap_stream_reconfig returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_reconfig_called_once(stream, BT_AUDIO_DIR_SOURCE, EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);
}

ZTEST_F(test_source_ase_state_transition, test_server_qos_configured_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_qos_configured(conn, ase_id, stream);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_server_enabling_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_server_enabling_to_enabling)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_MEDIA)),
	};
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = bt_bap_stream_metadata(stream, meta, ARRAY_SIZE(meta));
	zassert_false(err < 0, "bt_bap_stream_metadata returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_server_enabling_to_disabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = bt_bap_stream_disable(stream);
	zassert_false(err < 0, "bt_bap_stream_disable returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_server_streaming_to_streaming)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_MEDIA)),
	};
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	err = bt_bap_stream_metadata(stream, meta, ARRAY_SIZE(meta));
	zassert_false(err < 0, "bt_bap_stream_metadata returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_metadata_called_once(stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_metadata_updated_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
}

ZTEST_F(test_source_ase_state_transition, test_server_streaming_to_disabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	err = bt_bap_stream_disable(stream);
	zassert_false(err < 0, "bt_bap_stream_disable returned err %d", err);

	/* Verification */
	expect_bt_bap_unicast_server_cb_disable_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
}

ZTEST_F(test_source_ase_state_transition, test_server_streaming_to_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	struct bt_iso_chan *chan;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	err = bt_bap_stream_release(stream);
	zassert_false(err < 0, "bt_bap_stream_release returned err %d", err);

	/* Client disconnects the ISO */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	/* Verification */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_stopped_called_once(stream, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	expect_bt_bap_stream_ops_released_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);
}
