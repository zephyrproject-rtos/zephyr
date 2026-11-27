/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define OBJ_SIZE  16U
#define OBJ_PROPS 0x04U

/* Flags with the Current Size (bit 1) and Properties (bit 5) fields present, 16-bit type */
#define RECORD_FLAGS (BIT(1) | BIT(5))

/* Record Length, Object ID, Name Length, Name, Flags, 16-bit Object Type, Current Size and
 * Properties. The Record Length includes the Record Length field itself.
 */
#define RECORD_LEN(name_len) (2U + 6U + 1U + (name_len) + 1U + 2U + 4U + 4U)

#define RECORD(id, name_len, ...)                                                                  \
	RECORD_LEN(name_len) & 0xFFU, RECORD_LEN(name_len) >> 8, (id), 0x00, 0x00, 0x00, 0x00,     \
		0x00, (name_len), __VA_ARGS__, RECORD_FLAGS,                                       \
		BT_UUID_OTS_TYPE_UNSPECIFIED_VAL & 0xFFU, BT_UUID_OTS_TYPE_UNSPECIFIED_VAL >> 8,   \
		OBJ_SIZE, 0x00, 0x00, 0x00, OBJ_PROPS, 0x00, 0x00, 0x00

static struct bt_ots_obj_metadata decoded[2];
static size_t decoded_count;

static int dirlisting_cb(struct bt_ots_obj_metadata *meta)
{
	if (decoded_count < ARRAY_SIZE(decoded)) {
		decoded[decoded_count] = *meta;
	}

	decoded_count++;

	return BT_OTS_CONTINUE;
}

static void reset(void *fixture)
{
	ARG_UNUSED(fixture);

	decoded_count = 0;
	memset(decoded, 0, sizeof(decoded));
}

ZTEST_SUITE(ots_client_dir_list, NULL, NULL, reset, NULL, NULL);

/* The last record ends exactly at the end of the listing */
ZTEST(ots_client_dir_list, test_single_record)
{
	uint8_t listing[] = {RECORD(0x01, 3, 'a', 'b', 'c')};
	int count;

	count = bt_ots_client_decode_dirlisting(listing, sizeof(listing), dirlisting_cb);

	zassert_equal(count, 1, "decode returned %d", count);
	zassert_equal(decoded_count, 1);
	zassert_equal(decoded[0].id, 0x01);
	zassert_str_equal(decoded[0].name_c, "abc");
	zassert_equal(decoded[0].type.uuid_16.val, BT_UUID_OTS_TYPE_UNSPECIFIED_VAL);
	zassert_equal(decoded[0].size.cur, OBJ_SIZE);
	zassert_equal(decoded[0].props, OBJ_PROPS);
}

ZTEST(ots_client_dir_list, test_two_records)
{
	uint8_t listing[] = {RECORD(0x01, 1, 'a'), RECORD(0x02, 2, 'b', 'c')};
	int count;

	count = bt_ots_client_decode_dirlisting(listing, sizeof(listing), dirlisting_cb);

	zassert_equal(count, 2, "decode returned %d", count);
	zassert_equal(decoded_count, 2);
	zassert_str_equal(decoded[0].name_c, "a");
	zassert_equal(decoded[1].id, 0x02);
	zassert_str_equal(decoded[1].name_c, "bc");
}

/* The octets of a record after the fields it defines, such as an Extension field from a later
 * version of the service, are skipped to reach the next record
 */
ZTEST(ots_client_dir_list, test_record_extension)
{
	uint8_t ext_record[] = {RECORD(0x01, 1, 'a'), 0xAA, 0xBB, 0xCC};
	uint8_t next_record[] = {RECORD(0x02, 2, 'b', 'c')};
	uint8_t listing[sizeof(ext_record) + sizeof(next_record)];
	int count;

	/* The Record Length counts the Extension field */
	sys_put_le16(sizeof(ext_record), ext_record);
	memcpy(listing, ext_record, sizeof(ext_record));
	memcpy(&listing[sizeof(ext_record)], next_record, sizeof(next_record));

	count = bt_ots_client_decode_dirlisting(listing, sizeof(listing), dirlisting_cb);

	zassert_equal(count, 2, "decode returned %d", count);
	zassert_equal(decoded_count, 2);
	zassert_equal(decoded[0].id, 0x01);
	zassert_str_equal(decoded[0].name_c, "a");
	zassert_equal(decoded[0].props, OBJ_PROPS);
	zassert_equal(decoded[1].id, 0x02);
	zassert_str_equal(decoded[1].name_c, "bc");
	zassert_equal(decoded[1].size.cur, OBJ_SIZE);
	zassert_equal(decoded[1].props, OBJ_PROPS);
}

/* A record that claims more octets than the listing holds is still rejected */
ZTEST(ots_client_dir_list, test_truncated_record)
{
	uint8_t listing[] = {RECORD(0x01, 3, 'a', 'b', 'c')};
	int count;

	count = bt_ots_client_decode_dirlisting(listing, sizeof(listing) - 1, dirlisting_cb);

	zassert_equal(count, -EINVAL, "decode returned %d", count);
	zassert_equal(decoded_count, 0);
}
