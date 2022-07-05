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
#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/irq_offload.h>

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
 * @brief Verify thread context
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Check whether offloaded running function is in interrupt
 * context, on the IRQ stack or not.
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

/**
 * @brief Test the arch_nop() by invoking and measure it.
 *
 * @details This test is mainly for coverage of the code. arch_nop()
 * is a special implementation and it will behave differently on
 * different platforms. By the way, this also measures how many
 * cycles it spends for platforms that support it.
 *
 * FYI: The potential uses of arch_nop() could be:
 * - Code alignment: Although in this case it's much more likely the
 *   compiler doing so (or you're in an assembly file, in which case
 *   you're not calling arch_nop() anyway). And this would require
 *   that arch_nop() be ALWAYS_INLINE.
 * - Giving you a guaranteed place to put a breakpoint / trace trigger
 *   / etc. when debugging. This is on main usage of arch_nop(); it
 *   inherently is generally debugging code removed before actually
 *   pushing.
 * - Giving you a guaranteed place to put a patchpoint. E.g. ARMv7
 *   allows nop (and a few other instructions) to be modified
 *   concurrently with execution, but not most other instructions.
 * - Delaying a few instructions, e.g. for tight timing loops on
 *   M-cores.
 *
 * Our test here mainly aims at the 4th scenario mentioned above but
 * also potentially tests the 1st scenario. So no optimization here to
 * prevent arch_nop() has optimized by the compiler is necessary.
 *
 * @ingroup kernel_common_tests
 *
 * @see arch_nop()
 */
__no_optimization void nop(void)
{
	uint32_t t_get_time, t_before, t_after, diff;

	t_before = k_cycle_get_32();
	t_after = k_cycle_get_32();

	/* calculate time spent between two k_cycle_get_32() call */
	t_get_time = t_after - t_before;

	printk("time k_cycle_get_32() takes %d cycles\n", t_get_time);

	/*
	 *  If two k_cycle_get_32() call take zero cycle here, this
	 *  means it cannot comes out a correct result, in these
	 *  case, we skip this test, such as native posix.
	 */
	if (t_get_time == 0) {
		ztest_test_skip();
	}

	t_before = k_cycle_get_32();

	arch_nop();

#if defined(CONFIG_RISCV)
	/* do 2 nop instructions more to cost cycles */
	arch_nop();
	arch_nop();
#if defined(CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER)
	/* When the case machine timer clock uses the divided system clock,
	 * k_cycle_get_32() can't measure accurately how many cycles elapsed.
	 *
	 * For example, use the value as timer clock obtained by dividing
	 * the system clock by 4.
	 * In this case, measuring a duration with k_cycle_get32() has up to 3
	 * (4-1) cycles systematic error.
	 *
	 * To run this test, we need to insert an appropriate of nops
	 * with consideration for the errors.
	 * 'nop' can not repeat with for loop.
	 * Must insert as separated statement.
	 * But we don't have a convenient function such as
	 * BOOST_PP_REPEAT in C++.
	 *
	 * At this time, Implementing a generic test is a bit difficult.
	 * Skipping this test in the case.
	 */
	ztest_test_skip();
#endif
#elif defined(CONFIG_ARC)
	/* do 7 nop instructions more to cost cycles */
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
#elif defined(CONFIG_SPARC)
	/* do 9 nop instructions more to cost cycles */
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
#elif defined(CONFIG_ARM) || defined(CONFIG_ARM64)			\
	|| defined(CONFIG_BOARD_EHL_CRB) || (CONFIG_BOARD_UP_SQUARED)	\
	|| (CONFIG_SOC_FAMILY_INTEL_ADSP)
	/* ARM states the following:
	 * No Operation does nothing, other than advance the value of
	 * the program counter by 4. This instruction can be used for
	 * instruction alignment purposes.
	 * Note: The timing effects of including a NOP instruction in
	 * a program are not guaranteed. It can increase execution time
	 * ,leave it unchanged, or even reduce it. Therefore, NOP
	 * instructions are not suitable for timing loops.
	 *
	 * So we skip this test, it will get a negative cycles.
	 *
	 * And on physical EHL_CRB, up squared and INTEL ADSP boards,
	 * we also got a similar situation, we skip the check as well.
	 */
	ztest_test_skip();
#endif

	t_after = k_cycle_get_32();

	/* Calculate delta time of arch_nop(). */
	diff = t_after - t_before - t_get_time;
	printk("arch_nop() takes %d cycles\n", diff);

	/* An arch_nop() call should spend actual cpu cycles */
	zassert_true(diff > 0,
			"arch_nop() takes %d cpu cycles", diff);
}

ZTEST(irq_offload, test_nop)
{
	nop();
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

/* Invoke irq_offload() from an interrupt and verify that the
 * resulting nested interrupt doesn't explode
 */
ZTEST(common_1cpu, test_nested_irq_offload)
{
	if (!IS_ENABLED(CONFIG_IRQ_OFFLOAD_NESTED)) {
		ztest_test_skip();
	}

	k_thread_priority_set(k_current_get(), 1);

	k_timer_init(&nestoff_timer, nestoff_timer_fn, NULL);

	zassert_false(timer_executed, "timer ran too soon");
	zassert_false(nested_executed, "nested irq_offload ran too soon");

	/* Do this in a thread to exercise a regression case: the
	 * offload handler will suspend the thread it interrupted,
	 * ensuring that the interrupt returns back to this thread and
	 * effects a context switch of of the nested interrupt (see
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
