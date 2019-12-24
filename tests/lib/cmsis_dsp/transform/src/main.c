/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

extern void test_transform_cq15(void);
extern void test_transform_rq15(void);
extern void test_transform_cq31(void);
extern void test_transform_rq31(void);
extern void test_transform_cf32(void);
extern void test_transform_rf32(void);
extern void test_transform_cf64(void);
extern void test_transform_rf64(void);

void test_main(void)
{
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_CQ15
	test_transform_cq15();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_RQ15
	test_transform_rq15();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_CQ31
	test_transform_cq31();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_TRANSFORM_RQ31
	test_transform_rq31();
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
