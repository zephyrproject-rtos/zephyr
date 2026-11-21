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

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include "common_defs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(is_subscribed);
DEFINE_FLAG_STATIC(flag_data_length_updated);
DEFINE_FLAG_STATIC(flag_long_write_done);

static atomic_t nwrites;
static atomic_t indications;
static atomic_t notifications;

/* Defined in conn.c */
extern void bt_conn_suspend_tx(bool suspend);

static struct bt_conn *dconn;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	if (conn_err) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), conn_err);
		return;
	}

	LOG_DBG("%s", bt_conn_dst_str(conn));

	dconn = bt_conn_ref(conn);
	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("%p %s (reason 0x%02x)", conn, bt_conn_dst_str(conn), reason);

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
	TEST_ASSERT(err == 0, "Can't update data length (err %d)", err);

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
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	LOG_DBG("Connecting to %s", bt_addr_le_str(addr));

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		TEST_FAIL("Create conn failed (err %d)", err);
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
	TEST_ASSERT(!err, "Scanning failed to start (err %d)", err);

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
		bt_conn_suspend_tx(true);
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

	TEST_ASSERT(length >= sizeof(indication), "Unexpected data");
	TEST_ASSERT(length <= sizeof(notification), "Unexpected data");

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
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);

	TEST_ASSERT(params, "params is NULL");

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
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);

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
	TEST_ASSERT(!err, "Failed to transmit handle for write (err %d)", err);
}

void test_procedure_0(void)
{
	LOG_DBG("Test start: ATT sequential protocol");
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);
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
	 * - update data length (tinyhost doesn't have recombination)
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
	bt_conn_suspend_tx(false);

	TEST_PASS("DUT done");
}

static uint8_t long_write_err;

static void long_write_done(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	long_write_err = err;
	SET_FLAG(flag_long_write_done);
}

static void long_write(uint16_t offset, const uint8_t *data, uint16_t length)
{
	static struct bt_gatt_write_params params;
	int err;

	params.func = long_write_done;
	params.handle = LONG_WRITE_HANDLE;
	params.offset = offset;
	params.data = data;
	params.length = length;

	UNSET_FLAG(flag_long_write_done);

	err = bt_gatt_write(dconn, &params);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);

	WAIT_FOR_FLAG(flag_long_write_done);
	TEST_ASSERT(long_write_err == BT_ATT_ERR_INVALID_PDU,
		    "Long write reported 0x%02x, want a cancelled write (0x%02x)", long_write_err,
		    BT_ATT_ERR_INVALID_PDU);
}

