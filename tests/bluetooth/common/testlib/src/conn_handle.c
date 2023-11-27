/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/conn.h>

#include <testlib/conn.h>

int bt_testlib_get_conn_handle(struct bt_conn *conn, uint16_t *handle)
{
	struct bt_conn_info info;
	int err;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(handle);

	err = bt_conn_get_info(conn, &info);
	*handle = info.handle;

	return err;
}
