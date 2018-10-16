/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include "uki_utlis.h"

int vendor_log_level = LOG_LEVEL_INFO;
int stack_log_level = LOG_LEVEL_INFO;

void uki_hexdump(char *tag, unsigned char *bin, size_t binsz)
{
	char str[500], hex_str[]= "0123456789ABCDEF";
	size_t i;

	for (i = 0; i < binsz; i++) {
		str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
		str[(i * 3) + 1] = hex_str[(bin[i]     ) & 0x0F];
		str[(i * 3) + 2] = ' ';
	}
	str[(binsz * 3) - 1] = 0x00;
	if (tag)
		printk("%s %s\n", tag, str);
	else
		printk("%s\n", str);
}

void uki_hex(char *dst, unsigned char *src, size_t binsz)
{
	static const char hex_str[] = "0123456789abcdef";
	size_t i;

	for (i = 0; i < binsz; i++) {
		dst[(i * 2) + 0] = hex_str[(src[i] >> 4) & 0x0F];
		dst[(i * 2) + 1] = hex_str[(src[i]     ) & 0x0F];
	}
}


void uki_hex_dump_block(char *tag, unsigned char *bin, size_t binsz)
{
	int loop = binsz / HEX_DUMP_BLOCK_SIZE;
	int tail = binsz % HEX_DUMP_BLOCK_SIZE;
	int i;

	if (!loop) {
		uki_hexdump(tag, bin, binsz);
		return;
	}

	for (i = 0; i < loop; i++) {
		uki_hexdump(tag, bin + i * HEX_DUMP_BLOCK_SIZE, HEX_DUMP_BLOCK_SIZE);
	}

	if (tail)
		uki_hexdump(tag, bin + i * HEX_DUMP_BLOCK_SIZE, tail);
}

void uki_str2hex(u8_t *dst, u8_t *src, size_t size)
{
    int len;
    int i, j;
    u8_t c;

    len = strlen(src);
    if (len > size * 2) {
        BTE("Invalid hex string.\n");
        return;
    }

    for (i = 0, j = 0; i < len; i++) {
        c = *src;

        /*  0 - 9 */
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
        else {
            BTE("Invalid hex string.\n");
            return;
        }

        if (!j) {
            dst[i / 2] = c << 4;
            j++;
        } else {
            dst[i / 2] |= c;
            j--;
        }

        src++;
    }
}

void uki_hex_dump_block_ex(unsigned char type, char *tag, unsigned char *bin, size_t binsz)
{
	int loop = binsz / HEX_DUMP_BLOCK_SIZE;
	int tail = binsz % HEX_DUMP_BLOCK_SIZE;
	int i;

	uki_hexdump(tag, &type, 1);

	if (!loop) {
		uki_hexdump(tag, bin, binsz);
		return;
	}

	for (i = 0; i < loop; i++) {
		uki_hexdump(tag, bin + i * HEX_DUMP_BLOCK_SIZE, HEX_DUMP_BLOCK_SIZE);
	}

	if (tail)
		uki_hexdump(tag, bin + i * HEX_DUMP_BLOCK_SIZE, tail);
}

