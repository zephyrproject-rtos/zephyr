/* test_ase_control_params.c - ASE Control Operations with invalid arguments */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
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
#include "ztest_assert.h"

struct test_ase_control_params_fixture {
	struct bt_conn conn;
	struct bt_bap_stream stream;
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase;
};

static void *test_ase_control_params_setup(void)
{
	struct test_ase_control_params_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	test_conn_init(&fixture->conn);
	fixture->ase_cp = test_ase_control_point_get();

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		test_ase_snk_get(1, &fixture->ase);
	} else {
		test_ase_src_get(1, &fixture->ase);
	}

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
	k_ssize_t ret;

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
		0x01,           /* Opcode = Config Codec */
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

ZTEST_F(test_ase_control_params, test_codec_configure_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	const uint8_t buf[] = {
		0x01,             /* Opcode = Config Codec */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		0x01,             /* ASE_ID[0] */
		0x01,             /* Target_Latency[0] = Target low latency */
		0x02,             /* Target_PHY[0] = LE 2M PHY */
		0x06,             /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,       /* Codec_ID[0].Company_ID */
		0x00, 0x00,       /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,             /* Codec_Specific_Configuration_Length[0] */
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
ZTEST_F(test_ase_control_params, test_codec_configure_invalid_ase_id_0x00)
{
	const uint8_t ase_id_invalid = 0x00;
	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		ase_id_invalid, /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
	};
	const uint8_t data_expected[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		ase_id_invalid, /* ASE_ID[0] */
		0x03,           /* Response_Code[0] = Invalid ASE_ID */
		0x00,           /* Reason[0] */
	};

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

static struct bt_bap_stream test_stream;
static const struct bt_audio_codec_qos_pref qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000);

static int unicast_server_cb_config_custom_fake(struct bt_conn *conn, const struct bt_bap_ep *ep,
						enum bt_audio_dir dir,
						const struct bt_audio_codec_cfg *codec_cfg,
						struct bt_bap_stream **stream,
						struct bt_audio_codec_qos_pref *const pref,
						struct bt_bap_ascs_rsp *rsp)
{
	*stream = &test_stream;
	*pref = qos_pref;
	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);

	bt_bap_stream_cb_register(*stream, &mock_bap_stream_ops);

	return 0;
}

ZTEST_F(test_ase_control_params, test_codec_configure_invalid_ase_id_unavailable)
{
	/* Test requires support for at least 2 ASEs */
	if (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT < 2) {
		ztest_test_skip();
	}

	const uint8_t ase_id_valid = 0x01;
	const uint8_t ase_id_invalid = CONFIG_BT_ASCS_ASE_SNK_COUNT +
				       CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;
	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x02,           /* Number_of_ASEs */
		ase_id_invalid, /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x04,           /* Codec_Specific_Configuration_Length[0] */
		0x00, 0x00,     /* Codec_Specific_Configuration[0] */
		0x00, 0x00,
		ase_id_valid,   /* ASE_ID[1] */
		0x01,           /* Target_Latency[1] = Target low latency */
		0x02,           /* Target_PHY[1] = LE 2M PHY */
		0x06,           /* Codec_ID[1].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[1].Company_ID */
		0x00, 0x00,     /* Codec_ID[1].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[1] */
	};
	const uint8_t data_expected[] = {
		0x01,           /* Opcode = Config Codec */
		0x02,           /* Number_of_ASEs */
		ase_id_invalid, /* ASE_ID[0] */
		0x03,           /* Response_Code[0] = Invalid ASE_ID */
		0x00,           /* Reason[0] */
		ase_id_valid,   /* ASE_ID[1] */
		0x00,           /* Response_Code[1] = Success */
		0x00,           /* Reason[1] */
	};

	mock_bap_unicast_server_cb_config_fake.custom_fake = unicast_server_cb_config_custom_fake;

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
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
		0x01,           /* Opcode = Config Codec */
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
		0x01,           /* Opcode = Config Codec */
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

