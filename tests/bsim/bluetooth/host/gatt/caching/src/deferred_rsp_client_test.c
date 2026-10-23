/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "deferred_rsp.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_is_encrypted);
DEFINE_FLAG_STATIC(flag_discover_complete);

static const struct bt_uuid *async_chrc_uuid = TEST_ASYNC_CHRC_UUID;

static struct bt_conn *g_conn;
static uint16_t async_chrc_handle;
static uint16_t cp_chrc_handle;
static uint16_t async2_chrc_handle;
static uint16_t sync_chrc_handle;
static uint16_t fill_chrc_handle;

/* The bearer the requests are sent on, see test_main_common() */
static enum bt_att_chan_opt chan_opt = BT_ATT_CHAN_OPT_NONE;

static K_SEM_DEFINE(rsp_sem, 0, 1);
static int rsp_err;
static uint8_t rsp_data[BT_ATT_BUF_SIZE];
static uint16_t rsp_len;

static const uint8_t write_data[WRITE_SIZE] = {0xaa, 0xbb, 0xcc, 0xdd};

/* The last Error Response on the unenhanced bearer. A GATT client is only
 * given its error code, so the PDU is looked at on its way in.
 */
static struct bt_att_error_rsp last_err;
static const struct bt_l2cap_chan_ops *att_ops;

static int watch_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	if ((buf->len >= (sizeof(struct bt_att_hdr) + sizeof(last_err))) &&
	    (buf->data[0] == BT_ATT_OP_ERROR_RSP)) {
		memcpy(&last_err, &buf->data[sizeof(struct bt_att_hdr)], sizeof(last_err));
	}

	return att_ops->recv(chan, buf);
}

static void watch_err_rsp(void)
{
	static struct bt_l2cap_chan_ops watch_ops;
	struct bt_l2cap_chan *chan = bt_l2cap_le_lookup_rx_cid(g_conn, BT_L2CAP_CID_ATT);

	att_ops = chan->ops;
	watch_ops = *att_ops;
	watch_ops.recv = watch_recv;
	chan->ops = &watch_ops;
}

