/*
 * Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

void main(void)
{
	printk("d%" PRId8 "\n", INT8_C(8));
	printk("d%" PRId16 "\n", INT16_C(16));
	printk("d%" PRId32 "\n", INT32_C(32));
	printk("d%" PRId64 "\n", INT64_C(64));

	printk("i%" PRIi8 "\n", INT8_C(8));
	printk("i%" PRIi16 "\n", INT16_C(16));
	printk("i%" PRIi32 "\n", INT32_C(32));
	printk("i%" PRIi64 "\n", INT64_C(64));

	printk("o%" PRIo8 "\n", INT8_C(8));
	printk("o%" PRIo16 "\n", INT16_C(16));
	printk("o%" PRIo32 "\n", INT32_C(32));
	printk("o%" PRIo64 "\n", INT64_C(64));

	printk("u%" PRIu8 "\n", UINT8_C(8));
	printk("u%" PRIu16 "\n", UINT16_C(16));
	printk("u%" PRIu32 "\n", UINT32_C(32));
	printk("u%" PRIu64 "\n", UINT64_C(64));

	printk("x%" PRIx8 "\n", UINT8_C(8));
	printk("x%" PRIx16 "\n", UINT16_C(16));
	printk("x%" PRIx32 "\n", UINT32_C(32));
	printk("x%" PRIx64 "\n", UINT64_C(64));

	printk("X%" PRIX8 "\n", UINT8_C(8));
	printk("X%" PRIX16 "\n", UINT16_C(16));
	printk("X%" PRIX32 "\n", UINT32_C(32));
	printk("X%" PRIX64 "\n", UINT64_C(64));
}
