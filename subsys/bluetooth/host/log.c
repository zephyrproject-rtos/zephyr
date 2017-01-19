/* log.c - logging helpers */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */

#include <stddef.h>
#include <stdint.h>
#include <zephyr.h>
#include <misc/util.h>

const char *bt_hex(const void *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	static char hexbufs[4][129];
	static uint8_t curbuf;
	const uint8_t *b = buf;
	unsigned int mask;
	char *str;
	int i;

	mask = irq_lock();
	str = hexbufs[curbuf++];
	curbuf %= ARRAY_SIZE(hexbufs);
	irq_unlock(mask);

	len = min(len, (sizeof(hexbufs[0]) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
}