static void expect_err_rsp(uint8_t op, uint16_t handle, uint8_t err)
{
	if ((last_err.request != op) || (sys_le16_to_cpu(last_err.handle) != handle) ||
	    (last_err.error != err)) {
		TEST_FAIL("Error Response 0x%02x 0x%04x 0x%02x, expected 0x%02x 0x%04x 0x%02x",
			  last_err.request, sys_le16_to_cpu(last_err.handle), last_err.error, op,
			  handle, err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);

		return;
	}

	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	printk("Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	bt_conn_drop(&g_conn);

	UNSET_FLAG(flag_is_connected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err security_err)
{
	if (security_err == BT_SECURITY_ERR_SUCCESS && level > BT_SECURITY_L1) {
		SET_FLAG(flag_is_encrypted);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (g_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d", err);

		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (async_chrc_handle == 0U || cp_chrc_handle == 0U || async2_chrc_handle == 0U ||
		    sync_chrc_handle == 0U || fill_chrc_handle == 0U) {
			TEST_FAIL("Did not discover required characteristics");
		}

		(void)memset(params, 0, sizeof(*params));

		SET_FLAG(flag_discover_complete);

		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			TEST_FAIL("Discover failed (err %d)", err);
		}

		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc = attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, async_chrc_uuid) == 0) {
			async_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_CP_CHRC_UUID) == 0) {
			cp_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_ASYNC2_CHRC_UUID) == 0) {
			async2_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_SYNC_CHRC_UUID) == 0) {
			sync_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_FILL_CHRC_UUID) == 0) {
			fill_chrc_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(void)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	discover_params.uuid = TEST_DEFERRED_SERVICE_UUID;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		TEST_FAIL("Discover failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
}

static void att_rsp(struct bt_conn *conn, int err, const void *pdu, uint16_t length,
		    void *user_data)
{
	rsp_err = err;
	rsp_len = MIN(length, sizeof(rsp_data));
	if (rsp_len > 0U) {
		memcpy(rsp_data, pdu, rsp_len);
	}

	k_sem_give(&rsp_sem);
}

static struct net_buf *att_pdu(uint8_t op, const void *data, uint16_t len)
{
	struct net_buf *buf;

	buf = bt_att_create_pdu(g_conn, op, len);
	if (buf == NULL) {
		return NULL;
	}

	bt_att_set_tx_meta_data(buf, NULL, NULL, chan_opt);
	net_buf_add_mem(buf, data, len);

	return buf;
}

/* The requests are built here so that the test controls the exact opcode and
 * can look at the response as it arrives on air.
 */
static int att_request(uint8_t op, const void *data, uint16_t len)
{
	struct bt_att_req *req;
	struct net_buf *buf;
	int err;

	req = bt_att_req_alloc(K_FOREVER);
	if (req == NULL) {
		return -ENOMEM;
	}

	buf = att_pdu(op, data, len);
	if (buf == NULL) {
		bt_att_req_free(req);

		return -ENOMEM;
	}

	req->buf = buf;
	req->func = att_rsp;

	err = bt_att_req_send(g_conn, req);
	if (err != 0) {
		bt_att_req_free(req);

		return err;
	}

	if (k_sem_take(&rsp_sem, K_SECONDS(10)) != 0) {
		return -ETIMEDOUT;
	}

	return rsp_err;
}

static void set_stage(enum test_stage stage)
{
	uint8_t pdu[sizeof(uint16_t) + 1];
	int err;

	sys_put_le16(cp_chrc_handle, &pdu[0]);
	pdu[2] = (uint8_t)stage;

	err = att_request(BT_ATT_OP_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Control point write failed (err %d)", err);
	}
}

static void expect_value(uint16_t offset, uint16_t len)
{
	for (uint16_t i = 0U; i < len; i++) {
		if (rsp_data[i] != (uint8_t)(offset + i)) {
			TEST_FAIL("Byte %u is 0x%02x", i, rsp_data[i]);

			return;
		}
	}
}

static void test_read(void)
{
	uint16_t expected = MIN(VALUE_SIZE, bt_gatt_get_mtu(g_conn) - 1);
	uint8_t pdu[sizeof(uint16_t)];
	int err;

	sys_put_le16(async_chrc_handle, pdu);

	err = att_request(BT_ATT_OP_READ_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Read failed (err %d)", err);
	}

	if (rsp_len != expected) {
		TEST_FAIL("Read returned %u bytes, expected %u", rsp_len, expected);
	}

	expect_value(0, rsp_len);
}

static void test_read_blob(void)
{
	uint8_t pdu[2 * sizeof(uint16_t)];
	int err;

	sys_put_le16(async_chrc_handle, &pdu[0]);
	sys_put_le16(BLOB_OFFSET, &pdu[2]);

	err = att_request(BT_ATT_OP_READ_BLOB_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Read blob failed (err %d)", err);
	}

	if (rsp_len != (VALUE_SIZE - BLOB_OFFSET)) {
		TEST_FAIL("Read blob returned %u bytes", rsp_len);
	}

	expect_value(BLOB_OFFSET, rsp_len);
}

static void test_read_by_type(void)
{
	uint16_t expected = MIN(VALUE_SIZE, MIN(bt_gatt_get_mtu(g_conn) - 4, UINT8_MAX - 2));
	uint8_t pdu[2 * sizeof(uint16_t) + 16];
	int err;

	sys_put_le16(BT_ATT_FIRST_ATTRIBUTE_HANDLE, &pdu[0]);
	sys_put_le16(BT_ATT_LAST_ATTRIBUTE_HANDLE, &pdu[2]);
	memcpy(&pdu[4], BT_UUID_128(async_chrc_uuid)->val, 16);

	err = att_request(BT_ATT_OP_READ_TYPE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Read by type failed (err %d)", err);
	}

	/* Length of one Attribute Data, then the handle and the value */
	if (rsp_len != (1 + sizeof(uint16_t) + expected)) {
		TEST_FAIL("Read by type returned %u bytes", rsp_len);

		return;
	}

	if (rsp_data[0] != (sizeof(uint16_t) + expected)) {
		TEST_FAIL("Read by type item length is %u", rsp_data[0]);
	}

	if (sys_get_le16(&rsp_data[1]) != async_chrc_handle) {
		TEST_FAIL("Read by type handle is 0x%04x", sys_get_le16(&rsp_data[1]));
	}

	memmove(rsp_data, &rsp_data[3], expected);
	expect_value(0, expected);
}

static void test_error(void)
{
	uint8_t pdu[sizeof(uint16_t)];
	int err;

	sys_put_le16(async_chrc_handle, pdu);

	err = att_request(BT_ATT_OP_READ_REQ, pdu, sizeof(pdu));
	if (err != TEST_ASYNC_ERR) {
		TEST_FAIL("Read answered with %d", err);
	}
}

static void test_write(void)
{
	uint8_t pdu[sizeof(uint16_t) + WRITE_SIZE];
	int err;

	sys_put_le16(async_chrc_handle, &pdu[0]);
	memcpy(&pdu[2], write_data, sizeof(write_data));

	err = att_request(BT_ATT_OP_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Write failed (err %d)", err);
	}
}

static void test_prepare_write(void)
{
	uint8_t pdu[2 * sizeof(uint16_t) + WRITE_SIZE];
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	sys_put_le16(async_chrc_handle, &pdu[0]);
	sys_put_le16(0, &pdu[2]);
	memcpy(&pdu[4], write_data, sizeof(write_data));

	err = att_request(BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Prepare write failed (err %d)", err);
	}

	/* The host echoes the queued value, the application is not involved */
	if (rsp_len != sizeof(pdu)) {
		TEST_FAIL("Prepare write response is %u bytes", rsp_len);

		return;
	}

	if (memcmp(rsp_data, pdu, sizeof(pdu)) != 0) {
		TEST_FAIL("Prepare write response does not match the request");
	}

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write failed (err %d)", err);
	}
}

