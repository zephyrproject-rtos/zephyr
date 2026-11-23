/* uuid.c - Bluetooth UUID handling */

/*
 * Copyright (c) 2025 Xiaomi Corporation
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/uuid.h>

#define BT_UUID_STR_LEN_16  4U
#define BT_UUID_STR_LEN_32  8U
#define BT_UUID_STR_LEN_128 (UUID_STR_LEN - 1U)

/*
 * Bluetooth Base UUID (big-endian / RFC 9562):
 *   00000000-0000-1000-8000-00805F9B34FB
 */
static const struct uuid bt_uuid_base = {
	.val = { 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x10U, 0x00U,
		 0x80U, 0x00U, 0x00U, 0x80U, 0x5fU, 0x9bU, 0x34U, 0xfbU }
};

/**
 * @brief Expand a Bluetooth UUID to the generic UUID representation.
 *
 * 16-bit and 32-bit short UUIDs are expanded using the Bluetooth Base UUID.
 * 128-bit UUIDs are byte-swapped from BT wire order (LE) to RFC 9562 order (BE).
 */
static void bt_uuid_to_uuid(const struct bt_uuid *src, struct uuid *dst)
{
	switch (src->type) {
	case BT_UUID_TYPE_16:
		*dst = bt_uuid_base;
		sys_put_be16(BT_UUID_16(src)->val, &dst->val[2]);
		return;
	case BT_UUID_TYPE_32:
		*dst = bt_uuid_base;
		sys_put_be32(BT_UUID_32(src)->val, dst->val);
		return;
	case BT_UUID_TYPE_128:
		sys_memcpy_swap(dst->val, BT_UUID_128(src)->val, UUID_SIZE);
		return;
	default:
		__ASSERT(false, "Invalid UUID type: %d", src->type);
	}
}

/**
 * @brief Offset of the Bluetooth Base UUID tail (bytes [4..15]).
 */
#define BT_UUID_BASE_TAIL_OFFSET 4U

/**
 * @brief Check whether a generic UUID matches the Bluetooth Base UUID.
 *
 * Compares bytes [4..15] against the Bluetooth Base UUID tail.
 *
 * @return true if the UUID matches the Bluetooth Base UUID pattern.
 */
static bool is_bt_base_uuid(const struct uuid *u)
{
	return memcmp(&u->val[BT_UUID_BASE_TAIL_OFFSET],
		      &bt_uuid_base.val[BT_UUID_BASE_TAIL_OFFSET],
		      UUID_SIZE - BT_UUID_BASE_TAIL_OFFSET) == 0;
}

/**
 * @brief Determine the compact Bluetooth UUID type for a base-matching UUID.
 *
 * @pre is_bt_base_uuid(u) must be true.
 *
 * @return BT_UUID_TYPE_16 or BT_UUID_TYPE_32.
 */
static uint8_t bt_base_uuid_type(const struct uuid *u)
{
	if (u->val[0] == 0 && u->val[1] == 0) {
		return BT_UUID_TYPE_16;
	}

	return BT_UUID_TYPE_32;
}

int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	if (u1->type != u2->type) {
		struct uuid uu1, uu2;

		bt_uuid_to_uuid(u1, &uu1);
		bt_uuid_to_uuid(u2, &uu2);
		return memcmp(uu1.val, uu2.val, UUID_SIZE);
	}

	switch (u1->type) {
	case BT_UUID_TYPE_16:
		return (int)BT_UUID_16(u1)->val - (int)BT_UUID_16(u2)->val;
	case BT_UUID_TYPE_32:
		return (int)BT_UUID_32(u1)->val - (int)BT_UUID_32(u2)->val;
	case BT_UUID_TYPE_128:
		return memcmp(BT_UUID_128(u1)->val, BT_UUID_128(u2)->val, 16);
	}

	return -EINVAL;
}

bool bt_uuid_create(struct bt_uuid *uuid, const uint8_t *data, uint8_t data_len)
{
	/* Copy UUID from packet data/internal variable to internal bt_uuid */
	switch (data_len) {
	case BT_UUID_SIZE_16:
		uuid->type = BT_UUID_TYPE_16;
		BT_UUID_16(uuid)->val = sys_get_le16(data);
		break;
	case BT_UUID_SIZE_32:
		uuid->type = BT_UUID_TYPE_32;
		BT_UUID_32(uuid)->val = sys_get_le32(data);
		break;
	case BT_UUID_SIZE_128:
		uuid->type = BT_UUID_TYPE_128;
		memcpy(&BT_UUID_128(uuid)->val, data, 16);
		break;
	default:
		return false;
	}
	return true;
}

