/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "_common.h"
#ifdef CONFIG_POSIX_API
#include <stdio.h>
#else
#include <zephyr/posix/stdio.h>
#endif

/**
 * @brief Test existence and basic functionality of stdio.h
 * 
 * @see stdio.h
*/
ZTEST(posix_headers, test_stdio_h)
{
    #ifdef CONFIG_POSIX_API
    zassert_not_null((void *)getchar, "getchar is null");
    zassert_not_null((void *)getc, "getc is null");
    zassert_not_null((void *)fgetc, "fgetc is null");
    #endif
}