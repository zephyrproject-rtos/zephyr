/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <string.h>

#include "test.inc"

void test_cxx(void);
void test_cc(void);

void test_main(void)
{
	test_cc();
	test_cxx();
}
