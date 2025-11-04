/* Copyright (c) 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/uuid.h>

ZTEST_SUITE(bt_uuid_from_str, NULL, NULL, NULL, NULL, NULL);

ZTEST(bt_uuid_from_str, test_uuid_from_str_16)
{
	struct bt_uuid_16 uuid = {0};
	int ret;

	ret = bt_uuid_from_str("180d", &uuid.uuid);
	zassert_true(ret == 0, "Failed to parse 16-bit UUID");
	zassert_true(bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_16(0x180d)) == 0,
		     "Parsed UUID does not match expected 16-bit UUID");
}

ZTEST(bt_uuid_from_str, test_uuid_from_str_32)
{
	struct bt_uuid_32 uuid = {0};
	int ret;

	ret = bt_uuid_from_str("abcdef12", &uuid.uuid);
	zassert_true(ret == 0, "Failed to parse 32-bit UUID");
	zassert_true(bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_32(0xabcdef12)) == 0,
		     "Parsed UUID does not match expected 32-bit UUID");
}

ZTEST(bt_uuid_from_str, test_uuid_from_str_128)
{
	struct bt_uuid_128 uuid = {0};
	int ret;

	ret = bt_uuid_from_str("00001101-0000-1000-8000-00805f9b34fb", &uuid.uuid);
	zassert_true(ret == 0, "Failed to parse 128-bit UUID");
	struct bt_uuid_128 expected =
		BT_UUID_INIT_128(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00,
				 0x00, 0x01, 0x11, 0x00, 0x00);
	zassert_true(bt_uuid_cmp(&uuid.uuid, &expected.uuid) == 0,
		     "Parsed UUID does not match expected 128-bit UUID");
}

ZTEST(bt_uuid_from_str, test_uuid_roundtrip_to_str_and_back_16)
{
	struct bt_uuid_16 u1 = BT_UUID_INIT_16(0x180d);
	struct bt_uuid_16 uuid_tmp = {0};
	char str[BT_UUID_STR_LEN];
	int ret;

	/* uuid -> str */
	bt_uuid_to_str(&u1.uuid, str, sizeof(str));

	/* str -> uuid */
	ret = bt_uuid_from_str(str, &uuid_tmp.uuid);
	zassert_true(ret == 0, "bt_uuid_from_str failed for 16-bit");

	/* compare */
	zassert_true(bt_uuid_cmp(&u1.uuid, &uuid_tmp.uuid) == 0, "Round-trip 16-bit UUID mismatch");
}

ZTEST(bt_uuid_from_str, test_uuid_roundtrip_to_str_and_back_32)
{
	struct bt_uuid_32 u1 = BT_UUID_INIT_32(0xabcdef12);
	struct bt_uuid_32 uuid_tmp = {0};
	char str[BT_UUID_STR_LEN];
	int ret;

	/* uuid -> str */
	bt_uuid_to_str(&u1.uuid, str, sizeof(str));

	/* str -> uuid */
	ret = bt_uuid_from_str(str, &uuid_tmp.uuid);
	zassert_true(ret == 0, "bt_uuid_from_str failed for 32-bit");

	/* compare */
	zassert_true(bt_uuid_cmp(&u1.uuid, &uuid_tmp.uuid) == 0, "Round-trip 32-bit UUID mismatch");
}

ZTEST(bt_uuid_from_str, test_uuid_roundtrip_to_str_and_back_128)
{
	struct bt_uuid_128 u1 = BT_UUID_INIT_128(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
						 0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00);
	struct bt_uuid_128 uuid_tmp = {0};
	char str[BT_UUID_STR_LEN];
	int ret;

	/* uuid -> str */
	bt_uuid_to_str(&u1.uuid, str, sizeof(str));

	/* str -> uuid */
	ret = bt_uuid_from_str(str, &uuid_tmp.uuid);
	zassert_true(ret == 0, "bt_uuid_from_str failed for 128-bit");

	/* compare */
	zassert_true(bt_uuid_cmp(&u1.uuid, &uuid_tmp.uuid) == 0,
		     "Round-trip 128-bit UUID mismatch");
}

ZTEST(bt_uuid_from_str, test_uuid_from_str_invalid)
{
	struct bt_uuid_128 uuid = {0};
	int ret;

	ret = bt_uuid_from_str("not-a-uuid", &uuid.uuid);
	zassert_true(ret < 0, "Invalid UUID string should fail");

	ret = bt_uuid_from_str("", &uuid.uuid);
	zassert_true(ret < 0, "Empty string should fail");

	ret = bt_uuid_from_str("123", &uuid.uuid);
	zassert_true(ret < 0, "Too short string should fail");

	ret = bt_uuid_from_str("00001101-0000-1000-8000-00805f9b34fb00", &uuid.uuid);
	zassert_true(ret < 0, "Too long 128-bit string should fail");
}

ZTEST(bt_uuid_from_str, test_uuid_from_str_null_params)
{
	struct bt_uuid_128 uuid = {0};
	int ret;

	ret = bt_uuid_from_str("180d", NULL);
	zassert_true(ret < 0, "NULL uuid pointer should fail");

	ret = bt_uuid_from_str(NULL, &uuid.uuid);
	zassert_true(ret < 0, "NULL string pointer should fail");
}
