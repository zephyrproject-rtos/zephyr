/* Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <c_test_framework.h>

void test_main(void)
{
	fff_test_suite();
}
