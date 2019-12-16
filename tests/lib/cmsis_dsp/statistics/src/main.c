/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void test_statistics_q7(void);
extern void test_statistics_q15(void);
extern void test_statistics_q31(void);
extern void test_statistics_f32(void);
extern void test_statistics_f64(void);

void test_main(void)
{
	test_statistics_q7();
	test_statistics_q15();
	test_statistics_q31();
	test_statistics_f32();
	test_statistics_f64();
}
