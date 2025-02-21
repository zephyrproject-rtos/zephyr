/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test exporting symbols, imported by other LLEXTs */

#include <zephyr/llext/symbol.h>

long test_dependency(int a, int b)
{
	return (long)a * b;
}
EXPORT_SYMBOL(test_dependency);
