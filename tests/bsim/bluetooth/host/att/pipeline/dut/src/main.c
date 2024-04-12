/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#include "testlib/att_read.h"

#include <argparse.h>		/* For get_device_nbr() */
#include "utils.h"
#include "bstests.h"

#define strucc struct

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

DEFINE_FLAG(is_connected);
DEFINE_FLAG(is_subscribed);
DEFINE_FLAG(one_indication);
DEFINE_FLAG(two_notifications);
DEFINE_FLAG(flag_data_length_updated);

static struct bt_conn *dconn;

/* Defined in hci_core.c */
extern k_tid_t bt_testing_tx_tid_get(void);

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_DBG("%s", addr);

	dconn = bt_conn_ref(conn);
	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%p %s (reason 0x%02x)", conn, addr, reason);

	UNSET_FLAG(is_connected);
}

static void data_len_updated(struct bt_conn *conn,
			     struct bt_conn_le_data_len_info *info)
{
	LOG_DBG("Data length updated: TX %d RX %d",
		info->tx_max_len,
		info->rx_max_len);
	SET_FLAG(flag_data_length_updated);
}

static void do_dlu(struct bt_conn *conn)
{
	int err;
	struct bt_conn_le_data_len_param param;

	param.tx_max_len = CONFIG_BT_CTLR_DATA_LENGTH_MAX;
	param.tx_max_time = 2500;

	LOG_INF("update DL");
	err = bt_conn_le_data_len_update(conn, &param);
	ASSERT(err == 0, "Can't update data length (err %d)\n", err);

	WAIT_FOR_FLAG(flag_data_length_updated);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_data_len_updated = data_len_updated,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char str[BT_ADDR_LE_STR_LEN];
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	bt_addr_le_to_str(addr, str, sizeof(str));
	LOG_DBG("Connecting to %s", str);

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		FAIL("Create conn failed (err %d)", err);
		return;
	}
}

/* In your area */
#define ADV_PARAM_SINGLE BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME, \
					 BT_GAP_ADV_FAST_INT_MIN_2,	\
					 BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static strucc bt_conn *connecc(void)
{
	int err;
	struct bt_conn *conn;

	UNSET_FLAG(is_connected);

	err = bt_le_adv_start(ADV_PARAM_SINGLE, NULL, 0, NULL, 0);
	ASSERT(!err, "Adving failed to start (err %d)\n", err);

	LOG_DBG(" wait connecc...");

	WAIT_FOR_FLAG(is_connected);
	LOG_INF("conecd");

	conn = dconn;
	dconn = NULL;

	return conn;
}

static struct bt_conn *connect(void)
{
	int err;
	struct bt_conn *conn;

	UNSET_FLAG(is_connected);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, device_found);
	ASSERT(!err, "Scanning failed to start (err %d)\n", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG(is_connected);
	LOG_INF("Connected as central");

	conn = dconn;
	dconn = NULL;

	return conn;
}

static ssize_t read_from(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t buf_len, uint16_t offset)
{
	static uint16_t counter;

	LOG_INF("read from: len %d", buf_len);

	memset(buf, 0, buf_len);
	sys_put_le16(counter, buf);
	counter++;

	LOG_HEXDUMP_DBG(buf, buf_len, "Response data");

	return sizeof(uint16_t);
}

static ssize_t written_to(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_INF("written to: handle 0x%x len %d flags 0x%x",
		attr->handle,
		len,
		flags);

	LOG_HEXDUMP_DBG(buf, len, "Write data");

	return len;
}

#define test_service_uuid                                                                          \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf0debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))
#define test_characteristic_uuid                                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf2debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))

BT_GATT_SERVICE_DEFINE(test_gatt_service, BT_GATT_PRIMARY_SERVICE(test_service_uuid),
		       BT_GATT_CHARACTERISTIC(test_characteristic_uuid,
					      (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
					       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE),
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					      read_from, written_to, NULL),
		       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),);

static void send_write_handle(struct bt_conn *conn)
{
	int err;
	uint16_t handle;
	uint8_t data[sizeof(handle)];
	const struct bt_gatt_attr *attr = &test_gatt_service.attrs[2];

	/* Inform tester which handle it should write to */
	handle = bt_gatt_attr_get_handle(attr);
	sys_put_le16(handle, data);

	err = bt_gatt_notify(conn, attr, data, sizeof(data));
	ASSERT(!err, "Failed to transmit handle for write (err %d)\n", err);
}

static void gatt_read(struct bt_conn *conn, uint16_t handle)
{
	static uint16_t prev_val;
	uint16_t value;
	int err;

	NET_BUF_SIMPLE_DEFINE(buf, BT_ATT_MAX_ATTRIBUTE_LEN);

	err = bt_testlib_att_read_by_handle_sync(&buf, NULL, NULL, conn, 0, handle, 0);
	ASSERT(!err, "Failed read: err %d", err);

	value = net_buf_simple_pull_le16(&buf);

	ASSERT(prev_val == value, "Something's up: expected %d got %d", prev_val, value);
	prev_val++;

	LOG_INF("Read by handle: handle %x val %d err %d", handle, value, err);
}

