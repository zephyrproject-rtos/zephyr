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
#define SOME_UUID_VAL               0xfff2

/* The end of the range the client discovers */
#define RANGE_END 0x0020U

/* The only valid records the server has */
#define VALID_CHRC_HANDLE  0x0016U
#define VALID_VALUE_HANDLE 0x0017U
#define VALID_INCL_START   0x0030U
#define VALID_INCL_END     0x0031U

DEFINE_FLAG_STATIC(flag_discovered);

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
	case BT_UUID_IAS_VAL:
		raw_att_send(chan, empty_service, sizeof(empty_service));
		break;
	default:
		raw_att_error(chan, BT_ATT_OP_FIND_TYPE_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		break;
	}
}

static void raw_att_read_chrc(struct bt_l2cap_chan *chan, uint16_t start)
{
	const uint8_t invalid_value_handles[] = {
		BT_ATT_OP_READ_TYPE_RSP, 7U,
		BT_BYTES_LIST_LE16(0x0010), BT_GATT_CHRC_READ, BT_BYTES_LIST_LE16(0x0000),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
		BT_BYTES_LIST_LE16(0x0012), BT_GATT_CHRC_READ, BT_BYTES_LIST_LE16(0x0012),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
		BT_BYTES_LIST_LE16(0x0014), BT_GATT_CHRC_READ, BT_BYTES_LIST_LE16(RANGE_END + 1U),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t valid_value_handle[] = {
		BT_ATT_OP_READ_TYPE_RSP, 7U,
		BT_BYTES_LIST_LE16(VALID_CHRC_HANDLE), BT_GATT_CHRC_READ,
		BT_BYTES_LIST_LE16(VALID_VALUE_HANDLE), BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};

	if (start < VALID_CHRC_HANDLE - 1U) {
		raw_att_send(chan, invalid_value_handles, sizeof(invalid_value_handles));
	} else if (start < VALID_CHRC_HANDLE + 1U) {
		raw_att_send(chan, valid_value_handle, sizeof(valid_value_handle));
	} else {
		raw_att_error(chan, BT_ATT_OP_READ_TYPE_REQ, start, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	}
}

static void raw_att_read_include(struct bt_l2cap_chan *chan, uint16_t start)
{
	const uint8_t invalid_ranges[] = {
		BT_ATT_OP_READ_TYPE_RSP, 8U,
		BT_BYTES_LIST_LE16(0x0011), BT_BYTES_LIST_LE16(0x0000), BT_BYTES_LIST_LE16(0x0030),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
		BT_BYTES_LIST_LE16(0x0012), BT_BYTES_LIST_LE16(0x0031), BT_BYTES_LIST_LE16(0x0030),
		BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t valid_range[] = {
		BT_ATT_OP_READ_TYPE_RSP, 8U,
		BT_BYTES_LIST_LE16(0x0013), BT_BYTES_LIST_LE16(VALID_INCL_START),
		BT_BYTES_LIST_LE16(VALID_INCL_END), BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};
	const uint8_t handle_past_the_range[] = {
		BT_ATT_OP_READ_TYPE_RSP, 8U,
		BT_BYTES_LIST_LE16(RANGE_END + 1U), BT_BYTES_LIST_LE16(VALID_INCL_START),
		BT_BYTES_LIST_LE16(VALID_INCL_END), BT_BYTES_LIST_LE16(SOME_UUID_VAL),
	};

	if (start < 0x0013U) {
		raw_att_send(chan, invalid_ranges, sizeof(invalid_ranges));
	} else if (start == 0x0013U) {
		raw_att_send(chan, valid_range, sizeof(valid_range));
	} else {
		raw_att_send(chan, handle_past_the_range, sizeof(handle_past_the_range));
	}
}

static int raw_att_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	const uint8_t zero_handle[] = {
		BT_ATT_OP_FIND_INFO_RSP, BT_ATT_INFO_16,
		BT_BYTES_LIST_LE16(0x0000), BT_BYTES_LIST_LE16(BT_UUID_GATT_CCC_VAL),
	};
	const uint8_t op = net_buf_pull_u8(buf);

	switch (op) {
	case BT_ATT_OP_FIND_TYPE_REQ:
		raw_att_find_type(chan, sys_get_le16(&buf->data[0]), sys_get_le16(&buf->data[6]));
		break;
	case BT_ATT_OP_READ_TYPE_REQ:
		if (sys_get_le16(&buf->data[4]) == BT_UUID_GATT_CHRC_VAL) {
			raw_att_read_chrc(chan, sys_get_le16(&buf->data[0]));
		} else {
			raw_att_read_include(chan, sys_get_le16(&buf->data[0]));
		}
		break;
	case BT_ATT_OP_FIND_INFO_REQ:
		raw_att_send(chan, zero_handle, sizeof(zero_handle));
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

static uint8_t no_record_expected_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	TEST_ASSERT(attr == NULL, "Received a record at handle 0x%04x", attr->handle);

	SET_FLAG(flag_discovered);

	return BT_GATT_ITER_STOP;
}

static uint8_t valid_chrc_expected_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	if (attr == NULL) {
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	TEST_ASSERT(chrc->value_handle == VALID_VALUE_HANDLE, "Received value handle 0x%04x",
		    chrc->value_handle);
	records_found++;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t valid_incl_expected_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_include *incl;

	if (attr == NULL) {
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	incl = attr->user_data;
	TEST_ASSERT(incl->start_handle == VALID_INCL_START && incl->end_handle == VALID_INCL_END,
		    "Received include range 0x%04x-0x%04x", incl->start_handle, incl->end_handle);
	records_found++;

	return BT_GATT_ITER_CONTINUE;
}

static void discover(struct bt_conn *conn, const struct bt_uuid *uuid, uint8_t type,
		     bt_gatt_discover_func_t func)
{
	static struct bt_gatt_discover_params params;
	int err;

	params.uuid = uuid;
	params.func = func;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = RANGE_END;
	params.type = type;

	records_found = 0U;
	UNSET_FLAG(flag_discovered);

	err = bt_gatt_discover(conn, &params);
	TEST_ASSERT(err == 0, "Failed to start a discovery (err %d)", err);

	WAIT_FOR_FLAG(flag_discovered);
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

	discover(conn, BT_UUID_DECLARE_16(REVERSED_RANGE_SVC_UUID_VAL), BT_GATT_DISCOVER_PRIMARY,
		 no_record_expected_cb);
	discover(conn, BT_UUID_DECLARE_16(ZERO_START_SVC_UUID_VAL), BT_GATT_DISCOVER_PRIMARY,
		 no_record_expected_cb);

	discover(conn, NULL, BT_GATT_DISCOVER_CHARACTERISTIC, valid_chrc_expected_cb);
	TEST_ASSERT(records_found == 1U, "Found %u characteristics", records_found);

	discover(conn, NULL, BT_GATT_DISCOVER_INCLUDE, valid_incl_expected_cb);
	TEST_ASSERT(records_found == 1U, "Found %u includes", records_found);

	discover(conn, NULL, BT_GATT_DISCOVER_DESCRIPTOR, no_record_expected_cb);

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
