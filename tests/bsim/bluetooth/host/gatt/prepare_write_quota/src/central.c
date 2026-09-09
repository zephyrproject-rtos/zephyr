/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

#include "common.h"

LOG_MODULE_REGISTER(central, LOG_LEVEL_INF);

DEFINE_FLAG_STATIC(flag_rsp);

static struct bt_conn *conn;
static int rsp_err;
static bool disconnect_expected;

static void disconnected(struct bt_conn *c, uint8_t reason)
{
	/* An unexpected disconnect would release the prepare writes queued on the server */
	if (!disconnect_expected) {
		TEST_FAIL("Disconnected from %s (reason 0x%02x)", bt_conn_dst_str(c), reason);
	}
}

static struct bt_conn_cb conn_cb = {
	.disconnected = disconnected,
};

static uint16_t connect_and_find_chrc(void)
{
	bt_addr_le_t server = {};
	uint16_t svc_handle, svc_end_handle, chrc_handle, chrc_end_handle;
	int err;

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Failed to register connection callbacks (err %d)", err);

	err = bt_testlib_scan_find_name(&server, SERVER_NAME);
	TEST_ASSERT(err == 0, "Server not found (err %d)", err);

	err = bt_testlib_connect(&server, &conn);
	TEST_ASSERT(err == 0, "Connection failed (err %d)", err);

	err = bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn,
					       TEST_SERVICE_UUID, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					       BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "Service discovery failed (err %d)", err);

	err = bt_testlib_gatt_discover_characteristic(&chrc_handle, &chrc_end_handle, NULL, conn,
						      TEST_CHRC_UUID, svc_handle + 1,
						      svc_end_handle);
	TEST_ASSERT(err == 0, "Characteristic discovery failed (err %d)", err);

	LOG_INF("Connected to %s, characteristic value handle 0x%04x", bt_conn_dst_str(conn),
		chrc_handle);

	return chrc_handle;
}

struct prep_write {
	uint16_t handle;
	uint16_t offset;
	const uint8_t *value;
	uint16_t len;
};

static void prep_write_rsp(struct bt_conn *c, int err, const void *pdu, uint16_t length,
			   void *user_data)
{
	const struct prep_write *pw = user_data;
	const struct bt_att_prepare_write_rsp *rsp = pdu;

	if (err == 0) {
		TEST_ASSERT(length == sizeof(*rsp) + pw->len, "Unexpected response length %u",
			    length);
		TEST_ASSERT(sys_le16_to_cpu(rsp->handle) == pw->handle,
			    "Response handle 0x%04x differs from the request",
			    sys_le16_to_cpu(rsp->handle));
		TEST_ASSERT(sys_le16_to_cpu(rsp->offset) == pw->offset,
			    "Response offset %u differs from the request",
			    sys_le16_to_cpu(rsp->offset));
		TEST_ASSERT(memcmp(rsp->value, pw->value, pw->len) == 0,
			    "Response value differs from the request");
	}

	rsp_err = err;
	SET_FLAG(flag_rsp);
}

/* Send one Prepare Write Request and return the ATT error code of the
 * response. Unlike bt_gatt_write(), this never follows up with an Execute
 * Write Request, so the server keeps what it accepted queued.
 */
static int prepare_write(struct prep_write *pw)
{
	struct bt_att_prepare_write_req *req_pdu;
	struct bt_att_req *req;
	int err;

	req = bt_att_req_alloc(BT_ATT_TIMEOUT);
	TEST_ASSERT(req != NULL, "Failed to allocate ATT request");

	req->func = prep_write_rsp;
	req->user_data = pw;

	req->buf = bt_att_create_pdu(conn, BT_ATT_OP_PREPARE_WRITE_REQ, sizeof(*req_pdu) + pw->len);
	TEST_ASSERT(req->buf != NULL, "Failed to allocate ATT PDU");
	bt_att_set_tx_meta_data(req->buf, NULL, NULL, BT_ATT_CHAN_OPT_UNENHANCED_ONLY);

	req_pdu = net_buf_add(req->buf, sizeof(*req_pdu));
	req_pdu->handle = sys_cpu_to_le16(pw->handle);
	req_pdu->offset = sys_cpu_to_le16(pw->offset);
	(void)net_buf_add_mem(req->buf, pw->value, pw->len);

	UNSET_FLAG(flag_rsp);

	err = bt_att_req_send(conn, req);
	TEST_ASSERT(err == 0, "Failed to send Prepare Write Request (err %d)", err);

	WAIT_FOR_FLAG(flag_rsp);

	return rsp_err;
}

void holder_procedure(void)
{
	static uint8_t chunk[PREP_CHUNK_SIZE];
	struct prep_write pw = { .value = chunk, .len = sizeof(chunk) };
	unsigned int accepted;
	int err;

	TEST_START("holder");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	pw.handle = connect_and_find_chrc();
	(void)memset(chunk, HOLDER_FILL, sizeof(chunk));

	/* Queue prepare writes until the server rejects one, then leave them
	 * queued: nothing below executes or cancels them.
	 */
	for (accepted = 0; accepted <= CONFIG_BT_ATT_PREPARE_COUNT; accepted++) {
		pw.offset = accepted * PREP_CHUNK_SIZE;

		err = prepare_write(&pw);
		if (err != 0) {
			break;
		}
	}

	TEST_ASSERT(err == BT_ATT_ERR_PREPARE_QUEUE_FULL,
		    "Expected Prepare Queue Full after %u accepted prepare writes, got 0x%02x",
		    accepted, err);
	TEST_ASSERT(accepted == PREP_QUOTA,
		    "Server queued %u prepare writes for one connection, expected %d", accepted,
		    PREP_QUOTA);

	LOG_INF("Holding %u queued prepare writes", accepted);

	/* Let the writer go, and stay connected until it is done */
	bk_sync_send();
	bk_sync_wait();

	TEST_PASS("holder");
}

void writer_procedure(void)
{
	static uint8_t value[CHRC_SIZE];
	uint16_t handle;
	int err;

	TEST_START("writer");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	/* Wait for the holder to fill its share of the server's prepare write pool */
	bk_sync_wait();

	handle = connect_and_find_chrc();
	writer_value_fill(value, sizeof(value));

	/* CHRC_SIZE octets do not fit in one Write Request at the default ATT
	 * MTU, so this becomes WRITER_PREPARES Prepare Write Requests followed
	 * by an Execute Write Request.
	 */
	err = bt_testlib_att_write(conn, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, handle, value,
				   sizeof(value));
	if (WRITER_EXPECT_SUCCESS) {
		TEST_ASSERT(err == BT_ATT_ERR_SUCCESS, "Long write failed with ATT error 0x%02x",
			    err);
		LOG_INF("Long write of %zu octets succeeded", sizeof(value));
	} else {
		TEST_ASSERT(err == BT_ATT_ERR_PREPARE_QUEUE_FULL,
			    "Expected Prepare Queue Full, got ATT error 0x%02x", err);
		LOG_INF("Long write rejected with Prepare Queue Full, as expected");
	}

	/* Disconnecting tells the server that the write attempt is over */
	disconnect_expected = true;
	err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnect failed (err %d)", err);

	/* Release the holder */
	bk_sync_send();

	TEST_PASS("writer");
}
