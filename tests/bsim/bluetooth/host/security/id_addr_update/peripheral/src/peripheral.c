/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "utils.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/toolchain.h>

#include <stdint.h>
#include <string.h>

#include "babblekit/testcase.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(peripheral, LOG_LEVEL_INF);

static void verify_equal_address(struct bt_conn *conn_a, struct bt_conn *conn_b)
{
	int err;
	struct bt_conn_info info_a;
	struct bt_conn_info info_b;

	err = bt_conn_get_info(conn_a, &info_a);
	TEST_ASSERT(!err, "Unexpected info_a result.");

	err = bt_conn_get_info(conn_b, &info_b);
	TEST_ASSERT(!err, "Unexpected info_b result.");

	TEST_ASSERT(bt_addr_le_eq(info_a.le.dst, info_b.le.dst),
	       "Conn A address is not equal with the conn B address");
}

void peripheral(void)
{
	bs_bt_utils_setup();

	int id_a;
	int id_b;

	struct bt_conn *conn_a;
	struct bt_conn *conn_b;

	/* Create two identities that will simultaneously connect with the same central peer. */
	id_a = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_a >= 0, "bt_id_create id_a failed (err %d)", id_a);

	id_b = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_b >= 0, "bt_id_create id_b failed (err %d)", id_b);

	/* Connect with the first identity. */
	LOG_INF("adv");
	advertise_connectable(id_a);
	LOG_INF("wait conn");
	wait_connected(&conn_a);

	/* Send battery notification on the first connection. */
	wait_bas_ccc_subscription();
	bas_notify(conn_a);

	/* Connect with the second identity. */
	LOG_INF("adv id 2");
	advertise_connectable(id_b);
	wait_connected(&conn_b);

	/* Wait for the pairing completed callback on the second identity. */
	wait_pairing_completed();

	/* Both connections should relate to the identity address of the same Central peer. */
	verify_equal_address(conn_a, conn_b);

	/* Send notification after identity address resolution to the first connection object. */
	bas_notify(conn_a);

	/* Disconnect the first identity. */
	wait_disconnected();
	clear_conn(conn_a);

	/* Disconnect the second identity. */
	wait_disconnected();
	clear_conn(conn_b);

	TEST_PASS("PASS");
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "peripheral",
		.test_main_f = peripheral,
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
