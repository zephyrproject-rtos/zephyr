/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACKSIZE 1024

/* Priority level of the threads used in this test.
 * The priority level, itself, is arbitrary; we only
 * want to ensure they are cooperative threads.
 */
#define PRIORITY  K_PRIO_COOP(0)

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)
#define K_FP_OPTS K_FP_REGS
#elif defined(CONFIG_X86)
#define K_FP_OPTS (K_FP_REGS | K_SSE_REGS)
#else
#error "Architecture not supported for this test"
#endif

struct k_thread usr_fp_thread;
K_THREAD_STACK_DEFINE(usr_fp_thread_stack, STACKSIZE);

ZTEST_BMEM static volatile int ret = TC_PASS;

static void usr_fp_thread_entry_1(void)
{
	k_yield();
}

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV) || \
	(defined(CONFIG_X86) && defined(CONFIG_LAZY_FPU_SHARING))
#define K_FLOAT_DISABLE_SYSCALL_RETVAL 0
#else
#define K_FLOAT_DISABLE_SYSCALL_RETVAL -ENOSYS
#endif

static void usr_fp_thread_entry_2(void)
{
	k_yield();

	/* System call to disable FP mode */
	if (k_float_disable(k_current_get()) != K_FLOAT_DISABLE_SYSCALL_RETVAL) {

		TC_ERROR("k_float_disable() fail - should never see this\n");

		ret = TC_FAIL;
	}
}

void test_k_float_disable_common(void)
{
	ret = TC_PASS;

	/* Set thread priority level to the one used
	 * in this test suite for cooperative threads.
	 */
	k_thread_priority_set(k_current_get(), PRIORITY);

	/* Create an FP-capable User thread with the same cooperative
	 * priority as the current thread.
	 */
	k_thread_create(&usr_fp_thread, usr_fp_thread_stack, STACKSIZE,
		(k_thread_entry_t)usr_fp_thread_entry_1, NULL, NULL, NULL,
		PRIORITY, K_USER | K_FP_OPTS,
		K_NO_WAIT);

	/* Yield will swap-in usr_fp_thread */
	k_yield();

	/* Verify K_FP_OPTS are set properly */
	zassert_true(
		(usr_fp_thread.base.user_options & K_FP_OPTS) != 0,
		"usr_fp_thread FP options not set (0x%0x)",
		usr_fp_thread.base.user_options);

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)
	/* Verify FP mode can only be disabled for current thread */
	zassert_true((k_float_disable(&usr_fp_thread) == -EINVAL),
		"k_float_disable() successful on thread other than current!");

	/* Verify K_FP_OPTS are still set */
	zassert_true(
		(usr_fp_thread.base.user_options & K_FP_OPTS) != 0,
		"usr_fp_thread FP options cleared");
#elif defined(CONFIG_X86) && defined(CONFIG_LAZY_FPU_SHARING)
	zassert_true((k_float_disable(&usr_fp_thread) == 0),
		"k_float_disable() failure");

	/* Verify K_FP_OPTS are now cleared */
	zassert_true(
		(usr_fp_thread.base.user_options & K_FP_OPTS) == 0,
		"usr_fp_thread FP options not clear (0x%0x)",
		usr_fp_thread.base.user_options);
#elif defined(CONFIG_X86) && !defined(CONFIG_LAZY_FPU_SHARING)
	/* Verify k_float_disable() is not supported */
	zassert_true((k_float_disable(&usr_fp_thread) == -ENOSYS),
		"k_float_disable() successful when not supported");
#endif
}

