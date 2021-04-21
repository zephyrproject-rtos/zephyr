/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/uuid.h>

static struct bt_uuid_16 uuid_16 = BT_UUID_INIT_16(0xffff);

static struct bt_uuid_128 uuid_128 = BT_UUID_INIT_128(
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00);

static struct bt_uuid_128 le_128 = BT_UUID_INIT_128(
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00);

static void test_uuid_cmp(void)
{
	/* Compare UUID 16 bits */
	zassert_false(bt_uuid_cmp(&uuid_16.uuid, BT_UUID_DECLARE_16(0xffff)),
		     "Test UUIDs don't match");

	/* Compare UUID 128 bits */
	zassert_false(bt_uuid_cmp(&uuid_128.uuid, BT_UUID_DECLARE_16(0xffff)),
		     "Test UUIDs don't match");

	/* Compare UUID 16 bits with UUID 128 bits */
	zassert_false(bt_uuid_cmp(&uuid_16.uuid, &uuid_128.uuid),
		     "Test UUIDs don't match");

	/* Compare different UUID 16 bits */
	zassert_true(bt_uuid_cmp(&uuid_16.uuid, BT_UUID_DECLARE_16(0x0000)),
		     "Test UUIDs match");

	/* Compare different UUID 128 bits */
	zassert_true(bt_uuid_cmp(&uuid_128.uuid, BT_UUID_DECLARE_16(0x000)),
		     "Test UUIDs match");
}

static void test_uuid_create(void)
{
	uint8_t le16[] = { 0x01, 0x00 };
	uint8_t be16[] = { 0x00, 0x01 };
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	/* Create UUID from LE 16 bit byte array */
	zassert_true(bt_uuid_create(&u.uuid, le16, sizeof(le16)),
		     "Unable create UUID");

	/* Compare UUID 16 bits */
	zassert_true(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0001)) == 0,
		     "Test UUIDs don't match");

	/* Compare UUID 128 bits */
	zassert_true(bt_uuid_cmp(&u.uuid, &le_128.uuid) == 0,
		     "Test UUIDs don't match");

	/* Compare swapped UUID 16 bits */
	zassert_false(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0100)) == 0,
		     "Test UUIDs match");

	/* Create UUID from BE 16 bit byte array */
	zassert_true(bt_uuid_create(&u.uuid, be16, sizeof(be16)),
		     "Unable create UUID");

	/* Compare UUID 16 bits */
	zassert_false(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0001)) == 0,
		     "Test UUIDs match");

	/* Compare UUID 128 bits */
	zassert_false(bt_uuid_cmp(&u.uuid, &le_128.uuid) == 0,
		     "Test UUIDs match");

	/* Compare swapped UUID 16 bits */
	zassert_true(bt_uuid_cmp(&u.uuid, BT_UUID_DECLARE_16(0x0100)) == 0,
		     "Test UUIDs don't match");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_uuid,
			 ztest_unit_test(test_uuid_cmp),
			 ztest_unit_test(test_uuid_create));
	ztest_run_test_suite(test_uuid);
}