void test_procedure_1(void)
{
	/* More than one Prepare Write Request with the default ATT_MTU of 23 */
	static uint8_t value[40];
	int err;

	for (size_t i = 0U; i < sizeof(value); i++) {
		value[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Core v7.0 Vol 3 Part G 4.9.4 Write Long Characteristic Value
	 *
	 * 4.9.4 does not require the client to verify the Prepare Write
	 * Response, but the Host does, as 4.9.5 Reliable Writes requires. On a
	 * value it did not send, it cancels the write; verify the application
	 * is then told the write failed.
	 *
	 * Test procedure:
	 * - dut: long write; tester: echo with a wrong value, expect a cancel
	 * - dut: write at an offset; tester: echo one octet more than sent,
	 *   expect a cancel
	 *
	 * [verdict]
	 * - both writes report an error to the application
	 */
	connect();

	long_write(0U, value, sizeof(value));
	long_write(1U, value, 4U);

	TEST_PASS("DUT done");
}

static void two_done(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	TEST_ASSERT(err == 0, "Long write failed (err %d)", err);
	(void)atomic_inc(&nwrites);
}

void test_procedure_2(void)
{
	static uint8_t va[40], vb[40];
	static struct bt_gatt_write_params pa, pb;
	int err;

	for (size_t i = 0U; i < sizeof(va); i++) {
		va[i] = (uint8_t)(0xa0U + (i & 0x0fU));
		vb[i] = (uint8_t)(0xb0U + (i & 0x0fU));
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Core v7.0 Vol 3 Part F 3.4.6 shared prepare queue
	 *
	 * Two long writes to different handles are started on one connection
	 * without waiting for the first to finish. The server keeps one
	 * prepare queue per connection, so the host has to write one of them
	 * completely, Execute Write and all, before it prepares the other.
	 *
	 * [verdict]
	 * - the tester never sees the two handles' prepares interleave
	 * - neither params can be given again while its write is in use
	 */
	connect();

	pa = (struct bt_gatt_write_params){
		.func = two_done, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){
		.func = two_done, .handle = LONG_WRITE_HANDLE_2, .data = vb, .length = sizeof(vb)};

	err = bt_gatt_write(dconn, &pa);
	TEST_ASSERT(err == 0, "Failed to start the first long write (err %d)", err);
	err = bt_gatt_write(dconn, &pb);
	TEST_ASSERT(err == 0, "Failed to start the second long write (err %d)", err);

	/* The first one is being sent, the second one waits */
	err = bt_gatt_write(dconn, &pa);
	TEST_ASSERT(err == -EBUSY, "Giving the first long write again returned %d", err);
	err = bt_gatt_write(dconn, &pb);
	TEST_ASSERT(err == -EBUSY, "Giving the second long write again returned %d", err);

	WAIT_FOR_VAL(nwrites, 2);

	TEST_PASS("DUT done");
}

static uint8_t pe_err[2];
static atomic_t pe_done;

static void pe_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	atomic_val_t i = atomic_get(&pe_done);

	TEST_ASSERT((size_t)i < ARRAY_SIZE(pe_err), "Unexpected write callback");
	pe_err[i] = err;
	(void)atomic_inc(&pe_done);
}

void test_procedure_3(void)
{
	static uint8_t va[40], vb[40];
	static struct bt_gatt_write_params pa, pb;
	int err;

	for (size_t i = 0U; i < sizeof(va); i++) {
		va[i] = (uint8_t)i;
		vb[i] = (uint8_t)(0x40U + i);
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Issue #117590: a long write rejected part-way must clear the
	 * server's prepare queue so the next long write is not appended to it.
	 *
	 * [verdict]
	 * - the first write reports the server error, the second succeeds
	 */
	connect();

	pa = (struct bt_gatt_write_params){
		.func = pe_cb, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){
		.func = pe_cb, .handle = LONG_WRITE_HANDLE, .data = vb, .length = sizeof(vb)};

	err = bt_gatt_write(dconn, &pa);
	TEST_ASSERT(err == 0, "Failed to start the first long write (err %d)", err);
	err = bt_gatt_write(dconn, &pb);
	TEST_ASSERT(err == 0, "Failed to start the second long write (err %d)", err);

	WAIT_FOR_VAL(pe_done, 2);

	TEST_ASSERT(pe_err[0] == BT_ATT_ERR_PREPARE_QUEUE_FULL,
		    "The rejected write reported 0x%02x, want 0x%02x", pe_err[0],
		    BT_ATT_ERR_PREPARE_QUEUE_FULL);
	TEST_ASSERT(pe_err[1] == 0, "The second write failed (err %u)", pe_err[1]);

	TEST_PASS("DUT done");
}

static atomic_t rel_prepared;
static atomic_t rel_executed;
static uint8_t rel_exec_err;

static void rel_prepared_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	TEST_ASSERT(err == 0, "Reliable prepare failed (err %d)", err);
	(void)atomic_inc(&rel_prepared);
}

static void rel_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	rel_exec_err = err;
	(void)atomic_inc(&rel_executed);
}

void test_procedure_4(void)
{
	static uint8_t va[4] = {1, 2, 3, 4};
	static uint8_t vb[4] = {5, 6, 7, 8};
	static struct bt_gatt_reliable_write write = {.func = rel_exec_cb};
	static struct bt_gatt_write_params pa, pb;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Core v7.0 Vol 3 Part G 4.9.5 Characteristic Value Reliable Writes
	 *
	 * Prepare two values with bt_gatt_prepare_write(), then commit
	 * them with one bt_gatt_execute_write(). No Execute Write is sent
	 * until the application asks for it.
	 *
	 * [verdict]
	 * - both values are prepared, then one committing Execute Write
	 */
	connect();

	pa = (struct bt_gatt_write_params){.func = rel_prepared_cb,
					   .handle = LONG_WRITE_HANDLE,
					   .data = va,
					   .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){.func = rel_prepared_cb,
					   .handle = LONG_WRITE_HANDLE_2,
					   .data = vb,
					   .length = sizeof(vb)};

	err = bt_gatt_prepare_write(dconn, &write, &pa);
	TEST_ASSERT(err == 0, "First reliable prepare failed (err %d)", err);
	WAIT_FOR_VAL(rel_prepared, 1);

	err = bt_gatt_prepare_write(dconn, &write, &pb);
	TEST_ASSERT(err == 0, "Second reliable prepare failed (err %d)", err);
	WAIT_FOR_VAL(rel_prepared, 2);

	err = bt_gatt_execute_write(dconn, &write, true);
	TEST_ASSERT(err == 0, "Execute write failed (err %d)", err);
	WAIT_FOR_VAL(rel_executed, 1);
	TEST_ASSERT(rel_exec_err == 0, "Execute write reported err %u", rel_exec_err);

	/* A second session, cancelled this time. The first one consumed pa. */
	pa = (struct bt_gatt_write_params){.func = rel_prepared_cb,
					   .handle = LONG_WRITE_HANDLE,
					   .data = va,
					   .length = sizeof(va)};
	err = bt_gatt_prepare_write(dconn, &write, &pa);
	TEST_ASSERT(err == 0, "Reliable prepare (cancel case) failed (err %d)", err);
	WAIT_FOR_VAL(rel_prepared, 3);

	err = bt_gatt_execute_write(dconn, &write, false);
	TEST_ASSERT(err == 0, "Cancel write failed (err %d)", err);
	WAIT_FOR_VAL(rel_executed, 2);
	TEST_ASSERT(rel_exec_err == 0, "Cancel reported err %u", rel_exec_err);

	TEST_PASS("DUT done");
}

static atomic_t rq_long_done;
static atomic_t rq_prepared;
static atomic_t rq_executed;
static uint8_t rq_exec_err;

static void rq_long_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	TEST_ASSERT(err == 0, "Long write failed (err %d)", err);
	(void)atomic_inc(&rq_long_done);
}

