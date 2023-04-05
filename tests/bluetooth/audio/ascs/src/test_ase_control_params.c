/* test_ase_control_params.c - ASE Control Operations with invalid arguments */

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

struct test_ase_control_params_fixture {
	struct bt_conn conn;
	struct bt_bap_stream stream;
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase_snk;
};

static void *test_ase_control_params_setup(void)
{
	struct test_ase_control_params_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	test_conn_init(&fixture->conn);
	fixture->ase_cp = test_ase_control_point_get();
	test_ase_snk_get(1, &fixture->ase_snk);

	return fixture;
}

static void test_ase_control_params_before(void *f)
{
	struct test_ase_control_params_fixture *fixture = f;

	ARG_UNUSED(fixture);

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
}

static void test_ase_control_params_after(void *f)
{
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
}

static void test_ase_control_params_teardown(void *f)
{
	struct test_ase_control_params_fixture *fixture = f;

	free(fixture);
}

ZTEST_SUITE(test_ase_control_params, NULL, test_ase_control_params_setup,
	    test_ase_control_params_before, test_ase_control_params_after,
	    test_ase_control_params_teardown);

ZTEST_F(test_ase_control_params, test_sink_ase_control_operation_zero_length_write)
{
	uint8_t buf[] = {};
	ssize_t ret;

	ret = fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, (void *)buf, 0, 0, 0);
	zassert_true(ret < 0, "ase_cp_attr->write returned unexpected (err 0x%02x)",
		     BT_GATT_ERR(ret));
}

static void test_expect_unsupported_opcode(struct test_ase_control_params_fixture *fixture,
				           uint8_t opcode)
{
	const uint8_t buf[] = {
		opcode, /* Opcode */
		0x01,   /* Number_of_ASEs */
		0x01,   /* ASE_ID[0] */
	};
	const uint8_t data_expected[] = {
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

ZTEST_F(test_ase_control_params, test_unsupported_opcode_0x00)
{
	test_expect_unsupported_opcode(fixture, 0x00);
}

ZTEST_F(test_ase_control_params, test_unsupported_opcode_rfu)
{
	test_expect_unsupported_opcode(fixture, 0x09);
}

static void test_codec_configure_expect_invalid_length(
        struct test_ase_control_params_fixture *fixture, const uint8_t *buf, size_t len)
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
ZTEST_F(test_ase_control_params, test_codec_configure_number_of_ases_0x00)
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

	test_codec_configure_expect_invalid_length(fixture, buf, sizeof(buf));
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
ZTEST_F(test_ase_control_params, test_codec_configure_too_many_parameter_arrays)
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

	test_codec_configure_expect_invalid_length(fixture, buf, sizeof(buf));
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
ZTEST_F(test_ase_control_params, test_codec_specific_configuration_too_short)
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
		0x05,           /* Codec_Specific_Configuration_Length[0] */
		0x00, 0x00,     /* Codec_Specific_Configuration[0] */
		0x00, 0x00,
	};

	test_codec_configure_expect_invalid_length(fixture, buf, sizeof(buf));
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
ZTEST_F(test_ase_control_params, test_codec_specific_configuration_too_long)
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
		0x05,           /* Codec_Specific_Configuration_Length[0] */
		0x00, 0x00,     /* Codec_Specific_Configuration[0] */
		0x00, 0x00,
		0x00, 0x00,
	};

	test_codec_configure_expect_invalid_length(fixture, buf, sizeof(buf));
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
static void test_codec_configure_expect_invalid_ase_id(
	struct test_ase_control_params_fixture *fixture, uint8_t ase_id)
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

ZTEST_F(test_ase_control_params, test_codec_configure_invalid_ase_id_0x00)
{
	test_codec_configure_expect_invalid_ase_id(fixture, 0x00);
}

ZTEST_F(test_ase_control_params, test_codec_configure_invalid_ase_id_unavailable)
{
	test_codec_configure_expect_invalid_ase_id(fixture, (CONFIG_BT_ASCS_ASE_SNK_COUNT +
						      CONFIG_BT_ASCS_ASE_SRC_COUNT + 1));
}

static void test_target_latency_out_of_range(struct test_ase_control_params_fixture *fixture,
                                             uint8_t target_latency)
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

ZTEST_F(test_ase_control_params, test_target_latency_out_of_range_0x00)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_target_latency_out_of_range(fixture, 0x00);
}

ZTEST_F(test_ase_control_params, test_target_latency_out_of_range_0x04)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_target_latency_out_of_range(fixture, 0x04);
}

static void test_target_phy_out_of_range(struct test_ase_control_params_fixture *fixture,
                                         uint8_t target_phy)
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

ZTEST_F(test_ase_control_params, test_target_phy_out_of_range_0x00)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_target_phy_out_of_range(fixture, 0x00);
}

ZTEST_F(test_ase_control_params, test_target_phy_out_of_range_0x04)
{
	/* TODO: Remove once resolved */
	Z_TEST_SKIP_IFNDEF(BUG_55794);

	test_target_phy_out_of_range(fixture, 0x04);
}
