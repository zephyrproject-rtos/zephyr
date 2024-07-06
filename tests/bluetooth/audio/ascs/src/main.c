/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util_macro.h>

#include "assert.h"
#include "ascs_internal.h"
#include "bap_unicast_server.h"
#include "bap_unicast_server_expects.h"
#include "bap_stream.h"
#include "bap_stream_expects.h"
#include "conn.h"
#include "gatt.h"
#include "gatt_expects.h"
#include "iso.h"
#include "mock_kernel.h"
#include "pacs.h"

#include "test_common.h"

DEFINE_FFF_GLOBALS;

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

struct ascs_test_suite_fixture {
	const struct bt_gatt_attr *ase_cp;
	struct bt_bap_stream stream;
	struct bt_conn conn;
	struct {
		uint8_t id;
		const struct bt_gatt_attr *attr;
	} ase_snk, ase_src;
};

static void ascs_test_suite_fixture_init(struct ascs_test_suite_fixture *fixture)
{
	memset(fixture, 0, sizeof(*fixture));

	fixture->ase_cp = test_ase_control_point_get();

	test_conn_init(&fixture->conn);

	test_ase_snk_get(1, &fixture->ase_snk.attr);
	if (fixture->ase_snk.attr != NULL) {
		fixture->ase_snk.id = test_ase_id_get(fixture->ase_snk.attr);
	}

	test_ase_src_get(1, &fixture->ase_src.attr);
	if (fixture->ase_src.attr != NULL) {
		fixture->ase_src.id = test_ase_id_get(fixture->ase_src.attr);
	}
}

static void *ascs_test_suite_setup(void)
{
	struct ascs_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	ascs_test_suite_fixture_init(fixture);

	return fixture;
}

static void ascs_test_suite_teardown(void *f)
{
	free(f);
}

static void ascs_test_suite_after(void *f)
{
	bt_ascs_cleanup();
}

ZTEST_SUITE(ascs_test_suite, NULL, ascs_test_suite_setup, NULL, ascs_test_suite_after,
	    ascs_test_suite_teardown);

ZTEST_F(ascs_test_suite, test_has_sink_ase_chrc)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	zassert_not_null(fixture->ase_snk.attr);
}

ZTEST_F(ascs_test_suite, test_has_source_ase_chrc)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	zassert_not_null(fixture->ase_src.attr);
}

ZTEST_F(ascs_test_suite, test_has_control_point_chrc)
{
	zassert_not_null(fixture->ase_cp);
}

ZTEST_F(ascs_test_suite, test_sink_ase_read_state_idle)
{
	const struct bt_gatt_attr *ase = fixture->ase_snk.attr;
	struct bt_conn *conn = &fixture->conn;
	struct test_ase_chrc_value_hdr hdr = { 0xff };
	k_ssize_t ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);
	zexpect_not_null(fixture->ase_snk.attr);

	ret = ase->read(conn, ase, &hdr, sizeof(hdr), 0);
	zassert_false(ret < 0, "attr->read returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
	zassert_equal(0x00, hdr.ase_state, "unexpected ASE_State 0x%02x", hdr.ase_state);
}

ZTEST_F(ascs_test_suite, test_release_ase_on_callback_unregister)
{
	const struct test_ase_chrc_value_hdr *hdr;
	const struct bt_gatt_attr *ase;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_gatt_notify_params *notify_params;
	uint8_t ase_id;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}

	zexpect_not_null(ase);
	zexpect_true(ase_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	/* Set ASE to non-idle state */
	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Reset mock, as we expect ASE notification to be sent */
	bt_gatt_notify_cb_reset();

	/* Unregister the callbacks - whis will clean up the ASCS */
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);

	/* Expected to notify the upper layers */
	expect_bt_bap_unicast_server_cb_release_called_once(stream);
	expect_bt_bap_stream_ops_released_called_once(stream);

	/* Expected to notify the client */
	expect_bt_gatt_notify_cb_called_once(conn, ase->uuid, ase, EMPTY, sizeof(*hdr));

	notify_params = mock_bt_gatt_notify_cb_fake.arg1_val;
	hdr = (void *)notify_params->data;
	zassert_equal(0x00, hdr->ase_state, "unexpected ASE_State 0x%02x", hdr->ase_state);
}

