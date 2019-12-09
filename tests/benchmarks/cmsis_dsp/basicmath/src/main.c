/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void benchmark_basicmath_q7(void);
extern void benchmark_basicmath_q15(void);
extern void benchmark_basicmath_q31(void);
extern void benchmark_basicmath_f32(void);

void test_main(void)
{
	benchmark_basicmath_q7();
	benchmark_basicmath_q15();
	benchmark_basicmath_q31();
	benchmark_basicmath_f32();
}
