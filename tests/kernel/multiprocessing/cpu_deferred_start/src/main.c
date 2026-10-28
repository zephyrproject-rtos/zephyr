/* Copyright (c) 2021 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/smp.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>

/* Experimentally 10ms is enough time to get a secondary CPU to run on
 * all known platforms.
 */
#define CPU_START_DELAY 10000

/* IPIs happen  much faster than CPU startup */
#define CPU_IPI_DELAY 2500

BUILD_ASSERT(CONFIG_SMP);
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

#define CPU_DEFERRED_VALUE(node_id) DT_PROP_OR(node_id, zephyr_deferred_start, 0)

/* Per-CPU start deferral flags taken from the devicetree, built the same way
 * the kernel builds them in z_smp_init(): one entry per "cpu" child of /cpus,
 * indexed by logical CPU id.
 */
static const bool cpu_deferred[] = {DT_FOREACH_CPU_SEP(CPU_DEFERRED_VALUE, (,))};

/* The tests need at least one CPU that the kernel leaves for a run-time
 * start; the board overlays under boards/ provide it.
 */
BUILD_ASSERT((DT_FOREACH_CPU_SEP(CPU_DEFERRED_VALUE, (+)) 0) > 0,
	     "at least one cpu node needs zephyr,deferred-start for this test");

#define STACKSZ 2048

volatile bool mp_flag;

struct k_thread cpu_thr;
K_THREAD_STACK_DEFINE(thr_stack, STACKSZ);

/* Tracks the CPUs this test has brought up, so a test does not depend on
 * whether an earlier one already started the CPU it needs.
 */
static bool cpu_started[CONFIG_MP_MAX_NUM_CPUS];

volatile bool custom_init_flag;

static void thread_fn(void *a, void *b, void *c)
{
	mp_flag = true;
}

static void custom_init_fn(void *arg)
{
	volatile bool *flag = (void *)arg;

	*flag = true;
}

/* True if the devicetree defers the start of this CPU. CPUs beyond the cpu
 * nodes described in the devicetree cannot be deferred.
 */
static bool cpu_is_deferred(unsigned int cpu)
{
	return (cpu < ARRAY_SIZE(cpu_deferred)) && cpu_deferred[cpu];
}

/* Returns the id of the n-th secondary CPU whose start is deferred, or -1 if
 * there are fewer than n + 1 of them.
 */
static int nth_deferred_cpu(unsigned int n)
{
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 1; i < num_cpus; i++) {
		if (!cpu_is_deferred(i)) {
			continue;
		}

		if (n == 0) {
			return (int)i;
		}
		n--;
	}

	return -1;
}

static void start_cpu_once(int cpu, smp_init_fn fn, void *arg)
{
	if (cpu_started[cpu]) {
		return;
	}

	k_smp_cpu_start(cpu, fn, arg);
	cpu_started[cpu] = true;
}

/* How the test thread waits for the pinned thread to run. */
enum wait_mode {
	/* Sleep between polls. The thread under observation is pinned, so it
	 * cannot follow the test thread onto another CPU, and sleeping keeps
	 * the check from depending on how the host schedules an emulated CPU.
	 */
	WAIT_SLEEPING,
	/* Never give up the CPU. The target CPU has nothing to run and no
	 * reschedule point of its own, so it can only pick the thread up if
	 * it is signalled with an IPI.
	 */
	WAIT_SPINNING,
};

/* Pins a thread to the given CPU and reports whether that CPU ran it within
 * the timeout. The thread is of lower priority than the test thread, so it
 * can only run on the CPU it is pinned to.
 */
static bool cpu_runs_thread(unsigned int cpu, uint32_t timeout_us, enum wait_mode mode)
{
	k_tid_t thr;
	bool ran;

	mp_flag = false;

	thr = k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			      thread_fn, NULL, NULL, NULL,
			      1, 0, K_FOREVER);
	(void)k_thread_cpu_pin(thr, cpu);
	k_thread_start(thr);

	if (mode == WAIT_SPINNING) {
		ran = WAIT_FOR(mp_flag, timeout_us, k_busy_wait(10));
	} else {
		ran = WAIT_FOR(mp_flag, timeout_us, k_msleep(1));
	}

	k_thread_abort(thr);
	k_thread_join(thr, K_FOREVER);

	return ran;
}

/**
 * @brief Verify that the boot topology follows the devicetree deferral flags.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * A CPU whose devicetree node carries zephyr,deferred-start must be left
 * un-started by z_smp_init(), while every other secondary CPU must be brought
 * up during boot. Deferral is per CPU, so both kinds may be present in the
 * same system and each has to be checked individually. A thread pinned to a
 * CPU can only run if that CPU is scheduling, which makes the distinction
 * observable without any architecture-specific query.
 *
 * Test steps:
 * - For every secondary CPU, create a lower priority thread, pin it to that
 *   CPU and start it.
 * - Wait for the thread to run, up to the CPU start delay.
 * - Compare the outcome against the CPU's devicetree deferral flag.
 *
 * Expected result:
 * - Threads pinned to a deferred CPU do not run.
 * - Threads pinned to any other secondary CPU do run, as the kernel started
 *   those CPUs at boot.
 *
 * @see k_thread_cpu_pin()
 */
