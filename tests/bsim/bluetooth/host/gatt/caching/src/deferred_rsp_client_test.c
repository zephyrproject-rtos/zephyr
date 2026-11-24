/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "deferred_rsp.h"
#include "testlib/conn.h"
#include "testlib/scan.h"
#include "testlib/security.h"

/* Only stops a test that hangs: a response takes a few connection events */
#define RSP_TIMEOUT K_SECONDS(10)
/* How often a wait on the peer polls the host state it is waiting for */
#define POLL_INTERVAL K_MSEC(10)

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_discover_complete);

static const struct bt_uuid *deferred_chrc_uuid = TEST_DEFERRED_CHRC_UUID;

static struct bt_conn *g_conn;
static uint16_t deferred_chrc_handle;
static uint16_t cp_chrc_handle;
static uint16_t deferred2_chrc_handle;
static uint16_t sync_chrc_handle;
static uint16_t fill_chrc_handle;
static uint16_t syncprep_chrc_handle;
static uint16_t deferred3_chrc_handle;

/* The bearer the requests are sent on, see test_main_common() */
static enum bt_att_chan_opt chan_opt = BT_ATT_CHAN_OPT_NONE;

static K_SEM_DEFINE(rsp_sem, 0, 1);
static int rsp_err;
static uint8_t rsp_data[BT_ATT_BUF_SIZE];
static uint16_t rsp_len;

static const uint8_t write_data[WRITE_SIZE] = WRITE_DATA;

/* The last Error Response on the unenhanced bearer. A GATT client is only
 * given its error code, so the PDU is looked at on its way in.
 */
static struct bt_att_error_rsp last_err;
static const struct bt_l2cap_chan_ops *att_ops;

static int watch_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	if ((buf->len >= (sizeof(struct bt_att_hdr) + sizeof(last_err))) &&
	    (buf->data[0] == BT_ATT_OP_ERROR_RSP)) {
		(void)memcpy(&last_err, &buf->data[sizeof(struct bt_att_hdr)], sizeof(last_err));
	}

	return att_ops->recv(chan, buf);
}

static void watch_err_rsp(void)
{
	static struct bt_l2cap_chan_ops watch_ops;
	struct bt_l2cap_chan *chan = bt_l2cap_le_lookup_rx_cid(g_conn, BT_L2CAP_CID_ATT);

	if (chan == NULL) {
		TEST_FAIL("No unenhanced ATT bearer");

		return;
	}

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

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	printk("Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	bt_conn_drop(&g_conn);

	UNSET_FLAG(flag_is_connected);
}

static void register_conn_callbacks(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.disconnected = disconnected,
	};
	int err;

	err = bt_conn_cb_register(&conn_callbacks);
	if (err != 0) {
		TEST_FAIL("Registering the connection callbacks failed (err %d)", err);
	}
}

static void connect_to_server(void)
{
	bt_addr_le_t peer;
	int err;

	err = bt_testlib_scan_find_name(&peer, CONFIG_BT_DEVICE_NAME);
	if (err != 0) {
		TEST_FAIL("Scan failed (err %d)", err);

		return;
	}

	err = bt_testlib_connect(&peer, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer (err %d)", err);

		return;
	}

	SET_FLAG(flag_is_connected);
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (deferred_chrc_handle == 0U || cp_chrc_handle == 0U ||
		    deferred2_chrc_handle == 0U || sync_chrc_handle == 0U ||
		    fill_chrc_handle == 0U || syncprep_chrc_handle == 0U ||
		    deferred3_chrc_handle == 0U) {
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

		if (bt_uuid_cmp(chrc->uuid, deferred_chrc_uuid) == 0) {
			deferred_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_CP_CHRC_UUID) == 0) {
			cp_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_DEFERRED2_CHRC_UUID) == 0) {
			deferred2_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_SYNC_CHRC_UUID) == 0) {
			sync_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_FILL_CHRC_UUID) == 0) {
			fill_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_SYNCPREP_CHRC_UUID) == 0) {
			syncprep_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_DEFERRED3_CHRC_UUID) == 0) {
			deferred3_chrc_handle = chrc->value_handle;
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
		(void)memcpy(rsp_data, pdu, rsp_len);
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
	(void)net_buf_add_mem(buf, data, len);

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

	if (k_sem_take(&rsp_sem, RSP_TIMEOUT) != 0) {
		return -ETIMEDOUT;
	}

	return rsp_err;
}