static void rq_prepared_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	TEST_ASSERT(err == 0, "Waiting reliable prepare failed (err %d)", err);
	(void)atomic_inc(&rq_prepared);
}

static void rq_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	rq_exec_err = err;
	(void)atomic_inc(&rq_executed);
}

void test_procedure_5(void)
{
	static uint8_t va[40];
	static uint8_t vb[4] = {1, 2, 3, 4};
	static struct bt_gatt_reliable_write write = {.func = rq_exec_cb};
	static struct bt_gatt_write_params pa, pb;
	int err;

	for (size_t i = 0U; i < sizeof(va); i++) {
		va[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * A reliable write value given while a long write owns the prepare
	 * queue waits, and its session starts once the long write is done.
	 *
	 * [verdict]
	 * - the value is prepared after the long write, then committed
	 */
	connect();

	pa = (struct bt_gatt_write_params){
		.func = rq_long_cb, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){.func = rq_prepared_cb,
					   .handle = LONG_WRITE_HANDLE_2,
					   .data = vb,
					   .length = sizeof(vb)};

	err = bt_gatt_write(dconn, &pa);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);
	err = bt_gatt_prepare_write(dconn, &write, &pb);
	TEST_ASSERT(err == 0, "Reliable prepare failed (err %d)", err);

	WAIT_FOR_VAL(rq_prepared, 1);
	TEST_ASSERT(atomic_get(&rq_long_done) == 1, "The value was prepared before the long write");

	err = bt_gatt_execute_write(dconn, &write, true);
	TEST_ASSERT(err == 0, "Execute write failed (err %d)", err);
	WAIT_FOR_VAL(rq_executed, 1);
	TEST_ASSERT(rq_exec_err == 0, "Execute write reported err %u", rq_exec_err);

	TEST_PASS("DUT done");
}

enum {
	RR_FIRST,
	RR_REJECTED,
	RR_LONG,
	RR_COUNT,
};

static atomic_t rr_done;
static uint8_t rr_err[RR_COUNT];

static void rr_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	atomic_val_t i = atomic_get(&rr_done);

	TEST_ASSERT((size_t)i < ARRAY_SIZE(rr_err), "Unexpected write callback");
	rr_err[i] = err;
	(void)atomic_inc(&rr_done);
}

