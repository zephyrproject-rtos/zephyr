/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/__assert.h>

#include <testlib/adv.h>
#include <testlib/conn.h>

/* @file
 *
 * This app advertises connectable with the name "connectable".
 * It only receives one connection at a time. When the remote
 * disconnects, it starts advertising again.
 *
 * This app is added to the simulation simply to be a target for
 * a connection from the DUT.
 */

int main(void)
{
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_set_name("connectable");
	__ASSERT_NO_MSG(!err);

	while (true) {
		struct bt_conn *conn = NULL;

		bt_testlib_conn_wait_free();

		err = bt_testlib_adv_conn(
			&conn, BT_ID_DEFAULT,
			(BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD));
		__ASSERT_NO_MSG(!err);

		bt_testlib_wait_disconnected(conn);
		bt_testlib_conn_unref(&conn);
	}

	return 0;
}
