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

/* How long to wait for a CPU to act on an IPI. The IPI checks leave the CPU
 * no other way to reschedule, so a lost IPI cannot pass as a slow one: this
 * only turns it into a failure instead of a hang, and is generous to allow
 * for an emulated CPU that the host does not schedule promptly.
 */
#define CPU_IPI_DELAY 100000

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
BUILD_ASSERT(DT_FOREACH_CPU_SEP(CPU_DEFERRED_VALUE, (+)) > 0,
	     "at least one cpu node needs zephyr,deferred-start for this test");

#define STACKSZ 2048

volatile bool mp_flag;

struct k_thread cpu_thr;
K_THREAD_STACK_DEFINE(thr_stack, STACKSZ);

static struct k_thread probe_thr;
K_THREAD_STACK_DEFINE(probe_stack, STACKSZ);

static struct k_thread spinner_thr;
K_THREAD_STACK_DEFINE(spinner_stack, STACKSZ);

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
	/* Never give up the CPU. Offers the system no reschedule point, so
	 * the target CPU can only pick the thread up if it is signalled with
	 * an IPI. The caller must not be running on the target CPU, or the
	 * pinned thread is starved instead of observed.
	 */
	WAIT_SPINNING,
};

/* Creates, without starting it, a thread pinned to the given CPU. It is of
 * lower priority than the test thread, so it can only run on that CPU.
 */
static k_tid_t pinned_thread_create(unsigned int cpu)
{
	k_tid_t thr;

	mp_flag = false;

	thr = k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			      thread_fn, NULL, NULL, NULL,
			      1, 0, K_FOREVER);
	(void)k_thread_cpu_pin(thr, cpu);

	return thr;
}

