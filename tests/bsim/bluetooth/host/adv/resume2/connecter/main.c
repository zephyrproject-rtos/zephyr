/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <testlib/conn.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(connecter, LOG_LEVEL_INF);

int main(void)
{
	int err;
	bt_addr_le_t result;
	struct bt_conn *conn = NULL;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	while (true) {
		err = bt_testlib_scan_find_name(&result, "dut");
		__ASSERT_NO_MSG(!err);

		err = bt_testlib_connect(&result, &conn);
		__ASSERT_NO_MSG(!err);

		bt_testlib_wait_disconnected(conn);
		LOG_INF("Disconnected");
		bt_testlib_conn_unref(&conn);
	}

	return 0;
}
