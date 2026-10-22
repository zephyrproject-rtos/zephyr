/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "extflash.h"

const char extflash_string[] = EXPECTED_EXTFLASH_STRING;

void extflash_function1(void)
{
	TC_PRINT("Executing %s at %p\n", __func__, &extflash_function1);
}

void extflash_function2(void)
{
	TC_PRINT("Executing %s at %p\n", __func__, &extflash_function2);
}