/* Starts a pinned thread and reports whether it ran within the timeout. */
static bool pinned_thread_runs(k_tid_t thr, uint32_t timeout_us, enum wait_mode mode)
{
	bool ran;

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

/* Reports whether the given CPU runs a thread pinned to it within the
 * timeout.
 */
static bool cpu_runs_thread(unsigned int cpu, uint32_t timeout_us, enum wait_mode mode)
{
	return pinned_thread_runs(pinned_thread_create(cpu), timeout_us, mode);
}

/* Keeps a CPU busy with a thread of lower priority than the one under
 * observation, so that the CPU is never idle. The thread counts its loop
 * iterations, which lets another CPU confirm that the target CPU is running
 * it at that very moment.
 */
static volatile uint32_t spinner_beats;
static volatile bool spinner_stop;

static void spinner_fn(void *a, void *b, void *c)
{
	while (!spinner_stop) {
		spinner_beats++;
		k_busy_wait(10);
	}
}

static bool spinner_advances(uint32_t timeout_us)
{
	uint32_t beats = spinner_beats;

	return WAIT_FOR(spinner_beats != beats, timeout_us, k_busy_wait(10));
}

struct ipi_probe {
	unsigned int cpu;
	bool spinning;
	bool ran;
};

/* Runs pinned to CPU 0, which is never the target CPU, while the test thread
 * is blocked. The test thread cannot pin itself and may be running on the
 * target CPU when it starts the probe: seeing the spinner advance proves
 * that the target CPU has been handed over to the spinner, and the blocked
 * test thread cannot come back to it. The thread under observation is
 * created before that, as creating a thread maps its stack, which on an MMU
 * platform interrupts every other CPU for a TLB shootdown: by the time the
 * spinner is seen to advance that interrupt has been served, and starting
 * the thread is all that is left to do.
 */
static void ipi_probe_fn(void *a, void *b, void *c)
{
	struct ipi_probe *probe = a;
	k_tid_t thr = pinned_thread_create(probe->cpu);

	probe->spinning = spinner_advances(CPU_IPI_DELAY);
	if (probe->spinning) {
		probe->ran = pinned_thread_runs(thr, CPU_IPI_DELAY, WAIT_SPINNING);
	} else {
		k_thread_abort(thr);
		k_thread_join(thr, K_FOREVER);
	}
}

/* Checks that the given CPU acts on IPIs: first to run the spinner, then to
 * preempt the spinner for a newly started thread of higher priority. No
 * timeout is pending anywhere and time slicing is disabled, so no timer
 * interrupt can reach the CPU meanwhile and the IPI that comes with starting
 * the thread is the only thing that can make it reschedule.
 */
static void cpu_ipi_check(unsigned int cpu, struct ipi_probe *probe)
{
	k_tid_t spinner, thr;

	*probe = (struct ipi_probe){.cpu = cpu};

	spinner_stop = false;
	spinner = k_thread_create(&spinner_thr, spinner_stack, K_THREAD_STACK_SIZEOF(spinner_stack),
				  spinner_fn, NULL, NULL, NULL,
				  2, 0, K_FOREVER);
	(void)k_thread_cpu_pin(spinner, cpu);
	k_thread_start(spinner);

	thr = k_thread_create(&probe_thr, probe_stack, K_THREAD_STACK_SIZEOF(probe_stack),
			      ipi_probe_fn, probe, NULL, NULL,
			      0, 0, K_FOREVER);
	(void)k_thread_cpu_pin(thr, 0);
	k_thread_start(thr);
	k_thread_join(thr, K_FOREVER);

	spinner_stop = true;
	k_thread_join(spinner, K_FOREVER);
}

/* Which secondary CPUs were scheduling threads when the suite started, i.e.
 * before any test case had the chance to start one. Taken once in the suite
 * setup so the check does not depend on the order the cases run in, and
 * kept across suite repetitions as it can only be taken before the first
 * k_smp_cpu_start().
 */
struct cpu_deferred_start_fixture {
	bool ran_at_boot[CONFIG_MP_MAX_NUM_CPUS];
};

static struct cpu_deferred_start_fixture fixture_data;
static bool boot_snapshot_taken;

static void *cpu_deferred_start_setup(void)
{
	unsigned int num_cpus = arch_num_cpus();

	if (!boot_snapshot_taken) {
		for (unsigned int i = 1; i < num_cpus; i++) {
			fixture_data.ran_at_boot[i] =
				cpu_runs_thread(i, CPU_START_DELAY, WAIT_SLEEPING);
		}
		boot_snapshot_taken = true;
	}

	return &fixture_data;
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
 * observable without any architecture-specific query. The observation is
 * made by the suite setup, before any case can start a CPU, so the result
 * does not depend on the order the cases run in.
 *
 * Test steps:
 * - In the suite setup, for every secondary CPU, create a lower priority
 *   thread, pin it to that CPU and start it; record whether it ran within
 *   the CPU start delay.
 * - Compare the recorded outcome against each CPU's devicetree deferral
 *   flag.
 *
 * Expected result:
 * - Threads pinned to a deferred CPU did not run.
 * - Threads pinned to any other secondary CPU did run, as the kernel
 *   started those CPUs at boot.
 *
 * @see k_thread_cpu_pin()
 */
ZTEST_F(cpu_deferred_start, test_cpu_deferred_start_boot_topology)
{
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 1; i < num_cpus; i++) {
		if (cpu_is_deferred(i)) {
			zassert_false(fixture->ran_at_boot[i],
				      "deferred CPU%u ran a thread before being started",
				      i);
		} else {
			zassert_true(fixture->ran_at_boot[i],
				     "CPU%u was not started at boot", i);
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
 * created afterwards has to be dispatched to it by an IPI rather than
 * whenever the CPU next reschedules on its own. To rule the latter out, the
 * CPU is kept busy with a lower priority thread that never yields while no
 * timeout is pending and time slicing is disabled, so no timer interrupt
 * reaches it and an IPI is the only way for it to learn about the new
 * thread. The new thread is started from a probe thread pinned to CPU 0,
 * since the test thread may itself be running on the target CPU; the probe
 * first waits for the busy thread to advance, which proves the target CPU
 * has been handed over to it.
 *
 * Test steps:
 * - Pick the first secondary CPU whose start the devicetree defers, and make
 *   sure it has been brought up.
 * - Wait for a thread pinned to it to run, up to the CPU start delay.
 * - Pin a lower priority thread to it that spins until told to stop,
 *   counting its iterations.
 * - From a probe thread pinned to CPU 0, wait for the count to advance, then
 *   create a thread of higher priority than the spinning one, pin it to the
 *   target CPU and start it.
 * - Spin, without ever yielding, until the thread has run.
 *
 * Expected result:
 * - The spinning thread runs on the CPU.
 * - The pinned thread runs while the spinning thread never yields, so the
 *   CPU was signalled with an IPI.
 *
 * @see k_smp_cpu_start()
 */
ZTEST(cpu_deferred_start, test_cpu_deferred_start_ipi_ready)
{
	int cpu = nth_deferred_cpu(0);
	struct ipi_probe probe;

	zassert_true(cpu > 0, "no deferred secondary CPU to start");

	start_cpu_once(cpu, NULL, NULL);

	zassert_true(cpu_runs_thread(cpu, CPU_START_DELAY, WAIT_SLEEPING),
		     "CPU%d did not start", cpu);

	cpu_ipi_check(cpu, &probe);

	zassert_true(probe.spinning, "CPU%d did not run the spinning thread", cpu);
	zassert_true(probe.ran, "CPU%d did not run thread via IPI", cpu);
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
 * start is genuinely the first one for that CPU, whatever the other cases
 * have done with the first one; the test is skipped when the configuration
 * provides only one deferred CPU, or when the CPU has already been started
 * by an earlier run of this case.
 *
 * Test steps:
 * - Pick the second secondary CPU whose start the devicetree defers, skipping
 *   the test if there is none or if it has already been started.
 * - Confirm the suite setup saw it un-started.
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
ZTEST_F(cpu_deferred_start, test_cpu_deferred_start_custom_init)
{
	int cpu = nth_deferred_cpu(1);

	if (cpu < 0) {
		/* The only deferred CPU is the one the other cases bring up,
		 * so a first start with an init function cannot be observed.
		 */
		ztest_test_skip();
	}

	if (cpu_started[cpu]) {
		/* Repeated run of the suite: the first start has already
		 * been observed and cannot happen again.
		 */
		ztest_test_skip();
	}

	custom_init_flag = false;

	zassert_false(fixture->ran_at_boot[cpu],
		      "CPU%d must not be running before it is started", cpu);

	start_cpu_once(cpu, custom_init_fn, (void *)&custom_init_flag);

	zassert_true(cpu_runs_thread(cpu, CPU_START_DELAY, WAIT_SLEEPING),
		     "CPU%d did not start", cpu);

	zassert_true(custom_init_flag,
		     "custom init function has not been called");
}

ZTEST_SUITE(cpu_deferred_start, NULL, cpu_deferred_start_setup, NULL, NULL, NULL);
