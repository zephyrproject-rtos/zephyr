/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <testlib/conn.h>
#include <zephyr/sys/atomic_builtin.h>

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