static void test_config_qos_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
						  const struct bt_gatt_attr *ase_cp,
						  struct bt_bap_stream *stream,
						  const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x02,           /* Opcode = Config QoS */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};

	test_preamble_state_codec_configured(conn, ase_id, stream);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_config_qos_number_of_ases_0x00)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x02,                   /* Opcode = Config QoS */
		0x00,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
		0x01,                   /* CIG_ID[0] */
		0x01,                   /* CIS_ID[0] */
		0xFF, 0x00, 0x00,       /* SDU_Interval[0] */
		0x00,                   /* Framing[0] */
		0x02,                   /* PHY[0] */
		0x64, 0x00,             /* Max_SDU[0] */
		0x02,                   /* Retransmission_Number[0] */
		0x0A, 0x00,             /* Max_Transport_Latency[0] */
		0x40, 0x9C, 0x00,       /* Presentation_Delay[0] */
	};

	test_config_qos_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					      &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_config_qos_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x02,             /* Opcode = Config QoS */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
		0x01,             /* CIG_ID[0] */
		0x01,             /* CIS_ID[0] */
		0xFF, 0x00, 0x00, /* SDU_Interval[0] */
		0x00,             /* Framing[0] */
		0x02,             /* PHY[0] */
		0x64, 0x00,       /* Max_SDU[0] */
		0x02,             /* Retransmission_Number[0] */
		0x0A, 0x00,       /* Max_Transport_Latency[0] */
		0x40, 0x9C, 0x00, /* Presentation_Delay[0] */
	};

	test_config_qos_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					      &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_config_qos_too_short)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x02,                   /* Opcode = Config QoS */
		0x01,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
		0x01,                   /* CIG_ID[0] */
		0x01,                   /* CIS_ID[0] */
		0xFF, 0x00, 0x00,       /* SDU_Interval[0] */
		0x00,                   /* Framing[0] */
		0x02,                   /* PHY[0] */
		0x64, 0x00,             /* Max_SDU[0] */
		0x02,                   /* Retransmission_Number[0] */
		0x0A, 0x00,             /* Max_Transport_Latency[0] */
		0x40, 0x9C, 0x00,       /* Presentation_Delay[0] */
		0x00,
	};

	test_config_qos_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					      &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_config_qos_too_long)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x02,                   /* Opcode = Config QoS */
		0x01,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
		0x01,                   /* CIG_ID[0] */
		0x01,                   /* CIS_ID[0] */
		0xFF, 0x00, 0x00,       /* SDU_Interval[0] */
		0x00,                   /* Framing[0] */
		0x02,                   /* PHY[0] */
		0x64, 0x00,             /* Max_SDU[0] */
		0x02,                   /* Retransmission_Number[0] */
		0x0A, 0x00,             /* Max_Transport_Latency[0] */
		0x40, 0x9C,             /* Presentation_Delay[0] */
	};

	test_config_qos_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					      &fixture->stream, buf, sizeof(buf));
}

static void test_enable_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
					      const struct bt_gatt_attr *ase_cp,
					      struct bt_bap_stream *stream,
					      const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x03,           /* Opcode = Enable */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};

	test_preamble_state_qos_configured(conn, ase_id, stream);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_enable_number_of_ases_0x00)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x03,           /* Opcode = Enable */
		0x00,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,           /* Metadata_Length[0] */
	};

	test_enable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp, &fixture->stream,
					  buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_enable_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x03,             /* Opcode = Enable */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
		0x00,             /* Metadata_Length[0] */
	};

	test_enable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp, &fixture->stream,
					  buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_enable_too_long)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x03,           /* Opcode = Enable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,           /* Metadata_Length[0] */
		0x00,
	};

	test_enable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp, &fixture->stream,
					  buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_enable_too_short)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x03,           /* Opcode = Enable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
	};

	test_enable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp, &fixture->stream,
					  buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_enable_metadata_too_short)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x03,           /* Opcode = Enable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x03,           /* Metadata_Length[0] */
		0x02, 0x02,     /* Metadata[0] */
	};

	test_enable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp, &fixture->stream,
					  buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_enable_invalid_ase_id)
{
	/* Test requires support for at least 2 ASEs */
	if (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT < 2) {
		ztest_test_skip();
	}

	const uint8_t ase_id_valid = 0x01;
	const uint8_t ase_id_invalid = CONFIG_BT_ASCS_ASE_SNK_COUNT +
				       CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;
	const uint8_t buf[] = {
		0x03,                   /* Opcode = Enable */
		0x02,                   /* Number_of_ASEs */
		ase_id_invalid,         /* ASE_ID[0] */
		0x04,                   /* Metadata_Length[0] */
		0x03, 0x02, 0x04, 0x00, /* Metadata[0] = Streaming Context (Media) */
		ase_id_valid,           /* ASE_ID[1] */
		0x04,                   /* Metadata_Length[0] */
		0x03, 0x02, 0x04, 0x00, /* Metadata[0] = Streaming Context (Media) */
	};
	const uint8_t data_expected[] = {
		0x03,                   /* Opcode = Enable */
		0x02,                   /* Number_of_ASEs */
		ase_id_invalid,         /* ASE_ID[0] */
		0x03,                   /* Response_Code[0] = Invalid ASE_ID */
		0x00,                   /* Reason[0] */
		ase_id_valid,           /* ASE_ID[1] */
		0x00,                   /* Response_Code[1] = Success */
		0x00,                   /* Reason[1] */
	};

	test_preamble_state_qos_configured(&fixture->conn, ase_id_valid, &fixture->stream);

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_enable_metadata_prohibited_context)
{
	const uint8_t ase_id_valid = 0x01;
	const uint8_t buf[] = {
		0x03,                   /* Opcode = Enable */
		0x01,                   /* Number_of_ASEs */
		ase_id_valid,           /* ASE_ID[0] */
		0x04,                   /* Metadata_Length[0] */
		0x03, 0x02, 0x00, 0x00, /* Metadata[0] = Streaming Context (Prohibited) */
	};
	const uint8_t data_expected[] = {
		0x03,                   /* Opcode = Enable */
		0x01,                   /* Number_of_ASEs */
		ase_id_valid,           /* ASE_ID[0] */
		0x0C,                   /* Response_Code[0] = Invalid Metadata */
		0x02,                   /* Reason[0] = Streaming Context */
	};

	test_preamble_state_qos_configured(&fixture->conn, ase_id_valid, &fixture->stream);

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

static void test_receiver_start_ready_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
							    const struct bt_gatt_attr *ase_cp,
							    struct bt_bap_stream *stream,
							    const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};
	struct bt_iso_chan *chan;
	int err;

	test_preamble_state_enabling(conn, ase_id, stream);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_receiver_start_ready_number_of_ases_0x00)
{
	const struct bt_gatt_attr *ase;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	ase_id = test_ase_id_get(ase);

	const uint8_t buf[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x00,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
	};

	test_receiver_start_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
							&fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_receiver_start_ready_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;
	const struct bt_gatt_attr *ase;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	const uint8_t ase_id = test_ase_id_get(ase);

	const uint8_t buf[] = {
		0x04,             /* Opcode = Receiver Start Ready */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
	};

	test_receiver_start_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
							&fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_receiver_start_ready_too_long)
{
	const struct bt_gatt_attr *ase;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	ase_id = test_ase_id_get(fixture->ase);

	const uint8_t buf[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,
	};

	test_receiver_start_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
							&fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_receiver_start_ready_too_short)
{
	const uint8_t buf[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x01,           /* Number_of_ASEs */
	};
	const struct bt_gatt_attr *ase;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	ase_id = test_ase_id_get(fixture->ase);

	test_receiver_start_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
							&fixture->stream, buf, sizeof(buf));
}

