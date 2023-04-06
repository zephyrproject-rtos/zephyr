/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util_macro.h>

#include "bap_unicast_server.h"
#include "bap_stream.h"
#include "gatt_expects.h"
#include "conn.h"
#include "gatt.h"
#include "iso.h"
#include "mock_kernel.h"
#include "pacs.h"

#include "test_common.h"

void test_mocks_init(void)
{
	mock_bap_unicast_server_init();
	mock_bt_iso_init();
	mock_kernel_init();
	mock_bt_pacs_init();
	mock_bap_stream_init();
	mock_bt_gatt_init();
}

void test_mocks_cleanup(void)
{
	mock_bap_unicast_server_cleanup();
	mock_bt_iso_cleanup();
	mock_kernel_cleanup();
	mock_bt_pacs_cleanup();
	mock_bap_stream_cleanup();
	mock_bt_gatt_cleanup();
}

void test_mocks_reset(void)
{
	test_mocks_cleanup();
	test_mocks_init();
}

static uint8_t attr_found(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
	const struct bt_gatt_attr **result = user_data;

	*result = attr;

	return BT_GATT_ITER_STOP;
}

void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_PERIPHERAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}

const struct bt_gatt_attr *test_ase_control_point_get(void)
{
	static const struct bt_gatt_attr *attr;

	if (attr == NULL) {
		bt_gatt_foreach_attr_type(BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					  BT_ATT_LAST_ATTRIBUTE_HANDLE,
					  BT_UUID_ASCS_ASE_CP, NULL, 1, attr_found, &attr);
	}

	zassert_not_null(attr, "ASE Control Point not found");

	return attr;

}

uint8_t test_ase_get(const struct bt_uuid *uuid, int num_ase, ...)
{
	const struct bt_gatt_attr *attr = NULL;
	va_list attrs;
	uint8_t i;

	va_start(attrs, num_ase);

	for (i = 0; i < num_ase; i++) {
		const struct bt_gatt_attr *prev = attr;

		bt_gatt_foreach_attr_type(BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					  BT_ATT_LAST_ATTRIBUTE_HANDLE,
					  uuid, NULL, i + 1, attr_found, &attr);

		/* Another attribute was not found */
		if (attr == prev) {
			break;
		}

		*(va_arg(attrs, const struct bt_gatt_attr **)) = attr;
	}

	va_end(attrs);

	return i;
}

uint8_t test_ase_id_get(const struct bt_gatt_attr *ase)
{
	const struct test_ase_chrc_value_hdr *hdr;
	ssize_t ret;

	ret = ase->read(NULL, ase, NULL, 0, 0);
	zassert_false(ret < 0, "ase->read returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));

	expect_bt_gatt_attr_read_called_once(NULL, ase, EMPTY, EMPTY, 0, EMPTY, sizeof(*hdr));

	hdr = bt_gatt_attr_read_fake.arg5_val;

	/* Reset the mock state */
	bt_gatt_attr_read_reset();

	return hdr->ase_id;
}

static struct bt_bap_stream *stream_allocated;
static const struct bt_codec_qos_pref qos_pref = BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M,
								   0x02, 10, 40000, 40000,
								   40000, 40000);

static int unicast_server_cb_config_custom_fake(struct bt_conn *conn, const struct bt_bap_ep *ep,
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

void test_ase_control_client_config_codec(struct bt_conn *conn, uint8_t ase_id,
					  struct bt_bap_stream *stream)
{
	const struct bt_gatt_attr *attr = test_ase_control_point_get();
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

	ssize_t ret;

	stream_allocated = stream;
	mock_bap_unicast_server_cb_config_fake.custom_fake = unicast_server_cb_config_custom_fake;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "cp_attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));

	stream_allocated = NULL;
}

void test_ase_control_client_config_qos(struct bt_conn *conn, uint8_t ase_id)
{
	const struct bt_gatt_attr *attr = test_ase_control_point_get();
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
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

void test_ase_control_client_enable(struct bt_conn *conn, uint8_t ase_id)
{
	const struct bt_gatt_attr *attr = test_ase_control_point_get();
	const uint8_t buf[] = {
		0x03,                   /* Opcode = Enable */
		0x01,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
		0x00,                   /* Metadata_Length[0] */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

void test_ase_control_client_disable(struct bt_conn *conn, uint8_t ase_id)
{
	const struct bt_gatt_attr *attr = test_ase_control_point_get();
	const uint8_t buf[] = {
		0x05,                   /* Opcode = Disable */
		0x01,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

void test_ase_control_client_release(struct bt_conn *conn, uint8_t ase_id)
{
	const struct bt_gatt_attr *attr = test_ase_control_point_get();
	const uint8_t buf[] = {
		0x08,                   /* Opcode = Disable */
		0x01,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

void test_ase_control_client_update_metadata(struct bt_conn *conn, uint8_t ase_id)
{
	const struct bt_gatt_attr *attr = test_ase_control_point_get();
	const uint8_t buf[] = {
		0x07,                   /* Opcode = Update Metadata */
		0x01,                   /* Number_of_ASEs */
		ase_id,                 /* ASE_ID[0] */
		0x03,                   /* Metadata_Length[0] */
		0x02, 0x02, 0x04,       /* Metadata[0] = Streaming Context (Media) */
	};
	ssize_t ret;

	ret = attr->write(conn, attr, (void *)buf, sizeof(buf), 0, 0);
	zassert_false(ret < 0, "attr->write returned unexpected (err 0x%02x)", BT_GATT_ERR(ret));
}

void test_preamble_state_codec_configured(struct bt_conn *conn, uint8_t ase_id,
					  struct bt_bap_stream *stream)
{
	test_ase_control_client_config_codec(conn, ase_id, stream);
	test_mocks_reset();
}

void test_preamble_state_qos_configured(struct bt_conn *conn, uint8_t ase_id,
					struct bt_bap_stream *stream)
{
	test_ase_control_client_config_codec(conn, ase_id, stream);
	test_ase_control_client_config_qos(conn, ase_id);
	test_mocks_reset();
}

void test_preamble_state_enabling(struct bt_conn *conn, uint8_t ase_id,
				  struct bt_bap_stream *stream)
{
	test_ase_control_client_config_codec(conn, ase_id, stream);
	test_ase_control_client_config_qos(conn, ase_id);
	test_ase_control_client_enable(conn, ase_id);
	test_mocks_reset();
}

void test_preamble_state_streaming(struct bt_conn *conn, uint8_t ase_id,
				   struct bt_bap_stream *stream, struct bt_iso_chan **chan,
				   bool source)
{
	int err;

	test_ase_control_client_config_codec(conn, ase_id, stream);
	test_ase_control_client_config_qos(conn, ase_id);
	test_ase_control_client_enable(conn, ase_id);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, chan);
	zassert_equal(0, err, "Failed to connect iso: err %d", err);

	if (source) {
		test_ase_control_client_receiver_start_ready(conn, ase_id);
	}

	test_mocks_reset();
}