/* A callback that fails without deferring answers with the attribute handle,
 * as a synchronous one does
 */
static void test_now_error(void)
{
	enum bt_att_chan_opt opt = chan_opt;
	uint8_t pdu[2 * sizeof(uint16_t)];
	int err;

	sys_put_le16(async_chrc_handle, &pdu[0]);
	sys_put_le16(VALUE_SIZE + 1U, &pdu[2]);

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_READ_BLOB_REQ, pdu, sizeof(pdu));
	chan_opt = opt;

	if (err != BT_ATT_ERR_INVALID_OFFSET) {
		TEST_FAIL("Read blob answered with %d", err);

		return;
	}

	expect_err_rsp(BT_ATT_OP_READ_BLOB_REQ, async_chrc_handle, BT_ATT_ERR_INVALID_OFFSET);
}

static void prepare(uint16_t handle)
{
	uint8_t pdu[2 * sizeof(uint16_t) + WRITE_SIZE];
	int err;

	sys_put_le16(handle, &pdu[0]);
	sys_put_le16(0, &pdu[2]);
	memcpy(&pdu[4], write_data, sizeof(write_data));

	err = att_request(BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Prepare write of 0x%04x failed (err %d)", handle, err);
	}
}

/* One value prepared for each of two asynchronous attributes, both written
 * by a single Execute Write that the server answers asynchronously
 */
static void test_execute(int expected_err)
{
	enum bt_att_chan_opt opt = chan_opt;
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	prepare(async_chrc_handle);
	prepare(async2_chrc_handle);

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	chan_opt = opt;

	if (err != expected_err) {
		TEST_FAIL("Execute write answered with %d, expected %d", err, expected_err);

		return;
	}

	if (expected_err != 0) {
		/* The handle of the value that failed */
		expect_err_rsp(BT_ATT_OP_EXEC_WRITE_REQ, async2_chrc_handle, expected_err);
	}
}

/* After the failed Execute Write the queue is empty, so this writes nothing */
static void test_execute_empty(void)
{
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write of an empty queue answered with %d", err);
	}
}

static void test_command(void)
{
	uint8_t pdu[sizeof(uint16_t) + WRITE_SIZE];
	struct net_buf *buf;
	int err;

	sys_put_le16(async_chrc_handle, &pdu[0]);
	memcpy(&pdu[2], write_data, sizeof(write_data));

	buf = att_pdu(BT_ATT_OP_WRITE_CMD, pdu, sizeof(pdu));
	if (buf == NULL) {
		TEST_FAIL("Unable to create the command");

		return;
	}

	err = bt_att_send(g_conn, buf);
	if (err != 0) {
		TEST_FAIL("Write command failed (err %d)", err);
	}
}

/* One value of a Read Multiple [Variable Length] Response */
struct mult_item {
	uint16_t handle;
	uint8_t first;
	uint16_t len;
};

