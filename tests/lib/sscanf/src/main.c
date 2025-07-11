/*
 * Copyright (c) 2025 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESCRIPTION
 * This module contains the code for testing input functionality in minimal libc,
 * including getc(), fgetc(), fgets(), and getchar().
 */

#include <zephyr/ztest.h>
#include <stdio.h>
#include <string.h>

static const char *test_input = "Hello\nWorld";
static int input_pos;

static int mock_stdin_hook(void)
{
    if (test_input[input_pos] == '\0') {
        return EOF;
    }
    return test_input[input_pos++];
}

void setup_stdin_hook(void)
{
    input_pos = 0;
    extern void __stdin_hook_install(int (*hook)(void));
    __stdin_hook_install(mock_stdin_hook);
}

ZTEST(sscanf, test_getc)
{
    setup_stdin_hook();
    int c = getc(stdin);
    zassert_equal(c, 'H', "getc(stdin) did not return 'H'");
    c = getc(stdin);
    zassert_equal(c, 'e', "getc(stdin) did not return 'e'");
}

ZTEST(sscanf, test_fgetc)
{
    setup_stdin_hook();
    int c = fgetc(stdin);
    zassert_equal(c, 'H', "fgetc(stdin) did not return 'H'");
    c = fgetc(stdin);
    zassert_equal(c, 'e', "fgetc(stdin) did not return 'e'");
}

ZTEST(sscanf, test_getchar)
{
    setup_stdin_hook();
    int c = getchar();
    zassert_equal(c, 'H', "getchar() did not return 'H'");
    c = getchar();
    zassert_equal(c, 'e', "getchar() did not return 'e'");
}

ZTEST(sscanf, test_fgets)
{
    setup_stdin_hook();
    char buf[16];
    char *ret = fgets(buf, sizeof(buf), stdin);
    zassert_not_null(ret, "fgets returned NULL");
    zassert_true(strcmp(buf, "Hello\n") == 0, "fgets did not read 'Hello\\n'");
    ret = fgets(buf, sizeof(buf), stdin);
    zassert_not_null(ret, "fgets returned NULL on second call");
    zassert_true(strcmp(buf, "World") == 0, "fgets did not read 'World'");
}

ZTEST_SUITE(sscanf, NULL, NULL, NULL, NULL, NULL);