static void rr_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	TEST_FAIL("The ended reliable write was executed (err %u)", err);
}

void test_procedure_6(void)
{
	static uint8_t v1[4] = {1, 2, 3, 4};
	static uint8_t v2[4] = {5, 6, 7, 8};
	static uint8_t vl[40];
	static struct bt_gatt_reliable_write write = {.func = rr_exec_cb};
	static struct bt_gatt_write_params p1, p2, pl;
	int err;

	for (size_t i = 0U; i < sizeof(vl); i++) {
		vl[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Core v7.0 Vol 3 Part G 4.9.5 Characteristic Value Reliable Writes
	 *
	 * The server rejects the second value of a reliable write. The write
	 * ends: the host cancels the first value, which the server holds.
	 *
	 * [verdict]
	 * - the rejected value reports the server error
	 * - the first value is cancelled and the write cannot be executed
	 * - a long write afterwards succeeds
	 */
	connect();

	p1 = (struct bt_gatt_write_params){
		.func = rr_cb, .handle = LONG_WRITE_HANDLE, .data = v1, .length = sizeof(v1)};
	p2 = (struct bt_gatt_write_params){
		.func = rr_cb, .handle = LONG_WRITE_HANDLE_2, .data = v2, .length = sizeof(v2)};
	pl = (struct bt_gatt_write_params){
		.func = rr_cb, .handle = LONG_WRITE_HANDLE, .data = vl, .length = sizeof(vl)};

	err = bt_gatt_prepare_write(dconn, &write, &p1);
	TEST_ASSERT(err == 0, "First reliable prepare failed (err %d)", err);
	WAIT_FOR_VAL(rr_done, 1);
	TEST_ASSERT(rr_err[RR_FIRST] == 0, "The first value reported err %u", rr_err[RR_FIRST]);

	err = bt_gatt_prepare_write(dconn, &write, &p2);
	TEST_ASSERT(err == 0, "Second reliable prepare failed (err %d)", err);
	WAIT_FOR_VAL(rr_done, 2);
	TEST_ASSERT(rr_err[RR_REJECTED] == BT_ATT_ERR_PREPARE_QUEUE_FULL,
		    "The rejected value reported 0x%02x, want 0x%02x", rr_err[RR_REJECTED],
		    BT_ATT_ERR_PREPARE_QUEUE_FULL);

	err = bt_gatt_execute_write(dconn, &write, true);
	TEST_ASSERT(err == -EINVAL, "Execute write of the ended write returned %d", err);

	err = bt_gatt_write(dconn, &pl);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);
	WAIT_FOR_VAL(rr_done, 3);
	TEST_ASSERT(rr_err[RR_LONG] == 0, "The long write reported err %u", rr_err[RR_LONG]);

	TEST_PASS("DUT done");
}

static atomic_t cw_done;
static atomic_t cw_cancelled;
static uint8_t cw_cancel_err;

static void cw_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	TEST_ASSERT(err == 0, "Long write failed (err %d)", err);
	(void)atomic_inc(&cw_done);
}

static void cw_cancel_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	cw_cancel_err = err;
	(void)atomic_inc(&cw_cancelled);
}