static void set_stage(enum test_stage stage)
{
	uint8_t pdu[sizeof(struct bt_att_write_req) + sizeof(uint8_t)];
	struct bt_att_write_req *req = (void *)pdu;
	int err;

	req->handle = sys_cpu_to_le16(cp_chrc_handle);
	req->value[0] = (uint8_t)stage;

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
	uint16_t expected = MIN(VALUE_SIZE, bt_gatt_get_mtu(g_conn) - sizeof(struct bt_att_hdr));
	struct bt_att_read_req req = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
	};
	int err;

	err = att_request(BT_ATT_OP_READ_REQ, &req, sizeof(req));
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
	struct bt_att_read_blob_req req = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
		.offset = sys_cpu_to_le16(BLOB_OFFSET),
	};
	int err;

	err = att_request(BT_ATT_OP_READ_BLOB_REQ, &req, sizeof(req));
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
	uint16_t max_len =
		MIN(bt_gatt_get_mtu(g_conn) - sizeof(struct bt_att_hdr) -
			    sizeof(struct bt_att_read_type_rsp) - sizeof(struct bt_att_data),
		    UINT8_MAX - sizeof(struct bt_att_data));
	uint16_t expected = MIN(VALUE_SIZE, max_len);
	uint8_t pdu[sizeof(struct bt_att_read_type_req) + BT_UUID_SIZE_128];
	struct bt_att_read_type_req *req = (void *)pdu;
	const struct bt_att_read_type_rsp *rsp = (const void *)rsp_data;
	const struct bt_att_data *item = (const void *)rsp->data;
	int err;

	req->start_handle = sys_cpu_to_le16(BT_ATT_FIRST_ATTRIBUTE_HANDLE);
	req->end_handle = sys_cpu_to_le16(BT_ATT_LAST_ATTRIBUTE_HANDLE);
	(void)memcpy(req->uuid, BT_UUID_128(deferred_chrc_uuid)->val, BT_UUID_SIZE_128);

	err = att_request(BT_ATT_OP_READ_TYPE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Read by type failed (err %d)", err);
	}

	/* Length of one Attribute Data, then the handle and the value */
	if (rsp_len != (sizeof(*rsp) + sizeof(*item) + expected)) {
		TEST_FAIL("Read by type returned %u bytes", rsp_len);

		return;
	}

	if (rsp->len != (sizeof(*item) + expected)) {
		TEST_FAIL("Read by type item length is %u", rsp->len);
	}

	if (sys_le16_to_cpu(item->handle) != deferred_chrc_handle) {
		TEST_FAIL("Read by type handle is 0x%04x", sys_le16_to_cpu(item->handle));
	}

	(void)memmove(rsp_data, item->value, expected);
	expect_value(0, expected);
}

static void test_error(void)
{
	struct bt_att_read_req req = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
	};
	int err;

	err = att_request(BT_ATT_OP_READ_REQ, &req, sizeof(req));
	if (err != TEST_DEFERRED_ERR) {
		TEST_FAIL("Read responded with %d", err);
	}
}

static void test_write(void)
{
	uint8_t pdu[sizeof(struct bt_att_write_req) + WRITE_SIZE];
	struct bt_att_write_req *req = (void *)pdu;
	int err;

	req->handle = sys_cpu_to_le16(deferred_chrc_handle);
	(void)memcpy(req->value, write_data, sizeof(write_data));

	err = att_request(BT_ATT_OP_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Write failed (err %d)", err);
	}
}

static void test_prepare_write(void)
{
	uint8_t pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *req = (void *)pdu;
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	req->handle = sys_cpu_to_le16(deferred_chrc_handle);
	req->offset = sys_cpu_to_le16(0U);
	(void)memcpy(req->value, write_data, sizeof(write_data));

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

/* A callback that fails without deferring responds with the attribute handle,
 * as a non-deferred one does
 */
static void test_now_error(void)
{
	enum bt_att_chan_opt opt = chan_opt;
	struct bt_att_read_blob_req req = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
		.offset = sys_cpu_to_le16(VALUE_SIZE + 1U),
	};
	int err;

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_READ_BLOB_REQ, &req, sizeof(req));
	chan_opt = opt;

	if (err != BT_ATT_ERR_INVALID_OFFSET) {
		TEST_FAIL("Read blob responded with %d", err);

		return;
	}

	expect_err_rsp(BT_ATT_OP_READ_BLOB_REQ, deferred_chrc_handle, BT_ATT_ERR_INVALID_OFFSET);
}

