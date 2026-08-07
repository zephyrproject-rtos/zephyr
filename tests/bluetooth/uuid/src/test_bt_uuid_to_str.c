/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <string.h>

#include <zephyr/ztest.h>

#include <zephyr/bluetooth/uuid.h>

ZTEST_SUITE(bt_uuid_to_str, NULL, NULL, NULL, NULL, NULL);

static bool is_null_terminated(char *str, size_t size)
{
	return strnlen(str, size) < size;
}

static void result_is_null_terminated(const struct bt_uuid *uuid)
{
	char str[BT_UUID_STR_LEN];

	memset(str, 1, sizeof(str));
	bt_uuid_to_str(uuid, str, sizeof(str));
	zassert_true(is_null_terminated(str, sizeof(str)), "Result is not null-terminated.");
}

static void result_str_is(const struct bt_uuid *uuid, const char *expected_str)
{
	char str[BT_UUID_STR_LEN] = {};

	bt_uuid_to_str(uuid, str, sizeof(str));
	zassume_true(is_null_terminated(str, sizeof(str)), "Result is not a string.");
	zassert_true((strcmp(str, expected_str) == 0),
		     "Unexpected result.\n   Found: %s\nExpected: %s", str, expected_str);
}

ZTEST(bt_uuid_to_str, test_null_terminated_type_16)
{
	result_is_null_terminated(BT_UUID_DECLARE_16(0));
}

ZTEST(bt_uuid_to_str, test_null_terminated_type_32)
{
	result_is_null_terminated(BT_UUID_DECLARE_32(0));
}

ZTEST(bt_uuid_to_str, test_null_terminated_type_128)
{
	result_is_null_terminated(BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0l, 0, 0, 0, 0ll)));
}

ZTEST(bt_uuid_to_str, test_padding_type_16)
{
	result_str_is(BT_UUID_DECLARE_16(0), "0000");
}

ZTEST(bt_uuid_to_str, test_padding_type_32)
{
	result_str_is(BT_UUID_DECLARE_32(0), "00000000");
}

ZTEST(bt_uuid_to_str, test_padding_type_128)
{
	result_str_is(BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0l, 0, 0, 0, 0ll)),
		      "00000000-0000-0000-0000-000000000000");
}

ZTEST(bt_uuid_to_str, test_ordering_type_16)
{
	result_str_is(BT_UUID_DECLARE_16(0xabcd), "abcd");
}


ZTEST(bt_uuid_to_str, test_ordering_type_32)
{
	result_str_is(BT_UUID_DECLARE_32(0xabcdef12), "abcdef12");
}

ZTEST(bt_uuid_to_str, test_ordering_type_128)
{
	result_str_is(BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(
		0xabcdef12, 0x3456, 0x9999, 0x9999, 0x999999999999)),
		"abcdef12-3456-9999-9999-999999999999");

	result_str_is(BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(
		0x99999999, 0x9999, 0xabcd, 0xef12, 0x999999999999)),
		"99999999-9999-abcd-ef12-999999999999");

	result_str_is(BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(
		0x99999999, 0x9999, 0x9999, 0x9999, 0xabcdef123456)),
		"99999999-9999-9999-9999-abcdef123456");
}
