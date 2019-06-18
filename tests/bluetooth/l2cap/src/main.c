/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	return -ENOSYS;
}

static struct bt_l2cap_server test_server = {
	.accept		= l2cap_accept,
};

static struct bt_l2cap_server test_fixed_server = {
	.accept		= l2cap_accept,
	.psm		= 0x007f,
};

static struct bt_l2cap_server test_dyn_server = {
	.accept		= l2cap_accept,
	.psm		= 0x00ff,
};

static struct bt_l2cap_server test_inv_server = {
	.accept		= l2cap_accept,
	.psm		= 0xffff,
};

void test_l2cap_register(void)
{
	/* Attempt to register server with PSM auto allocation */
	zassert_false(bt_l2cap_server_register(&test_server),
		     "Test server registration failed");

	/* Attempt to register server with fixed PSM */
	zassert_false(bt_l2cap_server_register(&test_fixed_server),
		     "Test fixed PSM server registration failed");

	/* Attempt to register server with dynamic PSM */
	zassert_false(bt_l2cap_server_register(&test_dyn_server),
		     "Test dynamic PSM server registration failed");

	/* Attempt to register server with invalid PSM */
	zassert_true(bt_l2cap_server_register(&test_inv_server),
		     "Test invalid PSM server registration succeeded");

	/* Attempt to re-register server with PSM auto allocation */
	zassert_true(bt_l2cap_server_register(&test_server),
		     "Test server duplicate succeeded");

	/* Attempt to re-register server with fixed PSM */
	zassert_true(bt_l2cap_server_register(&test_fixed_server),
		     "Test fixed PSM server duplicate succeeded");

	/* Attempt to re-register server with dynamic PSM */
	zassert_true(bt_l2cap_server_register(&test_dyn_server),
		     "Test dynamic PSM server duplicate succeeded");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_l2cap,
			 ztest_unit_test(test_l2cap_register));
	ztest_run_test_suite(test_l2cap);
}
