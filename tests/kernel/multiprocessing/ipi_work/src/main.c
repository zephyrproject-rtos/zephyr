/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#if (CONFIG_MP_MAX_NUM_CPUS == 1)
#error "This test must have at least CONFIG_MP_MAX_NUM_CPUS=2 CPUs"
#endif

/**
 * @brief Tests for the IPI work item API
 *
 * @defgroup kernel_ipi_work_tests IPI Work Tests
 *
 * @ingroup all_tests
 *
 * These tests exercise k_ipi_work_add(), k_ipi_work_signal() and
 * k_ipi_work_wait(), which queue a function for execution on another CPU by
 * way of an interprocessor interrupt.
 * @{
 * @}
 */

/* structs */

struct test_ipi_work {
	struct k_ipi_work work;

	volatile unsigned int cpu_bit;
};

/* forward declarations */

static void timer_func(struct k_timer *tmr);

/* locals */

static struct test_ipi_work test_item;
static K_SEM_DEFINE(timer_sem, 0, 1);
static K_TIMER_DEFINE(timer, timer_func, NULL);
static uint32_t timer_target_cpu;

/* Body of the IPI work item: records the CPU it was executed on so the test
 * can verify it ran on the targeted CPU.
 */
static void test_function(struct k_ipi_work *item)
{
	struct test_ipi_work *my_work;
	unsigned int cpu;

	cpu = arch_curr_cpu()->id;

	my_work = CONTAINER_OF(item, struct test_ipi_work, work);
	my_work->cpu_bit = BIT(cpu);
}

/* Timer expiry handler: initiates the IPI work from an ISR and spins until the
 * work item has completed, then wakes the waiting thread.
 */
static void timer_func(struct k_timer *tmr)
{
	ARG_UNUSED(tmr);

	timer_target_cpu = _current_cpu->id == 0 ? BIT(1) : BIT(0);

	/* Add the work item to the IPI queue, signal and wait */

	k_ipi_work_add(&test_item.work, timer_target_cpu, test_function);
	k_ipi_work_signal();
	while (k_ipi_work_wait(&test_item.work, K_NO_WAIT) == -EAGAIN) {
	}

	/* Wake the thread waiting for the work item to complete */
	k_sem_give(&timer_sem);

}

/**
 * @brief Verify that an IPI work item executes on the targeted CPU.
 *
 * @ingroup kernel_ipi_work_tests
 *
 * @details
 * A work item queued to another CPU with k_ipi_work_add() and signalled with
 * k_ipi_work_signal() must run on that CPU, and k_ipi_work_wait() must not
 * return success before it has. The work item records the CPU it executed on,
 * which is compared against the requested target mask. Both ways of waiting
 * are covered: pending from thread level with K_FOREVER, and polling with
 * K_NO_WAIT from a k_timer expiry handler, where pending is not allowed.
 *
 * Test steps:
 * - Initialize the work item with k_ipi_work_init().
 * - With interrupts locked, target the other CPU, add the work item and
 *   signal it, then unlock.
 * - Wait for completion with k_ipi_work_wait(K_FOREVER) and check the CPU the
 *   item ran on.
 * - Start a timer whose handler adds and signals the same work item, then
 *   polls k_ipi_work_wait() with K_NO_WAIT until it stops returning -EAGAIN.
 * - Take the semaphore given by the timer handler and re-check the recorded
 *   CPU.
 *
 * Expected result:
 * - k_ipi_work_wait() returns 0 in both cases and the work item executed on
 *   the targeted CPU, not on the requesting one.
 *
 * @see k_ipi_work_init()
 * @see k_ipi_work_add()
 * @see k_ipi_work_signal()
 * @see k_ipi_work_wait()
 */
ZTEST(ipi_work, test_ipi_work_executes_on_target_cpu)
{
	unsigned int key;
	unsigned int cpu_id;
	unsigned int target_cpu_mask;
	int status;

	k_ipi_work_init(&test_item.work);

	/*
	 * Issue the IPI work item from thread level. The current thread will
	 * pend while waiting for work completion. Interrupts are locked to
	 * ensure that the current thread does not change CPUs while setting
	 * up the IPI work item.
	 */

	TC_PRINT("Thread level IPI\n");

	key = arch_irq_lock();
	cpu_id = arch_curr_cpu()->id;
	target_cpu_mask = _current_cpu->id == 0 ? BIT(1) : BIT(0);

	test_item.cpu_bit = 0xFFFFFFFFU;
	k_ipi_work_add(&test_item.work, target_cpu_mask, test_function);
	k_ipi_work_signal();
	arch_irq_unlock(key);

	/* Wait for the work item to complete */

	status = k_ipi_work_wait(&test_item.work, K_FOREVER);
	zassert_equal(status, 0, "k_ipi_work_wait failed: %d", status);

	zassert_equal(test_item.cpu_bit, target_cpu_mask,
		      "Work item was not executed on the expected CPU");

	/*
	 * Issue the IPI work item from a k_timer (ISR). The k_timer will spin
	 * while waiting for the IPI work item to complete.
	 */

	TC_PRINT("ISR level IPI\n");

	test_item.cpu_bit = 0xFFFFFFFFU;
	k_timer_start(&timer, K_TICKS(2), K_NO_WAIT);
	status = k_sem_take(&timer_sem, K_SECONDS(10));

	zassert_equal(status, 0, "k_sem_take failed: %d", status);
	zassert_equal(test_item.cpu_bit, timer_target_cpu,
		      "Work item was not executed on the expected CPU");
}

ZTEST_SUITE(ipi_work, NULL, NULL, NULL, NULL, NULL);
