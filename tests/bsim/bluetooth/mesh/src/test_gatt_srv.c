/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/proxy.h"
#include "mesh/proxy_cli.h"
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

#define DUT_ADDR    0x0001
#define TESTER_ADDR 0x0002
#define MSG_LEN     5

#define PENDING_RELAY_COUNT 4

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

static K_SEM_DEFINE(mesh_wq_blocked_sem, 0, 1);
static K_SEM_DEFINE(mesh_wq_release_sem, 0, 1);

static void mesh_wq_block(struct k_work *work)
{
	k_sem_give(&mesh_wq_blocked_sem);
	k_sem_take(&mesh_wq_release_sem, K_FOREVER);
}

static K_WORK_DEFINE(mesh_wq_block_work, mesh_wq_block);

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

static void test_dut_proxy_relay_pending(void)
{
	static const struct bt_mesh_test_cfg dut_cfg = {
		.addr = DUT_ADDR,
		.dev_key = { 0x01 },
	};
	struct bt_mesh_adv *advs[CONFIG_BT_MESH_ADV_BUF_COUNT];

	bt_mesh_test_cfg_set(&dut_cfg, WAIT_TIME);
	ASSERT_OK(bt_conn_cb_register(&conn_cb));
	bt_mesh_test_setup();

	/* Keep the local advertising buffers for the relayed messages only, and receive the
	 * tester's message over GATT only, so that the Proxy Server adds the tester to its filter.
	 */
	bt_mesh_beacon_set(false);
	ASSERT_OK(bt_mesh_scan_disable());

	ASSERT_OK(bt_mesh_test_recv(MSG_LEN, DUT_ADDR, NULL, K_SECONDS(WAIT_TIME)));
	ASSERT_OK(bt_mesh_test_send(TESTER_ADDR, NULL, MSG_LEN, 0, K_SECONDS(1)));

	/* Keep the relays in the proxy queue of the link until the link is terminated. */
	ASSERT_EQUAL(1, bt_mesh_wq_submit(&mesh_wq_block_work));
	ASSERT_OK(k_sem_take(&mesh_wq_blocked_sem, K_SECONDS(WAIT_TIME)));

	for (int i = 0; i < PENDING_RELAY_COUNT; i++) {
		ASSERT_OK(bt_mesh_test_send(TESTER_ADDR, NULL, MSG_LEN, 0, K_NO_WAIT));
	}

	ASSERT_OK(bt_conn_disconnect(link_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
	WAIT_FOR_COND(bt_mesh_proxy_srv_connected_cnt() == 0, WAIT_TIME);

	k_sem_give(&mesh_wq_release_sem);

	/* Let the advertiser send the messages and release them. */
	k_sleep(K_SECONDS(2));

	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		advs[i] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
					     BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
		ASSERT_TRUE_MSG(advs[i] != NULL, "Only %d advertising buffers free\n", i);
	}

	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		bt_mesh_adv_unref(advs[i]);
	}

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

static void test_tester_proxy_relay_pending(void)
{
	static const struct bt_mesh_test_cfg tester_cfg = {
		.addr = TESTER_ADDR,
		.dev_key = { 0x02 },
	};
	int err;

	bt_mesh_test_cfg_set(&tester_cfg, WAIT_TIME);
	bt_mesh_test_setup();

	ASSERT_OK(bt_mesh_proxy_connect(0));
	WAIT_FOR_COND(bt_mesh_proxy_cli_is_connected(0), WAIT_TIME);

	/* The Proxy Client relays messages only once it has enabled notifications on the link. */
	err = -ETIMEDOUT;
	for (int i = 0; i < WAIT_TIME && err == -ETIMEDOUT; i++) {
		ASSERT_OK(bt_mesh_test_send(DUT_ADDR, NULL, MSG_LEN, 0, K_SECONDS(1)));
		err = bt_mesh_test_recv(MSG_LEN, TESTER_ADDR, NULL, K_MSEC(500));
	}
	ASSERT_TRUE_MSG(err != -ETIMEDOUT, "No reply from the DUT\n");
	ASSERT_OK(err);

	WAIT_FOR_COND(!bt_mesh_proxy_cli_is_connected(0), WAIT_TIME);
	/* Do not reconnect once the DUT has terminated the link. */
	ASSERT_OK(bt_mesh_proxy_disconnect(0));

	PASS();
}

#define TEST_CASE(role, name, description)                                                         \
	{                                                                                          \
		.test_id = "gatt_srv_" #role "_" #name, .test_descr = description,                 \
		.test_tick_f = bt_mesh_test_timeout, .test_main_f = test_##role##_##name,          \
	}

static const struct bst_test_instance test_gatt_srv[] = {
	TEST_CASE(dut, proxy, "Provisioned node with a link the Proxy Server did not accept"),
	TEST_CASE(dut, pb_gatt, "PB-GATT node with a link the PB-GATT Server did not accept"),
	TEST_CASE(dut, proxy_relay_pending, "Proxy Server terminating the link with relays queued"),

	TEST_CASE(tester, proxy, "Write to the Mesh Proxy Service on a link not accepted"),
	TEST_CASE(tester, pb_gatt, "Write to the Mesh Provisioning Service on a link not accepted"),
	TEST_CASE(tester, proxy_relay_pending, "Proxy Client whose link the DUT terminates"),

	BSTEST_END_MARKER};

struct bst_test_list *test_gatt_srv_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_gatt_srv);
	return tests;
}
