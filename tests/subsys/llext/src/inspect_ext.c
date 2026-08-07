/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This extension exports a number of symbols to be used by the LLEXT
 * inspection APIs. Each symbol is exported within a different section
 * and only the addresses are checked in this test.
 */

#include <zephyr/llext/symbol.h>

int number_in_bss;
int number_in_data = 1;
const int number_in_rodata = 2;
const int number_in_my_rodata Z_GENERIC_SECTION(.my_rodata) = 3;

void function_in_text(void)
{
	/* only used for address check */
}

EXPORT_SYMBOL(number_in_bss);
EXPORT_SYMBOL(number_in_data);
EXPORT_SYMBOL(number_in_rodata);
EXPORT_SYMBOL(number_in_my_rodata);
EXPORT_SYMBOL(function_in_text);
