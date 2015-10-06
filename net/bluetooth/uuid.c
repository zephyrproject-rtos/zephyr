/* uuid.c - Bluetooth UUID handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>

#include <bluetooth/uuid.h>

/* TODO: Decide wether to continue using BLE format or switch to RFC 4122 */
static const struct bt_uuid uuid128_base = {
	.type = BT_UUID_128,
	.u128 = { 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

};

static void uuid_to_uuid128(const struct bt_uuid *src, struct bt_uuid *dst)
{
	switch (src->type) {
	case BT_UUID_16:
		*dst = uuid128_base;
		memcpy(&dst->u128[2], &src->u16, sizeof(src->u16));
		return;
	case BT_UUID_128:
		memcpy(dst, src, sizeof(*dst));
		return;
	}
}

static int uuid128_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	struct bt_uuid uuid1, uuid2;

	uuid_to_uuid128(u1, &uuid1);
	uuid_to_uuid128(u2, &uuid2);

	return memcmp(uuid1.u128, uuid2.u128, sizeof(uuid1.u128));
}

int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2)
{
	/* Convert to 128 bit if types don't match */
	if (u1->type != u2->type)
		return uuid128_cmp(u1, u2);

	switch (u1->type) {
	case BT_UUID_16:
		return (int)u1->u16 - (int)u2->u16;
	case BT_UUID_128:
		return memcmp(u1->u128, u2->u128, sizeof(u1->u128));
	}

	return -EINVAL;
}
