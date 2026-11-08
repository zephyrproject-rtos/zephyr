/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "bstests.h"
#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "testlib/adv.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

#define SERVER_NAME "invalid handles"

#define REVERSED_RANGE_SVC_UUID_VAL 0xfff0
#define ZERO_START_SVC_UUID_VAL     0xfff1
#define MIXED_SVC_UUID_VAL          0xfff4
#define SOME_UUID_VAL               0xfff2

/* Each check is a discovery that starts at its own handle, and the server answers by that */
#define VALUE_HANDLE_0_START       0x0001U
#define VALUE_BEFORE_DECL_START    0x0002U
#define DECL_PAST_RANGE_START      0x0003U
#define RANGE_END                  0x0020U

/* A record that is valid on its own; it shares a response with an invalid one */
#define VALID_DECL_HANDLE  0x0010U
#define VALID_VALUE_HANDLE 0x0011U
#define VALID_INCL_START   0x0030U
#define VALID_INCL_END     0x0031U

/* A value after the end of the service it is subscribed in */
#define VALUE_PAST_SERVICE_HANDLE 0x0030U

DEFINE_FLAG_STATIC(flag_discovered);
DEFINE_FLAG_STATIC(flag_unsubscribed);

NET_BUF_POOL_FIXED_DEFINE(raw_att_pool, 2, BT_L2CAP_BUF_SIZE(BT_ATT_DEFAULT_LE_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t records_found;
static int ias_discover_err;

static void raw_att_send(struct bt_l2cap_chan *chan, const uint8_t *pdu, size_t len)
{
	struct net_buf *buf = net_buf_alloc(&raw_att_pool, K_NO_WAIT);
	int err;

	TEST_ASSERT(buf != NULL, "No buffer for a response");

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, pdu, len);

	err = bt_l2cap_chan_send(chan, buf);
	TEST_ASSERT(err == 0, "Failed to send a response (err %d)", err);
}

static void raw_att_error(struct bt_l2cap_chan *chan, uint8_t op, uint16_t handle, uint8_t att_err)
{
	const uint8_t rsp[] = { BT_ATT_OP_ERROR_RSP, op, BT_BYTES_LIST_LE16(handle), att_err };

	raw_att_send(chan, rsp, sizeof(rsp));
}

static void raw_att_find_type(struct bt_l2cap_chan *chan, uint16_t start, uint16_t uuid)
{
	const uint8_t reversed_range[] = {
		BT_ATT_OP_FIND_TYPE_RSP,
		BT_BYTES_LIST_LE16(0x0011), BT_BYTES_LIST_LE16(0x0010),
	};
	const uint8_t zero_start[] = {
		BT_ATT_OP_FIND_TYPE_RSP,
		BT_BYTES_LIST_LE16(0x0000), BT_BYTES_LIST_LE16(0x0010),
	};
	const uint8_t valid_then_reversed[] = {
		BT_ATT_OP_FIND_TYPE_RSP,
		BT_BYTES_LIST_LE16(0x0010), BT_BYTES_LIST_LE16(0x0011),
		BT_BYTES_LIST_LE16(0x0013), BT_BYTES_LIST_LE16(0x0012),
	};
	const uint8_t empty_service[] = {
		BT_ATT_OP_FIND_TYPE_RSP,
		BT_BYTES_LIST_LE16(0x0010), BT_BYTES_LIST_LE16(0x0010),
	};

	switch (uuid) {
	case REVERSED_RANGE_SVC_UUID_VAL:
		raw_att_send(chan, reversed_range, sizeof(reversed_range));
		break;
	case ZERO_START_SVC_UUID_VAL:
		raw_att_send(chan, zero_start, sizeof(zero_start));
		break;
	case MIXED_SVC_UUID_VAL:
		raw_att_send(chan, valid_then_reversed, sizeof(valid_then_reversed));
		break;
	case BT_UUID_IAS_VAL:
		raw_att_send(chan, empty_service, sizeof(empty_service));
		break;
	default:
		raw_att_error(chan, BT_ATT_OP_FIND_TYPE_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		break;
	}
}

static void raw_att_read_group(struct bt_l2cap_chan *chan, uint16_t start)
{
	const uint8_t valid_then_past_range[] = {
		BT_ATT_OP_READ_GROUP_RSP, 6U,
		BT_BYTES_LIST_LE16(0x0010), BT_BYTES_LIST_LE16(0x0011),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
		BT_BYTES_LIST_LE16(RANGE_END + 1U), BT_BYTES_LIST_LE16(RANGE_END + 2U),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};

	if (start == BT_ATT_FIRST_ATTRIBUTE_HANDLE) {
		raw_att_send(chan, valid_then_past_range, sizeof(valid_then_past_range));
	} else {
		raw_att_error(chan, BT_ATT_OP_READ_GROUP_REQ, start,
			      BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	}
}

#define VALID_CHRC                                                                                 \
	BT_BYTES_LIST_LE16(VALID_DECL_HANDLE), BT_GATT_CHRC_READ,                                  \
		BT_BYTES_LIST_LE16(VALID_VALUE_HANDLE), BT_BYTES_LIST_LE16(SOME_UUID_VAL)

static void raw_att_read_chrc(struct bt_l2cap_chan *chan, uint16_t start)
{
	const uint8_t value_handle_0[] = {
		BT_ATT_OP_READ_TYPE_RSP, 7U, VALID_CHRC,
		BT_BYTES_LIST_LE16(0x0012), BT_GATT_CHRC_READ, BT_BYTES_LIST_LE16(0x0000),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t value_before_decl[] = {
		BT_ATT_OP_READ_TYPE_RSP, 7U, VALID_CHRC,
		BT_BYTES_LIST_LE16(0x0012), BT_GATT_CHRC_READ, BT_BYTES_LIST_LE16(0x0012),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t decl_past_range[] = {
		BT_ATT_OP_READ_TYPE_RSP, 7U, VALID_CHRC,
		BT_BYTES_LIST_LE16(RANGE_END + 1U), BT_GATT_CHRC_READ,
		BT_BYTES_LIST_LE16(RANGE_END + 2U), BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	/* The range ends at the declaration, the value is right after it */
	const uint8_t value_past_range[] = {
		BT_ATT_OP_READ_TYPE_RSP, 7U, VALID_CHRC,
	};

	switch (start) {
	case VALUE_HANDLE_0_START:
		raw_att_send(chan, value_handle_0, sizeof(value_handle_0));
		break;
	case VALUE_BEFORE_DECL_START:
		raw_att_send(chan, value_before_decl, sizeof(value_before_decl));
		break;
	case DECL_PAST_RANGE_START:
		raw_att_send(chan, decl_past_range, sizeof(decl_past_range));
		break;
	case VALID_DECL_HANDLE:
		raw_att_send(chan, value_past_range, sizeof(value_past_range));
		break;
	default:
		raw_att_error(chan, BT_ATT_OP_READ_TYPE_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		break;
	}
}

#define VALID_INCL                                                                                 \
	BT_BYTES_LIST_LE16(VALID_DECL_HANDLE), BT_BYTES_LIST_LE16(VALID_INCL_START),               \
		BT_BYTES_LIST_LE16(VALID_INCL_END), BT_BYTES_LIST_LE16(SOME_UUID_VAL)

static void raw_att_read_include(struct bt_l2cap_chan *chan, uint16_t start)
{
	const uint8_t start_0[] = {
		BT_ATT_OP_READ_TYPE_RSP, 8U, VALID_INCL,
		BT_BYTES_LIST_LE16(0x0011), BT_BYTES_LIST_LE16(0x0000), BT_BYTES_LIST_LE16(0x0030),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t reversed[] = {
		BT_ATT_OP_READ_TYPE_RSP, 8U, VALID_INCL,
		BT_BYTES_LIST_LE16(0x0011), BT_BYTES_LIST_LE16(0x0031), BT_BYTES_LIST_LE16(0x0030),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t decl_past_range[] = {
		BT_ATT_OP_READ_TYPE_RSP, 8U, VALID_INCL,
		BT_BYTES_LIST_LE16(RANGE_END + 1U), BT_BYTES_LIST_LE16(VALID_INCL_START),
		BT_BYTES_LIST_LE16(VALID_INCL_END), BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};

	switch (start) {
	case VALUE_HANDLE_0_START:
		raw_att_send(chan, start_0, sizeof(start_0));
		break;
	case VALUE_BEFORE_DECL_START:
		raw_att_send(chan, reversed, sizeof(reversed));
		break;
	case DECL_PAST_RANGE_START:
		raw_att_send(chan, decl_past_range, sizeof(decl_past_range));
		break;
	default:
		raw_att_error(chan, BT_ATT_OP_READ_TYPE_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		break;
	}
}

static void raw_att_read_ccc(struct bt_l2cap_chan *chan, uint16_t start)
{
	const uint8_t valid_then_handle_0[] = {
		BT_ATT_OP_READ_TYPE_RSP, 4U,
		BT_BYTES_LIST_LE16(VALID_DECL_HANDLE), BT_BYTES_LIST_LE16(0x0000),
		BT_BYTES_LIST_LE16(0x0000), BT_BYTES_LIST_LE16(0x0000),
	};

	if (start == BT_ATT_FIRST_ATTRIBUTE_HANDLE) {
		raw_att_send(chan, valid_then_handle_0, sizeof(valid_then_handle_0));
	} else {
		raw_att_error(chan, BT_ATT_OP_READ_TYPE_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	}
}

static void raw_att_find_info(struct bt_l2cap_chan *chan, uint16_t start, uint16_t end)
{
	const uint8_t valid_then_handle_0[] = {
		BT_ATT_OP_FIND_INFO_RSP, BT_ATT_INFO_16,
		BT_BYTES_LIST_LE16(VALID_DECL_HANDLE), BT_BYTES_LIST_LE16(BT_UUID_GATT_CCC_VAL),
		BT_BYTES_LIST_LE16(0x0000), BT_BYTES_LIST_LE16(BT_UUID_GATT_CCC_VAL),
	};

	/* A request for an invalid range would mean the client sent one */
	TEST_ASSERT(start != 0U && start <= end, "Find Information for 0x%04x-0x%04x", start, end);

	if (start == BT_ATT_FIRST_ATTRIBUTE_HANDLE) {
		raw_att_send(chan, valid_then_handle_0, sizeof(valid_then_handle_0));
	} else {
		raw_att_error(chan, BT_ATT_OP_FIND_INFO_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	}
}

static int raw_att_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	const uint8_t op = net_buf_pull_u8(buf);
	uint16_t start = 0x0000U;
	uint16_t type;

	if (buf->len >= sizeof(uint16_t)) {
		start = sys_get_le16(&buf->data[0]);
	}

	switch (op) {
	case BT_ATT_OP_FIND_TYPE_REQ:
		raw_att_find_type(chan, start, sys_get_le16(&buf->data[6]));
		break;
	case BT_ATT_OP_READ_GROUP_REQ:
		raw_att_read_group(chan, start);
		break;
	case BT_ATT_OP_READ_TYPE_REQ:
		type = sys_get_le16(&buf->data[4]);
		if (type == BT_UUID_GATT_CHRC_VAL) {
			raw_att_read_chrc(chan, start);
		} else if (type == BT_UUID_GATT_INCLUDE_VAL) {
			raw_att_read_include(chan, start);
		} else {
			raw_att_read_ccc(chan, start);
		}
		break;
	case BT_ATT_OP_FIND_INFO_REQ:
		raw_att_find_info(chan, start, sys_get_le16(&buf->data[2]));
		break;
	default:
		raw_att_error(chan, op, 0x0000, BT_ATT_ERR_NOT_SUPPORTED);
		break;
	}

	return 0;
}

/* A stock server does not send these records, so answer what its ATT channel receives here */
static void raw_att_take_over(struct bt_conn *conn)
{
	static struct bt_l2cap_chan_ops ops;
	struct bt_l2cap_chan *chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_ATT);

	TEST_ASSERT(chan != NULL, "No ATT channel");

	ops = *chan->ops;
	ops.recv = raw_att_recv;
	ops.sent = NULL;
	chan->ops = &ops;
}

static void server_main(void)
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("invalid handles server");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, SERVER_NAME);
	TEST_ASSERT(err == 0, "Failed to advertise (err %d)", err);

	raw_att_take_over(conn);

	err = bt_testlib_wait_disconnected(conn);
	TEST_ASSERT(err == 0, "Failed to wait for the disconnection (err %d)", err);

	bt_conn_unref(conn);

	TEST_PASS("invalid handles server");
}

static uint8_t count_records_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	if (attr == NULL) {
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	records_found++;

	return BT_GATT_ITER_CONTINUE;
}

/* A response with an invalid record delivers none of its records */
static void check_no_record(struct bt_conn *conn, const struct bt_uuid *uuid, uint8_t type,
			    uint16_t start_handle, uint16_t end_handle, uint8_t expected)
{
	static struct bt_gatt_discover_params params;
	int err;

	params.uuid = uuid;
	params.func = count_records_cb;
	params.start_handle = start_handle;
	params.end_handle = end_handle;
	params.type = type;

	records_found = 0U;
	UNSET_FLAG(flag_discovered);

	err = bt_gatt_discover(conn, &params);
	TEST_ASSERT(err == 0, "Failed to start a discovery (err %d)", err);

	WAIT_FOR_FLAG(flag_discovered);

	TEST_ASSERT(records_found == expected, "Type %u from 0x%04x: %u records, expected %u", type,
		    start_handle, records_found, expected);
}

static uint8_t unsubscribed_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			       const void *data, uint16_t length)
{
	TEST_ASSERT(data == NULL, "Notification on a subscription that cannot exist");

	SET_FLAG(flag_unsubscribed);

	return BT_GATT_ITER_STOP;
}

/* The raw server fails the test if the CCC discovery asks for an invalid range */
static void check_value_past_service(struct bt_conn *conn)
{
	static struct bt_gatt_discover_params disc_params;
	static struct bt_gatt_subscribe_params params;
	int err;

	params.notify = unsubscribed_cb;
	params.value = BT_GATT_CCC_NOTIFY;
	params.value_handle = VALUE_PAST_SERVICE_HANDLE;
	params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	params.end_handle = RANGE_END;
	params.disc_params = &disc_params;

	UNSET_FLAG(flag_unsubscribed);

	err = bt_gatt_subscribe(conn, &params);
	TEST_ASSERT(err == 0, "Failed to subscribe (err %d)", err);

	WAIT_FOR_FLAG(flag_unsubscribed);
}

static void ias_discover_cb(struct bt_conn *conn, int err)
{
	ias_discover_err = err;
	SET_FLAG(flag_discovered);
}

static void check_ias_client_empty_service(struct bt_conn *conn)
{
	static const struct bt_ias_client_cb ias_cb = {
		.discover = ias_discover_cb,
	};
	int err;

	err = bt_ias_client_cb_register(&ias_cb);
	TEST_ASSERT(err == 0, "Failed to register the IAS callbacks (err %d)", err);

	UNSET_FLAG(flag_discovered);

	err = bt_ias_discover(conn);
	TEST_ASSERT(err == 0, "Failed to start the IAS discovery (err %d)", err);

	WAIT_FOR_FLAG(flag_discovered);

	TEST_ASSERT(ias_discover_err == -ENOENT, "The IAS discovery ended with %d",
		    ias_discover_err);
}

static void client_main(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t server;
	int err;

	TEST_START("invalid handles client");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	err = bt_testlib_scan_find_name(&server, SERVER_NAME);
	TEST_ASSERT(err == 0, "Failed to find the server (err %d)", err);

	err = bt_testlib_connect(&server, &conn);
	TEST_ASSERT(err == 0, "Failed to connect (err %d)", err);

	check_no_record(conn, BT_UUID_DECLARE_16(REVERSED_RANGE_SVC_UUID_VAL),
			BT_GATT_DISCOVER_PRIMARY, BT_ATT_FIRST_ATTRIBUTE_HANDLE, RANGE_END, 0U);
	check_no_record(conn, BT_UUID_DECLARE_16(ZERO_START_SVC_UUID_VAL), BT_GATT_DISCOVER_PRIMARY,
			BT_ATT_FIRST_ATTRIBUTE_HANDLE, RANGE_END, 0U);
	check_no_record(conn, BT_UUID_DECLARE_16(MIXED_SVC_UUID_VAL), BT_GATT_DISCOVER_PRIMARY,
			BT_ATT_FIRST_ATTRIBUTE_HANDLE, RANGE_END, 0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_PRIMARY, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
			RANGE_END, 0U);

	check_no_record(conn, NULL, BT_GATT_DISCOVER_CHARACTERISTIC, VALUE_HANDLE_0_START,
			RANGE_END, 0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_CHARACTERISTIC, VALUE_BEFORE_DECL_START,
			RANGE_END, 0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_CHARACTERISTIC, DECL_PAST_RANGE_START,
			RANGE_END, 0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_CHARACTERISTIC, VALID_DECL_HANDLE,
			VALID_DECL_HANDLE, 1U);

	check_no_record(conn, NULL, BT_GATT_DISCOVER_INCLUDE, VALUE_HANDLE_0_START, RANGE_END, 0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_INCLUDE, VALUE_BEFORE_DECL_START, RANGE_END,
			0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_INCLUDE, DECL_PAST_RANGE_START, RANGE_END,
			0U);

	check_no_record(conn, BT_UUID_GATT_CCC, BT_GATT_DISCOVER_STD_CHAR_DESC,
			BT_ATT_FIRST_ATTRIBUTE_HANDLE, RANGE_END, 0U);
	check_no_record(conn, NULL, BT_GATT_DISCOVER_DESCRIPTOR, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
			RANGE_END, 0U);

	check_value_past_service(conn);
	check_ias_client_empty_service(conn);

	err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect (err %d)", err);

	TEST_PASS("invalid handles client");
}

static const struct bst_test_instance entrypoints[] = {
	{
		.test_id = "invalid_handles_server",
		.test_main_f = server_main,
	},
	{
		.test_id = "invalid_handles_client",
		.test_main_f = client_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *invalid_handles_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, entrypoints);
}
