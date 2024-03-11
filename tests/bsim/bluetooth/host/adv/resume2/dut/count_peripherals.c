/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bs_tracing.h>
#include <bstests.h>
#include <stdint.h>
#include <testlib/conn.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic_builtin.h>
#include <zephyr/sys/atomic_types.h>


static void _count_conn_marked_peripheral_loop(struct bt_conn *conn, void *count_)
{
	size_t *count = count_;
	struct bt_conn_info conn_info;

	bt_conn_get_info(conn, &conn_info);

	if (conn_info.role == BT_CONN_ROLE_PERIPHERAL) {
		(*count)++;
	}
}

size_t count_conn_marked_peripheral(void)
{
	size_t count = 0;

	bt_conn_foreach(BT_CONN_TYPE_LE, _count_conn_marked_peripheral_loop, &count);

	return count;
}