/* Builds what the response should carry, given how many bytes it can hold.
 * Every value is truncated to what is left, which also makes the ones after a
 * full response disappear.
 */
static uint16_t mult_expected(uint8_t *out, uint16_t budget, const struct mult_item *items,
			      size_t count, bool var_len)
{
	uint16_t used = 0U;

	for (size_t i = 0U; i < count; i++) {
		uint16_t room = budget - used;
		uint16_t len;

		if (var_len) {
			if (room < sizeof(uint16_t)) {
				continue;
			}

			room -= sizeof(uint16_t);
		}

		len = MIN(items[i].len, room);

		if (var_len) {
			sys_put_le16(len, &out[used]);
			used += sizeof(uint16_t);
		}

		for (uint16_t j = 0U; j < len; j++) {
			out[used + j] = items[i].first + j;
		}

		used += len;
	}

	return used;
}

static void test_read_multiple(bool var_len, const struct mult_item *items, size_t count)
{
	uint8_t expected[BT_ATT_BUF_SIZE];
	uint8_t pdu[BT_ATT_BUF_SIZE];
	uint16_t expected_len;
	uint8_t op;
	int err;

	for (size_t i = 0U; i < count; i++) {
		sys_put_le16(items[i].handle, &pdu[i * sizeof(uint16_t)]);
	}

	op = var_len ? BT_ATT_OP_READ_MULT_VL_REQ : BT_ATT_OP_READ_MULT_REQ;
	expected_len = mult_expected(expected, bt_gatt_get_mtu(g_conn) - 1, items, count, var_len);

	err = att_request(op, pdu, count * sizeof(uint16_t));
	if (err != 0) {
		TEST_FAIL("Read multiple 0x%02x failed (err %d)", op, err);

		return;
	}

	if (rsp_len != expected_len) {
		TEST_FAIL("Read multiple 0x%02x returned %u bytes, expected %u", op, rsp_len,
			  expected_len);

		return;
	}

	if (memcmp(rsp_data, expected, expected_len) != 0) {
		TEST_FAIL("Read multiple 0x%02x returned the wrong values", op);
	}
}

/* An error for any of the handles fails the whole request, so none of the
 * values collected before it may be sent
 */
static void test_read_multiple_error(uint8_t op, const struct mult_item *items, size_t count,
				     int expected_err)
{
	uint8_t pdu[BT_ATT_BUF_SIZE];
	int err;

	for (size_t i = 0U; i < count; i++) {
		sys_put_le16(items[i].handle, &pdu[i * sizeof(uint16_t)]);
	}

	err = att_request(op, pdu, count * sizeof(uint16_t));
	if (err != expected_err) {
		TEST_FAIL("Read multiple 0x%02x answered with %d, expected %d", op, err,
			  expected_err);
	}
}

/* An error answered from inside the callback must leave the bearer free for
 * the next request
 */
static void test_mult_inline_error(void)
{
	uint16_t expected = SYNC_VALUE_SIZE;
	uint8_t pdu[2 * sizeof(uint16_t)];
	int err;

	sys_put_le16(async_chrc_handle, &pdu[0]);
	sys_put_le16(sync_chrc_handle, &pdu[2]);

	err = att_request(BT_ATT_OP_READ_MULT_REQ, pdu, sizeof(pdu));
	if (err != TEST_ASYNC_ERR) {
		TEST_FAIL("Read multiple answered with %d, expected %d", err, TEST_ASYNC_ERR);

		return;
	}

	sys_put_le16(sync_chrc_handle, pdu);

	err = att_request(BT_ATT_OP_READ_REQ, pdu, sizeof(uint16_t));
	if (err != 0) {
		TEST_FAIL("Read after the error answered with %d", err);

		return;
	}

	if (rsp_len != expected) {
		TEST_FAIL("Read after the error returned %u bytes, expected %u", rsp_len, expected);

		return;
	}

	expect_value(0xf0, rsp_len);
}

/* STAGE_BEARERS keeps a request outstanding on each bearer at the same time,
 * so each one needs its own response state.
 */
struct bearer_rsp {
	struct k_sem sem;
	int err;
	uint8_t data[BT_ATT_BUF_SIZE];
	uint16_t len;
};

static struct bearer_rsp uatt_rsp;
static struct bearer_rsp eatt_rsp;

