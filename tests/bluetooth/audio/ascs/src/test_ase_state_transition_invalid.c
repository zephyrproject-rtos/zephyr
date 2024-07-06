/* test_ase_state_transition_invalid.c - ASE state transition tests */

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

struct test_ase_state_transition_invalid_fixture {
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase_snk;
	const struct bt_gatt_attr *ase_src;
	struct bt_bap_stream stream;
	struct bt_conn conn;
};

static void *test_ase_state_transition_invalid_setup(void)
{
	struct test_ase_state_transition_invalid_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	memset(fixture, 0, sizeof(*fixture));
	fixture->ase_cp = test_ase_control_point_get();
	test_conn_init(&fixture->conn);
	test_ase_snk_get(1, &fixture->ase_snk);
	test_ase_src_get(1, &fixture->ase_src);

	return fixture;
}

static void test_ase_state_transition_invalid_before(void *f)
{
	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
}

static void test_ase_state_transition_invalid_after(void *f)
{
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

static void test_ase_state_transition_invalid_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(test_ase_state_transition_invalid, NULL, test_ase_state_transition_invalid_setup,
	    test_ase_state_transition_invalid_before, test_ase_state_transition_invalid_after,
	    test_ase_state_transition_invalid_teardown);

static void test_client_config_codec_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							     const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_config_codec(conn, ase_id, NULL);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_config_qos_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							   const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x02,           /* Opcode = Config QoS */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_config_qos(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_enable_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
						       const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x03,           /* Opcode = Enable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_enable(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_start_ready_expect_transition_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_start_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_start_ready_expect_ase_direction_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x05,           /* Response_Code[0] = Invalid ASE direction */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_start_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_disable_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x05,           /* Opcode = Disable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_disable(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_stop_ready_expect_transition_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_stop_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_stop_ready_expect_ase_direction_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x05,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_stop_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_update_metadata_expect_transition_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x07,           /* Opcode = Update Metadata */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_update_metadata(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_release_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x08,           /* Opcode = Release */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_release(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

ZTEST_F(test_ase_state_transition_invalid, test_client_sink_state_idle)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);

	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
	test_client_release_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_sink_state_codec_configured)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_sink_state_qos_configured)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_sink_state_enabling)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_sink_client_state_streaming)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
}

static void expect_ase_state_releasing(struct bt_conn *conn, const struct bt_gatt_attr *ase)
{
	struct test_ase_chrc_value_hdr hdr = { 0xff };
	k_ssize_t ret;

	zexpect_not_null(conn);
	zexpect_not_null(ase);

	ret = ase->read(conn, ase, &hdr, sizeof(hdr), 0);
	zassert_false(ret < 0, "attr->read returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
	zassert_equal(BT_BAP_EP_STATE_RELEASING, hdr.ase_state,
		      "unexpected ASE_State 0x%02x", hdr.ase_state);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_sink_state_releasing)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, false);
	expect_ase_state_releasing(conn, fixture->ase_snk);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_idle)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);

	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
	test_client_release_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_codec_configured)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_qos_configured)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_enabling)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_streaming)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_disabling)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_disabling(conn, ase_id, stream, &chan);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

ZTEST_F(test_ase_state_transition_invalid, test_client_source_state_releasing)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, true);
	expect_ase_state_releasing(conn, fixture->ase_src);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_server_config_codec_expect_error(struct bt_bap_stream *stream)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int err;

	err = bt_bap_stream_reconfig(stream, &codec_cfg);
	zassert_false(err == 0, "bt_bap_stream_reconfig unexpected success");
}

static void test_server_receiver_start_ready_expect_error(struct bt_bap_stream *stream)
{
	int err;

	err = bt_bap_stream_start(stream);
	zassert_false(err == 0, "bt_bap_stream_start unexpected success");
}

static void test_server_disable_expect_error(struct bt_bap_stream *stream)
{
	int err;

	err = bt_bap_stream_disable(stream);
	zassert_false(err == 0, "bt_bap_stream_disable unexpected success");
}

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
#include "bap_endpoint.h"

static void test_server_config_qos_expect_error(struct bt_bap_stream *stream)
{
	struct bt_bap_unicast_group group;
	int err;

	sys_slist_init(&group.streams);
	sys_slist_append(&group.streams, &stream->_node);

	err = bt_bap_stream_qos(stream->conn, &group);
	zassert_false(err == 0, "bt_bap_stream_qos unexpected success");
}

static void test_server_enable_expect_error(struct bt_bap_stream *stream)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_RINGTONE)),
	};
	int err;

	err = bt_bap_stream_enable(stream, meta, ARRAY_SIZE(meta));
	zassert_false(err == 0, "bt_bap_stream_enable unexpected success");
}

static void test_server_receiver_stop_ready_expect_error(struct bt_bap_stream *stream)
{
	int err;

	err = bt_bap_stream_stop(stream);
	zassert_false(err == 0, "bt_bap_stream_stop unexpected success");
}
#else
#define test_server_config_qos_expect_error(...)
#define test_server_enable_expect_error(...)
#define test_server_receiver_stop_ready_expect_error(...)
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

static void test_server_update_metadata_expect_error(struct bt_bap_stream *stream)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_RINGTONE)),
	};
	int err;

	err = bt_bap_stream_metadata(stream, meta, ARRAY_SIZE(meta));
	zassert_false(err == 0, "bt_bap_stream_metadata unexpected success");
}

ZTEST_F(test_ase_state_transition_invalid, test_server_sink_state_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_sink_state_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_sink_state_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_sink_state_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_sink_state_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, false);
	expect_ase_state_releasing(conn, fixture->ase_snk);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_source_state_codec_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_source_state_qos_configured)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_source_state_enabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_source_state_streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_source_state_disabling)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_disabling(conn, ase_id, stream, &chan);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

ZTEST_F(test_ase_state_transition_invalid, test_server_source_state_releasing)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, true);
	expect_ase_state_releasing(conn, fixture->ase_src);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}
