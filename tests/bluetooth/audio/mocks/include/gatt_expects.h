/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_GATT_EXPECTS_H_
#define MOCKS_GATT_EXPECTS_H_

#include <zephyr/bluetooth/gatt.h>

#include "gatt.h"
#include "expects_util.h"

#define expect_bt_gatt_attr_read_called_once(_conn, _attr, _buf, _buf_len, _offset, _value,        \
					     _value_len)                                           \
do {                                                                                               \
	const char *func_name = "bt_gatt_attr_read";                                               \
												   \
	zassert_equal(1, bt_gatt_attr_read_fake.call_count,                                        \
		      "'%s()' was called %u times, but expected once",                             \
		      func_name, bt_gatt_attr_read_fake.call_count);                               \
												   \
	IF_NOT_EMPTY(_conn, (                                                                      \
		     zassert_equal_ptr(_conn, bt_gatt_attr_read_fake.arg0_val,                     \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "conn");))                                       \
												   \
	IF_NOT_EMPTY(_attr, (                                                                      \
		     zassert_equal_ptr(_attr, bt_gatt_attr_read_fake.arg1_val,                     \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "attr");))                                       \
												   \
	IF_NOT_EMPTY(_buf, (                                                                       \
		     zassert_equal_ptr(_buf, bt_gatt_attr_read_fake.arg2_val,                      \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "buf");))                                        \
												   \
	IF_NOT_EMPTY(_buf_len, (                                                                   \
		     zassert_equal(_buf_len, bt_gatt_attr_read_fake.arg3_val,                      \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "_buf_len");))                                   \
												   \
	IF_NOT_EMPTY(_offset, (                                                                    \
		     zassert_equal(_offset, bt_gatt_attr_read_fake.arg4_val,                       \
				   "'%s()' was called with incorrect '%s' value",                  \
				   func_name, "offset");))                                         \
												   \
	/* assert if _data is valid, but _len is empty */                                          \
	IF_EMPTY(_value_len, (IF_NOT_EMPTY(_value, (zassert_unreachable();))))                     \
												   \
	IF_NOT_EMPTY(_value_len, (                                                                 \
		     zassert_equal(_value_len, bt_gatt_attr_read_fake.arg6_val,                    \
				   "'%s()' was called with incorrect '%s' value",                  \
				   func_name, "value_len");                                        \
		     expect_data(func_name, "value", _value, bt_gatt_attr_read_fake.arg5_val,      \
				 _value_len);))                                                    \
} while (0)

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

static inline void expect_bt_gatt_attr_read_not_called(void)
{
	const char *func_name = "bt_gatt_attr_read";

	zassert_equal(0, bt_gatt_attr_read_fake.call_count,
		      "'%s()' was called unexpectedly", func_name);
}

static inline void expect_bt_gatt_notify_cb_not_called(void)
{
	const char *func_name = "bt_gatt_notify_cb";

	zassert_equal(0, mock_bt_gatt_notify_cb_fake.call_count,
		      "'%s()' was called unexpectedly", func_name);
}

#endif /* MOCKS_GATT_EXPECTS_H_ */
