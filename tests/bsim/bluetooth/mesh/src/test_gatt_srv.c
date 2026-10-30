/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"
#include "mesh/proxy_msg.h"

#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>

#include <testlib/att_read.h>
#include <testlib/att_write.h>
#include <testlib/scan.h>

#define WAIT_TIME 10 /* seconds */

#define TESTER_NAME "gatt_srv tester"

extern const struct bt_mesh_comp comp;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static struct bt_conn *link_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	ASSERT_EQUAL(0, err);

	link_conn = bt_conn_ref(conn);
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_drop(&link_conn);
	k_sem_give(&disconnected_sem);
}

static struct bt_conn_cb conn_cb = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Connect to the tester as central, a link that neither Mesh GATT server accepts, and wait until
 * the tester is done with it.
 */
static void dut_serve_tester(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t addr;

	ASSERT_OK(bt_conn_cb_register(&conn_cb));

	/* Mesh does not need the scanner in this test: use it to find the tester. */
	ASSERT_OK(bt_mesh_scan_disable());
	ASSERT_OK(bt_testlib_scan_find_name(&addr, TESTER_NAME));

	ASSERT_OK(bt_conn_le_create(&addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				    &conn));
	ASSERT_OK_MSG(k_sem_take(&connected_sem, K_SECONDS(WAIT_TIME)), "No connection");
	ASSERT_OK_MSG(k_sem_take(&disconnected_sem, K_SECONDS(WAIT_TIME)), "No disconnection");

	bt_conn_unref(conn);
}

static void test_dut_proxy(void)
{
	static const struct bt_mesh_test_cfg dut_cfg = {
		.addr = 0x0001,
		.dev_key = { 0x01 },
	};

	bt_mesh_test_cfg_set(&dut_cfg, WAIT_TIME);
	bt_mesh_test_setup();

	dut_serve_tester();

	PASS();
}

static void test_dut_pb_gatt(void)
{
	static const uint8_t dev_uuid[16] = { 0x6d, 0x65, 0x73, 0x68 };
	static const struct bt_mesh_prov prov = {
		.uuid = dev_uuid,
	};

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_GATT));

	dut_serve_tester();

	PASS();
}

/* Write to Data In and to the CCC of Data Out of a Mesh GATT service on a link that the server
 * did not accept: the write to the CCC must fail and the DUT must still respond afterwards.
 */
static void tester_write(const struct bt_uuid *svc_uuid, const struct bt_uuid *data_in_uuid,
			 const struct bt_uuid *data_out_uuid, uint8_t pdu_type)
{
	const struct bt_data ad[] = {
		BT_DATA(BT_DATA_NAME_COMPLETE, TESTER_NAME, sizeof(TESTER_NAME) - 1),
	};
	const uint8_t ccc_write[] = { BT_BYTES_LIST_LE16(BT_GATT_CCC_NOTIFY) };
	uint16_t svc_start, svc_end, data_in, data_out, ccc;
	/* The Proxy PDU header of a complete message, without any payload. */
	const uint8_t pdu[] = { pdu_type };
	struct bt_conn *conn;
	int err;

	NET_BUF_SIMPLE_DEFINE(ccc_read, sizeof(uint16_t));

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	ASSERT_OK(bt_conn_cb_register(&conn_cb));
	ASSERT_OK(bt_enable(NULL));
	ASSERT_OK(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0));
	ASSERT_OK_MSG(k_sem_take(&connected_sem, K_SECONDS(WAIT_TIME)), "No connection");
	conn = bt_conn_ref(link_conn);

	ASSERT_OK(bt_testlib_gatt_discover_primary(&svc_start, &svc_end, conn, svc_uuid,
						   BT_ATT_FIRST_ATTRIBUTE_HANDLE,
						   BT_ATT_LAST_ATTRIBUTE_HANDLE));
	ASSERT_OK(bt_testlib_gatt_discover_characteristic(&data_in, NULL, NULL, conn, data_in_uuid,
							  svc_start, svc_end));
	ASSERT_OK(bt_testlib_gatt_discover_characteristic(&data_out, NULL, NULL, conn,
							  data_out_uuid, svc_start, svc_end));
	ASSERT_OK(bt_testlib_att_read_by_type_sync(NULL, NULL, &ccc, NULL, conn,
						   BT_ATT_CHAN_OPT_NONE, BT_UUID_GATT_CCC,
						   data_out + 1, svc_end));

	ASSERT_OK(bt_gatt_write_without_response(conn, data_in, pdu, sizeof(pdu), false));

	err = bt_testlib_att_write(conn, BT_ATT_CHAN_OPT_NONE, ccc, ccc_write, sizeof(ccc_write));
	ASSERT_TRUE_MSG(err > 0, "CCC write not refused (err %d)", err);

	ASSERT_OK(bt_testlib_att_read_by_handle_sync(&ccc_read, NULL, NULL, conn,
						     BT_ATT_CHAN_OPT_NONE, ccc, 0));
	ASSERT_EQUAL(2, ccc_read.len);
	ASSERT_EQUAL(0, sys_get_le16(ccc_read.data));

	ASSERT_OK(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
	bt_conn_unref(conn);

	PASS();
}

static void test_tester_proxy(void)
{
	tester_write(BT_UUID_MESH_PROXY, BT_UUID_MESH_PROXY_DATA_IN, BT_UUID_MESH_PROXY_DATA_OUT,
		     BT_MESH_PROXY_NET_PDU);
}

static void test_tester_pb_gatt(void)
{
	tester_write(BT_UUID_MESH_PROV, BT_UUID_MESH_PROV_DATA_IN, BT_UUID_MESH_PROV_DATA_OUT,
		     BT_MESH_PROXY_PROV);
}

#define TEST_CASE(role, name, description)                                                         \
	{                                                                                          \
		.test_id = "gatt_srv_" #role "_" #name, .test_descr = description,                 \
		.test_tick_f = bt_mesh_test_timeout, .test_main_f = test_##role##_##name,          \
	}

static const struct bst_test_instance test_gatt_srv[] = {
	TEST_CASE(dut, proxy, "Provisioned node with a link the Proxy Server did not accept"),
	TEST_CASE(dut, pb_gatt, "PB-GATT node with a link the PB-GATT Server did not accept"),

	TEST_CASE(tester, proxy, "Write to the Mesh Proxy Service on a link not accepted"),
	TEST_CASE(tester, pb_gatt, "Write to the Mesh Provisioning Service on a link not accepted"),

	BSTEST_END_MARKER};

struct bst_test_list *test_gatt_srv_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_gatt_srv);
	return tests;
}
