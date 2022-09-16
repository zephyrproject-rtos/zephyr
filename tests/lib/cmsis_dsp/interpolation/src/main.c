/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

extern void test_interpolation_q7(void);
extern void test_interpolation_q15(void);
extern void test_interpolation_q31(void);
extern void test_interpolation_f16(void);
extern void test_interpolation_f32(void);

void test_main(void)
{
	test_interpolation_q7();
	test_interpolation_q15();
	test_interpolation_q31();
#ifdef CONFIG_CMSIS_DSP_FLOAT16
	test_interpolation_f16();
#endif
	test_interpolation_f32();
}
