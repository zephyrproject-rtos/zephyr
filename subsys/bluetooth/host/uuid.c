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
	}
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
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		snprintk(str, len, "%08x", BT_UUID_32(uuid)->val);
		break;
	case BT_UUID_TYPE_128: {
		struct uuid gen_uuid;
		char full[UUID_STR_LEN];

		bt_uuid_to_uuid(uuid, &gen_uuid);
		if (uuid_to_string(&gen_uuid, full) != 0) {
			(void)memset(str, 0, len);
			return;
		}

		snprintk(str, len, "%s", full);
		break;
	}
	default:
		(void)memset(str, 0, len);
		return;
	}
}
