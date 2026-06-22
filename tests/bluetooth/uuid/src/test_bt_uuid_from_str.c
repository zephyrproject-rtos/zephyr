/* Copyright (c) 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/uuid.h>

ZTEST_SUITE(bt_uuid_from_str, NULL, NULL, NULL, NULL, NULL);

/* Short hex "180d" -> 16-bit UUID */
ZTEST(bt_uuid_from_str, test_uuid_from_str_16)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("180d", &uuid);
	zassert_true(ret == 0, "Failed to parse 16-bit UUID");
	zassert_equal(uuid.uuid.type, BT_UUID_TYPE_16, "Should be 16-bit");
	zassert_true(bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_16(0x180dU)) == 0,
		     "Parsed UUID does not match expected 16-bit UUID");
}

/* Short hex "abcdef12" -> 32-bit UUID */
ZTEST(bt_uuid_from_str, test_uuid_from_str_32)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("abcdef12", &uuid);
	zassert_true(ret == 0, "Failed to parse 32-bit UUID");
	zassert_equal(uuid.uuid.type, BT_UUID_TYPE_32, "Should be 32-bit");
	zassert_true(bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_32(0xabcdef12U)) == 0,
		     "Parsed UUID does not match expected 32-bit UUID");
}

/* 8-hex-digit with leading zeros -> always 32-bit, never auto-compacted to 16-bit */
ZTEST(bt_uuid_from_str, test_uuid_from_str_32_leading_zeros)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("0000180d", &uuid);
	zassert_true(ret == 0, "Failed to parse 32-bit UUID with leading zeros");
	zassert_equal(uuid.uuid.type, BT_UUID_TYPE_32, "Should be 32-bit");
	zassert_equal(BT_UUID_32(&uuid.uuid)->val, 0x0000180dU, "Value mismatch");
}

/* Full UUID matching Base UUID -> still 128-bit (no auto-compaction) */
ZTEST(bt_uuid_from_str, test_uuid_from_str_base_uuid_stays_128)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("0000180d-0000-1000-8000-00805f9b34fb", &uuid);
	zassert_true(ret == 0, "Failed to parse UUID");
	zassert_equal(uuid.uuid.type, BT_UUID_TYPE_128, "Full string should stay 128-bit");
	zassert_true(bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_16(0x180dU)) == 0,
		     "Value should still compare equal to 16-bit equivalent");

	ret = bt_uuid_from_str("abcdef12-0000-1000-8000-00805f9b34fb", &uuid);
	zassert_true(ret == 0, "Failed to parse UUID");
	zassert_equal(uuid.uuid.type, BT_UUID_TYPE_128, "Full string should stay 128-bit");
	zassert_true(bt_uuid_cmp(&uuid.uuid, BT_UUID_DECLARE_32(0xabcdef12U)) == 0,
		     "Value should still compare equal to 32-bit equivalent");
}

/* Full UUID (non-Base) -> 128-bit */
ZTEST(bt_uuid_from_str, test_uuid_from_str_128)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("6e400001-b5a3-f393-e0a9-e50e24dcca9e", &uuid);
	zassert_true(ret == 0, "Failed to parse 128-bit UUID");
	zassert_equal(uuid.uuid.type, BT_UUID_TYPE_128, "Should be 128-bit");
	const struct bt_uuid *expected =
		BT_UUID_DECLARE_128(0x9eU, 0xcaU, 0xdcU, 0x24U, 0x0eU, 0xe5U, 0xa9U, 0xe0U,
				    0x93U, 0xf3U, 0xa3U, 0xb5U, 0x01U, 0x00U, 0x40U, 0x6eU);
	zassert_true(bt_uuid_cmp(&uuid.uuid, expected) == 0,
		     "Parsed UUID does not match expected 128-bit UUID");
}

/* Roundtrip: 16-bit -> to_str -> from_str -> compare */
ZTEST(bt_uuid_from_str, test_uuid_roundtrip_to_str_and_back_16)
{
	struct bt_uuid_16 u1 = BT_UUID_INIT_16(0x180dU);
	struct bt_uuid_any uuid_tmp = {0};
	char str[BT_UUID_STR_LEN];
	int ret;

	bt_uuid_to_str(&u1.uuid, str, sizeof(str));
	ret = bt_uuid_from_str(str, &uuid_tmp);
	zassert_true(ret == 0, "bt_uuid_from_str failed for 16-bit");
	zassert_true(bt_uuid_cmp(&u1.uuid, &uuid_tmp.uuid) == 0,
		     "Round-trip 16-bit UUID mismatch");
}

/* Roundtrip: 32-bit -> to_str -> from_str -> compare */
ZTEST(bt_uuid_from_str, test_uuid_roundtrip_to_str_and_back_32)
{
	struct bt_uuid_32 u1 = BT_UUID_INIT_32(0xabcdef12U);
	struct bt_uuid_any uuid_tmp = {0};
	char str[BT_UUID_STR_LEN];
	int ret;

	bt_uuid_to_str(&u1.uuid, str, sizeof(str));
	ret = bt_uuid_from_str(str, &uuid_tmp);
	zassert_true(ret == 0, "bt_uuid_from_str failed for 32-bit");
	zassert_true(bt_uuid_cmp(&u1.uuid, &uuid_tmp.uuid) == 0,
		     "Round-trip 32-bit UUID mismatch");
}

