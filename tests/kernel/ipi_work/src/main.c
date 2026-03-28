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

/**
 * This function is called when the IPI work item is executed on a CPU.
 * It sets the CPU ID in the test_item structure to indicate which CPU
 * executed the work item.
 *
 * @param item Pointer to the IPI work item to be executed
 */
static void test_function(struct k_ipi_work *item)
{
	struct test_ipi_work *my_work;
	unsigned int cpu;

	cpu = arch_curr_cpu()->id;

	my_work = CONTAINER_OF(item, struct test_ipi_work, work);
	my_work->cpu_bit = BIT(cpu);
}

/**
 * Timer function that is called to initiate the IPI work from
 * an ISR and to wait for the work item to complete.
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
 * This test covers the simplest working cases of IPI work
 * item execution and waiting. It adds a single IPI work item
 * to another CPU's queue, signals it and waits for it to complete.
 * Waiting covers two scenarios
 *  1. From thread level
 *  2. From a k_timer (ISR).
 */
ZTEST(ipi_work, test_ipi_work_simple)
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
