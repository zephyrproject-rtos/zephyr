/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void test_support_q7(void);
extern void test_support_q15(void);
extern void test_support_q31(void);
extern void test_support_f32(void);
extern void test_support_barycenter_f32(void);

void test_main(void)
{
	test_support_q7();
	test_support_q15();
	test_support_q31();
	test_support_f32();
	test_support_barycenter_f32();
}
