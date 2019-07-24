/* log.c - logging helpers */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr.h>
#include <sys/util.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

const char *bt_hex_real(const void *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	static char str[129];
	const u8_t *b = buf;
	int i;

	len = MIN(len, (sizeof(str) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
}

const char *bt_addr_str_real(const bt_addr_t *addr)
{
	static char str[BT_ADDR_STR_LEN];

	bt_addr_to_str(addr, str, sizeof(str));

	return str;
}

const char *bt_addr_le_str_real(const bt_addr_le_t *addr)
{
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	return str;
}

static void format_bt_addr_le_t(memobj_t *memobj, u8_t flags,
				printk_out_func_t out_func, void *ctx)
{
	bt_addr_le_t addr_buf;
	bt_addr_le_t *addr;
	const char *type = NULL;

	if (flags & PRINTK_CUST_FORMAT_PTR) {
		printk_custom_format_data_get(memobj, (u8_t *)&addr,
					    sizeof(addr), 0);
	} else {
		printk_custom_format_data_get(memobj, (u8_t *)&addr_buf,
					    sizeof(addr_buf), 0);
		addr = &addr_buf;
	}

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		type = "public";
		break;
	case BT_ADDR_LE_RANDOM:
		type = "random";
		break;
	case BT_ADDR_LE_PUBLIC_ID:
		type = "public id";
		break;
	case BT_ADDR_LE_RANDOM_ID:
		type = "random id";
		break;
	}

	while (*type) {
		out_func(*type++, ctx);
	}

	out_func(' ', ctx);
	for (int i = 0; i < 6; i++) {
		z_printk_hex_ulong(out_func, ctx, addr->a.val[i],
				PRINTK_PAD_ZERO_BEFORE, 2);
		if (i != 5) {
			out_func(':', ctx);
		}
	}
}

void *z_bt_addr_le_t_dup(u8_t *buf, const bt_addr_le_t *addr)
{
	return printk_cust_format_dup(buf, (void *)addr, sizeof(bt_addr_le_t),
				      format_bt_addr_le_t);
}
