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

DEFINE_FFF_GLOBALS;

struct attr_found_data {
	const struct bt_gatt_attr *attr;
	uint16_t found_cnt;
};

static uint8_t attr_found_func(const struct bt_gatt_attr *attr, uint16_t handle,
			       void *user_data)
{
	struct attr_found_data *data = user_data;

	data->attr = attr;
	data->found_cnt++;

	return BT_GATT_ITER_CONTINUE;
}

static const struct bt_gatt_attr *ascs_get_attr(const struct bt_uuid *uuid, uint16_t nth)
{
	struct attr_found_data data = {
		.attr = NULL,
		.found_cnt = 0,
	};

	if (nth == 0) {
		return NULL;
	}

	bt_gatt_foreach_attr_type(0x0001, 0xFFFF, uuid, NULL, nth, attr_found_func, &data);

	if (data.found_cnt != nth) {
		return NULL;
	}

	return data.attr;
}

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	mock_bap_unicast_server_init();
	mock_bt_iso_init();
	mock_kernel_init();
	PACS_FFF_FAKES_LIST(RESET_FAKE);
	mock_bap_stream_init();
	mock_bt_gatt_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	mock_bap_unicast_server_cleanup();
	mock_bt_iso_cleanup();
	mock_kernel_cleanup();
	mock_bap_stream_cleanup();
	mock_bt_gatt_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

ZTEST_SUITE(ascs_attrs_test_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(ascs_attrs_test_suite, test_has_sink_ase_chrc)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	zassert_not_null(ascs_get_attr(BT_UUID_ASCS_ASE_SNK, 1));
	zassert_not_null(ascs_get_attr(BT_UUID_ASCS_ASE_SNK, CONFIG_BT_ASCS_ASE_SNK_COUNT));
}

ZTEST(ascs_attrs_test_suite, test_has_source_ase_chrc)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SRC);

	zassert_not_null(ascs_get_attr(BT_UUID_ASCS_ASE_SRC, 1));
	zassert_not_null(ascs_get_attr(BT_UUID_ASCS_ASE_SRC, CONFIG_BT_ASCS_ASE_SRC_COUNT));
}

ZTEST(ascs_attrs_test_suite, test_has_single_control_point_chrc)
{
	zassert_not_null(ascs_get_attr(BT_UUID_ASCS_ASE_CP, 1));
	zassert_is_null(ascs_get_attr(BT_UUID_ASCS_ASE_CP, 2));
}

static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 120u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

static const struct bt_codec_qos_pref qos_pref = BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M,
								   0x02, 10, 40000, 40000,
								   40000, 40000);

static void pacs_cap_foreach_custom_fake(enum bt_audio_dir dir, bt_pacs_cap_foreach_func_t func,
					 void *user_data)
{
	static const struct bt_pacs_cap cap[] = {
		{
			&lc3_codec,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cap); i++) {
		if (func(&cap[i], user_data) == false) {
			break;
		}
	}
}

struct ascs_ase_control_test_suite_fixture {
	struct bt_conn conn;
	struct bt_bap_stream stream;
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase_snk;
};

static void conn_init(struct bt_conn *conn)
{
	memset(conn, 0, sizeof(*conn));

	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_PERIPHERAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L3;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}

static void *ascs_ase_control_test_suite_setup(void)
{
	struct ascs_ase_control_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	conn_init(&fixture->conn);
	fixture->ase_cp = ascs_get_attr(BT_UUID_ASCS_ASE_CP, 1);
	fixture->ase_snk = ascs_get_attr(BT_UUID_ASCS_ASE_SNK, 1);

	return fixture;
}

static void ascs_ase_control_test_suite_before(void *f)
{
	struct ascs_ase_control_test_suite_fixture *fixture = f;

	ARG_UNUSED(fixture);

	bt_pacs_cap_foreach_fake.custom_fake = pacs_cap_foreach_custom_fake;

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

static struct bt_bap_stream *stream_allocated;

int unicast_server_cb_config_custom_fake(struct bt_conn *conn, const struct bt_bap_ep *ep,
					 enum bt_audio_dir dir, const struct bt_codec *codec,
					 struct bt_bap_stream **stream,
					 struct bt_codec_qos_pref *const pref,
					 struct bt_bap_ascs_rsp *rsp)
{
	*stream = stream_allocated;
	*pref = qos_pref;
	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);

	bt_bap_stream_cb_register(*stream, &mock_bap_stream_ops);

	return 0;
}

