/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "common.h"

static void test_stub(void)
{
}

ztest_register_test_suite(run_null_predicate_once, NULL, ztest_unit_test(test_stub));
