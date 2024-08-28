/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This very basic extension only contains a global variable "number", which is
 * expected to be mapped at offset 0 in the .data section. This can be used in
 * CONFIG_LLEXT_STORAGE_WRITABLE-enabled builds to verify that the variable
 * address, as calculated by the llext loader, matches the start of the .data
 * section in the ELF buffer as returned by llext_find_section.
 *
 * If only this test fails, the llext_find_section API is likely broken.
 */

#include <zephyr/llext/symbol.h>

int number = 42;

void test_entry(void)
{
	/* unused */
}

EXPORT_SYMBOL(number);