static void prepare(uint16_t handle)
{
	uint8_t pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *req = (void *)pdu;
	int err;

	req->handle = sys_cpu_to_le16(handle);
	req->offset = 0U;
	(void)memcpy(req->value, write_data, sizeof(write_data));

	err = att_request(BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Prepare write of 0x%04x failed (err %d)", handle, err);
	}
}

/* One value prepared for each of two deferred attributes, both written
 * by a single Execute Write that the server responds to with a deferred response
 */
static void test_execute(int expected_err)
{
	enum bt_att_chan_opt opt = chan_opt;
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	prepare(deferred_chrc_handle);
	prepare(deferred2_chrc_handle);

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	chan_opt = opt;

	if (err != expected_err) {
		TEST_FAIL("Execute write responded with %d, expected %d", err, expected_err);

		return;
	}

	if (expected_err != 0) {
		/* The handle of the value that failed, the first one */
		expect_err_rsp(BT_ATT_OP_EXEC_WRITE_REQ, deferred_chrc_handle, expected_err);
	}
}

/* After the failed Execute Write the queue is empty, so this writes nothing */
static void test_execute_empty(void)
{
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write of an empty queue responded with %d", err);
	}
}

/* A deferred and a non-deferred value in one Execute Write, which is then not
 * deferred. On error the Error Response names the deferred attribute.
 */
static void test_execute_mixed(int expected_err)
{
	enum bt_att_chan_opt opt = chan_opt;
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	prepare(deferred_chrc_handle);
	prepare(syncprep_chrc_handle);

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	chan_opt = opt;

	if (err != expected_err) {
		TEST_FAIL("Mixed execute write responded with %d, expected %d", err,
			  expected_err);

		return;
	}

	if (expected_err != 0) {
		expect_err_rsp(BT_ATT_OP_EXEC_WRITE_REQ, deferred_chrc_handle, expected_err);
	}
}

/* A deferred write rejected by the server, with a deferred error response or
 * by the method itself
 */
static void test_write_error(void)
{
	enum bt_att_chan_opt opt = chan_opt;
	uint8_t pdu[sizeof(struct bt_att_write_req) + WRITE_SIZE];
	struct bt_att_write_req *req = (void *)pdu;
	int err;

	req->handle = sys_cpu_to_le16(deferred_chrc_handle);
	(void)memcpy(req->value, write_data, sizeof(write_data));

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_WRITE_REQ, pdu, sizeof(pdu));
	chan_opt = opt;

	if (err != TEST_DEFERRED_ERR) {
		TEST_FAIL("Write responded with %d, expected %d", err, TEST_DEFERRED_ERR);

		return;
	}

	expect_err_rsp(BT_ATT_OP_WRITE_REQ, deferred_chrc_handle, TEST_DEFERRED_ERR);
}

/* A rejected Prepare Write is not queued, so the Execute Write after it has
 * nothing to write
 */
static void test_prepare_error(void)
{
	enum bt_att_chan_opt opt = chan_opt;
	uint8_t pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *req = (void *)pdu;
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	req->handle = sys_cpu_to_le16(deferred_chrc_handle);
	req->offset = sys_cpu_to_le16(0U);
	(void)memcpy(req->value, write_data, sizeof(write_data));

	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	err = att_request(BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
	chan_opt = opt;

	if (err != TEST_DEFERRED_ERR) {
		TEST_FAIL("Prepare write responded with %d, expected %d", err, TEST_DEFERRED_ERR);

		return;
	}

	expect_err_rsp(BT_ATT_OP_PREPARE_WRITE_REQ, deferred_chrc_handle, TEST_DEFERRED_ERR);

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write after the rejected prepare responded with %d", err);
	}
}

/* The host queues the prepared value of an attribute without
 * BT_GATT_PERM_PREPARE_WRITE itself, and the write method first sees it at
 * Execute time
 */
static void test_execute_noprep(void)
{
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	prepare(deferred3_chrc_handle);

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write responded with %d", err);
	}
}