ZTEST_F(ascs_test_suite, test_abort_client_operation_if_callback_not_registered)
{
	const struct test_ase_cp_chrc_value_param *param;
	const struct test_ase_cp_chrc_value_hdr *hdr;
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_gatt_notify_params *notify_params;
	uint8_t ase_id;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase_id = fixture->ase_snk.id;
	} else {
		ase_id = fixture->ase_src.id;
	}

	zexpect_not_null(ase_cp);
	zexpect_true(ase_id != 0x00);

	/* Set ASE to non-idle state */
	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Expected ASE Control Point notification with Unspecified Error was sent */
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp,
					     EMPTY, TEST_ASE_CP_CHRC_VALUE_SIZE(1));

	notify_params = mock_bt_gatt_notify_cb_fake.arg1_val;
	hdr = (void *)notify_params->data;
	zassert_equal(0x01, hdr->opcode, "unexpected Opcode 0x%02x", hdr->opcode);
	zassert_equal(0x01, hdr->number_of_ases, "unexpected Number_of_ASEs 0x%02x",
		      hdr->number_of_ases);
	param = (void *)hdr->params;
	zassert_equal(ase_id, param->ase_id, "unexpected ASE_ID 0x%02x", param->ase_id);
	/* Expect Unspecified Error */
	zassert_equal(0x0E, param->response_code, "unexpected Response_Code 0x%02x",
		      param->response_code);
	zassert_equal(0x00, param->reason, "unexpected Reason 0x%02x", param->reason);
}

ZTEST_F(ascs_test_suite, test_release_ase_on_acl_disconnection)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}

	zexpect_not_null(ase);
	zexpect_true(ase_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	/* Set ASE to non-idle state */
	test_preamble_state_streaming(conn, ase_id, stream, &chan,
				      !IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK));

	/* Mock ACL disconnection */
	mock_bt_conn_disconnected(conn, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected to notify the upper layers */
	expect_bt_bap_stream_ops_released_called_once(stream);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

ZTEST_F(ascs_test_suite, test_release_ase_pair_on_acl_disconnection)
{
	const struct bt_gatt_attr *ase_snk, *ase_src;
	struct bt_bap_stream snk_stream, src_stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_snk_id, ase_src_id;
	struct bt_iso_chan *chan;
	int err;

	if (CONFIG_BT_ASCS_MAX_ACTIVE_ASES < 2) {
		ztest_test_skip();
	}

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);
	memset(&snk_stream, 0, sizeof(snk_stream));
	ase_snk = fixture->ase_snk.attr;
	zexpect_not_null(ase_snk);
	ase_snk_id = fixture->ase_snk.id;
	zexpect_true(ase_snk_id != 0x00);

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);
	memset(&src_stream, 0, sizeof(src_stream));
	ase_src = fixture->ase_src.attr;
	zexpect_not_null(ase_src);
	ase_src_id = fixture->ase_src.id;
	zexpect_true(ase_src_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_ase_control_client_config_codec(conn, ase_snk_id, &snk_stream);
	test_ase_control_client_config_qos(conn, ase_snk_id);
	test_ase_control_client_enable(conn, ase_snk_id);

	test_ase_control_client_config_codec(conn, ase_src_id, &src_stream);
	test_ase_control_client_config_qos(conn, ase_src_id);
	test_ase_control_client_enable(conn, ase_src_id);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	test_ase_control_client_receiver_start_ready(conn, ase_src_id);

	err = bt_bap_stream_start(&snk_stream);
	zassert_equal(0, err, "bt_bap_stream_start err %d", err);

	test_mocks_reset();

	/* Mock ACL disconnection */
	mock_bt_conn_disconnected(conn, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected to notify the upper layers */
	const struct bt_bap_stream *streams[2] = { &snk_stream, &src_stream };

	expect_bt_bap_stream_ops_released_called(streams, 2);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

ZTEST_F(ascs_test_suite, test_recv_in_streaming_state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase_snk.id;
	struct bt_iso_recv_info info = {
		.seq_num = 1,
		.flags = BT_ISO_FLAGS_VALID,
	};
	struct bt_iso_chan *chan;
	struct net_buf buf;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	chan->ops->recv(chan, &info, &buf);

	/* Verification */
	expect_bt_bap_stream_ops_recv_called_once(stream, &info, &buf);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

ZTEST_F(ascs_test_suite, test_recv_in_enabling_state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase_snk.id;
	struct bt_iso_recv_info info = {
		.seq_num = 1,
		.flags = BT_ISO_FLAGS_VALID,
	};
	struct bt_iso_chan *chan;
	struct net_buf buf;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	test_mocks_reset();

	chan->ops->recv(chan, &info, &buf);

	/* Verification */
	expect_bt_bap_stream_ops_recv_not_called();

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

ZTEST_F(ascs_test_suite, test_cis_link_loss_in_streaming_state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}
	zexpect_not_null(ase);
	zexpect_true(ase_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_preamble_state_streaming(conn, ase_id, stream, &chan,
				      !IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK));

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected to notify the upper layers */
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_called_once(stream);
	expect_bt_bap_stream_ops_released_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

static void test_cis_link_loss_in_disabling_state(struct ascs_test_suite_fixture *fixture,
						  bool streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	ase = fixture->ase_src.attr;
	ase_id = fixture->ase_src.id;
	zexpect_not_null(ase);
	zexpect_true(ase_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_preamble_state_enabling(conn, ase_id, stream);
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	if (streaming) {
		test_ase_control_client_receiver_start_ready(conn, ase_id);
	}

	test_ase_control_client_disable(conn, ase_id);

	expect_bt_bap_stream_ops_disabled_called_once(stream);

	test_mocks_reset();

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected to notify the upper layers */
	expect_bt_bap_stream_ops_qos_set_called_once(stream);
	expect_bt_bap_stream_ops_disabled_not_called();
	expect_bt_bap_stream_ops_released_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

ZTEST_F(ascs_test_suite, test_cis_link_loss_in_disabling_state_v1)
{
	/* Enabling -> Streaming -> Disabling */
	test_cis_link_loss_in_disabling_state(fixture, true);
}

ZTEST_F(ascs_test_suite, test_cis_link_loss_in_disabling_state_v2)
{
	/* Enabling -> Disabling */
	test_cis_link_loss_in_disabling_state(fixture, false);
}

ZTEST_F(ascs_test_suite, test_cis_link_loss_in_enabling_state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}
	zexpect_not_null(ase);
	zexpect_true(ase_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_preamble_state_enabling(conn, ase_id, stream);
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected no change in ASE state */
	expect_bt_bap_stream_ops_qos_set_not_called();
	expect_bt_bap_stream_ops_released_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);

	err = bt_bap_stream_disable(stream);
	zassert_equal(0, err, "Failed to disable stream: err %d", err);

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		expect_bt_bap_stream_ops_qos_set_called_once(stream);
		expect_bt_bap_stream_ops_disabled_called_once(stream);
	} else {
		/* Server-initiated disable operation that shall not cause transition to QoS */
		expect_bt_bap_stream_ops_qos_set_not_called();
	}

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

ZTEST_F(ascs_test_suite, test_cis_link_loss_in_enabling_state_client_retries)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}
	zexpect_not_null(ase);
	zexpect_true(ase_id != 0x00);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	test_preamble_state_enabling(conn, ase_id, stream);
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);
	expect_bt_bap_stream_ops_connected_called_once(stream);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_FAIL_TO_ESTAB);

	/* Expected to not notify the upper layers */
	expect_bt_bap_stream_ops_qos_set_not_called();
	expect_bt_bap_stream_ops_released_not_called();
	expect_bt_bap_stream_ops_disconnected_called_once(stream);

	/* Client retries to establish CIS */
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);
	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		test_ase_control_client_receiver_start_ready(conn, ase_id);
	} else {
		err = bt_bap_stream_start(stream);
		zassert_equal(0, err, "bt_bap_stream_start err %d", err);
	}

	expect_bt_bap_stream_ops_connected_called_twice(stream);
	expect_bt_bap_stream_ops_started_called_once(stream);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

