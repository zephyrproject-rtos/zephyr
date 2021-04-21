/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/util.h>
#include <string.h>

#include "test.inc"

void test_cxx(void);
void test_cc(void);

void test_main(void)
{
	test_cc();
	test_cxx();
}
