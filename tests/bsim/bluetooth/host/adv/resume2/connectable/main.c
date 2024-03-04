/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <testlib/adv.h>
#include <testlib/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/__assert.h>

int main(void)
{
	int err;
	struct bt_conn *conn;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	while (true) {
		err = bt_set_name("connectable");
		__ASSERT_NO_MSG(!err);

		err = bt_testlib_adv_conn(
			&conn, BT_ID_DEFAULT,
			(BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD));
		__ASSERT_NO_MSG(!err);

		bt_testlib_wait_disconnected(conn);
		bt_testlib_conn_unref(&conn);
	}

	return 0;
}