void test_procedure_7(void)
{
	static uint8_t va[40], vb[40];
	static struct bt_gatt_write_params pa, pb, pc;
	int err;

	for (size_t i = 0U; i < sizeof(va); i++) {
		va[i] = (uint8_t)(0xa0U + (i & 0x0fU));
		vb[i] = (uint8_t)(0xb0U + (i & 0x0fU));
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * bt_gatt_cancel() completes a long write that waits for the prepare
	 * queue, as it completes one with a request in flight.
	 *
	 * [verdict]
	 * - the waiting write reports BT_ATT_ERR_UNLIKELY once cancelled
	 * - it is never sent; the writes before and after it are
	 */
	connect();

	pa = (struct bt_gatt_write_params){
		.func = cw_cb, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){.func = cw_cancel_cb,
					   .handle = LONG_WRITE_HANDLE_2,
					   .data = vb,
					   .length = sizeof(vb)};
	pc = (struct bt_gatt_write_params){
		.func = cw_cb, .handle = LONG_WRITE_HANDLE_2, .data = vb, .length = sizeof(vb)};

	err = bt_gatt_write(dconn, &pa);
	TEST_ASSERT(err == 0, "Failed to start the first long write (err %d)", err);
	err = bt_gatt_write(dconn, &pb);
	TEST_ASSERT(err == 0, "Failed to start the second long write (err %d)", err);

	bt_gatt_cancel(dconn, &pb);
	TEST_ASSERT(atomic_get(&cw_cancelled) == 1, "The waiting write was not completed");
	TEST_ASSERT(cw_cancel_err == BT_ATT_ERR_UNLIKELY, "The waiting write reported 0x%02x",
		    cw_cancel_err);

	err = bt_gatt_write(dconn, &pc);
	TEST_ASSERT(err == 0, "Failed to start the third long write (err %d)", err);

	WAIT_FOR_VAL(cw_done, 2);
	TEST_ASSERT(atomic_get(&cw_cancelled) == 1, "The cancelled write reported again");

	TEST_PASS("DUT done");
}

enum {
	RD_FAILED,
	RD_DROPPED,
	RD_LONG,
	RD_COUNT,
};

static atomic_t rd_done;
static uint8_t rd_err[RD_COUNT];

static void rd_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	atomic_val_t i = atomic_get(&rd_done);

	TEST_ASSERT((size_t)i < ARRAY_SIZE(rd_err), "Unexpected write callback");
	rd_err[i] = err;
	(void)atomic_inc(&rd_done);
}

static void rd_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	TEST_FAIL("The failed reliable write was executed (err %u)", err);
}

void test_procedure_8(void)
{
	static uint8_t v1[40];
	static uint8_t v2[4] = {5, 6, 7, 8};
	static uint8_t vl[40];
	static struct bt_gatt_reliable_write write = {.func = rd_exec_cb};
	static struct bt_gatt_write_params p1, p2, pl;
	int err;

	for (size_t i = 0U; i < sizeof(v1); i++) {
		v1[i] = (uint8_t)i;
		vl[i] = (uint8_t)(0x40U + i);
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Core v7.0 Vol 3 Part G 4.9.5 Characteristic Value Reliable Writes
	 *
	 * Two values of a reliable write are given at once, and the server
	 * rejects the second Prepare Write Request of the first one. The
	 * write ends, so the value that waits is not sent.
	 *
	 * [verdict]
	 * - the first value reports the server error, then the waiting one
	 *   BT_ATT_ERR_UNLIKELY, and the write cannot be executed
	 * - a long write afterwards succeeds
	 */
	connect();

	p1 = (struct bt_gatt_write_params){
		.func = rd_cb, .handle = LONG_WRITE_HANDLE, .data = v1, .length = sizeof(v1)};
	p2 = (struct bt_gatt_write_params){
		.func = rd_cb, .handle = LONG_WRITE_HANDLE_2, .data = v2, .length = sizeof(v2)};
	pl = (struct bt_gatt_write_params){
		.func = rd_cb, .handle = LONG_WRITE_HANDLE, .data = vl, .length = sizeof(vl)};

	err = bt_gatt_prepare_write(dconn, &write, &p1);
	TEST_ASSERT(err == 0, "First reliable prepare failed (err %d)", err);
	err = bt_gatt_prepare_write(dconn, &write, &p2);
	TEST_ASSERT(err == 0, "Second reliable prepare failed (err %d)", err);

	WAIT_FOR_VAL(rd_done, 2);
	TEST_ASSERT(rd_err[RD_FAILED] == BT_ATT_ERR_PREPARE_QUEUE_FULL,
		    "The rejected value reported 0x%02x, want 0x%02x", rd_err[RD_FAILED],
		    BT_ATT_ERR_PREPARE_QUEUE_FULL);
	TEST_ASSERT(rd_err[RD_DROPPED] == BT_ATT_ERR_UNLIKELY,
		    "The waiting value reported 0x%02x, want 0x%02x", rd_err[RD_DROPPED],
		    BT_ATT_ERR_UNLIKELY);

	err = bt_gatt_execute_write(dconn, &write, true);
	TEST_ASSERT(err == -EINVAL, "Execute write of the ended write returned %d", err);

	err = bt_gatt_write(dconn, &pl);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);
	WAIT_FOR_VAL(rd_done, 3);
	TEST_ASSERT(rd_err[RD_LONG] == 0, "The long write reported err %u", rd_err[RD_LONG]);

	TEST_PASS("DUT done");
}

