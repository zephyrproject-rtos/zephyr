/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

extern void test_svm_f16(void);
extern void test_svm_f32(void);

void test_main(void)
{
#ifdef CONFIG_CMSIS_DSP_FLOAT16
	test_svm_f16();
#endif
	test_svm_f32();
}
