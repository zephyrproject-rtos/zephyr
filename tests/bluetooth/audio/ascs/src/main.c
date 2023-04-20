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

ZTEST_SUITE(ascs_attrs_test_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(ascs_attrs_test_suite, test_has_sink_ase_chrc)
{
	const struct bt_gatt_attr *attr = NULL;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	test_ase_snk_get(1, &attr);
	zassert_not_null(attr);
}

ZTEST(ascs_attrs_test_suite, test_has_source_ase_chrc)
{
	const struct bt_gatt_attr *attr = NULL;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &attr);
	zassert_not_null(attr);
}

ZTEST(ascs_attrs_test_suite, test_has_control_point_chrc)
{
	const struct bt_gatt_attr *attr;

	attr = test_ase_control_point_get();
	zassert_not_null(attr, "ASE Control Point not found");
}

struct ascs_ase_control_test_suite_fixture {
	struct bt_conn conn;
	struct bt_bap_stream stream;
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase_snk;
};

static void *ascs_ase_control_test_suite_setup(void)
{
	struct ascs_ase_control_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	test_conn_init(&fixture->conn);
	fixture->ase_cp = test_ase_control_point_get();
	test_ase_snk_get(1, &fixture->ase_snk);

	return fixture;
}

static void ascs_ase_control_test_suite_before(void *f)
{
	struct ascs_ase_control_test_suite_fixture *fixture = f;

	ARG_UNUSED(fixture);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
}

static void ascs_ase_control_test_suite_after(void *f)
{
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

static void ascs_ase_control_test_suite_teardown(void *f)
{
	struct ascs_ase_control_test_suite_fixture *fixture = f;

	free(fixture);
}

ZTEST_SUITE(ascs_ase_control_test_suite, NULL, ascs_ase_control_test_suite_setup,
	    ascs_ase_control_test_suite_before, ascs_ase_control_test_suite_after,
	    ascs_ase_control_test_suite_teardown);

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_read_state_idle)
{
	uint8_t data_expected[] = {
		0x01,   /* ASE_ID */
		0x00,   /* ASE_State = Idle */
	};
	ssize_t ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ret = fixture->ase_snk->read(&fixture->conn, fixture->ase_snk, NULL, 0, 0);
	zassert_false(ret < 0, "attr->read returned unexpected (err 0x%02x)",
		      BT_GATT_ERR(ret));

	expect_bt_gatt_attr_read_called_once(&fixture->conn, fixture->ase_snk, EMPTY,
					     EMPTY, 0x0000, data_expected, sizeof(data_expected));
}

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_control_operation_zero_length_write)
{
	uint8_t buf[] = {};
	ssize_t ret;

	ret = fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, (void *)buf, 0, 0, 0);
	zassert_true(ret < 0, "ase_cp_attr->write returned unexpected (err 0x%02x)",
		     BT_GATT_ERR(ret));
}

static void test_unsupported_opcode(struct ascs_ase_control_test_suite_fixture *fixture,
				    uint8_t opcode)
{
	uint8_t buf[] = {
		opcode, /* Opcode */
		0x01,   /* Number_of_ASEs */
		0x01,   /* ASE_ID[0] */
	};
	uint8_t data_expected[] = {
		opcode, /* Opcode */
		0xFF,   /* Number_of_ASEs */
		0x00,   /* ASE_ID[0] */
		0x01,   /* Response_Code[0] = Unsupported Opcode */
		0x00,   /* Reason[0] */
	};

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, (void *)buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

ZTEST_F(ascs_ase_control_test_suite, test_unsupported_opcode_0x00)
{
	test_unsupported_opcode(fixture, 0x00);
}

ZTEST_F(ascs_ase_control_test_suite, test_unsupported_opcode_rfu)
{
	test_unsupported_opcode(fixture, 0x09);
}

static void test_codec_configure_invalid_length(struct ascs_ase_control_test_suite_fixture *fixture,
						const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x01,           /* Opcode */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

/*
 *  Test correctly formatted ASE Control Point 'Invalid Length' notification is sent
 *
 *  ASCS_v1.0; 5 ASE Control operations
 *  "A client-initiated ASE Control operation shall be defined as an invalid length operation
 *   if the Number_of_ASEs parameter value is less than 1"
 *
 *  Constraints:
 *   - Number_of_ASEs is set to 0
 *   - Config Codec operation parameter array is valid
 *
 *  Expected behaviour:
 *   - "If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be set to 0xFF."
 *   - ASE Control Point notification is correctly formatted
 */
ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_invalid_length_number_of_ases_0x00)
{
	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x00,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
	};

	test_codec_configure_invalid_length(fixture, buf, sizeof(buf));
}

