/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/crypto.h>

#define ADDR_RESOLVED_BITMASK (0x02)

static inline int create_random_addr(bt_addr_le_t *addr)
{
	addr->type = BT_ADDR_LE_RANDOM;

	return bt_rand(addr->a.val, 6);
}

int bt_addr_le_create_nrpa(bt_addr_le_t *addr)
{
	int err;

	err = create_random_addr(addr);
	if (err) {
		return err;
	}

	BT_ADDR_SET_NRPA(&addr->a);

	return 0;
}

int bt_addr_le_create_static(bt_addr_le_t *addr)
{
	int err;

	err = create_random_addr(addr);
	if (err) {
		return err;
	}

	BT_ADDR_SET_STATIC(&addr->a);

	return 0;
}

int bt_addr_from_str(const char *str, bt_addr_t *addr)
{
	/* Parse a null-terminated string with a Bluetooth address in
	 * canonical "XX:XX:XX:XX:XX:XX" format.
	 */

	const size_t len = strlen(str);

	/* Verify length. */
	if (len != BT_ADDR_STR_LEN - 1) {
		return -EINVAL;
	}

	/* Verify that all the colons are present. */
	for (size_t i = 2; i < len; i += 3) {
		if (str[i] != ':') {
			return -EINVAL;
		}
	}

	/* Parse each octet as hex and populate `addr->val`. It must be
	 * reversed since `bt_addr_t` is in 'on-air' format.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(addr->val); i++) {
		const size_t reverse_i = ARRAY_SIZE(addr->val) - 1 - i;

		if (!hex2bin(&str[i * 3], 2, &addr->val[reverse_i], 1)) {
			return -EINVAL;
		}
	}

	return 0;
}

int bt_addr_le_from_str(const char *str, const char *type, bt_addr_le_t *addr)
{
	int err;

	err = bt_addr_from_str(str, &addr->a);
	if (err < 0) {
		return err;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else if (!strcmp(type, "public-id") || !strcmp(type, "(public-id)")) {
		addr->type = BT_ADDR_LE_PUBLIC_ID;
	} else if (!strcmp(type, "random-id") || !strcmp(type, "(random-id)")) {
		addr->type = BT_ADDR_LE_RANDOM_ID;
	} else {
		return -EINVAL;
	}

	return 0;
}

void bt_addr_le_copy_resolved(bt_addr_le_t *dst, const bt_addr_le_t *src)
{
	bt_addr_le_copy(dst, src);
	/* translate to "regular" address type */
	dst->type &= ~ADDR_RESOLVED_BITMASK;
}

bool bt_addr_le_is_resolved(const bt_addr_le_t *addr)
{
	return (addr->type & ADDR_RESOLVED_BITMASK) != 0;
}