static void test_disable_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
					       const struct bt_gatt_attr *ase_cp,
					       struct bt_bap_stream *stream,
					       const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x05,           /* Opcode = Disable */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};

	test_preamble_state_enabling(conn, ase_id, stream);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_disable_number_of_ases_0x00)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x05,           /* Opcode = Disable */
		0x00,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
	};

	test_disable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_disable_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x05,             /* Opcode = Disable */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
	};

	test_disable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_disable_too_long)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x05,           /* Opcode = Disable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,
	};

	test_disable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_disable_too_short)
{
	uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x05,           /* Opcode = Disable */
		0x01,           /* Number_of_ASEs */
	};

	test_disable_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

static void test_receiver_stop_ready_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
							   const struct bt_gatt_attr *ase_cp,
							   struct bt_bap_stream *stream,
							   const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};
	struct bt_iso_chan *chan;

	test_preamble_state_disabling(conn, ase_id, stream, &chan);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_receiver_stop_ready_number_of_ases_0x00)
{
	const struct bt_gatt_attr *ase;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	ase_id = test_ase_id_get(ase);

	const uint8_t buf[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x00,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
	};

	test_receiver_stop_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						       &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_receiver_stop_ready_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;
	const struct bt_gatt_attr *ase;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	const uint8_t ase_id = test_ase_id_get(ase);

	const uint8_t buf[] = {
		0x06,             /* Opcode = Receiver Stop Ready */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
	};

	test_receiver_stop_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						       &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_receiver_stop_ready_too_long)
{
	const struct bt_gatt_attr *ase;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	ase_id = test_ase_id_get(fixture->ase);

	const uint8_t buf[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,
	};

	test_receiver_stop_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						       &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_receiver_stop_ready_too_short)
{
	const uint8_t buf[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x01,           /* Number_of_ASEs */
	};
	const struct bt_gatt_attr *ase;
	uint8_t ase_id;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	test_ase_src_get(1, &ase);
	zassume_not_null(ase);
	ase_id = test_ase_id_get(fixture->ase);

	test_receiver_stop_ready_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						       &fixture->stream, buf, sizeof(buf));
}