static atomic_t rc_done;
static uint8_t rc_err[2];

static void rc_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	TEST_FAIL("The failed reliable write was executed (err %u)", err);
}

static struct bt_gatt_reliable_write rc_write = {.func = rc_exec_cb};

static void rc_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	atomic_val_t i = atomic_get(&rc_done);

	TEST_ASSERT((size_t)i < ARRAY_SIZE(rc_err), "Unexpected write callback");
	rc_err[i] = err;
	(void)atomic_inc(&rc_done);

	/* The write has ended for the application, which may reuse it */
	if (i == 0) {
		(void)memset(&rc_write, 0, sizeof(rc_write));
		rc_write.func = rc_exec_cb;
	}
}

void test_procedure_9(void)
{
	static uint8_t va[4] = {1, 2, 3, 4};
	static uint8_t vb[4] = {5, 6, 7, 8};
	static struct bt_gatt_write_params pa, pb;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * bt_gatt_cancel() on a value of a reliable write that waits fails the
	 * write, as a value the server rejects does.
	 *
	 * [verdict]
	 * - the cancelled value reports BT_ATT_ERR_UNLIKELY, then the value in
	 *   flight once the server's copy is cancelled
	 * - the write cannot be executed
	 * - the host does not rely on the write once the cancelled value has
	 *   reported, when the application reuses it
	 */
	connect();

	pa = (struct bt_gatt_write_params){
		.func = rc_cb, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){
		.func = rc_cb, .handle = LONG_WRITE_HANDLE_2, .data = vb, .length = sizeof(vb)};

	err = bt_gatt_prepare_write(dconn, &rc_write, &pa);
	TEST_ASSERT(err == 0, "First reliable prepare failed (err %d)", err);
	err = bt_gatt_prepare_write(dconn, &rc_write, &pb);
	TEST_ASSERT(err == 0, "Second reliable prepare failed (err %d)", err);

	bt_gatt_cancel(dconn, &pb);
	TEST_ASSERT(atomic_get(&rc_done) == 1, "The waiting value was not completed");
	TEST_ASSERT(rc_err[0] == BT_ATT_ERR_UNLIKELY, "The waiting value reported 0x%02x",
		    rc_err[0]);

	err = bt_gatt_execute_write(dconn, &rc_write, true);
	TEST_ASSERT(err == -EBUSY, "Execute write while the cancel runs returned %d", err);

	WAIT_FOR_VAL(rc_done, 2);
	TEST_ASSERT(rc_err[1] == BT_ATT_ERR_UNLIKELY, "The value in flight reported 0x%02x",
		    rc_err[1]);

	err = bt_gatt_execute_write(dconn, &rc_write, true);
	TEST_ASSERT(err == -EINVAL, "Execute write of the failed write returned %d", err);

	TEST_PASS("DUT done");
}

enum {
	RI_FIRST,
	RI_CANCELLED,
	RI_LONG,
	RI_COUNT,
};

static struct bt_gatt_write_params ri_params[RI_COUNT];
static atomic_t ri_done;
static uint8_t ri_err[RI_COUNT];

static void ri_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	size_t i = (size_t)(params - ri_params);

	TEST_ASSERT(i < ARRAY_SIZE(ri_params), "Unexpected write callback");
	ri_err[i] = err;
	(void)atomic_inc(&ri_done);

	/* The first value is prepared, so nothing of the session is in flight */
	if ((i == RI_FIRST) && (err == 0U)) {
		bt_gatt_cancel(conn, &ri_params[RI_CANCELLED]);
	}
}

