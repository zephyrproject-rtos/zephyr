/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <ctype.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/llext/symbol.h>

#define HEXDUMP_BYTES_IN_LINE 8U

void z_log_minimal_printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}
EXPORT_SYMBOL(z_log_minimal_printk);

void z_log_minimal_vprintk(const char *fmt, va_list ap)
{
	vprintk(fmt, ap);
}

static void minimal_hexdump_line_print(const char *data, size_t length)
{
	for (size_t i = 0U; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i < length) {
			printk("%02x ", (unsigned char)data[i] & 0xFFu);
		} else {
			printk("   ");
		}
	}

	printk("|");

	for (size_t i = 0U; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i < length) {
			unsigned char c = data[i];

			printk("%c", (isprint((int)c) != 0) ? c : '.');
		} else {
			printk(" ");
		}
	}
	printk("\n");
}

void z_log_minimal_hexdump_print(int level, const void *data, size_t size)
{
	const char *data_buffer = (const char *)data;
	while (size > 0U) {
		printk("%c: ", z_log_minimal_level_to_char(level));
		minimal_hexdump_line_print(data_buffer, size);

		if (size < HEXDUMP_BYTES_IN_LINE) {
			break;
		}

		size -= HEXDUMP_BYTES_IN_LINE;
		data_buffer += HEXDUMP_BYTES_IN_LINE;
	}
}