static struct bt_bap_stream *stream_allocated;
static const struct bt_audio_codec_qos_pref qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000);

static int unicast_server_cb_config_custom_fake(struct bt_conn *conn, const struct bt_bap_ep *ep,
						enum bt_audio_dir dir,
						const struct bt_audio_codec_cfg *codec_cfg,
						struct bt_bap_stream **stream,
						struct bt_audio_codec_qos_pref *const pref,
						struct bt_bap_ascs_rsp *rsp)
{
	*stream = stream_allocated;
	*pref = qos_pref;
	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);

	bt_bap_stream_cb_register(*stream, &mock_bap_stream_ops);

	return 0;
}

ZTEST_F(ascs_test_suite, test_ase_state_notification_retry)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase, *cp;
	struct bt_conn_info info;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}

	zexpect_not_null(ase);
	zassert_not_equal(ase_id, 0x00);

	cp = test_ase_control_point_get();
	zexpect_not_null(cp);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	stream_allocated = stream;
	mock_bap_unicast_server_cb_config_fake.custom_fake = unicast_server_cb_config_custom_fake;

	/* Mock out of buffers case */
	mock_bt_gatt_notify_cb_fake.return_val = -ENOMEM;

	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
	};

	cp->write(conn, cp, (void *)buf, sizeof(buf), 0, 0);

	/* Verification */
	expect_bt_bap_stream_ops_configured_not_called();

	mock_bt_gatt_notify_cb_fake.return_val = 0;

	err = bt_conn_get_info(conn, &info);
	zassert_equal(err, 0);

	/* Wait for ASE state notification retry */
	k_sleep(K_MSEC(BT_CONN_INTERVAL_TO_MS(info.le.interval)));

	expect_bt_bap_stream_ops_configured_called_once(stream, EMPTY);

	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}
