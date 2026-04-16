/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2024, Marvin Ouma <pancakesdeath@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

void test_env_before(void);
void test_env_after(void);

static void before(void *arg)
{
	test_env_before();
}

static void after(void *arg)
{
	test_env_after();
}

ZTEST_SUITE(posix_single_process, NULL, NULL, before, after, NULL);
