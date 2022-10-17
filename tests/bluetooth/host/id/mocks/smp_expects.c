/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/smp.h"
#include "mocks/smp_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob)
{
	const char *func_name = "bt_smp_le_oob_generate_sc_data";

	zassert_equal(bt_smp_le_oob_generate_sc_data_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);
}

void expect_not_called_bt_smp_le_oob_generate_sc_data(void)
{
	const char *func_name = "bt_smp_le_oob_generate_sc_data";

	zassert_equal(bt_smp_le_oob_generate_sc_data_fake.call_count, 0,
		      "'%s()' was called unexpectedly", func_name);
}

void expect_single_call_bt_smp_le_oob_set_tk(struct bt_conn *conn, const uint8_t *tk)
{
	const char *func_name = "bt_smp_le_oob_set_tk";

	zassert_equal(bt_smp_le_oob_set_tk_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(bt_smp_le_oob_set_tk_fake.arg0_val, conn,
			  "'%s()' was called with incorrect '%s' value", func_name, "conn");
	zassert_equal_ptr(bt_smp_le_oob_set_tk_fake.arg1_val, tk,
			  "'%s()' was called with incorrect '%s' value", func_name, "tk");
}

void expect_single_call_bt_smp_le_oob_set_sc_data(struct bt_conn *conn,
						  const struct bt_le_oob_sc_data *oobd_local,
						  const struct bt_le_oob_sc_data *oobd_remote)
{
	const char *func_name = "bt_smp_le_oob_set_sc_data";

	zassert_equal(bt_smp_le_oob_set_sc_data_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);

	zassert_equal_ptr(bt_smp_le_oob_set_sc_data_fake.arg0_val, conn,
			  "'%s()' was called with incorrect '%s' value", func_name, "conn");
	zassert_equal_ptr(bt_smp_le_oob_set_sc_data_fake.arg1_val, oobd_local,
			  "'%s()' was called with incorrect '%s' value", func_name, "oobd_local");
	zassert_equal_ptr(bt_smp_le_oob_set_sc_data_fake.arg2_val, oobd_remote,
			  "'%s()' was called with incorrect '%s' value", func_name, "oobd_remote");
}

void expect_single_call_bt_smp_le_oob_get_sc_data(struct bt_conn *conn,
						  const struct bt_le_oob_sc_data **oobd_local,
						  const struct bt_le_oob_sc_data **oobd_remote)
{
	const char *func_name = "bt_smp_le_oob_get_sc_data";

	zassert_equal(bt_smp_le_oob_get_sc_data_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);

	zassert_equal_ptr(bt_smp_le_oob_get_sc_data_fake.arg0_val, conn,
			  "'%s()' was called with incorrect '%s' value", func_name, "conn");
	zassert_equal_ptr(bt_smp_le_oob_get_sc_data_fake.arg1_val, oobd_local,
			  "'%s()' was called with incorrect '%s' value", func_name, "oobd_local");
	zassert_equal_ptr(bt_smp_le_oob_get_sc_data_fake.arg2_val, oobd_remote,
			  "'%s()' was called with incorrect '%s' value", func_name, "oobd_remote");
}
