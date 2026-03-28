/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/kernel_structs.h>

#if (CONFIG_MP_MAX_NUM_CPUS < 2)
#error "This test requires at least 2 CPUs"
#endif

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define ONE_BUSY_SECOND 1000000

#define NUM_HELPER_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)

/* Define an array of stacks */
static K_THREAD_STACK_ARRAY_DEFINE(helper_stacks, NUM_HELPER_THREADS, STACK_SIZE);
static K_THREAD_STACK_DEFINE(meta_irq_stack, STACK_SIZE);
static K_SEM_DEFINE(helper_sem, 0, NUM_HELPER_THREADS);

static struct k_thread helper_threads[NUM_HELPER_THREADS];
static struct k_thread meta_irq_thread;
static struct k_thread *test_thread;

static volatile struct _cpu *test_cpu;
static volatile bool metairq_thread_has_started;
static volatile bool loop_forever = true;

static void meta_irq_thread_entry(void *p1, void *p2, void *p3)
{
	unsigned int state;
	struct _cpu *cpu = arch_curr_cpu();

	zassert_equal(cpu, test_cpu,
		      "Expected MetaIRQ on CPU%u, not CPU%u\n",
		      test_cpu->id, cpu->id);

	/*
	 * The pre-empted cooperative test thread should not be blocked
	 * or tracked in a readyQ. It should however be tracked in the
	 * current CPU's 'metairq_preempted' field.
	 */

	state = test_thread->base.thread_state;
	zassert_equal(state, 0x0,
		      "Test thread has unexpected thread state (0x%x)\n",
		      state);

	zassert_equal(cpu->metairq_preempted, test_thread,
		      "Test thread not found in CPU%u metairq_preempted\n",
		      cpu->id);

	TC_PRINT("MetaIRQ thread running on CPU%u\n", cpu->id);

	metairq_thread_has_started = true;

	/* Send IPI to all other CPUs to force a reschedule*/
	arch_sched_broadcast_ipi();

	/*
	 * Busy wait for one second to allow other CPUs to process the IPI.
	 * The scheduler should not try to schedule the cooperative test
	 * thread. If it for some reason does, we can detect it in the test
	 * thread.
	 */

	k_busy_wait(ONE_BUSY_SECOND);

	k_thread_suspend(k_current_get());

	zassert_true(false, "MetaIRQ thread resumed unexpectedly");
}

static void irq_handler(const void *arg)
{
	ARG_UNUSED(arg);

	k_thread_start(&meta_irq_thread);
}

/*
 * The helper thread is preemptible. It locks interrupts and waits for the
 * metaIRQ thread to start. This is to guarantee that the metaIRQ thread runs
 * on the same CPU as the cooperative test thread.
 */
static void helper_thread_entry(void *p1, void *p2, void *p3)
{
	unsigned int key;

	k_sem_give(&helper_sem);

	/* Lock interrupts until metaIRQ thread runs */

	key = arch_irq_lock();
	while (metairq_thread_has_started == false) {
		arch_spin_relax();  /* Requires interrupts to be masked */
	}
	arch_irq_unlock(key);

	while (loop_forever) {
		/* Busy wait */
	}
}

ZTEST(smp_metairq, test_smp_metairq_no_migration)
{
	unsigned int i;
	unsigned int id1;
	unsigned int id2;

	test_thread = k_current_get();

	/* Create, but do not start the meta-irq thread */

	k_thread_create(&meta_irq_thread, meta_irq_stack,
			STACK_SIZE,
			meta_irq_thread_entry,
			NULL, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_FOREVER);

	/* Create preemptible helper threads on all other CPUs */

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS - 1; i++) {
		k_thread_create(&helper_threads[i], helper_stacks[i],
				STACK_SIZE,
				helper_thread_entry,
				NULL, NULL, NULL,
				K_PRIO_PREEMPT(2), 0, K_NO_WAIT);

		k_sem_take(&helper_sem, K_FOREVER);
	}

	TC_PRINT("Test thread running on CPU%u\n", arch_curr_cpu()->id);

	k_busy_wait(ONE_BUSY_SECOND);

	test_cpu = arch_curr_cpu();
	id1 = test_cpu->id;

	/* Force an interrupt that will schedule the metaIRQ on this CPU */
	irq_offload(irq_handler, NULL);

	TC_PRINT("Test thread resuming on CPU%u\n", arch_curr_cpu()->id);

	test_cpu = arch_curr_cpu();
	id2 = test_cpu->id;

	zassert_equal(id1, id2,
		      "Thread migrated from CPU%u to CPU%u during IRQ",
		      id1, id2);

	/* Clean up threads */

	k_thread_abort(&meta_irq_thread);
	loop_forever = false;

	for (i = 0; i < NUM_HELPER_THREADS; i++) {
		k_thread_join(&helper_threads[i], K_FOREVER);
	}

	k_thread_join(&meta_irq_thread, K_FOREVER);
}

ZTEST_SUITE(smp_metairq, NULL, NULL, NULL, NULL, NULL);