static void ase_cp_write_codec_config(const struct bt_gatt_attr *attr, struct bt_conn *conn,
				      struct bt_bap_stream *stream)
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
	};

	ssize_t ret;

	stream_allocated = stream;
	mock_bap_unicast_server_cb_config_fake.custom_fake = unicast_server_cb_config_custom_fake;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "cp_attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));

	stream_allocated = NULL;
}

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_control_codec_configure_from_idle)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	ase_cp_write_codec_config(fixture->ase_cp, &fixture->conn, &fixture->stream);
	expect_bt_bap_unicast_server_cb_config_called_once(&fixture->conn, EMPTY, BT_AUDIO_DIR_SINK,
							   EMPTY);
	expect_bt_bap_stream_ops_configured_called_once(&fixture->stream, EMPTY);
}

static void ase_cp_write_config_qos(const struct bt_gatt_attr *attr, struct bt_conn *conn,
				    struct bt_bap_stream *stream)
{
	const uint8_t buf[] = {
		0x02,                   /* Opcode = Config QoS */
		0x01,                   /* Number_of_ASEs */
		0x01,                   /* ASE_ID[0] */
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
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_control_qos_configure_from_codec_configured)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	/* Preamble */
	ase_cp_write_codec_config(fixture->ase_cp, &fixture->conn, &fixture->stream);

	ase_cp_write_config_qos(fixture->ase_cp, &fixture->conn, &fixture->stream);
	expect_bt_bap_unicast_server_cb_qos_called_once(&fixture->stream, EMPTY);
	expect_bt_bap_stream_ops_qos_set_called_once(&fixture->stream);
}

static void ase_cp_write_enable(const struct bt_gatt_attr *attr, struct bt_conn *conn,
				struct bt_bap_stream *stream)
{
	const uint8_t buf[] = {
		0x03,                   /* Opcode = Enable */
		0x01,                   /* Number_of_ASEs */
		0x01,                   /* ASE_ID[0] */
		0x00,                   /* Metadata_Length[0] */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_control_enable_from_qos_configured)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	/* Preamble */
	ase_cp_write_codec_config(fixture->ase_cp, &fixture->conn, &fixture->stream);
	ase_cp_write_config_qos(fixture->ase_cp, &fixture->conn, &fixture->stream);

	ase_cp_write_enable(fixture->ase_cp, &fixture->conn, &fixture->stream);
	expect_bt_bap_unicast_server_cb_enable_called_once(&fixture->stream, EMPTY, EMPTY);
	expect_bt_bap_stream_ops_enabled_called_once(&fixture->stream);
}

static void ase_cp_write_disable(const struct bt_gatt_attr *attr, struct bt_conn *conn,
				 struct bt_bap_stream *stream)
{
	const uint8_t buf[] = {
		0x05,                   /* Opcode = Disable */
		0x01,                   /* Number_of_ASEs */
		0x01,                   /* ASE_ID[0] */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_control_disable_from_enabling)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	/* Preamble */
	ase_cp_write_codec_config(fixture->ase_cp, &fixture->conn, &fixture->stream);
	ase_cp_write_config_qos(fixture->ase_cp, &fixture->conn, &fixture->stream);
	ase_cp_write_enable(fixture->ase_cp, &fixture->conn, &fixture->stream);

	/* Reset the bap_stream_ops.qos_set callback that is expected to be called again */
	mock_bap_stream_qos_set_cb_reset();

	ase_cp_write_disable(fixture->ase_cp, &fixture->conn, &fixture->stream);
	expect_bt_bap_unicast_server_cb_disable_called_once(&fixture->stream);
	expect_bt_bap_stream_ops_qos_set_called_once(&fixture->stream);
}

