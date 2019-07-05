/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/printk_custom_format.h>

static void put(memobj_t *memobj, printk_custom_formatter_t func, u8_t flags,
		u8_t *data, u32_t len)
{
	u32_t offset = 0;

	offset += memobj_write(memobj, (u8_t *)&func,
			       sizeof(printk_custom_formatter_t), 0);
	offset += memobj_write(memobj, (u8_t *)&flags, sizeof(u8_t), offset);
	offset += memobj_write(memobj, data, len, offset);
	__ASSERT_NO_MSG(offset ==
		      (sizeof(printk_custom_formatter_t) + sizeof(u8_t) + len));
}

void *printk_cust_format_dup(u8_t *buf, void *data, u32_t size,
			     printk_custom_formatter_t func)
{
	memobj_t *memobj;

	__ASSERT_NO_MSG(buf);
	memobj = memobjectize(buf, Z_PRINTK_CUST_FORMAT_SIZE(sizeof(void *)));

	put(memobj, func, PRINTK_CUST_FORMAT_PTR, &data, sizeof(data));

	return memobj;
}

static printk_custom_formatter_t func_get(memobj_t *memobj)
{
	printk_custom_formatter_t func;
	u32_t read;

	read = memobj_read(memobj, (u8_t *)&func,
			   sizeof(printk_custom_formatter_t), 0);
	__ASSERT_NO_MSG(read == sizeof(printk_custom_formatter_t));

	return func;
}

static u8_t flags_get(memobj_t *memobj)
{
	u8_t flags;
	u32_t read;

	read = memobj_read(memobj, &flags, sizeof(u8_t),
			   sizeof(printk_custom_formatter_t));
	__ASSERT_NO_MSG(read == sizeof(u8_t));

	return flags;
}

u32_t printk_custom_format_data_get(memobj_t *memobj, u8_t *data,
						u32_t len, u32_t offset)
{
	return memobj_read(memobj, data, len,
			   offset + Z_PRINTK_CUST_FORMAT_OVERHEAD);
}

static void obj_free(memobj_t *memobj)
{
	memobj_free(memobj);
}

void printk_custom_format_handle(memobj_t *memobj,
				 printk_out_func_t out, void *ctx)
{
	printk_custom_formatter_t func;
	u8_t flags;

	func = func_get(memobj);
	flags = flags_get(memobj);

	func(memobj, flags, out, ctx);

	if (flags & PRINTK_CUST_FORMAT_FREE) {
		obj_free(memobj);
	}
}

static void printk_custom_format_string(memobj_t *memobj, u8_t flags,
				 printk_out_func_t out_func, void *ctx)
{
	u32_t offset = 0;
	char c;

	if (flags & PRINTK_CUST_FORMAT_PTR) {
		char *str;

		printk_custom_format_data_get(memobj, (u8_t *)&str,
					    sizeof(str), 0);
		while (*str) {
			out_func(*str++, ctx);
		}
		return;
	}

	while (printk_custom_format_data_get(memobj, (u8_t *)&c,
					   sizeof(c), offset++) > 0) {
		out_func(c, ctx);
	}
}

void *z_printk_custom_format_string_dup(u8_t *buf, char *str)
{

	return printk_cust_format_dup(buf, str, strlen(str),
				      printk_custom_format_string);
}