static void bearer_rsp_cb(struct bt_conn *conn, int err, const void *pdu, uint16_t length,
			  void *user_data)
{
	struct bearer_rsp *rsp = user_data;

	rsp->err = err;
	rsp->len = MIN(length, sizeof(rsp->data));
	if (rsp->len > 0U) {
		memcpy(rsp->data, pdu, rsp->len);
	}

	k_sem_give(&rsp->sem);
}

/* Sends without waiting for the response */
static int bearer_request(struct bearer_rsp *rsp, enum bt_att_chan_opt opt, uint8_t op,
			  const void *data, uint16_t len)
{
	struct bt_att_req *req;
	struct net_buf *buf;
	int err;

	k_sem_init(&rsp->sem, 0, 1);

	req = bt_att_req_alloc(K_FOREVER);
	if (req == NULL) {
		return -ENOMEM;
	}

	buf = bt_att_create_pdu(g_conn, op, len);
	if (buf == NULL) {
		bt_att_req_free(req);

		return -ENOMEM;
	}

	bt_att_set_tx_meta_data(buf, NULL, NULL, opt);
	net_buf_add_mem(buf, data, len);

	req->buf = buf;
	req->func = bearer_rsp_cb;
	req->user_data = rsp;

	err = bt_att_req_send(g_conn, req);
	if (err != 0) {
		bt_att_req_free(req);
	}

	return err;
}

static bool bearer_value_ok(const struct bearer_rsp *rsp, uint16_t offset, uint16_t len,
			    const char *what)
{
	if (rsp->err != 0) {
		TEST_FAIL("%s answered with %d", what, rsp->err);

		return false;
	}

	if (rsp->len != len) {
		TEST_FAIL("%s returned %u bytes, expected %u", what, rsp->len, len);

		return false;
	}

	for (uint16_t i = 0U; i < len; i++) {
		if (rsp->data[i] != (uint8_t)(offset + i)) {
			TEST_FAIL("%s byte %u is 0x%02x", what, i, rsp->data[i]);

			return false;
		}
	}

	return true;
}

static void test_bearers(void)
{
	/* Each bearer truncates to its own MTU */
	uint16_t uatt_len = MIN(VALUE_SIZE, bt_att_get_uatt_mtu(g_conn) - 1);
	uint8_t read_pdu[sizeof(uint16_t)];
	uint8_t blob_pdu[2 * sizeof(uint16_t)];
	int err;

	sys_put_le16(async_chrc_handle, read_pdu);
	sys_put_le16(async_chrc_handle, &blob_pdu[0]);
	sys_put_le16(BLOB_OFFSET, &blob_pdu[2]);

	/* One request per bearer, both outstanding at once. The server
	 * answers them in reverse order, and each response has to come
	 * back on the bearer its own request arrived on.
	 */
	err = bearer_request(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, BT_ATT_OP_READ_REQ,
			     read_pdu, sizeof(read_pdu));
	if (err != 0) {
		TEST_FAIL("Unenhanced read failed (err %d)", err);

		return;
	}

	err = bearer_request(&eatt_rsp, BT_ATT_CHAN_OPT_ENHANCED_ONLY, BT_ATT_OP_READ_BLOB_REQ,
			     blob_pdu, sizeof(blob_pdu));
	if (err != 0) {
		TEST_FAIL("Enhanced read blob failed (err %d)", err);

		return;
	}

	if (k_sem_take(&uatt_rsp.sem, K_SECONDS(10)) != 0) {
		TEST_FAIL("The unenhanced bearer was never answered");

		return;
	}

	if (k_sem_take(&eatt_rsp.sem, K_SECONDS(10)) != 0) {
		TEST_FAIL("The enhanced bearer was never answered");

		return;
	}

	if (!bearer_value_ok(&uatt_rsp, 0, uatt_len, "Unenhanced read")) {
		return;
	}

	(void)bearer_value_ok(&eatt_rsp, BLOB_OFFSET, VALUE_SIZE - BLOB_OFFSET,
			      "Enhanced read blob");
}

/* A Prepare Write the application has not accepted yet must not be executed,
 * not even by an Execute Write that arrives on another bearer
 */
