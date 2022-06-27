/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

extern void test_filtering_biquad_q15(void);
extern void test_filtering_biquad_q31(void);
extern void test_filtering_biquad_f16(void);
extern void test_filtering_biquad_f32(void);
extern void test_filtering_biquad_f64(void);

extern void test_filtering_decim_q15(void);
extern void test_filtering_decim_q31(void);
extern void test_filtering_decim_f32(void);

extern void test_filtering_fir_q7(void);
extern void test_filtering_fir_q15(void);
extern void test_filtering_fir_q31(void);
extern void test_filtering_fir_f16(void);
extern void test_filtering_fir_f32(void);

extern void test_filtering_misc_q7(void);
extern void test_filtering_misc_q15(void);
extern void test_filtering_misc_q31(void);
extern void test_filtering_misc_f16(void);
extern void test_filtering_misc_f32(void);

void test_main(void)
{
#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_BIQUAD
	test_filtering_biquad_q15();
	test_filtering_biquad_q31();
#ifdef CONFIG_CMSIS_DSP_FLOAT16
	test_filtering_biquad_f16();
#endif
	test_filtering_biquad_f32();
	test_filtering_biquad_f64();
#endif

#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_DECIM
	test_filtering_decim_q15();
	test_filtering_decim_q31();
	test_filtering_decim_f32();
#endif

#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_FIR
	test_filtering_fir_q7();
	test_filtering_fir_q15();
	test_filtering_fir_q31();
#ifdef CONFIG_CMSIS_DSP_FLOAT16
	test_filtering_fir_f16();
#endif
	test_filtering_fir_f32();
#endif

#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_MISC
	test_filtering_misc_q7();
	test_filtering_misc_q15();
	test_filtering_misc_q31();
#ifdef CONFIG_CMSIS_DSP_FLOAT16
	test_filtering_misc_f16();
#endif
	test_filtering_misc_f32();
#endif
}