static void test_update_metadata_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
						       const struct bt_gatt_attr *ase_cp,
						       struct bt_bap_stream *stream,
						       const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x07,           /* Opcode = Update Metadata */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};

	test_preamble_state_enabling(conn, ase_id, stream);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_update_metadata_number_of_ases_0x00)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x07,           /* Opcode = Update Metadata */
		0x00,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,           /* Metadata_Length[0] */
	};

	test_update_metadata_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_update_metadata_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x07,             /* Opcode = Update Metadata */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
		0x00,             /* Metadata_Length[0] */
	};

	test_update_metadata_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_update_metadata_too_long)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x07,           /* Opcode = Update Metadata */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,           /* Metadata_Length[0] */
		0x00,
	};

	test_update_metadata_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_update_metadata_too_short)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x07,           /* Opcode = Update Metadata */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
	};

	test_update_metadata_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_update_metadata_metadata_too_short)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x07,           /* Opcode = Update Metadata */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x03,           /* Metadata_Length[0] */
		0x02, 0x02,     /* Metadata[0] */
	};

	test_update_metadata_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
						   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_update_metadata_invalid_ase_id)
{
	/* Test requires support for at least 2 ASEs */
	if (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT < 2) {
		ztest_test_skip();
	}

	const uint8_t ase_id_valid = 0x01;
	const uint8_t ase_id_invalid = CONFIG_BT_ASCS_ASE_SNK_COUNT +
				       CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;
	const uint8_t buf[] = {
		0x07,                   /* Opcode = Update Metadata */
		0x02,                   /* Number_of_ASEs */
		ase_id_invalid,         /* ASE_ID[0] */
		0x04,                   /* Metadata_Length[0] */
		0x03, 0x02, 0x04, 0x00, /* Metadata[0] = Streaming Context (Media) */
		ase_id_valid,           /* ASE_ID[1] */
		0x04,                   /* Metadata_Length[0] */
		0x03, 0x02, 0x04, 0x00, /* Metadata[0] = Streaming Context (Media) */
	};
	const uint8_t data_expected[] = {
		0x07,                   /* Opcode = Update Metadata */
		0x02,                   /* Number_of_ASEs */
		ase_id_invalid,         /* ASE_ID[0] */
		0x03,                   /* Response_Code[0] = Invalid ASE_ID */
		0x00,                   /* Reason[0] */
		ase_id_valid,           /* ASE_ID[1] */
		0x00,                   /* Response_Code[1] = Success */
		0x00,                   /* Reason[1] */
	};

	test_preamble_state_enabling(&fixture->conn, ase_id_valid, &fixture->stream);

	fixture->ase_cp->write(&fixture->conn, fixture->ase_cp, buf, sizeof(buf), 0, 0);

	expect_bt_gatt_notify_cb_called_once(&fixture->conn, BT_UUID_ASCS_ASE_CP,
					     fixture->ase_cp, data_expected, sizeof(data_expected));
}

static void test_release_expect_invalid_length(struct bt_conn *conn, uint8_t ase_id,
					       const struct bt_gatt_attr *ase_cp,
					       struct bt_bap_stream *stream,
					       const uint8_t *buf, size_t len)
{
	const uint8_t data_expected[] = {
		0x08,           /* Opcode = Release */
		0xFF,           /* Number_of_ASEs */
		0x00,           /* ASE_ID[0] */
		0x02,           /* Response_Code[0] = Invalid Length */
		0x00,           /* Reason[0] */
	};

	test_preamble_state_enabling(conn, ase_id, stream);

	ase_cp->write(conn, ase_cp, buf, len, 0, 0);

	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, data_expected,
					     sizeof(data_expected));
}

ZTEST_F(test_ase_control_params, test_release_number_of_ases_0x00)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x08,           /* Opcode = Release */
		0x00,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
	};

	test_release_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_release_number_of_ases_above_max)
{
	const uint16_t ase_cnt = CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT + 1;

	/* Skip if number of ASEs configured is high enough to support any value in the write req */
	if (ase_cnt > UINT8_MAX) {
		ztest_test_skip();
	}

	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x08,             /* Opcode = Release */
		(uint8_t)ase_cnt, /* Number_of_ASEs */
		ase_id,           /* ASE_ID[0] */
	};

	test_release_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_release_too_long)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x08,           /* Opcode = Release */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x00,
	};

	test_release_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}

ZTEST_F(test_ase_control_params, test_release_too_short)
{
	const uint8_t ase_id = test_ase_id_get(fixture->ase);
	const uint8_t buf[] = {
		0x08,           /* Opcode = Release */
		0x01,           /* Number_of_ASEs */
	};

	test_release_expect_invalid_length(&fixture->conn, ase_id, fixture->ase_cp,
					   &fixture->stream, buf, sizeof(buf));
}
