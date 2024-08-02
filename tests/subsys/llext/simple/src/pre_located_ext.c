/*
 * Copyright (c) 2024 Arduino SA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains a simple test extension that defines an entry point
 * in the .text section. The entry point is never called, but the symbol
 * address is fixed via a linker call and then checked by the test.
 */

#include <zephyr/llext/symbol.h>

void test_entry(void)
{
	/* This function is never called */
}
LL_EXTENSION_SYMBOL(test_entry);
