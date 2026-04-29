/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

#ifndef CONFIG_FPU
#error "This test requires CONFIG_FPU"
#endif

static volatile float isr_float_result;
#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
static volatile double isr_double_result;
#endif

static float absf_local(float val)
{
	return (val < 0.0f) ? -val : val;
}

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
static double absd_local(double val)
{
	return (val < 0.0) ? -val : val;
}
#endif

static void fp_isr(const void *arg)
{
	ARG_UNUSED(arg);

	volatile float a = 1.25f;
	volatile float b = 2.5f;
	volatile float c = 0.75f;

	isr_float_result = (a * b) + c;

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	volatile double da = 1.5;
	volatile double db = 2.0;
	volatile double dc = 0.25;

	isr_double_result = (da * db) + dc;
#endif
}

ZTEST(riscv_fpu, test_thread_fp_math)
{
	volatile float a = 1.5f;
	volatile float b = 3.0f;
	volatile float c = 0.25f;
	float result = (a * b) + c;
	float expected = 4.75f;
	float diff = absf_local(result - expected);

	zassert_true(diff < 0.0001f,
		     "float mismatch: got %f expected %f",
		     (double)result,
		     (double)expected);

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	volatile double da = 1.75;
	volatile double db = 2.0;
	double dresult = da * db;
	double dexpected = 3.5;
	double ddiff = absd_local(dresult - dexpected);

	zassert_true(ddiff < 0.0000001,
		     "double mismatch: got %f expected %f",
		     dresult,
		     dexpected);
#endif
}

ZTEST(riscv_fpu, test_irq_offload_fp_math)
{
	isr_float_result = 0.0f;
#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	isr_double_result = 0.0;
#endif

	irq_offload(fp_isr, NULL);

	zassert_true(absf_local(isr_float_result - 3.875f) < 0.0001f,
		     "ISR float mismatch: got %f expected %f",
		     (double)isr_float_result,
		     3.875);

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	zassert_true(absd_local(isr_double_result - 3.25) < 0.0000001,
		     "ISR double mismatch: got %f expected %f",
		     isr_double_result,
		     3.25);
#endif
}

ZTEST_SUITE(riscv_fpu, NULL, NULL, NULL, NULL, NULL);
