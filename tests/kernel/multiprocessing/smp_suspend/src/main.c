/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error "SMP test requires at least two CPUs!"
#endif

#define STACK_SIZE 1024 + CONFIG_TEST_EXTRA_STACK_SIZE

#define NUM_THREADS 6

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, NUM_THREADS, STACK_SIZE);
struct k_thread thread[NUM_THREADS];

volatile uint64_t thread_counter[NUM_THREADS];

static void thread_entry(void *p1, void *p2, void *p3)
{
	struct k_thread *resume_thread = p1;
	unsigned int self_index = (unsigned int)(uintptr_t)p2;

	while (1) {
		if (resume_thread != NULL) {
			k_thread_resume(resume_thread);
		}

		thread_counter[self_index]++;

		/*
		 * Contentious spinlocks embedded within tight loops (such as
		 * this one) have a CPU bias induced by arch_spin_relax(). We
		 * counteract this by introducing a configurable delay so that
		 * other threads have a chance to acquire the spinlock and
		 * prevent starvation.
		 */

		for (volatile unsigned int i = 0;
		     i < CONFIG_SMP_TEST_RELAX;
		     i++) {
		}

		if (self_index != 0) {
			k_thread_suspend(k_current_get());
		}
	}
}

ZTEST(smp_suspend_resume, test_smp_thread_suspend_resume_stress)
{
	unsigned int  i;
	uint64_t counter[NUM_THREADS] = {};

	/* Create the threads */

	printk("Starting ...\n");

	for (i = 0; i < NUM_THREADS; i++) {

		k_thread_create(&thread[i], thread_stack[i],
				STACK_SIZE, thread_entry,
				i < (NUM_THREADS - 1) ? &thread[i + 1] : NULL,
				(void *)(uintptr_t)i, NULL,
				10 - i, 0, K_FOREVER);

		k_thread_suspend(&thread[i]);

		k_thread_start(&thread[i]);
	}

	/*
	 * All newly created test threads are currently in the suspend state.
	 * Start the first thread.
	 */

	k_thread_resume(&thread[0]);

	for (unsigned int iteration = 0; iteration < 15; iteration++) {
		k_sleep(K_MSEC(1000));

		for (i = 0; i < NUM_THREADS; i++) {
			zassert_false(counter[i] == thread_counter[i],
				      " -- Thread %u is starving: %llu\n",
				      i, thread_counter[i]);

			counter[i] = thread_counter[i];

		}
	}
}

#define TIMEOUT_TEST_MS 2000
#define TIMEOUT_STALL_MS 200

K_THREAD_STACK_DEFINE(victim_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(suspender_stack, STACK_SIZE);
static struct k_thread victim, suspender;
static K_SEM_DEFINE(never_given, 0, 1);
static volatile bool timeout_test_stop;
static volatile uint32_t victim_wakeups;
static bool timeout_threads_started;

/* Cheap pseudo-random numbers (Marsaglia's 32-bit xorshift; *s must not be
 * 0) to vary the busy waits below, so that the suspend lands at a different
 * point of the victim's wait each time instead of in lockstep.
 */
static uint32_t xorshift(uint32_t *s)
{
	*s ^= *s << 13;
	*s ^= *s >> 17;
	*s ^= *s << 5;
	return *s;
}

/* Run briefly, then wait with a 1 ms timeout: either sleep or wait on a
 * semaphore that is never given. Every wait must end.
 */
static void victim_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	bool use_sem = (bool)(uintptr_t)p1;
	uint32_t s = 0x12345u;

	while (!timeout_test_stop) {
		k_busy_wait(xorshift(&s) % 20);
		if (use_sem) {
			(void)k_sem_take(&never_given, K_MSEC(1));
		} else {
			k_msleep(1);
		}
		victim_wakeups++;
	}
}

/* Keep suspending and resuming the victim, catching it running on another
 * CPU, about to wait, or waiting.
 */
static void suspender_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint32_t s = 0x9876u;

	while (!timeout_test_stop) {
		k_thread_suspend(&victim);
		k_thread_resume(&victim);
		k_busy_wait(xorshift(&s) % 50);
	}
}

/* The victim is waiting and has no timeout left to end the wait. */
static bool victim_lost_timeout(void)
{
	return (victim.base.thread_state & (_THREAD_SLEEPING | _THREAD_PENDING)) != 0U &&
	       k_thread_timeout_remaining_ticks(&victim) == 0;
}

static void check_suspend_keeps_timeout(bool use_sem)
{
	int64_t end = k_uptime_get() + TIMEOUT_TEST_MS;
	int64_t since = k_uptime_get();
	uint32_t last = 0;
	bool lost = false;

	timeout_test_stop = false;
	victim_wakeups = 0;
	k_thread_create(&victim, victim_stack, K_THREAD_STACK_SIZEOF(victim_stack),
			victim_entry, (void *)(uintptr_t)use_sem, NULL, NULL,
			K_PRIO_PREEMPT(5), 0, K_FOREVER);
	k_thread_create(&suspender, suspender_stack, K_THREAD_STACK_SIZEOF(suspender_stack),
			suspender_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_FOREVER);
	k_thread_start(&victim);
	k_thread_start(&suspender);
	timeout_threads_started = true;

	while (k_uptime_get() < end) {
		k_msleep(10);
		if (victim_wakeups != last) {
			last = victim_wakeups;
			since = k_uptime_get();
		} else if (k_uptime_get() - since > TIMEOUT_STALL_MS && victim_lost_timeout()) {
			lost = true;
			break;
		}
	}

	timeout_test_stop = true;
	k_thread_join(&suspender, K_FOREVER);
	/* The timeout may also have been lost within the last stall period:
	 * give the victim one more to make progress.
	 */
	if (!lost) {
		uint32_t before = victim_wakeups;

		k_msleep(TIMEOUT_STALL_MS);
		lost = victim_wakeups == before && victim_lost_timeout();
	}
	/* Aborting needs no wakeup, so a victim stuck without a timeout ends
	 * too.
	 */
	k_thread_abort(&victim);
	k_sem_reset(&never_given);

	zassert_false(lost, "%s: the victim lost its timeout after %u wakeups",
		      use_sem ? "k_sem_take()" : "k_msleep()", last);
}

/**
 * @brief Suspending a thread must not cancel its timeout
 *
 * @details k_thread_suspend() does not affect a timeout the target is
 * waiting on. Suspend and resume a thread running on another CPU while it
 * keeps entering a timed wait, and check that every wait still ends.
 */
ZTEST(smp_suspend_resume, test_smp_suspend_keeps_timeout)
{
	check_suspend_keeps_timeout(false);
	check_suspend_keeps_timeout(true);
}

/* Stop the timeout test's threads if a failed assertion ended it early. */
static void smp_suspend_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (timeout_threads_started) {
		timeout_test_stop = true;
		k_thread_abort(&suspender);
		k_thread_abort(&victim);
		timeout_threads_started = false;
	}
}

ZTEST_SUITE(smp_suspend_resume, NULL, NULL, NULL, smp_suspend_after, NULL);
