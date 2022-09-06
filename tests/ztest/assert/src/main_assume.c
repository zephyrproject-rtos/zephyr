/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define SUITE_NAME assumptions

#define API_TYPE(method) zassume_ ## method
#define ZTEST_EXPECT_FAIL_OR_SKIP ZTEST_EXPECT_SKIP

ZTEST_SUITE(assumptions, NULL, NULL, NULL, NULL, NULL);

#include"tests.inc"
