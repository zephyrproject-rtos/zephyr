/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

ZTEST(stdio, test_fgetc_empty_stream)
{
    char test_char = EOF;
    int expected_return = 0;
    int retErr = ferror(stdin);
    int ret = fgetc(stdin);
    printf("ERRNO: %d\nretError: %d", errno, retErr);

    zassert_equal(ret, test_char, "Expected return value %d, got %d", test_char, ret);
    zassert_not_equal(feof(stdin), expected_return);
}

ZTEST(stdio, test_fgetc_one_char)
{

    char test_char = 'A';
    int expected_return = 0;
    clearerr(stdin);
    fputc(test_char, stdin);
    printf("ERRNO: %d\n", errno);
    fflush(stdin);
    printf("ERRNO: %d\n", errno);
    int ret = fgetc(stdin);
    printf("ERRNO: %d\n", errno);


    zassert_equal(ret, test_char, "Expected return value %d, got %d", test_char, ret);
    zassert_not_equal(feof(stdin), expected_return);


}

ZTEST_SUITE(stdio, NULL, NULL, NULL, NULL, NULL);