/*
 *  Test correctly formatted ASE Control Point 'Invalid Length' notification is sent
 *
 *  ASCS_v1.0; 5 ASE Control operations
 *  "A client-initiated ASE Control operation shall be defined as an invalid length operation(...)
 *   if the Number_of_ASEs parameter value does not match the number of parameter arrays written by
 *   the client"
 *
 *  Constraints:
 *   - Number_of_ASEs is set to 1
 *   - Config Codec operation parameter arrays != Number_of_ASEs and is set to 2
 *
 *  Expected behaviour:
 *   - "If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be set to 0xFF."
 *   - ASE Control Point notification is correctly formatted
 */
ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_invalid_length_too_many_parameter_arrays)
{
	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
		0x02,           /* ASE_ID[1] */
		0x01,           /* Target_Latency[1] = Target low latency */
		0x02,           /* Target_PHY[1] = LE 2M PHY */
		0x06,           /* Codec_ID[1].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[1].Company_ID */
		0x00, 0x00,     /* Codec_ID[1].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[1] */
	};

	test_codec_configure_invalid_length(fixture, buf, sizeof(buf));
}

/*
 *  Test correctly formatted ASE Control Point 'Invalid Length' notification is sent
 *
 *  ASCS_v1.0; 5 ASE Control operations
 *  "A client-initiated ASE Control operation shall be defined as an invalid length operation
 *   if the total length of all parameters written by the client is not equal to the total length
 *   of all fixed parameters plus the length of any variable length parameters for that operation"
 *
 *  Constraints:
 *   - Number_of_ASEs is set to 1
 *   - Config Codec operation parameter arrays == Number_of_ASEs
 *   - Codec_Specific_Configuration_Length[i] > sizeof(Codec_Specific_Configuration[i])
 *
 *  Expected behaviour:
 *   - "If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be set to 0xFF."
 *   - ASE Control Point notification is correctly formatted
 */
ZTEST_F(ascs_ase_control_test_suite,
	test_codec_configure_invalid_length_codec_specific_configuration_too_short)
{
	uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x05,           /* Codec_Specific_Configuration_Length[0] */
		0x00, 0x00,     /* Codec_Specific_Configuration[0] */
		0x00, 0x00,
	};

	test_codec_configure_invalid_length(fixture, buf, sizeof(buf));
}

/*
 *  Test correctly formatted ASE Control Point 'Invalid Length' notification is sent
 *
 *  ASCS_v1.0; 5 ASE Control operations
 *  "A client-initiated ASE Control operation shall be defined as an invalid length operation
 *   if the total length of all parameters written by the client is not equal to the total length
 *   of all fixed parameters plus the length of any variable length parameters for that operation"
 *
 *  Constraints:
 *   - Number_of_ASEs is set to 1
 *   - Config Codec operation parameter arrays == Number_of_ASEs
 *   - Codec_Specific_Configuration_Length[i] < sizeof(Codec_Specific_Configuration[i])
 *
 *  Expected behaviour:
 *   - "If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be set to 0xFF."
 *   - ASE Control Point notification is correctly formatted
 */
ZTEST_F(ascs_ase_control_test_suite,
	test_codec_configure_invalid_length_codec_specific_configuration_too_long)
{
	uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x05,           /* Codec_Specific_Configuration_Length[0] */
		0x00, 0x00,     /* Codec_Specific_Configuration[0] */
		0x00, 0x00,
		0x00, 0x00,
	};

	test_codec_configure_invalid_length(fixture, buf, sizeof(buf));
}

/*
 *  Test correctly formatted ASE Control Point 'Invalid ASE_ID' notification is sent
 *
 *  Constraints:
 *   - Number_of_ASEs is set to 1
 *   - Requested ASE_ID is not present on the server.
 *
 *  Expected behaviour:
 *   - Correctly formatted ASE Control Point notification is sent with Invalid ASE_ID response code.
 */
