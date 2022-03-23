/* Copyright (c) 2023 Codecoup
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "../common_defs.h"
#include "../test_utils.h"

#include <testlib/conn.h>
#include "testlib/scan.h"
#include "testlib/security.h"

LOG_MODULE_REGISTER(client, LOG_LEVEL_DBG);

DEFINE_FLAG(flag_attr_read_success);

static uint8_t gatt_attr_read_cb(struct bt_conn *conn, uint8_t att_err,
				 struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	__ASSERT_NO_MSG(!att_err);

	SET_FLAG(flag_attr_read_success);

	return BT_GATT_ITER_STOP;
}

static void gatt_attr_read(struct bt_conn *conn)
{
	static struct bt_gatt_read_params params;
	static struct bt_uuid_128 uuid;
	int err;

	memset(&params, 0, sizeof(params));
	params.func = gatt_attr_read_cb;
	params.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	memcpy(&uuid.uuid, TEST_CHRC_UUID, sizeof(uuid));
	params.by_uuid.uuid = &uuid.uuid;

	err = bt_gatt_read(conn, &params);
	__ASSERT_NO_MSG(!err);
}

DEFINE_FLAG(flag_conn_encrypted);

static void security_changed_cb(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err)
{
	if (err != BT_SECURITY_ERR_SUCCESS || level < BT_SECURITY_L2) {
		return;
	}

	SET_FLAG(flag_conn_encrypted);
}

static struct bt_conn_cb conn_cb = {
	.security_changed = security_changed_cb,
};

static void test_client(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t scan_result;
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	bt_conn_cb_register(&conn_cb);

	err = bt_testlib_scan_find_name(&scan_result, "d1");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_connect(&scan_result, &conn);
	__ASSERT_NO_MSG(!err);

	/* Read characteristic value that requires encryption */
	gatt_attr_read(conn);

	/* Expect link encryption  */
	WAIT_FOR_FLAG(flag_conn_encrypted);

	/* Wait for successful Read Response */
	WAIT_FOR_FLAG(flag_attr_read_success);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(!err);

	bt_conn_unref(conn);
	conn = NULL;

	PASS("PASS\n");
}

DEFINE_FLAG(flag_pairing_in_progress);

static void auth_cancel_cb(struct bt_conn *conn)
{

}

static void auth_pairing_confirm_cb(struct bt_conn *conn)
{
	SET_FLAG(flag_pairing_in_progress);
}

static struct bt_conn_auth_cb auth_cb = {
	.cancel = auth_cancel_cb,
	.pairing_confirm = auth_pairing_confirm_cb,
};

static void test_client_security_request(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t scan_result;
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	bt_conn_cb_register(&conn_cb);

	err = bt_conn_auth_cb_register(&auth_cb);
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_scan_find_name(&scan_result, "d1");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_connect(&scan_result, &conn);
	__ASSERT_NO_MSG(!err);

	/* Wait for peripheral to initaiate pairing */
	WAIT_FOR_FLAG(flag_pairing_in_progress);

	/* Read characteristic value that requires encryption */
	gatt_attr_read(conn);

	/* Accept pairing */
	err = bt_conn_auth_pairing_confirm(conn);
	__ASSERT_NO_MSG(!err);

	/* Expect link encryption  */
	WAIT_FOR_FLAG(flag_conn_encrypted);

	/* Wait for successful Read Response */
	WAIT_FOR_FLAG(flag_attr_read_success);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(!err);

	bt_conn_unref(conn);
	conn = NULL;

	PASS("PASS\n");
}

static const struct bst_test_instance client_tests[] = {
	{
		.test_id = "test_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client,
	},
	{
		.test_id = "test_client_security_request",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client_security_request,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *client_tests_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, client_tests);
};

bst_test_install_t test_installers[] = {
	client_tests_install,
	NULL
};

int main(void)
{
	bst_main();

	return 0;
}