static void ri_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	TEST_FAIL("The failed reliable write was executed (err %u)", err);
}

void test_procedure_10(void)
{
	static uint8_t va[4] = {1, 2, 3, 4};
	static uint8_t vb[4] = {5, 6, 7, 8};
	static uint8_t vc[40];
	static struct bt_gatt_reliable_write write = {.func = ri_exec_cb};
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * bt_gatt_cancel() on a waiting value of a reliable write, from the
	 * callback of the value before it, when nothing of the write is in
	 * flight, fails the write.
	 *
	 * [verdict]
	 * - the cancelled value reports BT_ATT_ERR_UNLIKELY
	 * - the server's copy of the first value is cancelled, and a long
	 *   write given afterwards is written
	 * - the write cannot be executed
	 */
	connect();

	ri_params[RI_FIRST] = (struct bt_gatt_write_params){
		.func = ri_cb, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	ri_params[RI_CANCELLED] = (struct bt_gatt_write_params){
		.func = ri_cb, .handle = LONG_WRITE_HANDLE_2, .data = vb, .length = sizeof(vb)};
	ri_params[RI_LONG] = (struct bt_gatt_write_params){
		.func = ri_cb, .handle = LONG_WRITE_HANDLE, .data = vc, .length = sizeof(vc)};

	err = bt_gatt_prepare_write(dconn, &write, &ri_params[RI_FIRST]);
	TEST_ASSERT(err == 0, "First reliable prepare failed (err %d)", err);
	err = bt_gatt_prepare_write(dconn, &write, &ri_params[RI_CANCELLED]);
	TEST_ASSERT(err == 0, "Second reliable prepare failed (err %d)", err);

	WAIT_FOR_VAL(ri_done, 2);
	TEST_ASSERT(ri_err[RI_FIRST] == 0, "The first value reported 0x%02x", ri_err[RI_FIRST]);
	TEST_ASSERT(ri_err[RI_CANCELLED] == BT_ATT_ERR_UNLIKELY,
		    "The cancelled value reported 0x%02x", ri_err[RI_CANCELLED]);

	err = bt_gatt_write(dconn, &ri_params[RI_LONG]);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);
	WAIT_FOR_VAL(ri_done, 3);
	TEST_ASSERT(ri_err[RI_LONG] == 0, "The long write reported 0x%02x", ri_err[RI_LONG]);

	err = bt_gatt_execute_write(dconn, &write, true);
	TEST_ASSERT(err == -EINVAL, "Execute write of the failed write returned %d", err);

	TEST_PASS("DUT done");
}

static atomic_t re_done;
static uint8_t re_err;

static void re_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	re_err = err;
	(void)atomic_inc(&re_done);
}

static void re_exec_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_reliable_write *write)
{
	TEST_FAIL("A reliable write with a wrong echo was executed (err %u)", err);
}

void test_procedure_11(void)
{
	static uint8_t va[4] = {1, 2, 3, 4};
	static uint8_t vc[40];
	static struct bt_gatt_reliable_write write = {.func = re_exec_cb};
	static struct bt_gatt_write_params params;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * Core v7.0 Vol 3 Part G 4.9.5: a reliable write value whose echo is not
	 * what was sent cancels the write. The tester echoes another handle,
	 * then one octet less.
	 *
	 * [verdict]
	 * - each value reports BT_ATT_ERR_INVALID_PDU and its write cannot be
	 *   executed
	 * - a long write given afterwards is written
	 */
	connect();

	for (int i = 0; i < 2; i++) {
		params = (struct bt_gatt_write_params){.func = re_cb,
						       .handle = LONG_WRITE_HANDLE,
						       .data = va,
						       .length = sizeof(va)};

		err = bt_gatt_prepare_write(dconn, &write, &params);
		TEST_ASSERT(err == 0, "Reliable prepare failed (err %d)", err);
		WAIT_FOR_VAL(re_done, i + 1);
		TEST_ASSERT(re_err == BT_ATT_ERR_INVALID_PDU,
			    "The value reported 0x%02x, want 0x%02x", re_err,
			    BT_ATT_ERR_INVALID_PDU);

		err = bt_gatt_execute_write(dconn, &write, true);
		TEST_ASSERT(err == -EINVAL, "Execute write of the failed write returned %d", err);
	}

	params = (struct bt_gatt_write_params){
		.func = re_cb, .handle = LONG_WRITE_HANDLE, .data = vc, .length = sizeof(vc)};
	err = bt_gatt_write(dconn, &params);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);
	WAIT_FOR_VAL(re_done, 3);
	TEST_ASSERT(re_err == 0, "The long write reported 0x%02x", re_err);

	TEST_PASS("DUT done");
}