/* Roundtrip: 128-bit -> to_str -> from_str -> compare */
ZTEST(bt_uuid_from_str, test_uuid_roundtrip_to_str_and_back_128)
{
	const struct bt_uuid *expected =
		BT_UUID_DECLARE_128(0x9eU, 0xcaU, 0xdcU, 0x24U, 0x0eU, 0xe5U, 0xa9U, 0xe0U,
				    0x93U, 0xf3U, 0xa3U, 0xb5U, 0x01U, 0x00U, 0x40U, 0x6eU);
	struct bt_uuid_any uuid_tmp = {0};
	char str[BT_UUID_STR_LEN];
	int ret;

	bt_uuid_to_str(expected, str, sizeof(str));
	ret = bt_uuid_from_str(str, &uuid_tmp);
	zassert_true(ret == 0, "bt_uuid_from_str failed for 128-bit");
	zassert_true(bt_uuid_cmp(expected, &uuid_tmp.uuid) == 0,
		     "Round-trip 128-bit UUID mismatch");
}

ZTEST(bt_uuid_from_str, test_uuid_from_str_invalid)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("not-a-uuid", &uuid);
	zassert_true(ret < 0, "Invalid UUID string should fail");

	ret = bt_uuid_from_str("", &uuid);
	zassert_true(ret < 0, "Empty string should fail");

	ret = bt_uuid_from_str("123", &uuid);
	zassert_true(ret < 0, "Too short string should fail");

	ret = bt_uuid_from_str("zzzz", &uuid);
	zassert_true(ret < 0, "Non-hex 4-char string should fail");

	ret = bt_uuid_from_str("00001101-0000-1000-8000-00805f9b34fb00", &uuid);
	zassert_true(ret < 0, "Too long 128-bit string should fail");
}

ZTEST(bt_uuid_from_str, test_uuid_from_str_null_params)
{
	struct bt_uuid_any uuid = {0};
	int ret;

	ret = bt_uuid_from_str("180d", NULL);
	zassert_true(ret < 0, "NULL uuid pointer should fail");

	ret = bt_uuid_from_str(NULL, &uuid);
	zassert_true(ret < 0, "NULL string pointer should fail");
}

/* bt_uuid_compress: 128-bit base UUID with 16-bit value -> 16-bit */
ZTEST(bt_uuid_from_str, test_uuid_compress_128_to_16)
{
	const struct bt_uuid *src =
		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(
			0x0000180dU, 0x0000U, 0x1000U, 0x8000U, 0x00805f9b34fbU));
	struct bt_uuid_any dst = {0};
	int ret;

	ret = bt_uuid_compress(src, &dst);
	zassert_equal(ret, 0, "Compress should succeed");
	zassert_equal(dst.uuid.type, BT_UUID_TYPE_16, "Should be 16-bit");
	zassert_equal(BT_UUID_16(&dst.uuid)->val, 0x180dU, "Value mismatch");
}

/* bt_uuid_compress: 128-bit base UUID with 32-bit value -> 32-bit */
ZTEST(bt_uuid_from_str, test_uuid_compress_128_to_32)
{
	const struct bt_uuid *src =
		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(
			0xabcdef12U, 0x0000U, 0x1000U, 0x8000U, 0x00805f9b34fbU));
	struct bt_uuid_any dst = {0};
	int ret;

	ret = bt_uuid_compress(src, &dst);
	zassert_equal(ret, 0, "Compress should succeed");
	zassert_equal(dst.uuid.type, BT_UUID_TYPE_32, "Should be 32-bit");
	zassert_equal(BT_UUID_32(&dst.uuid)->val, 0xabcdef12U, "Value mismatch");
}

/* bt_uuid_compress: non-base 128-bit UUID -> fails with -ENOTSUP */
ZTEST(bt_uuid_from_str, test_uuid_compress_non_base_128)
{
	const struct bt_uuid *src =
		BT_UUID_DECLARE_128(0x9eU, 0xcaU, 0xdcU, 0x24U, 0x0eU, 0xe5U, 0xa9U, 0xe0U,
				    0x93U, 0xf3U, 0xa3U, 0xb5U, 0x01U, 0x00U, 0x40U, 0x6eU);
	struct bt_uuid_any dst = {0};
	int ret;

	ret = bt_uuid_compress(src, &dst);
	zassert_equal(ret, -ENOTSUP, "Non-base UUID should return -ENOTSUP");
}

/* bt_uuid_compress: 16-bit input -> copied as-is */
ZTEST(bt_uuid_from_str, test_uuid_compress_passthrough_16)
{
	struct bt_uuid_16 src = BT_UUID_INIT_16(0x180dU);
	struct bt_uuid_any dst = {0};
	int ret;

	ret = bt_uuid_compress(&src.uuid, &dst);
	zassert_equal(ret, 0, "Should succeed");
	zassert_equal(dst.uuid.type, BT_UUID_TYPE_16, "Should stay 16-bit");
	zassert_equal(BT_UUID_16(&dst.uuid)->val, 0x180dU, "Value mismatch");
}