ZTEST(cpu_deferred_start, test_cpu_deferred_start_boot_topology)
{
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 1; i < num_cpus; i++) {
		bool ran = cpu_runs_thread(i, CPU_START_DELAY, WAIT_SLEEPING);

		if (cpu_is_deferred(i)) {
			zassert_false(ran,
				      "deferred CPU%u ran a thread before being started",
				      i);
		} else {
			zassert_true(ran, "CPU%u was not started at boot", i);
		}
	}
}

/**
 * @brief Verify that a deferred CPU can be brought up at run time.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * A CPU the kernel skipped at boot must become a fully scheduling CPU once
 * the application starts it with k_smp_cpu_start(). The observed thread is
 * pinned to that CPU, so it running at all means the CPU picked it up.
 *
 * Test steps:
 * - Pick the first secondary CPU whose start the devicetree defers.
 * - Bring it up with k_smp_cpu_start() and no initialization function.
 * - Create a lower priority thread, pin it to that CPU and start it.
 * - Wait for the thread to run, up to the CPU start delay.
 *
 * Expected result:
 * - The pinned thread runs, so the CPU started and is scheduling threads.
 *
 * @see k_smp_cpu_start()
 */
ZTEST(cpu_deferred_start, test_cpu_deferred_start_runtime_bringup)
{
	int cpu = nth_deferred_cpu(0);

	zassert_true(cpu > 0, "no deferred secondary CPU to start");

	start_cpu_once(cpu, NULL, NULL);

	zassert_true(cpu_runs_thread(cpu, CPU_START_DELAY, WAIT_SLEEPING),
		     "CPU%d did not start", cpu);
}

/**
 * @brief Verify that IPIs are set up on a CPU started at run time.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Starting a CPU late must leave it as well connected as one started at
 * boot, which means its interprocessor interrupts have to work: a thread
 * created afterwards has to be dispatched to it promptly rather than waiting
 * for a timer tick. Unlike the other cases here the test thread spins instead
 * of sleeping, so it offers the system no reschedule point of its own and the
 * target CPU can only learn about the new thread from an IPI. The bound is
 * the much shorter IPI delay rather than the CPU start delay.
 *
 * Test steps:
 * - Pick the first secondary CPU whose start the devicetree defers, and make
 *   sure it has been brought up.
 * - Create a lower priority thread, pin it to that CPU and start it.
 * - Spin, without ever yielding, for at most the IPI delay.
 *
 * Expected result:
 * - The pinned thread runs within the IPI delay, so the run-time-started CPU
 *   was signalled rather than reaching the thread on its next tick.
 *
 * @see k_smp_cpu_start()
 */
ZTEST(cpu_deferred_start, test_cpu_deferred_start_ipi_ready)
{
	int cpu = nth_deferred_cpu(0);

	zassert_true(cpu > 0, "no deferred secondary CPU to start");

	start_cpu_once(cpu, NULL, NULL);

	zassert_true(cpu_runs_thread(cpu, CPU_IPI_DELAY, WAIT_SPINNING),
		     "CPU%d did not run thread via IPI", cpu);
}

/**
 * @brief Verify that a caller-supplied function runs on the started CPU.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * k_smp_cpu_start() takes an optional initialization function and argument,
 * which the target CPU must invoke before it begins scheduling threads. The
 * function sets a flag through the argument pointer, so a pass proves it ran
 * and was given the right argument. A second deferred CPU is used so the
 * start is genuinely the first one for that CPU; the test is skipped when the
 * configuration provides only one deferred CPU.
 *
 * Test steps:
 * - Pick the second secondary CPU whose start the devicetree defers, skipping
 *   the test if there is none.
 * - Confirm a thread pinned to it does not run while it is still un-started.
 * - Start it with k_smp_cpu_start(), passing the initialization function and
 *   the address of a flag.
 * - Wait for a pinned thread to run, then check the flag.
 *
 * Expected result:
 * - The pinned thread runs only after the CPU is started, and the
 *   initialization function was invoked on that CPU with the given argument.
 *
 * @see k_smp_cpu_start()
 */
ZTEST(cpu_deferred_start, test_cpu_deferred_start_custom_init)
{
	int cpu = nth_deferred_cpu(1);

	if (cpu < 0) {
		/* Only one deferred CPU, and it has already been started by
		 * the run-time bring-up test.
		 */
		ztest_test_skip();
	}

	custom_init_flag = false;

	zassert_false(cpu_runs_thread(cpu, CPU_START_DELAY, WAIT_SLEEPING),
		      "CPU%d must not be running yet", cpu);

	start_cpu_once(cpu, custom_init_fn, (void *)&custom_init_flag);

	zassert_true(cpu_runs_thread(cpu, CPU_START_DELAY, WAIT_SLEEPING),
		     "CPU%d did not start", cpu);

	zassert_true(custom_init_flag,
		     "custom init function has not been called");
}

ZTEST_SUITE(cpu_deferred_start, NULL, NULL, NULL, NULL, NULL);