static void test_codec_configure_invalid_ase_id(struct ascs_ase_control_test_suite_fixture *fixture,
						uint8_t ase_id)
{
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
	const uint8_t data_expected[] = {
		0x01,           /* Opcode */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x03,           /* Response_Code[0] = Invalid ASE_ID */
		0x00,           /* Reason[0] */
	};

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_invalid_ase_id_0x00)
{
	test_codec_configure_invalid_ase_id(fixture, 0x00);
}

ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_invalid_ase_id_unavailable)
{
	test_codec_configure_invalid_ase_id(fixture, (CONFIG_BT_ASCS_ASE_SNK_COUNT +
						      CONFIG_BT_ASCS_ASE_SRC_COUNT + 1));
}

static void test_codec_configure_target_latency_out_of_range(
	struct ascs_ase_control_test_suite_fixture *fixture, uint8_t target_latency)
{
	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		target_latency, /* Target_Latency[0] */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
	};
	const uint8_t data_expected[] = {
		0x01,           /* Opcode */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x00,           /* Response_Code[0] = Success */
		0x00,           /* Reason[0] */
	};

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_target_latency_out_of_range_0x00)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_codec_configure_target_latency_out_of_range(fixture, 0x00);
}

ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_target_latency_out_of_range_0x04)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_codec_configure_target_latency_out_of_range(fixture, 0x04);
}

static void test_codec_configure_target_phy_out_of_range(
	struct ascs_ase_control_test_suite_fixture *fixture, uint8_t target_phy)
{
	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] */
		target_phy,     /* Target_PHY[0] */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
	};
	const uint8_t data_expected[] = {
		0x01,           /* Opcode */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x00,           /* Response_Code[0] = Success */
		0x00,           /* Reason[0] */
	};

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_target_phy_out_of_range_0x00)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_codec_configure_target_phy_out_of_range(fixture, 0x00);
}

ZTEST_F(ascs_ase_control_test_suite, test_codec_configure_target_phy_out_of_range_0x04)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_codec_configure_target_phy_out_of_range(fixture, 0x04);
}

struct ascs_test_suite_fixture {
	const struct bt_gatt_attr *ase_cp;
	struct bt_conn conn;
	struct bt_bap_stream stream;
	struct {
		uint8_t id;
		const struct bt_gatt_attr *attr;
	} ase;
};

static void ascs_test_suite_fixture_init(struct ascs_test_suite_fixture *fixture)
{
	const struct bt_uuid *uuid = COND_CODE_1(CONFIG_BT_ASCS_ASE_SNK,
						 (BT_UUID_ASCS_ASE_SNK), (BT_UUID_ASCS_ASE_SRC));

	memset(fixture, 0, sizeof(*fixture));

	test_conn_init(&fixture->conn);

	test_ase_get(uuid, 1, &fixture->ase.attr);
	zassert_not_null(fixture->ase.attr);

	fixture->ase.id = test_ase_id_get(fixture->ase.attr);
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

ZTEST_F(ascs_test_suite, test_release_ase_on_callback_unregister)
{
	const struct bt_gatt_attr *ase = fixture->ase.attr;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	const uint8_t expect_ase_state_idle[] = {
		ase_id, /* ASE_ID */
		0x00,   /* ASE_State = Idle */
	};

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
	expect_bt_gatt_notify_cb_called_once(conn, ase->uuid, ase, expect_ase_state_idle,
					     sizeof(expect_ase_state_idle));
}

ZTEST_F(ascs_test_suite, test_abort_client_operation_if_callback_not_registered)
{
	const struct bt_gatt_attr *ase_cp = test_ase_control_point_get();
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase.id;
	const uint8_t expect_ase_cp_unspecified_error[] = {
		0x01,           /* Opcode */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x0E,           /* Response_Code[0] = Unspecified Error */
		0x00,           /* Reason[0] */
	};

	/* Set ASE to non-idle state */
	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Expected ASE Control Point notification with Unspecified Error was sent */
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp,
					     expect_ase_cp_unspecified_error,
					     sizeof(expect_ase_cp_unspecified_error));
}
