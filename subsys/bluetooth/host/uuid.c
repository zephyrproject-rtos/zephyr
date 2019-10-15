/* uuid.c - Bluetooth UUID handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/uuid.h>

#define UUID_16_BASE_OFFSET 12

/* TODO: Decide whether to continue using BLE format or switch to RFC 4122 */

/* Base UUID : 0000[0000]-0000-1000-8000-00805F9B34FB
 * 0x2800    : 0000[2800]-0000-1000-8000-00805F9B34FB
 *  little endian 0x2800 : [00 28] -> no swapping required
 *  big endian 0x2800    : [28 00] -> swapping required
 */
static const struct bt_uuid_128 uuid128_base = {
	.uuid.type = BT_UUID_TYPE_128,
	.val = { BT_UUID_128_ENCODE(
		0x00000000, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB) }
};

static void uuid_to_uuid128(const struct bt_uuid *src, struct bt_uuid_128 *dst)
{
	switch (src->type) {
	case BT_UUID_TYPE_16:
		*dst = uuid128_base;
		sys_put_le16(BT_UUID_16(src)->val,
			     &dst->val[UUID_16_BASE_OFFSET]);
		return;
	case BT_UUID_TYPE_32:
		*dst = uuid128_base;
		sys_put_le32(BT_UUID_32(src)->val,
			     &dst->val[UUID_16_BASE_OFFSET]);
		return;
	case BT_UUID_TYPE_128:
		memcpy(dst, src, sizeof(*dst));
		return;
	}
}

static int uuid128_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	struct bt_uuid_128 uuid1, uuid2;

	uuid_to_uuid128(u1, &uuid1);
	uuid_to_uuid128(u2, &uuid2);

	return memcmp(uuid1.val, uuid2.val, 16);
}

int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	/* Convert to 128 bit if types don't match */
	if (u1->type != u2->type) {
		return uuid128_cmp(u1, u2);
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

bool bt_uuid_create(struct bt_uuid *uuid, const u8_t *data, u8_t data_len)
{
	/* Copy UUID from packet data/internal variable to internal bt_uuid */
	switch (data_len) {
	case 2:
		uuid->type = BT_UUID_TYPE_16;
		BT_UUID_16(uuid)->val = sys_get_le16(data);
		break;
	case 4:
		uuid->type = BT_UUID_TYPE_32;
		BT_UUID_32(uuid)->val = sys_get_le32(data);
		break;
	case 16:
		uuid->type = BT_UUID_TYPE_128;
		memcpy(&BT_UUID_128(uuid)->val, data, 16);
		break;
	default:
		return false;
	}
	return true;
}

#if defined(CONFIG_BT_DEBUG)
void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len)
{
	u32_t tmp1, tmp5;
	u16_t tmp0, tmp2, tmp3, tmp4;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		snprintk(str, len, "%04x", BT_UUID_32(uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		memcpy(&tmp0, &BT_UUID_128(uuid)->val[0], sizeof(tmp0));
		memcpy(&tmp1, &BT_UUID_128(uuid)->val[2], sizeof(tmp1));
		memcpy(&tmp2, &BT_UUID_128(uuid)->val[6], sizeof(tmp2));
		memcpy(&tmp3, &BT_UUID_128(uuid)->val[8], sizeof(tmp3));
		memcpy(&tmp4, &BT_UUID_128(uuid)->val[10], sizeof(tmp4));
		memcpy(&tmp5, &BT_UUID_128(uuid)->val[12], sizeof(tmp5));

		snprintk(str, len, "%08x-%04x-%04x-%04x-%08x%04x",
			 tmp5, tmp4, tmp3, tmp2, tmp1, tmp0);
		break;
	default:
		(void)memset(str, 0, len);
		return;
	}
}

const char *bt_uuid_str_real(const struct bt_uuid *uuid)
{
	static char str[37];

	bt_uuid_to_str(uuid, str, sizeof(str));

	return str;
}
#endif /* CONFIG_BT_DEBUG */
