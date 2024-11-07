/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2024, Marvin Ouma <pancakesdeath@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern void before(void *arg);
extern void after(void *arg);

ZTEST_SUITE(posix_threads_base, NULL, NULL, before, after, NULL);
