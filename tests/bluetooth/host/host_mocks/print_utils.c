/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "host_mocks/print_utils.h"

static int bt_addr_le_to_str_mod(const bt_addr_le_t *addr, char *str,
					size_t len)
{
	char type[10];

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		strcpy(type, "public");
		break;
	case BT_ADDR_LE_RANDOM:
		strcpy(type, "random");
		break;
	case BT_ADDR_LE_PUBLIC_ID:
		strcpy(type, "public-id");
		break;
	case BT_ADDR_LE_RANDOM_ID:
		strcpy(type, "random-id");
		break;
	default:
		snprintf(type, sizeof(type), "0x%02x", addr->type);
		break;
	}

	return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X (%s)",
			addr->a.val[5], addr->a.val[4], addr->a.val[3],
			addr->a.val[2], addr->a.val[1], addr->a.val[0], type);
}

static int bt_addr_to_str_mod(const bt_addr_t *addr, char *str, size_t len)
{
	return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
			addr->val[5], addr->val[4], addr->val[3],
			addr->val[2], addr->val[1], addr->val[0]);
}

const char *bt_addr_str_real(const bt_addr_t *addr)
{
	static char str[BT_ADDR_STR_LEN];

	bt_addr_to_str_mod(addr, str, sizeof(str));

	return str;
}

const char *bt_addr_le_str_real(const bt_addr_le_t *addr)
{
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str_mod(addr, str, sizeof(str));

	return str;
}

const char *bt_hex_real(const void *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	static char str[129];
	const uint8_t *b = buf;
	size_t i;

	len = MIN(len, (sizeof(str) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
}

void z_log_minimal_printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}
