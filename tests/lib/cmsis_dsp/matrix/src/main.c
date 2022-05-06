/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

extern void test_matrix_unary_q7(void);
extern void test_matrix_unary_q15(void);
extern void test_matrix_unary_q31(void);
extern void test_matrix_unary_f16(void);
extern void test_matrix_unary_f32(void);
extern void test_matrix_unary_f64(void);

extern void test_matrix_binary_q7(void);
extern void test_matrix_binary_q15(void);
extern void test_matrix_binary_q31(void);
extern void test_matrix_binary_f16(void);
extern void test_matrix_binary_f32(void);
extern void test_matrix_binary_f64(void);

void test_main(void)
{
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_Q7
	test_matrix_unary_q7();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_Q15
	test_matrix_unary_q15();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_Q31
	test_matrix_unary_q31();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_F16
	test_matrix_unary_f16();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_F32
	test_matrix_unary_f32();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_F64
	test_matrix_unary_f64();
#endif

#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_Q7
	test_matrix_binary_q7();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_Q15
	test_matrix_binary_q15();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_Q31
	test_matrix_binary_q31();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_F16
	test_matrix_binary_f16();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_F32
	test_matrix_binary_f32();
#endif
#ifdef CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_F64
	test_matrix_binary_f64();
#endif
}
