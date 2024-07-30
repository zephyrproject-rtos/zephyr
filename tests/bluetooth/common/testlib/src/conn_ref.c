/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/atomic_builtin.h>

#include <testlib/conn.h>

/**
 * @file
 * @brief Reified connection reference counting.
 * @ingroup testlib conn ref
 *
 * This file provides functions to reify the moving and cloning of @ref
 * bt_conn references for increased safety.
 *
 * Reifying means that the existence of a reference is always tied
 * one-to-one with a non-NULL value in a owning pointer variable.
 *
 * The functions in this file will trigger an assert if they attempt to
 * overwrite a non-NULL value in a owning pointer variable. This is to
 * prevent leaking the reference that presumable is tied the value that
 * would be overwritten.
 *
 * The functions in this file are intended to guard against undefined
 * behavor due to NULL pointer dereferencing. They will assert on any
 * relevant pointers.
 */

void bt_testlib_conn_unref(struct bt_conn **connp)
{
	struct bt_conn *conn;

	__ASSERT_NO_MSG(connp);
	conn = atomic_ptr_set((void **)connp, NULL);
	__ASSERT_NO_MSG(conn);
	bt_conn_unref(conn);
}

struct find_by_index_data {
	uint8_t wanted_index;
	struct bt_conn *found_conn;
};

static void find_by_index(struct bt_conn *conn, void *user_data)
{
	struct find_by_index_data *data = user_data;
	uint8_t index = bt_conn_index(conn);

	if (index == data->wanted_index) {
		data->found_conn = bt_conn_ref(conn);
	}
}

struct bt_conn *bt_testlib_conn_unindex(enum bt_conn_type conn_type, uint8_t conn_index)
{
	struct find_by_index_data data = {
		.wanted_index = conn_index,
	};

	bt_conn_foreach(conn_type, find_by_index, &data);

	return data.found_conn;
}
