/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define SUITE_NAME asserts

#define API_TYPE(method) zassert_ ## method
#define ZTEST_EXPECT_FAIL_OR_SKIP ZTEST_EXPECT_FAIL

ZTEST_SUITE(asserts, NULL, NULL, NULL, NULL, NULL);

#include"tests.inc"