static void ase_cp_write_release(const struct bt_gatt_attr *attr, struct bt_conn *conn,
				 struct bt_bap_stream *stream)
{
	const uint8_t buf[] = {
		0x08,                   /* Opcode = Disable */
		0x01,                   /* Number_of_ASEs */
		0x01,                   /* ASE_ID[0] */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

ZTEST_F(ascs_ase_control_test_suite, test_sink_ase_control_release_from_qos_configured)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_ASCS_ASE_SNK);

	/* Preamble */
	ase_cp_write_codec_config(fixture->ase_cp, &fixture->conn, &fixture->stream);
	ase_cp_write_config_qos(fixture->ase_cp, &fixture->conn, &fixture->stream);
	ase_cp_write_enable(fixture->ase_cp, &fixture->conn, &fixture->stream);
	ase_cp_write_disable(fixture->ase_cp, &fixture->conn, &fixture->stream);

	/* QoS configured state */

	ase_cp_write_release(fixture->ase_cp, &fixture->conn, &fixture->stream);
	expect_bt_bap_unicast_server_cb_release_called_once(&fixture->stream);
	expect_bt_bap_stream_ops_released_called_once(&fixture->stream);
}

static void ascs_test_suite_after(void *f)
{
	bt_ascs_cleanup();
}

ZTEST_SUITE(ascs_test_suite, NULL, NULL, NULL, ascs_test_suite_after, NULL);

ZTEST(ascs_test_suite, test_release_ase_on_callback_unregister)
{
	const struct bt_uuid *ase_uuid = COND_CODE_1(CONFIG_BT_ASCS_ASE_SNK,
						     (BT_UUID_ASCS_ASE_SNK),
						     (BT_UUID_ASCS_ASE_SRC));
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase;
	struct bt_bap_stream stream;
	struct bt_conn conn;
	const uint8_t expect_ase_state_idle[] = {
		0x01,   /* ASE_ID */
		0x00,   /* ASE_State = Idle */
	};

	ase_cp = ascs_get_attr(BT_UUID_ASCS_ASE_CP, 1);
	zassert_not_null(ase_cp);

	ase = ascs_get_attr(ase_uuid, 1);
	zassert_not_null(ase);

	conn_init(&conn);

	bt_pacs_cap_foreach_fake.custom_fake = pacs_cap_foreach_custom_fake;

	bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);

	/* Set ASE to non-idle state */
	ase_cp_write_codec_config(ase_cp, &conn, &stream);

	/* Reset mock, as we expect ASE notification to be sent */
	bt_gatt_notify_cb_reset();

	/* Unregister the callbacks - whis will clean up the ASCS */
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);

	/* Expected to notify the upper layers */
	expect_bt_bap_unicast_server_cb_release_called_once(&stream);
	expect_bt_bap_stream_ops_released_called_once(&stream);

	/* Expected to notify the client */
	expect_bt_gatt_notify_cb_called_once(&conn, ase_uuid, ase, expect_ase_state_idle,
					     sizeof(expect_ase_state_idle));
}

ZTEST(ascs_test_suite, test_abort_client_operation_if_callback_not_registered)
{
	const struct bt_gatt_attr *ase_cp;
	struct bt_bap_stream stream;
	struct bt_conn conn;
	const uint8_t expect_ase_cp_unspecified_error[] = {
		0x01,           /* Opcode */
		0x01,           /* Number_of_ASEs */
		0x01,           /* ASE_ID[0] */
		0x0E,           /* Response_Code[0] = Unspecified Error */
		0x00,           /* Reason[0] */
	};

	ase_cp = ascs_get_attr(BT_UUID_ASCS_ASE_CP, 1);
	zassert_not_null(ase_cp);

	conn_init(&conn);

	bt_pacs_cap_foreach_fake.custom_fake = pacs_cap_foreach_custom_fake;

	/* Set ASE to non-idle state */
	ase_cp_write_codec_config(ase_cp, &conn, &stream);

	/* Expected ASE Control Point notification with Unspecified Error was sent */
	expect_bt_gatt_notify_cb_called_once(&conn, BT_UUID_ASCS_ASE_CP, ase_cp,
					     expect_ase_cp_unspecified_error,
					     sizeof(expect_ase_cp_unspecified_error));
}