static void find_the_chrc(struct bt_conn *conn, const struct bt_uuid *svc,
			  const struct bt_uuid *chrc, uint16_t *chrc_value_handle)
{
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;
	int err;

	err = bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, svc, 1, 0xffff);
	ASSERT(!err, "Failed to discover service %d");

	LOG_DBG("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	err = bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle,
						      NULL, conn, chrc, (svc_handle + 1),
						      svc_end_handle);
	ASSERT(!err, "Failed to get value handle %d");

	LOG_DBG("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);
}

void good_peer_procedure(void)
{
	LOG_DBG("Test 0 start: good peer");
	int err;
	uint16_t handle;
	struct bt_conn *conn;

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central: Bluetooth initialized.");

	conn = connecc();

	find_the_chrc(conn, test_service_uuid, test_characteristic_uuid, &handle);

	uint32_t timeout_ms = PROCEDURE_1_TIMEOUT_MS;
	uint32_t start_time = k_uptime_get_32();

	while (k_uptime_get_32() - start_time < timeout_ms) {
		gatt_read(conn, handle);
	}

	PASS("Good peer done\n");
}

void dut_procedure(void)
{
	LOG_DBG("Test 0 start: DUT");
	int err;

	struct bt_conn *good, *bad;

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central: Bluetooth initialized.");

	LOG_DBG("Central: Connect to good peer");
	good = connect();

	LOG_DBG("Central: Connect to bad peer");
	bad = connect();

	LOG_DBG("Central: Connected to both peers");

	do_dlu(bad);
	send_write_handle(bad);

	/* Pass unless some assert in callbacks fails. */
	PASS("DUT done\n");
}

void test_procedure_0(void)
{
	/* Test purpose:
	 *
	 * Verify that a Zephyr host server/client combo can tolerate a spec
	 * violating peer that batches ATT requests without waiting for
	 * responses.
	 *
	 * To do this, the application on the DUT will be connected to two
	 * peers:
	 *
	 * - a "nice" peer, running a legal stress test, that is, running a
	 * discovery procedure over and over again.
	 * - a "bad" peer, spamming ATT requests as fast as possible.
	 *
	 * The good peer uses the Zephyr host to send requests.
	 * The bad peer uses the tinyhost (raw hci) to send requests.
	 *
	 * The DUT is allowed to disconnect the ACL of the bad peer.
	 * If that happens, the bad peer will reconnect and continue.
	 * The connection with the good peer must remain stable.
	 *
	 * Test procedure:
	 * At the same time, and for T > ATT_TIMEOUT:
	 * - Good peer sends valid ATT write requests to DUT
	 * - Good peer validates ATT responses from DUT
	 * - Bad peer sends ATT requests as fast as it can
	 *
	 * [verdict]
	 * - no buffer allocation failures for responding to the good peer,
	 * timeouts or stalls.
	 */
	bool dut = (get_device_nbr() == DUT_DEVICE_NBR);

	/* We use the same image for both to lighten build load. */
	if (dut) {
		dut_procedure();
	} else {
		good_peer_procedure();
	}
}

static void write_done(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_write_params *params)
{
	LOG_INF("Write done: err %d", err);
}

static void gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params)
{
	static uint8_t data[10] = {1};
	int err;

	memset(params, 0, sizeof(*params));
	params->handle = 0x1337;
	params->func = write_done;
	params->length = sizeof(data);
	params->data = data;

	LOG_INF("Queue GATT write");

	err = bt_gatt_write(conn, params);
	ASSERT(!err, "Failed write: err %d", err);
}

void test_procedure_1(void)
{
	/* Test purpose:
	 *
	 * Verify that the Zephyr host does not pipeline ATT requests.
	 * I.e. always waits for a response before enqueuing the next request.
	 *
	 * Test procedure:
	 *
	 * - DUT sends a bunch of ATT reads in a loop
	 * - Tester delays responses to allow for the LL to transport any other requests.
	 * - Tester fails if it detects another request before it has sent the response
	 *
	 */

	LOG_DBG("Test start: ATT pipeline protocol");
	int err;

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central: Bluetooth initialized.");

	struct bt_conn *tester = connect();

	do_dlu(tester);

	static struct bt_gatt_write_params parmesans[100];

	for (int i = 0; i < ARRAY_SIZE(parmesans); i++) {
		gatt_write(tester, &parmesans[i]);
	}

	bt_conn_disconnect(tester, BT_HCI_ERR_REMOTE_POWER_OFF);
	bt_conn_unref(tester);

	/* Pass unless some assert in callbacks fails. */
	PASS("DUT done\n");
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "dut",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_procedure_0,
	},
	{
		.test_id = "dut_1",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_procedure_1,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