static void test_prepare_execute(void)
{
	uint8_t prep_pdu[2 * sizeof(uint16_t) + WRITE_SIZE];
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	sys_put_le16(async_chrc_handle, &prep_pdu[0]);
	sys_put_le16(0, &prep_pdu[2]);
	memcpy(&prep_pdu[4], write_data, sizeof(write_data));

	err = bearer_request(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
			     BT_ATT_OP_PREPARE_WRITE_REQ, prep_pdu, sizeof(prep_pdu));
	if (err != 0) {
		TEST_FAIL("Unenhanced prepare write failed (err %d)", err);

		return;
	}

	/* The server holds that one long enough for this to arrive while it is
	 * still deferred
	 */
	k_sleep(K_MSEC(50));

	err = bearer_request(&eatt_rsp, BT_ATT_CHAN_OPT_ENHANCED_ONLY, BT_ATT_OP_EXEC_WRITE_REQ,
			     &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Enhanced execute write failed (err %d)", err);

		return;
	}

	if (k_sem_take(&eatt_rsp.sem, K_SECONDS(10)) != 0) {
		TEST_FAIL("The execute write was never answered");

		return;
	}

	if (eatt_rsp.err != 0) {
		TEST_FAIL("Execute write answered with %d", eatt_rsp.err);

		return;
	}

	if (k_sem_take(&uatt_rsp.sem, K_SECONDS(10)) != 0) {
		TEST_FAIL("The prepare write was never answered");

		return;
	}

	if ((uatt_rsp.err != 0) || (uatt_rsp.len != sizeof(prep_pdu)) ||
	    (memcmp(uatt_rsp.data, prep_pdu, sizeof(prep_pdu)) != 0)) {
		TEST_FAIL("Prepare write answered with %d and %u bytes", uatt_rsp.err,
			  uatt_rsp.len);

		return;
	}

	/* Accepted by now, so this one does reach the attribute */
	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write after the prepare failed (err %d)", err);
	}
}

/* Sends two requests without waiting, so the second one arrives while the
 * first is still deferred
 */
static void send_pipelined(void)
{
	uint8_t pdu[sizeof(uint16_t)];
	struct net_buf *buf;

	sys_put_le16(async_chrc_handle, pdu);

	for (int i = 0; i < 2; i++) {
		if (chan_opt == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
			/* An enhanced bearer is credit flow controlled, so let
			 * the peer return the credit first. The first request
			 * stays deferred, which is what the second runs into.
			 */
			k_sleep(K_MSEC(100));
		}

		buf = att_pdu(BT_ATT_OP_READ_REQ, pdu, sizeof(pdu));
		if (buf == NULL) {
			TEST_FAIL("Unable to create the request");

			return;
		}

		if (bt_att_send(g_conn, buf) != 0) {
			TEST_FAIL("Unable to send the request");

			return;
		}
	}
}

static void test_pipeline_eatt(void)
{
	send_pipelined();

	/* Only the enhanced bearer is disconnected */
	while (bt_eatt_count(g_conn) > 0U) {
		k_sleep(K_MSEC(10));
	}

	if (!IS_FLAG_SET(flag_is_connected)) {
		TEST_FAIL("The connection went with the enhanced bearer");

		return;
	}

	chan_opt = BT_ATT_CHAN_OPT_NONE;
}

static void test_pipeline(void)
{
	send_pipelined();

	/* The unenhanced bearer takes the connection with it */
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
}