static atomic_t lf_done;
static uint8_t lf_err[2];

static void lf_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	atomic_val_t i = atomic_get(&lf_done);

	TEST_ASSERT((size_t)i < ARRAY_SIZE(lf_err), "Unexpected write callback");
	lf_err[i] = err;
	(void)atomic_inc(&lf_done);
}

void test_procedure_12(void)
{
	static uint8_t va[40];
	static uint8_t vb[40];
	static struct bt_gatt_write_params pa, pb;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	/* Test purpose:
	 * bt_gatt_cancel() on a long write whose first Prepare Write Request is
	 * in flight cancels what the server may have queued of it, so the next
	 * long write is not written together with it.
	 *
	 * [verdict]
	 * - the cancelled write reports BT_ATT_ERR_UNLIKELY
	 * - the server gets a cancel before the next long write is executed
	 */
	connect();

	pa = (struct bt_gatt_write_params){
		.func = lf_cb, .handle = LONG_WRITE_HANDLE, .data = va, .length = sizeof(va)};
	pb = (struct bt_gatt_write_params){
		.func = lf_cb, .handle = LONG_WRITE_HANDLE_2, .data = vb, .length = sizeof(vb)};

	err = bt_gatt_write(dconn, &pa);
	TEST_ASSERT(err == 0, "Failed to start the long write (err %d)", err);
	bt_gatt_cancel(dconn, &pa);

	WAIT_FOR_VAL(lf_done, 1);
	TEST_ASSERT(lf_err[0] == BT_ATT_ERR_UNLIKELY, "The cancelled write reported 0x%02x",
		    lf_err[0]);

	err = bt_gatt_write(dconn, &pb);
	TEST_ASSERT(err == 0, "Failed to start the second long write (err %d)", err);
	WAIT_FOR_VAL(lf_done, 2);
	TEST_ASSERT(lf_err[1] == 0, "The second long write reported 0x%02x", lf_err[1]);

	TEST_PASS("DUT done");
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "dut",
		.test_main_f = test_procedure_0,
	},
	{
		.test_id = "dut_long_write_echo",
		.test_main_f = test_procedure_1,
	},
	{
		.test_id = "dut_long_write_serialised",
		.test_main_f = test_procedure_2,
	},
	{
		.test_id = "dut_long_write_prep_error",
		.test_main_f = test_procedure_3,
	},
	{
		.test_id = "dut_reliable_write",
		.test_main_f = test_procedure_4,
	},
	{
		.test_id = "dut_reliable_write_queued",
		.test_main_f = test_procedure_5,
	},
	{
		.test_id = "dut_reliable_write_rejected",
		.test_main_f = test_procedure_6,
	},
	{
		.test_id = "dut_long_write_cancel_waiting",
		.test_main_f = test_procedure_7,
	},
	{
		.test_id = "dut_reliable_write_dropped",
		.test_main_f = test_procedure_8,
	},
	{
		.test_id = "dut_reliable_write_cancel_waiting",
		.test_main_f = test_procedure_9,
	},
	{
		.test_id = "dut_reliable_write_cancel_idle",
		.test_main_f = test_procedure_10,
	},
	{
		.test_id = "dut_reliable_write_bad_echo",
		.test_main_f = test_procedure_11,
	},
	{
		.test_id = "dut_long_write_cancel_in_flight",
		.test_main_f = test_procedure_12,
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
