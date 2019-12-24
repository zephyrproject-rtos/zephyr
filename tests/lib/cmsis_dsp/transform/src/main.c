/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void test_transform_q15(void);
extern void test_transform_q31(void);
extern void test_transform_cf32(void);
extern void test_transform_rf32(void);
extern void test_transform_cf64(void);
extern void test_transform_rf64(void);

void test_main(void)
{
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_Q15
    test_transform_q15();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_Q31
    test_transform_q31();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_CF32
    test_transform_cf32();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_RF32
    test_transform_rf32();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_CF64
    test_transform_cf64();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_RF64
    test_transform_rf64();
#endif
}
