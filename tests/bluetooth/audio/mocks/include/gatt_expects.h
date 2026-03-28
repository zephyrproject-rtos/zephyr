/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_GATT_EXPECTS_H_
#define MOCKS_GATT_EXPECTS_H_

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/ztest_assert.h>

#include "gatt.h"
#include "expects_util.h"

#define expect_bt_gatt_notify_cb_called_once(_conn, _uuid, _attr, _data, _len)                     \
do {                                                                                               \
	const char *func_name = "bt_gatt_notify_cb";                                               \
	struct bt_gatt_notify_params *params;                                                      \
												   \
	IF_NOT_EMPTY(_conn, (                                                                      \
		     zassert_equal_ptr(_conn, mock_bt_gatt_notify_cb_fake.arg0_val,                \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "conn");))                                       \
												   \
	params = mock_bt_gatt_notify_cb_fake.arg1_val;                                             \
												   \
	/* params->uuid is optional */                                                             \
	if (params->uuid) {                                                                        \
		IF_NOT_EMPTY(_uuid, (                                                              \
			     zassert_true(bt_uuid_cmp(_uuid, params->uuid) == 0,                   \
					  "'%s()' was called with incorrect '%s' value",           \
					  func_name, "params->uuid");))                            \
	} else {                                                                                   \
		IF_NOT_EMPTY(_attr, (                                                              \
			     zassert_equal_ptr(_attr, params->attr,                                \
					       "'%s()' was called with incorrect '%s' value",      \
					       func_name, "params->attr");))                       \
	}                                                                                          \
												   \
	/* assert if _data is valid, but _len is empty */                                          \
	IF_EMPTY(_len, (IF_NOT_EMPTY(_data, (zassert_unreachable();))))                            \
												   \
	IF_NOT_EMPTY(_len, (                                                                       \
		     zassert_equal(_len, params->len,                                              \
				   "'%s()' was called with incorrect '%s' value",                  \
				   func_name, "params->len");                                      \
		     expect_data(func_name, "params->data", _data, params->data, _len);))          \
} while (0)

static inline void expect_bt_gatt_notify_cb_not_called(void)
{
	const char *func_name = "bt_gatt_notify_cb";

	zassert_equal(0, mock_bt_gatt_notify_cb_fake.call_count,
		      "'%s()' was called unexpectedly", func_name);
}

#endif /* MOCKS_GATT_EXPECTS_H_ */