void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len)
{
	__maybe_unused int err;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		err = snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
		__ASSERT(err == BT_UUID_STR_LEN_16,
			 "Unexpected snprintk result: %d", err);
		break;
	case BT_UUID_TYPE_32:
		err = snprintk(str, len, "%08x", BT_UUID_32(uuid)->val);
		__ASSERT(err == BT_UUID_STR_LEN_32,
			 "Unexpected snprintk result: %d", err);
		break;
	case BT_UUID_TYPE_128: {
		struct uuid gen_uuid;
		char full[UUID_STR_LEN];

		bt_uuid_to_uuid(uuid, &gen_uuid);
		if (uuid_to_string(&gen_uuid, full) != 0) {
			(void)memset(str, 0, len);
			return;
		}

		err = snprintk(str, len, "%s", full);
		__ASSERT(err == BT_UUID_STR_LEN_128,
			 "Unexpected snprintk result: %d", err);
		break;
	}
	default:
		(void)memset(str, 0, len);
		return;
	}
}

int bt_uuid_from_str(const char *str, struct bt_uuid_any *uuid)
{
	struct uuid gen_uuid;

	if (str == NULL || uuid == NULL) {
		return -EINVAL;
	}

	switch (strlen(str)) {
	case BT_UUID_STR_LEN_16: {
		char full[UUID_STR_LEN];

		if (uuid_to_string(&bt_uuid_base, full) != 0) {
			return -EINVAL;
		}

		(void)memcpy(&full[4], str, BT_UUID_STR_LEN_16);

		if (uuid_from_string(full, &gen_uuid) != 0) {
			return -EINVAL;
		}

		/* Enforce 16-bit UUID type for 4-hex-digit input strings. */
		uuid->uuid.type = BT_UUID_TYPE_16;
		BT_UUID_16(&uuid->uuid)->val = sys_get_be16(&gen_uuid.val[2]);
		return 0;
	}
	case BT_UUID_STR_LEN_32: {
		char full[UUID_STR_LEN];

		if (uuid_to_string(&bt_uuid_base, full) != 0) {
			return -EINVAL;
		}

		(void)memcpy(full, str, BT_UUID_STR_LEN_32);
		if (uuid_from_string(full, &gen_uuid) != 0) {
			return -EINVAL;
		}

		/* Enforce 32-bit UUID type for 8-hex-digit input strings. */
		uuid->uuid.type = BT_UUID_TYPE_32;
		BT_UUID_32(&uuid->uuid)->val = sys_get_be32(gen_uuid.val);
		return 0;
	}
	case BT_UUID_STR_LEN_128: /* full string -> always 128-bit */
		if (uuid_from_string(str, &gen_uuid) != 0) {
			return -EINVAL;
		}

		uuid->uuid.type = BT_UUID_TYPE_128;
		sys_memcpy_swap(BT_UUID_128(&uuid->uuid)->val, gen_uuid.val, UUID_SIZE);
		return 0;
	default:
		return -EINVAL;
	}
}

int bt_uuid_compress(const struct bt_uuid *src, struct bt_uuid_any *dst)
{
	struct uuid u;

	if (src == NULL || dst == NULL) {
		return -EINVAL;
	}

	switch (src->type) {
	case BT_UUID_TYPE_16:
		dst->uuid.type = BT_UUID_TYPE_16;
		BT_UUID_16(&dst->uuid)->val = BT_UUID_16(src)->val;
		return 0;
	case BT_UUID_TYPE_32:
		dst->uuid.type = BT_UUID_TYPE_32;
		BT_UUID_32(&dst->uuid)->val = BT_UUID_32(src)->val;
		return 0;
	case BT_UUID_TYPE_128:
		bt_uuid_to_uuid(src, &u);

		if (!is_bt_base_uuid(&u)) {
			return -ENOTSUP;
		}

		dst->uuid.type = bt_base_uuid_type(&u);

		if (dst->uuid.type == BT_UUID_TYPE_16) {
			BT_UUID_16(&dst->uuid)->val = sys_get_be16(&u.val[2]);
		} else {
			BT_UUID_32(&dst->uuid)->val = sys_get_be32(u.val);
		}

		return 0;
	default:
		return -EINVAL;
	}
}
