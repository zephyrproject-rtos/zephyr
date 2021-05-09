/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <ctype.h>
#include <logging/log.h>
#include <sys/printk.h>

#define HEXDUMP_BYTES_IN_LINE 8

void z_log_minimal_printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

void z_log_minimal_vprintk(const char *fmt, va_list ap)
{
	vprintk(fmt, ap);
}

static void minimal_hexdump_line_print(const char *data, size_t length)
{
	for (size_t i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i < length) {
			printk("%02x ", data[i] & 0xFF);
		} else {
			printk("   ");
		}
	}

	printk("|");

	for (size_t i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i < length) {
			unsigned char c = data[i];

			printk("%c", isprint((int)c) ? c : '.');
		} else {
			printk(" ");
		}
	}
	printk("\n");
}

void z_log_minimal_hexdump_print(int level, const void *_data, size_t size)
{
	const char *data = (const char *)_data;
	while (size > 0) {
		printk("%c: ", z_log_minimal_level_to_char(level));
		minimal_hexdump_line_print(data, size);

		if (size < HEXDUMP_BYTES_IN_LINE) {
			break;
		}

		size -= HEXDUMP_BYTES_IN_LINE;
		data += HEXDUMP_BYTES_IN_LINE;
	}
}