static void test_command(void)
{
	uint8_t pdu[sizeof(struct bt_att_write_cmd) + WRITE_SIZE];
	struct bt_att_write_cmd *cmd = (void *)pdu;
	struct net_buf *buf;
	int err;

	cmd->handle = sys_cpu_to_le16(deferred_chrc_handle);
	(void)memcpy(cmd->value, write_data, sizeof(write_data));

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
			if (room < sizeof(struct bt_att_read_mult_vl_rsp)) {
				continue;
			}

			room -= sizeof(struct bt_att_read_mult_vl_rsp);
		}

		len = MIN(items[i].len, room);

		if (var_len) {
			struct bt_att_read_mult_vl_rsp *vl = (void *)&out[used];

			vl->len = sys_cpu_to_le16(len);
			used += sizeof(*vl);
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
	/* The Variable Length request has the same layout */
	struct bt_att_read_mult_req *req = (void *)pdu;
	uint16_t expected_len;
	uint8_t op;
	int err;

	for (size_t i = 0U; i < count; i++) {
		req->handles[i] = sys_cpu_to_le16(items[i].handle);
	}

	op = var_len ? BT_ATT_OP_READ_MULT_VL_REQ : BT_ATT_OP_READ_MULT_REQ;
	expected_len = mult_expected(expected, bt_gatt_get_mtu(g_conn) - sizeof(struct bt_att_hdr),
				     items, count, var_len);

	err = att_request(op, pdu, count * sizeof(req->handles[0]));
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
	struct bt_att_read_mult_req *req = (void *)pdu;
	int err;

	for (size_t i = 0U; i < count; i++) {
		req->handles[i] = sys_cpu_to_le16(items[i].handle);
	}

	err = att_request(op, pdu, count * sizeof(req->handles[0]));
	if (err != expected_err) {
		TEST_FAIL("Read multiple 0x%02x responded with %d, expected %d", op, err,
			  expected_err);
	}
}

/* An error returned from inside the callback must leave the bearer free for
 * the next request
 */
