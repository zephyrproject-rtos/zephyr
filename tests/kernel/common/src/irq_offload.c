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

extern void *common_setup(void);
ZTEST_SUITE(irq_offload, NULL, common_setup, NULL, NULL, NULL);
