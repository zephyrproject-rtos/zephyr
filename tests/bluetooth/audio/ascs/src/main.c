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
	struct test_ase_chrc_value_hdr *hdr;
	ssize_t ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);
	zexpect_not_null(fixture->ase_snk.attr);

	ret = ase->read(conn, ase, NULL, 0, 0);
	zassert_false(ret < 0, "attr->read returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));

	expect_bt_gatt_attr_read_called_once(conn, ase, EMPTY, EMPTY, 0x0000, EMPTY, sizeof(*hdr));

	hdr = (void *)bt_gatt_attr_read_fake.arg5_val;
	zassert_equal(0x00, hdr->ase_state, "unexpected ASE_State 0x%02x", hdr->ase_state);
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
