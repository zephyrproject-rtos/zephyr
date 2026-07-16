/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * This test case verifies the correctness of irq_offload(), an important
 * routine used in many other test cases for running a function in interrupt
 * context, on the IRQ stack.
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

/**
 * @defgroup kernel_irq_offload_tests IRQ Offload
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_irq_offload_tests
 * @{
 */

volatile uint32_t sentinel;
#define SENTINEL_VALUE 0xDEADBEEF

K_THREAD_STACK_DEFINE(offload_stack, 384 + CONFIG_TEST_EXTRA_STACK_SIZE);
struct k_thread offload_thread;

static void offload_function(const void *param)
{
	uint32_t x = POINTER_TO_INT(param);

	/* Make sure we're in IRQ context */
	zassert_true(k_is_in_isr(), "Not in IRQ context!");

	sentinel = x;
}

/**
 * @brief Verify irq_offload() runs a function in interrupt context and passes its argument.
 *
 * @ingroup kernel_irq_offload_tests
 *
 * @details
 * Confirms that irq_offload() synchronously executes the supplied callback in ISR
 * context on the IRQ stack and forwards the caller-provided parameter. Passing
 * proves the offload mechanism transitions to interrupt context correctly and
 * that nested irq lock/unlock key handling reports the right lock state.
 *
 * Test steps:
 * - Nest arch_irq_lock() twice and assert the returned keys report unlocked then
 *   locked state, then unlock in reverse order.
 * - Call irq_offload() with a known sentinel value as the parameter.
 * - The callback asserts it runs in ISR context and stores the parameter.
 *
 * Expected result:
 * - The sentinel global equals the value passed to irq_offload().
 *
 * @see irq_offload()
 * @verifies ZEP-SRS-7-13
 */
ZTEST(irq_offload, test_irq_offload)
{
	/* Simple validation of nested locking. */
	unsigned int key1, key2;

	key1 = arch_irq_lock();
	zassert_true(arch_irq_unlocked(key1),
		     "IRQs should have been unlocked, but key is 0x%x\n",
		     key1);
	key2 = arch_irq_lock();
	zassert_false(arch_irq_unlocked(key2),
		      "IRQs should have been locked, but key is 0x%x\n",
		      key2);
	arch_irq_unlock(key2);
	arch_irq_unlock(key1);

	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (const void *)SENTINEL_VALUE);

	zassert_equal(sentinel, SENTINEL_VALUE,
		      "irq_offload() didn't work properly");
}

static struct k_timer nestoff_timer;
static bool timer_executed, nested_executed;

void nestoff_offload(const void *parameter)
{
	/* Suspend the thread we interrupted so we context switch, see below */
	k_thread_suspend(&offload_thread);

	nested_executed = true;
}


static void nestoff_timer_fn(struct k_timer *timer)
{
	zassert_false(nested_executed, "nested irq_offload ran too soon");
	irq_offload(nestoff_offload, NULL);
	zassert_true(nested_executed, "nested irq_offload did not run");

	/* Set this last, to be sure we return to this context and not
	 * the enclosing interrupt
	 */
	timer_executed = true;
}

static void offload_thread_fn(void *p0, void *p1, void *p2)
{
	k_timer_start(&nestoff_timer, K_TICKS(1), K_FOREVER);

	while (true) {
		zassert_false(timer_executed, "should not return to this thread");
	}
}

/**
 * @brief Verify a nested irq_offload() invoked from within an interrupt is handled safely.
 *
 * @ingroup kernel_irq_offload_tests
 *
 * @details
 * Exercises the regression where irq_offload() is called from an already-running
 * interrupt (a timer ISR) and the nested handler suspends the interrupted thread,
 * forcing a context switch out of the nested interrupt. Passing proves nested
 * interrupt offload runs the inner handler exactly once and returns control to the
 * correct context without corruption. Skipped on SMP or when nested offload is off.
 *
 * Test steps:
 * - Raise the test thread priority and start a timer whose ISR calls irq_offload().
 * - In a separate thread, spin asserting it is never resumed after the timer fires.
 * - The timer ISR invokes the nested offload handler which suspends that thread.
 *
 * Expected result:
 * - Both the timer handler and the nested offload handler report having executed.
 *
 * @see irq_offload()
 * @verifies ZEP-SRS-7-13
 */
ZTEST(common_1cpu, test_nested_irq_offload)
{
	if (arch_num_cpus() > 1 || !IS_ENABLED(CONFIG_IRQ_OFFLOAD_NESTED)) {
		ztest_test_skip();
	}

	k_thread_priority_set(k_current_get(), 1);

	k_timer_init(&nestoff_timer, nestoff_timer_fn, NULL);

	zassert_false(timer_executed, "timer ran too soon");
	zassert_false(nested_executed, "nested irq_offload ran too soon");

	/* Do this in a thread to exercise a regression case: the
	 * offload handler will suspend the thread it interrupted,
	 * ensuring that the interrupt returns back to this thread and
	 * effects a context switch of the nested interrupt (see
	 * #45779).  Requires that this be a 1cpu test case,
	 * obviously.
	 */
	k_thread_create(&offload_thread,
			offload_stack, K_THREAD_STACK_SIZEOF(offload_stack),
			offload_thread_fn, NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	zassert_true(timer_executed, "timer did not run");
	zassert_true(nested_executed, "nested irq_offload did not run");

	k_thread_abort(&offload_thread);
}
/**
 * @}
 */
extern void *common_setup(void);
ZTEST_SUITE(irq_offload, NULL, common_setup, NULL, NULL, NULL);