static void test_mult_inline_error(void)
{
	uint16_t expected = SYNC_VALUE_SIZE;
	const uint16_t handles[] = {
		sys_cpu_to_le16(deferred_chrc_handle),
		sys_cpu_to_le16(sync_chrc_handle),
	};
	struct bt_att_read_req read = {
		.handle = sys_cpu_to_le16(sync_chrc_handle),
	};
	int err;

	err = att_request(BT_ATT_OP_READ_MULT_REQ, handles, sizeof(handles));
	if (err != TEST_DEFERRED_ERR) {
		TEST_FAIL("Read multiple responded with %d, expected %d", err, TEST_DEFERRED_ERR);

		return;
	}

	err = att_request(BT_ATT_OP_READ_REQ, &read, sizeof(read));
	if (err != 0) {
		TEST_FAIL("Read after the error responded with %d", err);

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
		(void)memcpy(rsp->data, pdu, rsp->len);
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

	err = k_sem_init(&rsp->sem, 0, 1);
	if (err != 0) {
		return err;
	}

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
	(void)net_buf_add_mem(buf, data, len);

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
		TEST_FAIL("%s responded with %d", what, rsp->err);

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
	uint16_t uatt_len =
		MIN(VALUE_SIZE, bt_att_get_uatt_mtu(g_conn) - sizeof(struct bt_att_hdr));
	struct bt_att_read_req read = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
	};
	struct bt_att_read_blob_req blob = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
		.offset = sys_cpu_to_le16(BLOB_OFFSET),
	};
	int err;

	/* One request per bearer, both outstanding at once. The server
	 * responds to them in reverse order, and each response has to come
	 * back on the bearer its own request arrived on.
	 */
	err = bearer_request(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, BT_ATT_OP_READ_REQ,
			     &read, sizeof(read));
	if (err != 0) {
		TEST_FAIL("Unenhanced read failed (err %d)", err);

		return;
	}

	err = bearer_request(&eatt_rsp, BT_ATT_CHAN_OPT_ENHANCED_ONLY, BT_ATT_OP_READ_BLOB_REQ,
			     &blob, sizeof(blob));
	if (err != 0) {
		TEST_FAIL("Enhanced read blob failed (err %d)", err);

		return;
	}

	if (k_sem_take(&uatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The unenhanced bearer got no response");

		return;
	}

	if (k_sem_take(&eatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The enhanced bearer got no response");

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
	uint8_t prep_pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *prep = (void *)prep_pdu;
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	prep->handle = sys_cpu_to_le16(deferred_chrc_handle);
	prep->offset = 0U;
	(void)memcpy(prep->value, write_data, sizeof(write_data));

	err = bearer_request(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
			     BT_ATT_OP_PREPARE_WRITE_REQ, prep_pdu, sizeof(prep_pdu));
	if (err != 0) {
		TEST_FAIL("Unenhanced prepare write failed (err %d)", err);

		return;
	}

	/* The server holds that one long enough for this to arrive while it is
	 * still deferred
	 */
	bk_sync_wait();

	err = bearer_request(&eatt_rsp, BT_ATT_CHAN_OPT_ENHANCED_ONLY, BT_ATT_OP_EXEC_WRITE_REQ,
			     &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Enhanced execute write failed (err %d)", err);

		return;
	}

	if (k_sem_take(&eatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The execute write got no response");

		return;
	}

	if (eatt_rsp.err != 0) {
		TEST_FAIL("Execute write responded with %d", eatt_rsp.err);

		return;
	}

	if (k_sem_take(&uatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The prepare write got no response");

		return;
	}

	if ((uatt_rsp.err != 0) || (uatt_rsp.len != sizeof(prep_pdu)) ||
	    (memcmp(uatt_rsp.data, prep_pdu, sizeof(prep_pdu)) != 0)) {
		TEST_FAIL("Prepare write responded with %d and %u bytes", uatt_rsp.err,
			  uatt_rsp.len);

		return;
	}

	/* Accepted by now, so this one does reach the attribute */
	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write after the prepare failed (err %d)", err);
	}
}

/* Sends a Prepare Write without waiting for its response, so two of them, one
 * per bearer, can be deferred at the same time
 */
static int prepare_deferred(struct bearer_rsp *rsp, enum bt_att_chan_opt opt, uint16_t offset,
			    uint8_t first)
{
	uint8_t pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *req = (void *)pdu;

	req->handle = sys_cpu_to_le16(deferred_chrc_handle);
	req->offset = sys_cpu_to_le16(offset);
	for (uint16_t i = 0U; i < WRITE_SIZE; i++) {
		req->value[i] = (uint8_t)(first + i);
	}

	return bearer_request(rsp, opt, BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
}

static bool prepare_echo_ok(const struct bearer_rsp *rsp, uint16_t offset, uint8_t first,
			    const char *what)
{
	uint8_t expected[sizeof(struct bt_att_prepare_write_rsp) + WRITE_SIZE];
	struct bt_att_prepare_write_rsp *echo = (void *)expected;

	echo->handle = sys_cpu_to_le16(deferred_chrc_handle);
	echo->offset = sys_cpu_to_le16(offset);
	for (uint16_t i = 0U; i < WRITE_SIZE; i++) {
		echo->value[i] = (uint8_t)(first + i);
	}

	if ((rsp->err != 0) || (rsp->len != sizeof(expected)) ||
	    (memcmp(rsp->data, expected, sizeof(expected)) != 0)) {
		TEST_FAIL("%s responded with %d and %u bytes", what, rsp->err, rsp->len);

		return false;
	}

	return true;
}

/* Two Prepare Writes to the same handle at adjacent offsets, one per bearer,
 * which the server accepts in reverse arrival order before a single Execute
 * Write. The reassembled value must follow the order the client sent, so the
 * fragment at offset 0 comes first (Core v6.3 Vol 3 Part F 3.4.6.3).
 */
static void test_prepare_reorder(void)
{
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	err = prepare_deferred(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, 0, REORDER_FIRST_BYTE);
	if (err != 0) {
		TEST_FAIL("First prepare write failed (err %d)", err);

		return;
	}

	/* The first one is deferred before the second arrives, so the host
	 * numbers them in that order
	 */
	bk_sync_wait();

	err = prepare_deferred(&eatt_rsp, BT_ATT_CHAN_OPT_ENHANCED_ONLY, WRITE_SIZE,
			       REORDER_SECOND_BYTE);
	if (err != 0) {
		TEST_FAIL("Second prepare write failed (err %d)", err);

		return;
	}

	if (k_sem_take(&uatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The first prepare write got no response");

		return;
	}

	if (k_sem_take(&eatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The second prepare write got no response");

		return;
	}

	if (!prepare_echo_ok(&uatt_rsp, 0, REORDER_FIRST_BYTE, "First prepare write")) {
		return;
	}

	if (!prepare_echo_ok(&eatt_rsp, WRITE_SIZE, REORDER_SECOND_BYTE, "Second prepare write")) {
		return;
	}

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write of the reordered prepares failed (err %d)", err);
	}
}

/* A deferred prepare on one bearer, then a non-deferred one on the other while
 * the first still waits. The Execute Write writes them in arrival order.
 */
static void test_prepare_mixed(void)
{
	uint8_t flags = BT_ATT_FLAG_EXEC;
	int err;

	err = prepare_deferred(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, 0, REORDER_FIRST_BYTE);
	if (err != 0) {
		TEST_FAIL("Deferred prepare write failed (err %d)", err);

		return;
	}

	/* Deferred before the non-deferred one arrives */
	bk_sync_wait();
	prepare(syncprep_chrc_handle);

	if (k_sem_take(&uatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The deferred prepare write got no response");

		return;
	}

	if (!prepare_echo_ok(&uatt_rsp, 0, REORDER_FIRST_BYTE, "Deferred prepare write")) {
		return;
	}

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Execute write of the mixed prepares failed (err %d)", err);
	}
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
/* STAGE_PREP_POOL counts on the connection being allowed the whole pool. The
 * prepare-limit image, which does not run it, allows less.
 */
BUILD_ASSERT((CONFIG_BT_MAX_CONN == 1) && (CONFIG_BT_ATT_PREPARE_COUNT >= 2) &&
		     ((CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN == 0) ||
		      (CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN == TEST_PREP_LIMIT) ||
		      (CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN >= CONFIG_BT_ATT_PREPARE_COUNT)),
	     "STAGE_PREP_POOL needs the whole prepare pool for one connection");
#endif /* CONFIG_BT_ATT_PREPARE_COUNT > 0 */

/* While a deferred prepare waits, the non-deferred prepares still get every
 * buffer of the prepare pool. The deferred one is accepted once the queue is
 * full, so it is rejected as a non-deferred one would be.
 */
static void test_prepare_pool(void)
{
	uint8_t flags = BT_ATT_FLAG_CANCEL;
	int err;

	for (uint16_t i = 1U; i < CONFIG_BT_ATT_PREPARE_COUNT; i++) {
		prepare(syncprep_chrc_handle);
	}

	err = prepare_deferred(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, 0, REORDER_FIRST_BYTE);
	if (err != 0) {
		TEST_FAIL("Deferred prepare write failed (err %d)", err);

		return;
	}

	bk_sync_wait();
	prepare(syncprep_chrc_handle);

	if (k_sem_take(&uatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The deferred prepare write got no response");

		return;
	}

	if (uatt_rsp.err != BT_ATT_ERR_PREPARE_QUEUE_FULL) {
		TEST_FAIL("Deferred prepare into a full queue responded with %d", uatt_rsp.err);

		return;
	}

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
	if (err != 0) {
		TEST_FAIL("Cancelling the prepared writes failed (err %d)", err);
	}
}

/* Whether the enhanced bearer can send, that is has a credit from the peer */
static bool eatt_can_send(void)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&g_conn->channels, chan, node) {
		if ((BT_L2CAP_LE_CHAN(chan)->psm == BT_EATT_PSM) &&
		    atomic_test_bit(chan->status, BT_L2CAP_STATUS_OUT)) {
			return true;
		}
	}

	return false;
}

/* Sends two requests without waiting, so the second one arrives while the
 * first is still deferred
 */
static void send_pipelined(void)
{
	struct bt_att_read_req req = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
	};
	struct net_buf *buf;

	for (int i = 0; i < 2; i++) {
		/* A request sent raw bypasses the host's flow control of an
		 * enhanced bearer, so wait for the peer's credit first
		 */
		while ((chan_opt == BT_ATT_CHAN_OPT_ENHANCED_ONLY) && !eatt_can_send()) {
			k_sleep(POLL_INTERVAL);
		}

		buf = att_pdu(BT_ATT_OP_READ_REQ, &req, sizeof(req));
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
		k_sleep(POLL_INTERVAL);
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

/* Encrypts the link, on which the peer opens its enhanced bearer, and uses it */
static void open_eatt(void)
{
	int err;

	err = bt_testlib_secure(g_conn, BT_SECURITY_L2);
	if (err != 0) {
		TEST_FAIL("Failed to encrypt the link (err %d)", err);

		return;
	}

	/* The peer opens the bearer once the link is encrypted */
	while (bt_eatt_count(g_conn) < 1U) {
		k_sleep(POLL_INTERVAL);
	}

	chan_opt = BT_ATT_CHAN_OPT_ENHANCED_ONLY;
}

static void test_main_common(bool use_eatt)
{
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	register_conn_callbacks();

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	connect_to_server();

	if (use_eatt) {
		open_eatt();
	}

	gatt_discover();
	watch_err_rsp();

	/* No deferred attribute is involved, so the server needs no ATT
	 * buffer beyond the one of the response
	 */
	const struct mult_item sync_items[] = {
		{sync_chrc_handle, 0xf0, SYNC_VALUE_SIZE},
		{fill_chrc_handle, 0x40, FILL_VALUE_SIZE},
	};
	/* A non-deferred value next to two deferred ones, so that one
	 * response needs more than one deferred value
	 */
	const struct mult_item mult_items[] = {
		{deferred2_chrc_handle, 0x80, VALUE2_SIZE},
		{deferred_chrc_handle, 0x00, VALUE_SIZE},
		{sync_chrc_handle, 0xf0, SYNC_VALUE_SIZE},
	};
	/* The second handle is write only */
	const struct mult_item perm_items[] = {
		{deferred2_chrc_handle, 0x80, VALUE2_SIZE},
		{cp_chrc_handle, 0x00, 0},
	};
	/* The non-deferred value fills the response, so the deferred handle
	 * after it is the first one of the request and finds no room
	 */
	const struct mult_item full_items[] = {
		{fill_chrc_handle, 0x40, FILL_VALUE_SIZE},
		{deferred2_chrc_handle, 0x80, VALUE2_SIZE},
	};
	/* With an ATT MTU of 23 these leave room for the length of the last
	 * value but not for the value itself
	 */
	const struct mult_item tuple_items[] = {
		{deferred2_chrc_handle, 0x80, VALUE2_SIZE},
		{sync_chrc_handle, 0xf0, SYNC_VALUE_SIZE},
		{deferred_chrc_handle, 0x00, VALUE_SIZE},
	};

	set_stage(STAGE_MULT_SYNC);
	/* Let the server take all but one of its ATT buffers */
	bk_sync_wait();
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
				 TEST_DEFERRED_ERR);

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
	test_execute(TEST_DEFERRED_ERR);

	set_stage(STAGE_EXEC_EMPTY);
	test_execute_empty();

	set_stage(STAGE_EXEC_MIXED);
	test_execute_mixed(0);

	set_stage(STAGE_EXEC_MIXED_ERR);
	test_execute_mixed(TEST_DEFERRED_ERR);

	set_stage(STAGE_WRITE_ERR);
	test_write_error();

	set_stage(STAGE_WRITE_NOW_ERR);
	test_write_error();

	set_stage(STAGE_PREP_ERR);
	test_prepare_error();

	set_stage(STAGE_PREP_NOW_ERR);
	test_prepare_error();

	set_stage(STAGE_EXEC_NOPREP);
	test_execute_noprep();

	if (use_eatt) {
		set_stage(STAGE_BEARERS);
		test_bearers();

		set_stage(STAGE_PREP_EXEC);
		test_prepare_execute();

		set_stage(STAGE_PREP_REORDER);
		test_prepare_reorder();

		set_stage(STAGE_PREP_MIXED);
		test_prepare_mixed();

		set_stage(STAGE_PREP_POOL);
		test_prepare_pool();

		set_stage(STAGE_PIPELINE_EATT);
		test_pipeline_eatt();
	}

	set_stage(STAGE_PIPELINE);
	test_pipeline();

	TEST_PASS("GATT client passed");
}

/* With CONFIG_BT_ATT_PREPARE_COUNT 0 the Prepare Write handler is compiled
 * out. A Prepare Write arriving while a Read is deferred must still be rejected
 * as an unexpected second request on the bearer, so the bearer is disconnected
 * rather than a Request Not Supported from the no-handler path.
 */
static void test_prep0_pipeline(void)
{
	struct bt_att_read_req read = {
		.handle = sys_cpu_to_le16(deferred_chrc_handle),
	};
	uint8_t prep_pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *prep = (void *)prep_pdu;
	struct net_buf *buf;

	prep->handle = sys_cpu_to_le16(deferred_chrc_handle);
	prep->offset = 0U;
	(void)memcpy(prep->value, write_data, sizeof(write_data));

	/* The read stays deferred, which is what the Prepare Write runs into */
	buf = att_pdu(BT_ATT_OP_READ_REQ, &read, sizeof(read));
	if ((buf == NULL) || (bt_att_send(g_conn, buf) != 0)) {
		TEST_FAIL("Unable to send the read");

		return;
	}

	/* Left deferred before the Prepare Write arrives */
	bk_sync_wait();

	buf = att_pdu(BT_ATT_OP_PREPARE_WRITE_REQ, prep_pdu, sizeof(prep_pdu));
	if ((buf == NULL) || (bt_att_send(g_conn, buf) != 0)) {
		TEST_FAIL("Unable to send the prepare write");

		return;
	}

	/* The unenhanced bearer takes the connection with it */
	WAIT_FOR_FLAG_UNSET(flag_is_connected);

	if (last_err.request == BT_ATT_OP_PREPARE_WRITE_REQ) {
		TEST_FAIL("Prepare write got error 0x%02x instead of a disconnect",
			  last_err.error);
	}
}

static void test_main_prep0(void)
{
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	register_conn_callbacks();

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	connect_to_server();

	gatt_discover();
	watch_err_rsp();

	set_stage(STAGE_PREP0);
	test_prep0_pipeline();

	TEST_PASS("GATT client passed");
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
#if CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN == TEST_PREP_LIMIT
/* Below the pool, so that a Prepare Write past the limit is not refused for an
 * empty pool instead
 */
BUILD_ASSERT(CONFIG_BT_ATT_PREPARE_COUNT > TEST_PREP_LIMIT,
	     "The prepare-limit image needs a pool above the limit");

/* A Prepare Write past the per-connection limit gets Prepare Queue Full */
static void prepare_full(uint16_t handle)
{
	uint8_t pdu[sizeof(struct bt_att_prepare_write_req) + WRITE_SIZE];
	struct bt_att_prepare_write_req *req = (void *)pdu;
	int err;

	req->handle = sys_cpu_to_le16(handle);
	req->offset = 0U;
	(void)memcpy(req->value, write_data, sizeof(write_data));

	err = att_request(BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
	if (err != BT_ATT_ERR_PREPARE_QUEUE_FULL) {
		TEST_FAIL("Prepare write of 0x%04x past the limit responded with %d", handle, err);
	}
}

/* What a deferred Execute Write has left to write counts against the
 * per-connection prepare limit: while its first value is held, the other bearer
 * can only prepare up to the limit, with a non-deferred value or a deferred one
 */
static void test_prepare_limit(void)
{
	uint8_t exec = BT_ATT_FLAG_EXEC;
	uint8_t cancel = BT_ATT_FLAG_CANCEL;
	int err;

	/* Two deferred values make the limit */
	chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY;
	prepare(deferred_chrc_handle);
	prepare(deferred2_chrc_handle);

	err = bearer_request(&uatt_rsp, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, BT_ATT_OP_EXEC_WRITE_REQ,
			     &exec, sizeof(exec));
	if (err != 0) {
		TEST_FAIL("Unenhanced execute write failed (err %d)", err);

		return;
	}

	/* The server holds the first value, so the second is left to write */
	bk_sync_wait();

	chan_opt = BT_ATT_CHAN_OPT_ENHANCED_ONLY;
	prepare(syncprep_chrc_handle);
	prepare_full(syncprep_chrc_handle);
	prepare_full(deferred_chrc_handle);

	if (k_sem_take(&uatt_rsp.sem, RSP_TIMEOUT) != 0) {
		TEST_FAIL("The execute write got no response");

		return;
	}

	if (uatt_rsp.err != 0) {
		TEST_FAIL("Execute write responded with %d", uatt_rsp.err);

		return;
	}

	/* Written, so the value no longer counts */
	prepare(syncprep_chrc_handle);

	err = att_request(BT_ATT_OP_EXEC_WRITE_REQ, &cancel, sizeof(cancel));
	if (err != 0) {
		TEST_FAIL("Cancelling the prepared writes failed (err %d)", err);
	}
}

static void test_main_prep_limit(void)
{
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	register_conn_callbacks();

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	connect_to_server();
	open_eatt();
	gatt_discover();
	watch_err_rsp();

	set_stage(STAGE_PREP_LIMIT);
	test_prepare_limit();

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		TEST_FAIL("Disconnect failed (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG_UNSET(flag_is_connected);

	TEST_PASS("GATT client passed");
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN == TEST_PREP_LIMIT */
#endif /* CONFIG_BT_ATT_PREPARE_COUNT > 0 */

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
	{
		/* Prepare-count-0 image: a pipelined Prepare Write whose
		 * handler is compiled out drops only the enhanced bearer
		 */
		.test_id = "gatt_client_prep0",
		.test_main_f = test_main_prep0,
	},
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
#if CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN == TEST_PREP_LIMIT
	{
		/* Prepare-limit image: what a deferred Execute Write has left to
		 * write counts against the per-connection limit
		 */
		.test_id = "gatt_client_prep_limit",
		.test_main_f = test_main_prep_limit,
	},
#endif /* CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN == TEST_PREP_LIMIT */
#endif /* CONFIG_BT_ATT_PREPARE_COUNT > 0 */
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_deferred_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_deferred_client);
}
