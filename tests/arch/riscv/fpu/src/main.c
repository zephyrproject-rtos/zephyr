/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/arch/riscv/csr.h>

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

#ifdef CONFIG_FPU_SHARING

/* frm values: 0 = RNE (default), 1 = RTZ, 2 = RDN, 3 = RUP */
#define FRM_RTZ 1U
#define FRM_RDN 2U
#define FRM_RUP 3U

/* fflags accrued exception bits */
#define FFLAGS_NX BIT(0)
#define FFLAGS_UF BIT(1)
#define FFLAGS_OF BIT(2)

#define FCSR_STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(fcsr_stack, FCSR_STACK_SIZE);
static struct k_thread fcsr_thread;
static K_SEM_DEFINE(fcsr_sem, 0, 1);
static volatile bool fcsr_thread_ok;

static void fcsr_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Clobber the rounding mode and raise an accrued exception flag. */
	csr_write(frm, FRM_RUP);
	csr_write(fflags, FFLAGS_OF);
}

ZTEST(riscv_fpu, test_fcsr_preserved_across_isr)
{
	csr_write(frm, FRM_RTZ);
	csr_write(fflags, FFLAGS_NX);

	irq_offload(fcsr_isr, NULL);

	zassert_equal(csr_read(frm), FRM_RTZ, "frm not restored after ISR");
	zassert_equal(csr_read(fflags), FFLAGS_NX, "fflags not restored after ISR");

	csr_write(fcsr, 0);
}

static void fcsr_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	csr_write(frm, FRM_RDN);
	csr_write(fflags, FFLAGS_UF);

	/* Cooperative switch back to the test thread. */
	k_sem_give(&fcsr_sem);
	k_yield();

	if ((csr_read(frm) != FRM_RDN) || (csr_read(fflags) != FFLAGS_UF)) {
		return;
	}

	/* Switch triggered from the timer interrupt this time. */
	k_msleep(10);

	fcsr_thread_ok = (csr_read(frm) == FRM_RDN) && (csr_read(fflags) == FFLAGS_UF);
	csr_write(fcsr, 0);
}

ZTEST(riscv_fpu, test_fcsr_preserved_across_threads)
{
	k_tid_t tid;

	fcsr_thread_ok = false;
	k_sem_reset(&fcsr_sem);

	csr_write(frm, FRM_RTZ);
	csr_write(fflags, FFLAGS_NX);

	tid = k_thread_create(&fcsr_thread, fcsr_stack, FCSR_STACK_SIZE, fcsr_thread_entry,
			      NULL, NULL, NULL, k_thread_priority_get(k_current_get()), 0,
			      K_NO_WAIT);

	zassert_equal(k_sem_take(&fcsr_sem, K_SECONDS(1)), 0, "thread did not run");
	zassert_equal(csr_read(frm), FRM_RTZ, "frm not restored after cooperative switch");
	zassert_equal(csr_read(fflags), FFLAGS_NX, "fflags not restored after cooperative switch");

	/* Let the other thread run its checks and sleep. */
	k_yield();
	zassert_equal(csr_read(frm), FRM_RTZ, "frm not restored after yield");
	zassert_equal(csr_read(fflags), FFLAGS_NX, "fflags not restored after yield");

	zassert_equal(k_thread_join(tid, K_SECONDS(1)), 0, "thread did not finish");
	zassert_equal(csr_read(frm), FRM_RTZ, "frm not restored after preemptive switch");
	zassert_equal(csr_read(fflags), FFLAGS_NX, "fflags not restored after preemptive switch");
	zassert_true(fcsr_thread_ok, "fcsr of the other thread was not preserved");

	csr_write(fcsr, 0);
}

#endif /* CONFIG_FPU_SHARING */

ZTEST_SUITE(riscv_fpu, NULL, NULL, NULL, NULL, NULL);
