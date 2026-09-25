/* Copyright (c) 2023 Codecoup
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bsim_args_runner.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include "testlib/adv.h"
#include "testlib/security.h"

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include "../common_defs.h"


LOG_MODULE_REGISTER(server, LOG_LEVEL_DBG);

static ssize_t read_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t buf_len, uint16_t offset)
{
	return 0;
}

BT_GATT_SERVICE_DEFINE(test_svc,
		       BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ_ENCRYPT, read_chrc, NULL, NULL));

static void test_common(struct bt_conn **conn)
{
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	__ASSERT_NO_MSG(get_device_nbr() == 1);
	err = bt_set_name("d1");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_adv_conn(conn, BT_ID_DEFAULT, bt_get_name());
	__ASSERT_NO_MSG(!err);
}

static void test_server(void)
{
	test_common(NULL);

	TEST_PASS("PASS");
}

static void test_server_security_request(void)
{
	struct bt_conn *conn = NULL;
	int err;

	test_common(&conn);

	err = bt_testlib_secure(conn, BT_SECURITY_L2);
	__ASSERT(!err, "err %d", err);

	TEST_PASS("PASS");
}

DEFINE_FLAG_STATIC(flag_pairing_confirm_requested);

static void auth_cancel_cb(struct bt_conn *conn)
{
}

static void auth_pairing_confirm_cb(struct bt_conn *conn)
{
	SET_FLAG(flag_pairing_confirm_requested);
}

static struct bt_conn_auth_cb auth_cb = {
	.cancel = auth_cancel_cb,
	.pairing_confirm = auth_pairing_confirm_cb,
};

static void test_server_cancel_retrying(void)
{
	struct bt_conn *conn = NULL;
	int err;

	err = bt_conn_auth_cb_register(&auth_cb);
	__ASSERT_NO_MSG(!err);

	test_common(&conn);

	/* Hold the pairing that the client's read triggers, so that the
	 * client cancels the read while it is waiting for its retry.
	 */
	WAIT_FOR_FLAG(flag_pairing_confirm_requested);
	k_sleep(K_SECONDS(3));

	err = bt_conn_auth_pairing_confirm(conn);
	__ASSERT_NO_MSG(!err);

	bt_conn_drop(&conn);

	TEST_PASS("PASS");
}

static const struct bst_test_instance server_tests[] = {
	{
		.test_id = "test_server",
		.test_main_f = test_server,
	},
	{
		.test_id = "test_server_security_request",
		.test_main_f = test_server_security_request,
	},
	{
		.test_id = "test_server_cancel_retrying",
		.test_main_f = test_server_cancel_retrying,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *server_tests_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, server_tests);
};

bst_test_install_t test_installers[] = {
	server_tests_install,
	NULL
};

int main(void)
{
	bst_main();

	return 0;
}
