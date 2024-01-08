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

#include "utils.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

DEFINE_FLAG(is_connected);
DEFINE_FLAG(is_subscribed);
DEFINE_FLAG(one_indication);
DEFINE_FLAG(two_notifications);
DEFINE_FLAG(flag_data_length_updated);

static atomic_t nwrites;
static atomic_t indications;
static atomic_t notifications;

/* Defined in hci_core.c */
extern k_tid_t bt_testing_tx_tid_get(void);

static struct bt_conn *dconn;

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

	bt_conn_unref(dconn);
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

static void do_dlu(void)
{
	int err;
	struct bt_conn_le_data_len_param param;

	param.tx_max_len = CONFIG_BT_CTLR_DATA_LENGTH_MAX;
	param.tx_max_time = 2500;

	err = bt_conn_le_data_len_update(dconn, &param);
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

static void connect(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	UNSET_FLAG(is_connected);

	err = bt_le_scan_start(&scan_param, device_found);
	ASSERT(!err, "Scanning failed to start (err %d)\n", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG(is_connected);
	LOG_INF("Connected as central");

	/* No security support on the tinyhost unfortunately */
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

	if (atomic_get(&nwrites) == 0) {
		/* Suspend on the first write, which is an ATT Request */
		LOG_INF("suspending HCI TX thread");
		k_thread_suspend(bt_testing_tx_tid_get());
	}

	atomic_inc(&nwrites);

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
					      NULL, written_to, NULL),
		       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),);

static uint8_t notified(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	static uint8_t notification[] = NOTIFICATION_PAYLOAD;
	static uint8_t indication[] = INDICATION_PAYLOAD;
	bool is_nfy;

	ASSERT(length >= sizeof(indication), "Unexpected data");
	ASSERT(length <= sizeof(notification), "Unexpected data");

	LOG_HEXDUMP_DBG(data, length, "HVx data");

	is_nfy = memcmp(data, notification, length) == 0;

	LOG_INF("%s from 0x%x", is_nfy ? "notified" : "indicated",
		params->value_handle);

	if (is_nfy) {
		atomic_inc(&notifications);
	} else {
		atomic_inc(&indications);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void subscribed(struct bt_conn *conn,
		       uint8_t err,
		       struct bt_gatt_subscribe_params *params)
{
	ASSERT(!err, "Subscribe failed (err %d)\n", err);

	ASSERT(params, "params is NULL\n");

	SET_FLAG(is_subscribed);
	/* spoiler: tester doesn't really have attributes */
	LOG_INF("Subscribed to Tester attribute");
}

void subscribe(void)
{
	int err;

	/* Handle values don't matter, as long as they match on the tester */
	static struct bt_gatt_subscribe_params params = {
		.notify = notified,
		.subscribe = subscribed,
		.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
		.value_handle = HVX_HANDLE,
		.ccc_handle = (HVX_HANDLE + 1),
	};

	err = bt_gatt_subscribe(dconn, &params);
	ASSERT(!err, "Subscribe failed (err %d)\n", err);

	WAIT_FOR_FLAG(is_subscribed);
}

static void send_write_handle(void)
{
	int err;
	uint16_t handle;
	uint8_t data[sizeof(handle)];
	const struct bt_gatt_attr *attr = &test_gatt_service.attrs[2];

	/* Inform tester which handle it should write to */
	handle = bt_gatt_attr_get_handle(attr);
	sys_put_le16(handle, data);

	err = bt_gatt_notify(dconn, attr, data, sizeof(data));
	ASSERT(!err, "Failed to transmit handle for write (err %d)\n", err);
}

void test_procedure_0(void)
{
	LOG_DBG("Test start: ATT sequential protocol");
	int err;

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central: Bluetooth initialized.");

	/* Test purpose:
	 * Test Spec V.3 P.F 3.3.2 Sequential protocol
	 *
	 * Verify that a Zephyr host server/client combo can process
	 * concurrently: one Request, one Indication, multiple
	 * Notifications and multiple Commands.
	 *
	 * To do this, the application on the DUT will purposefully stall the
	 * HCI TX thread, ensuring that the responses are not sent until the
	 * tester has finished sending everything.
	 *
	 * Test procedure:
	 *
	 * [setup]
	 * - connect ACL
	 * - update data length (tinyhost doens't have recombination)
	 * - dut: subscribe to INDICATE and NOTIFY on tester CHRC
	 * - dut: send a handle the tester can write to
	 *
	 * [proc]
	 * - tester: send one ATT write request
	 * - tester: send one ATT indication
	 * - tester: send two ATT notifications
	 * - tester: send two ATT commands
	 *
	 * - dut: handle the REQuest, build & put the RSP PDU on the HCI TX queue
	 * - dut: suspend the HCI TX thread
	 * - dut: handle the INDication
	 * - dut: handle the notifications
	 * - dut: handle the (write) commands
	 * - dut: resume the TX thread after a short while
	 *
	 * [verdict]
	 * - all procedures complete successfully, no buffer allocation failures
	 *   or timeouts.
	 */
	connect();
	subscribe();

	do_dlu();

	send_write_handle();

	WAIT_FOR_VAL(indications, 1);
	WAIT_FOR_VAL(notifications, 2);
	/* One REQ, two CMDs */
	WAIT_FOR_VAL(nwrites, 3);

	/* Send RSP to LL */
	k_thread_resume(bt_testing_tx_tid_get());

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