void test_k_float_disable_syscall(void)
{
	ret = TC_PASS;

	k_thread_priority_set(k_current_get(), PRIORITY);

	/* Create an FP-capable User thread with the same cooperative
	 * priority as the current thread. The thread will disable its
	 * FP mode.
	 */
	k_thread_create(&usr_fp_thread, usr_fp_thread_stack, STACKSIZE,
		(k_thread_entry_t)usr_fp_thread_entry_2, NULL, NULL, NULL,
		PRIORITY, K_INHERIT_PERMS | K_USER | K_FP_OPTS,
		K_NO_WAIT);

	/* Yield will swap-in usr_fp_thread */
	k_yield();

	/* Verify K_FP_OPTS are set properly */
	zassert_true(
		(usr_fp_thread.base.user_options & K_FP_OPTS) != 0,
		"usr_fp_thread FP options not set (0x%0x)",
		usr_fp_thread.base.user_options);

	/* Yield will swap-in usr_fp_thread */
	k_yield();

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV) || \
	(defined(CONFIG_X86) && defined(CONFIG_LAZY_FPU_SHARING))

	/* Verify K_FP_OPTS are now cleared by the user thread itself */
	zassert_true(
		(usr_fp_thread.base.user_options & K_FP_OPTS) == 0,
		"usr_fp_thread FP options not clear (0x%0x)",
		usr_fp_thread.base.user_options);

	/* ret is volatile, static analysis says we can't use in assert */
	bool ok = ret == TC_PASS;

	zassert_true(ok, "");
#else
	/* Check skipped for x86 without support for Lazy FP Sharing */
#endif
}

#if defined(CONFIG_ARM) && defined(CONFIG_DYNAMIC_INTERRUPTS)

#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

struct k_thread sup_fp_thread;
K_THREAD_STACK_DEFINE(sup_fp_thread_stack, STACKSIZE);

void arm_test_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	if (k_float_disable(&sup_fp_thread) != -EINVAL) {

		TC_ERROR("k_float_disable() successful in ISR\n");

		ret = TC_FAIL;
	}
}

static void sup_fp_thread_entry(void)
{
	/* Verify K_FP_REGS flag is set */
	if ((sup_fp_thread.base.user_options & K_FP_REGS) == 0) {

		TC_ERROR("sup_fp_thread FP options cleared\n");
		ret = TC_FAIL;
	}

	/* Determine an NVIC IRQ line that is not currently in use. */
	int i;

	for (i = CONFIG_NUM_IRQS - 1; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * not enabled, thus, currently not in use by Zephyr.
			 */
			break;
		}
	}

	zassert_true(i >= 0,
		"No available IRQ line to use in the test\n");

	TC_PRINT("Available IRQ line: %u\n", i);

	arch_irq_connect_dynamic(i,
		0,
		arm_test_isr_handler,
		NULL,
		0);

	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);
	NVIC_SetPendingIRQ(i);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before program proceeds.
	 */
	__DSB();
	__ISB();

	/* Verify K_FP_REGS flag is still set */
	if ((sup_fp_thread.base.user_options & K_FP_REGS) == 0) {

		TC_ERROR("sup_fp_thread FP options cleared\n");
		ret = TC_FAIL;
	}
}

void test_k_float_disable_irq(void)
{
	ret = TC_PASS;

	k_thread_priority_set(k_current_get(), PRIORITY);


	/* Create an FP-capable Supervisor thread with the same cooperative
	 * priority as the current thread.
	 */
	k_thread_create(&sup_fp_thread, sup_fp_thread_stack, STACKSIZE,
		(k_thread_entry_t)sup_fp_thread_entry, NULL, NULL, NULL,
		PRIORITY, K_FP_REGS,
		K_NO_WAIT);

	/* Yield will swap-in sup_fp_thread */
	k_yield();

	/* ret is volatile, static analysis says we can't use in assert */
	bool ok = ret == TC_PASS;

	zassert_true(ok, "");
}
#else
void test_k_float_disable_irq(void)
{
	TC_PRINT("Skipped for x86 or ARM without support for dynamic IRQs\n");
	ztest_test_skip();
}
#endif /* CONFIG_ARM && CONFIG_DYNAMIC_INTERRUPTS */
