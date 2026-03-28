/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/conn.h"
#include "mocks/conn_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_conn_lookup_state_le(uint8_t id, const bt_addr_le_t *peer,
						const bt_conn_state_t state)
{
	const char *func_name = "bt_conn_lookup_state_le";

	zassert_equal(bt_conn_lookup_state_le_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);

	zassert_equal(bt_conn_lookup_state_le_fake.arg0_val, id,
		      "'%s()' was called with incorrect '%s' value", func_name, "id");
	zassert_equal_ptr(bt_conn_lookup_state_le_fake.arg1_val, peer,
			  "'%s()' was called with incorrect '%s' value", func_name, "peer");
	zassert_equal(bt_conn_lookup_state_le_fake.arg2_val, state,
		      "'%s()' was called with incorrect '%s' value", func_name, "state");
}

void expect_not_called_bt_conn_lookup_state_le(void)
{
	const char *func_name = "bt_conn_lookup_state_le";

	zassert_equal(bt_conn_lookup_state_le_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}

void expect_single_call_bt_conn_unref(struct bt_conn *conn)
{
	const char *func_name = "bt_conn_unref";

	zassert_equal(bt_conn_unref_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(bt_conn_unref_fake.arg0_val, conn,
			  "'%s()' was called with incorrect '%s' value", func_name, "conn");
}

void expect_not_called_bt_conn_unref(void)
{
	const char *func_name = "bt_conn_unref";

	zassert_equal(bt_conn_unref_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
