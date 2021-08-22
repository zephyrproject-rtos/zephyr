/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void test_interpolation_q7(void);
extern void test_interpolation_q15(void);
extern void test_interpolation_q31(void);

void test_main(void)
{
	test_interpolation_q7();
	test_interpolation_q15();
	test_interpolation_q31();
}
