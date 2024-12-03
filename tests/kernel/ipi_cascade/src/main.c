/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test is intended to run on an SMP platform with 2 CPUs. It engineers
 * a scenario where unless CONFIG_SCHED_IPI_CASCADE is enabled, the highest and
 * 3rd highest priority threads will be scheduled to execute on the 2 CPUs
 * instead of the highest and 2nd highest priority threads.
 *
 * Setup Conditions:
 * Thread T1 (main thread) starts on core X at a med-high priority.
 * Thread T2 starts on core Y (but is not pinned) at a low priority.
 * Thread T3 is blocked, pinned to core X and runs at a high priority.
 * Thread T4 is blocked, not pinned to a core and runs at a med-low priority.
 *
 * T1 (main thread) locks interrupts to force it to be last to service any IPIs.
 * T2 unpends both T3 and T4 and generates an IPI.
 * T4 should get scheduled to run on core Y.
 * T1 unlocks interrupts, processes the IPI and T3 runs on core X.
 *
 * Since T1 is of higher priority than T4, T4 should get switched out for T1
 * leaving T3 and T1 executing on the 2 CPUs. However, this final step will
 * only occur when IPI cascades are enabled.
 *
 * If this test is executed with IPI cascades disabled then the test will fail
 * after about 5 seconds because a monitoring k_timer will expire and
 * terminate the test.
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <ipi.h>
#include <zephyr/kernel_structs.h>

#if (CONFIG_MP_MAX_NUM_CPUS != 2)
#error "This test must have CONFIG_MP_MAX_NUM_CPUS=2"
#endif

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define NUM_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)

#define DELAY_FOR_IPIS 200

#define PRIORITY_HIGH     5
#define PRIORITY_MED_HIGH 6
#define PRIORITY_MED_LOW  7
#define PRIORITY_LOW      9

K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack3, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack4, STACK_SIZE);

K_EVENT_DEFINE(my_event);

static struct k_thread thread2;
static struct k_thread thread3;
static struct k_thread thread4;

static bool thread1_ready;
static bool thread2_ready;

static int cpu_t1;
static int cpu_t2;
static int cpu_t3;
static int cpu_t4;

static struct k_timer my_timer;

static volatile bool timer_expired;

static void show_executing_threads(const char *str)
{
	printk("%s - CPU[0]: %p '%s' @ priority %d\n",
	       str, _kernel.cpus[0].current,
	       _kernel.cpus[0].current->name,
	       _kernel.cpus[0].current->base.prio);
	printk("%s - CPU[1]: %p '%s' @ priority %d\n",
	       str, _kernel.cpus[1].current,
	       _kernel.cpus[1].current->name,
	       _kernel.cpus[1].current->base.prio);
}

/**
 * Should the threads not be scheduled as expected, abort threads T2,
 * T3 and T4 and allow the system to recover. The main thread
 * (T1/test_ipi_cascade) will verify that the timer did not execute.
 */
static void timer_expiry_fn(struct k_timer *timer)
{
	timer_expired = true;

	k_thread_abort(&thread2);
	k_thread_abort(&thread3);
	k_thread_abort(&thread4);
}

/* T3 executes at PRIORITY_HIGH - will get pinned to T1's CPU */
void thread3_entry(void *p1, void *p2, void *p3)
{
	int  id;
	int  key;

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);

	 /* 2.1 - Block on my_event */

	k_event_wait(&my_event, 0x1, false, K_FOREVER);

	/* 9.1 - T3 should be executing on the same CPU that T1 was. */

	cpu_t3 = arch_current_thread()->base.cpu;

	zassert_true(cpu_t3 == cpu_t1, "T3 not executing on T1's original CPU");

	for (;;) {
		/* Inifite loop to prevent reschedule from T3 ending. */
	}
}

/* T4 executes at PRIORITY_MED_LOW */
void thread4_entry(void *p1, void *p2, void *p3)
{
	/* 2.2 - Block on my_event */

	k_event_wait(&my_event, 0x2, false, K_FOREVER);

	/* 8.1 - T4 has been switched in. Flag that it is now ready.
	 * It is expected to execute on the same CPU that T2 did.
	 */

	cpu_t4 = arch_current_thread()->base.cpu;

	zassert_true(cpu_t4 == cpu_t2, "T4 on unexpected CPU");

	for (;;) {
		/*
		 * Inifite loop to prevent reschedule from T4 ending.
		 * Due to the IPI cascades, T4 will get switched out for T1.
		 */
	}
}

