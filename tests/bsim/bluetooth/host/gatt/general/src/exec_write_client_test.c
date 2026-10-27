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
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "host/att_internal.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "common.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

DEFINE_FLAG_STATIC(flag_discover_complete);

static struct bt_conn *g_conn;
static uint16_t first_chrc_handle;
static uint16_t second_chrc_handle;
static uint16_t cp_chrc_handle;

static K_SEM_DEFINE(rsp_sem, 0, 1);
static int rsp_err;

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (first_chrc_handle == 0U || second_chrc_handle == 0U || cp_chrc_handle == 0U) {
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

		if (bt_uuid_cmp(chrc->uuid, EXEC_WRITE_FIRST_CHRC_UUID) == 0) {
			first_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, EXEC_WRITE_SECOND_CHRC_UUID) == 0) {
			second_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, EXEC_WRITE_CP_CHRC_UUID) == 0) {
			cp_chrc_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(void)
{
	static const struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(EXEC_WRITE_SVC_UUID_VAL);
	static struct bt_gatt_discover_params discover_params;
	int err;

	discover_params.uuid = &svc_uuid.uuid;
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
	k_sem_give(&rsp_sem);
}

static int att_request(uint8_t op, const void *data, uint16_t len)
{
	struct bt_att_req *req;
	struct net_buf *buf;
	int err;

	req = bt_att_req_alloc(K_FOREVER);
	if (req == NULL) {
		return -ENOMEM;
	}

	buf = bt_att_create_pdu(g_conn, op, len);
	if (buf == NULL) {
		bt_att_req_free(req);

		return -ENOMEM;
	}

	bt_att_set_tx_meta_data(buf, NULL, NULL, BT_ATT_CHAN_OPT_NONE);
	(void)net_buf_add_mem(buf, data, len);

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

static int prepare_write(uint16_t handle, uint16_t offset)
{
	static const uint8_t chrc_data[EXEC_WRITE_CHRC_SIZE] = {0x01, 0x02, 0x03, 0x04};
	uint8_t pdu[2 * sizeof(uint16_t) + sizeof(chrc_data)];

	sys_put_le16(handle, &pdu[0]);
	sys_put_le16(offset, &pdu[2]);
	(void)memcpy(&pdu[4], chrc_data, sizeof(chrc_data));

	return att_request(BT_ATT_OP_PREPARE_WRITE_REQ, pdu, sizeof(pdu));
}

static int execute_write(uint8_t flags)
{
	return att_request(BT_ATT_OP_EXEC_WRITE_REQ, &flags, sizeof(flags));
}

static void control_point_write(enum exec_write_stage stage)
{
	uint8_t pdu[sizeof(uint16_t) + sizeof(uint8_t)];
	int err;

	sys_put_le16(cp_chrc_handle, &pdu[0]);
	pdu[2] = (uint8_t)stage;

	err = att_request(BT_ATT_OP_WRITE_REQ, pdu, sizeof(pdu));
	if (err != 0) {
		TEST_FAIL("Control point write failed (err %d)", err);
	}
}

static void test_cancel(void)
{
	int err;

	err = prepare_write(first_chrc_handle, 0);
	if (err != 0) {
		TEST_FAIL("Prepare write failed (err %d)", err);
	}

	err = prepare_write(first_chrc_handle, EXEC_WRITE_CHRC_SIZE + 1U);
	if (err != 0) {
		TEST_FAIL("Prepare write failed (err %d)", err);
	}

	err = execute_write(BT_ATT_FLAG_CANCEL);
	if (err != 0) {
		TEST_FAIL("Cancelled execute write answered with 0x%02x", err);
	}

	control_point_write(EXEC_WRITE_STAGE_CHECK_CANCEL);
}

static void test_error(void)
{
	int err;

	err = prepare_write(first_chrc_handle, 0);
	if (err != 0) {
		TEST_FAIL("Prepare write failed (err %d)", err);
	}

	err = prepare_write(second_chrc_handle, 0);
	if (err != 0) {
		TEST_FAIL("Prepare write failed (err %d)", err);
	}

	err = execute_write(BT_ATT_FLAG_EXEC);
	if (err != EXEC_WRITE_APP_ERR) {
		TEST_FAIL("Execute write answered with %d", err);
	}

	control_point_write(EXEC_WRITE_STAGE_CHECK_ERROR);
}

static void test_main(void)
{
	bt_addr_le_t peer = {};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	err = bt_testlib_scan_find_name(&peer, CONFIG_BT_DEVICE_NAME);
	if (err != 0) {
		TEST_FAIL("Scan failed (err %d)", err);

		return;
	}

	err = bt_testlib_connect(&peer, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);

		return;
	}

	gatt_discover();

	test_cancel();
	test_error();

	TEST_PASS("GATT client passed");
}

static const struct bst_test_instance test_exec_write_client[] = {
	{
		.test_id = "exec_write_client",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_exec_write_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_exec_write_client);
}