static void test_main_common(bool use_eatt)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG(flag_is_connected);

	if (use_eatt) {
		err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
		if (err != 0) {
			TEST_FAIL("Failed to start encryption (err %d)", err);

			return;
		}

		WAIT_FOR_FLAG(flag_is_encrypted);

		/* The peer opens the bearer once the link is encrypted */
		while (bt_eatt_count(g_conn) < 1U) {
			k_sleep(K_MSEC(10));
		}

		chan_opt = BT_ATT_CHAN_OPT_ENHANCED_ONLY;
	}

	gatt_discover();
	watch_err_rsp();

	/* No asynchronous attribute is involved, so the server needs no ATT
	 * buffer beyond the one of the response
	 */
	const struct mult_item sync_items[] = {
		{sync_chrc_handle, 0xf0, SYNC_VALUE_SIZE},
		{fill_chrc_handle, 0x40, FILL_VALUE_SIZE},
	};
	/* A synchronous value next to two asynchronous ones, so that one
	 * response needs more than one deferred value
	 */
	const struct mult_item mult_items[] = {
		{async2_chrc_handle, 0x80, VALUE2_SIZE},
		{async_chrc_handle, 0x00, VALUE_SIZE},
		{sync_chrc_handle, 0xf0, SYNC_VALUE_SIZE},
	};
	/* The second handle is write only */
	const struct mult_item perm_items[] = {
		{async2_chrc_handle, 0x80, VALUE2_SIZE},
		{cp_chrc_handle, 0x00, 0},
	};
	/* The synchronous value fills the response, so the asynchronous handle
	 * after it is the first one of the request and finds no room
	 */
	const struct mult_item full_items[] = {
		{fill_chrc_handle, 0x40, FILL_VALUE_SIZE},
		{async2_chrc_handle, 0x80, VALUE2_SIZE},
	};
	/* With an ATT MTU of 23 these leave room for the length of the last
	 * value but not for the value itself
	 */
	const struct mult_item tuple_items[] = {
		{async2_chrc_handle, 0x80, VALUE2_SIZE},
		{sync_chrc_handle, 0xf0, SYNC_VALUE_SIZE},
		{async_chrc_handle, 0x00, VALUE_SIZE},
	};

	set_stage(STAGE_MULT_SYNC);
	/* Let the server take all but one of its ATT buffers */
	k_sleep(K_MSEC(500));
	test_read_multiple(false, sync_items, ARRAY_SIZE(sync_items));
	test_read_multiple(true, sync_items, ARRAY_SIZE(sync_items));

	set_stage(STAGE_READ);
	test_read();

	set_stage(STAGE_BLOB);
	test_read_blob();

	set_stage(STAGE_TYPE);
	test_read_by_type();

	set_stage(STAGE_ERROR);
	test_error();

	set_stage(STAGE_WRITE);
	test_write();

	set_stage(STAGE_PREPARE);
	test_prepare_write();

	set_stage(STAGE_CMD);
	test_command();

	set_stage(STAGE_MULT);
	test_read_multiple(false, mult_items, ARRAY_SIZE(mult_items));

	set_stage(STAGE_MULT_VL);
	test_read_multiple(true, mult_items, ARRAY_SIZE(mult_items));

	set_stage(STAGE_MULT_ERROR);
	test_read_multiple_error(BT_ATT_OP_READ_MULT_REQ, mult_items, ARRAY_SIZE(mult_items),
				 TEST_ASYNC_ERR);

	set_stage(STAGE_MULT_PERM);
	test_read_multiple_error(BT_ATT_OP_READ_MULT_VL_REQ, perm_items, ARRAY_SIZE(perm_items),
				 BT_ATT_ERR_READ_NOT_PERMITTED);

	set_stage(STAGE_MULT_FULL);
	test_read_multiple(false, full_items, ARRAY_SIZE(full_items));
	test_read_multiple(true, full_items, ARRAY_SIZE(full_items));

	set_stage(STAGE_MULT_TUPLE);
	test_read_multiple(true, tuple_items, ARRAY_SIZE(tuple_items));

	set_stage(STAGE_MULT_INLINE_ERR);
	test_mult_inline_error();

	set_stage(STAGE_NOW_ERR);
	test_now_error();

	set_stage(STAGE_EXEC);
	test_execute(0);

	set_stage(STAGE_EXEC_ERR);
	test_execute(TEST_ASYNC_ERR);

	set_stage(STAGE_EXEC_EMPTY);
	test_execute_empty();

	if (use_eatt) {
		set_stage(STAGE_BEARERS);
		test_bearers();

		set_stage(STAGE_PREP_EXEC);
		test_prepare_execute();

		set_stage(STAGE_PIPELINE_EATT);
		test_pipeline_eatt();
	}

	set_stage(STAGE_PIPELINE);
	test_pipeline();

	TEST_PASS("GATT client passed");
}

static void test_main_eatt(void)
{
	test_main_common(true);
}

static void test_main_no_eatt(void)
{
	test_main_common(false);
}

static const struct bst_test_instance test_deferred_client[] = {
	{
		.test_id = "gatt_client_deferred_eatt",
		.test_main_f = test_main_eatt,
	},
	{
		.test_id = "gatt_client_deferred_no_eatt",
		.test_main_f = test_main_no_eatt,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_deferred_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_deferred_client);
}
