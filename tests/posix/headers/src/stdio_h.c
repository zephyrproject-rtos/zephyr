/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "_common.h"
#include <stdio.h>

/**
 * @brief Test existence and basic functionality of stdio.h
 * 
 * @see stdio.h
*/
ZTEST(posix_headers, test_stdio_h)
{
    zassert_not_null((void *)getchar, "getchar is null");
    zassert_not_null((void *)getc, "getc is null");
    zassert_not_null((void *)fgetc, "fgetc is null");
}