/* T2 executes at PRIORITY_LOW */
void thread2_entry(void *p1, void *p2, void *p3)
{
	int  key;

	/* 5. Indicate T2 is ready. Allow T1 to proceed. */

	thread2_ready = true;

	/* 5.1. Spin until T1 is ready. */

	while (!thread1_ready) {
		key = arch_irq_lock();
		arch_spin_relax();
		arch_irq_unlock(key);
	}

	cpu_t2 = arch_current_thread()->base.cpu;

	zassert_false(cpu_t2 == cpu_t1, "T2 and T1 unexpectedly on the same CPU");

	/*
	 * 8. Wake T3 and T4. As T3 is restricted to T1's CPU, waking both
	 * will result in executing T4 on T2's CPU.
	 */

	k_event_set(&my_event, 0x3);

	zassert_true(false, "This message should not appear!");
}

ZTEST(ipi_cascade, test_ipi_cascade)
{
	int  key;
	int  status;

	/* 1. Set main thread priority and create threads T3 and T4. */

	k_thread_priority_set(k_current_get(), PRIORITY_MED_HIGH);

	k_thread_create(&thread3, stack3, K_THREAD_STACK_SIZEOF(stack3),
			thread3_entry, NULL, NULL, NULL,
			PRIORITY_HIGH, 0, K_NO_WAIT);

	k_thread_create(&thread4, stack4, K_THREAD_STACK_SIZEOF(stack3),
			thread4_entry, NULL, NULL, NULL,
			PRIORITY_MED_LOW, 0, K_NO_WAIT);

	k_thread_name_set(&thread3, "T3");
	k_thread_name_set(&thread4, "T4");

	/* 2. Give threads T3 and T4 time to block on my_event. */

	k_sleep(K_MSEC(1000));

	/* 3. T3 and T4 are blocked. Pin T3 to this CPU */

	cpu_t1 = arch_current_thread()->base.cpu;
	status = k_thread_cpu_pin(&thread3, cpu_t1);

	zassert_true(status == 0, "Failed to pin T3 to %d : %d\n", cpu_t1, status);

	/* 4. Create T2 and spin until it is ready. */

	k_thread_create(&thread2, stack2, K_THREAD_STACK_SIZEOF(stack2),
			thread2_entry, NULL, NULL, NULL,
			PRIORITY_LOW, 0, K_NO_WAIT);
	k_thread_name_set(&thread2, "T2");

	k_timer_init(&my_timer, timer_expiry_fn, NULL);
	k_timer_start(&my_timer, K_MSEC(5000), K_NO_WAIT);

	while (!thread2_ready) {
		key = arch_irq_lock();
		arch_spin_relax();
		arch_irq_unlock(key);
	}

	/* 6. Lock interrupts to delay handling of any IPIs. */

	key = arch_irq_lock();

	/* 7. Inform T2 we are ready. */

	thread1_ready = true;

	k_busy_wait(1000);   /* Busy wait for 1 ms */

	/*
	 * 9. Unlocking interrupts allows the IPI from to be processed.
	 * This will cause the current thread (T1) to be switched out for T3.
	 * An IPI cascade is expected to occur resulting in switching
	 * out T4 for T1. Busy wait again to ensure that the IPI is detected
	 * and processed.
	 */

	arch_irq_unlock(key);
	k_busy_wait(1000);   /* Busy wait for 1 ms */

	zassert_false(timer_expired, "Test terminated by timer");

	zassert_true(cpu_t1 != arch_current_thread()->base.cpu,
		     "Main thread (T1) did not change CPUs\n");

	show_executing_threads("Final");

	k_timer_stop(&my_timer);

	k_thread_abort(&thread2);
	k_thread_abort(&thread3);
	k_thread_abort(&thread4);
}

ZTEST_SUITE(ipi_cascade, NULL, NULL, NULL, NULL, NULL);
