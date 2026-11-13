/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver-agnostic contract tests for the system clock driver.
 *
 * These tests target the sys_clock_set_timeout() / sys_clock_announce()
 * contract rather than any particular driver. They deliberately concentrate
 * on the state the rest of the test tree barely reaches: an *empty* kernel
 * timeout queue. Nearly every timer test in tests/kernel keeps a timeout
 * armed at all times, which hides driver bugs that only bite once the kernel
 * stops asking for a deadline -- at that point sys_clock_announce() passes
 * SYS_CLOCK_MAX_WAIT and most drivers stop reprogramming their compare
 * register entirely, so a bad value programmed just beforehand is never
 * corrected.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

/* Length of the busy window used to check that the tick keeps advancing. */
#define UPTIME_WINDOW_US 50000U

/*
 * Number of sub-tick offsets swept when aborting the last pending timeout.
 * The offsets cover slightly more than one whole tick so that the abort (and
 * the sys_clock_set_timeout() it triggers) lands at every phase relative to
 * the driver's compare register.
 */
#define SWEEP_STEPS 64U

/* A timeout far enough out that it never expires during the sweep. */
#define SWEEP_TIMEOUT_MS 10000

/*
 * How much slower a fixed amount of work is allowed to be with an empty
 * timeout queue than it is with a timeout armed. A driver that leaves the CPU
 * stuck in its tick ISR blows past this by orders of magnitude; ordinary
 * measurement noise does not come close.
 */
#define SLOWDOWN_TOLERANCE 8

/*
 * Upper bound on the spin used to align to a tick edge. Any real free-running
 * clock advances long before this; a platform whose clock only moves when the
 * kernel asks for a timeout never will, and must not hang here.
 */
#define TICK_EDGE_SPIN_LIMIT 10000000UL

/* Ceiling on the calibrated work loop, to bound the runtime of this test. */
#define WORK_ITERATIONS_MAX (1UL << 26)

/* Kept volatile so the calibrated work loop cannot be optimized away. */
static volatile uint32_t work_sink;

static struct k_timer sweep_timer;

/*
 * Two very different situations can leave the suite without a usable work
 * loop, and they deserve different verdicts. A clock that does not advance
 * while the CPU spins with a 1-tick periodic timeout armed is a driver bug
 * in its own right -- the most serious one this suite can detect -- and is
 * asserted, not skipped. A CPU that runs WORK_ITERATIONS_MAX increments in
 * under 4 ticks is merely too fast for tick-granularity measurement, which
 * is no fault of the driver, so the measurement tests skip.
 */
enum calibration_result {
	CALIBRATION_OK,
	CALIBRATION_CLOCK_STUCK,
	CALIBRATION_WORK_TOO_FAST,
};

static enum calibration_result calibration;

/* Number of work_sink increments that takes at least a few ticks. */
static uint32_t work_iterations;

static bool wait_tick_edge(k_ticks_t *edge)
{
	k_ticks_t start = k_uptime_ticks();

	for (unsigned long spins = 0; spins < TICK_EDGE_SPIN_LIMIT; spins++) {
		k_ticks_t now = k_uptime_ticks();

		if (now != start) {
			*edge = now;
			return true;
		}
	}

	return false;
}

static k_ticks_t ticks_for_work(uint32_t iterations)
{
	k_ticks_t edge;

	/* Start on a tick edge so the measurement is not truncated. */
	if (!wait_tick_edge(&edge)) {
		return -1;
	}

	for (uint32_t i = 0; i < iterations; i++) {
		work_sink++;
	}

	return k_uptime_ticks() - edge;
}

/*
 * Size the work loop so that it spans a few ticks: long enough that the
 * measurement is not dominated by tick quantization, short enough that the
 * sweep stays quick. Calibrated with a periodic timeout armed, which is both
 * the slower and the better-covered configuration, so the resulting budget is
 * conservative.
 *
 * Deliberately called from the test bodies rather than from the suite setup:
 * the loop must be sized under the same conditions it is later measured under.
 * Emulated SMP targets in particular can execute this loop orders of magnitude
 * faster before the first test starts than they do inside it, and calibrating
 * there sizes the loop for the wrong machine -- every subsequent measurement
 * then costs seconds instead of milliseconds.
 */
static void calibrate_work(void)
{
	struct k_timer calib_timer;

	calibration = CALIBRATION_WORK_TOO_FAST;

	k_timer_init(&calib_timer, NULL, NULL);
	k_timer_start(&calib_timer, K_TICKS(1), K_TICKS(1));

	for (uint32_t iterations = 1024; iterations <= WORK_ITERATIONS_MAX; iterations *= 2) {
		k_ticks_t taken = ticks_for_work(iterations);

		if (taken < 0) {
			calibration = CALIBRATION_CLOCK_STUCK;
			break;
		}
		if (taken >= 4) {
			work_iterations = iterations;
			calibration = CALIBRATION_OK;
			break;
		}
	}

	k_timer_stop(&calib_timer);
}

static void *system_clock_setup(void)
{
	k_timer_init(&sweep_timer, NULL, NULL);
	return NULL;
}

/* Calibrate once, on first use from a test body. */
static void ensure_calibrated(void)
{
	static bool done;

	if (!done) {
		calibrate_work();
		done = true;
	}
}

static void assert_clock_not_stuck(void)
{
	ensure_calibrated();

	zassert_true(calibration != CALIBRATION_CLOCK_STUCK,
		     "clock did not advance while the CPU spun with a 1-tick "
		     "periodic timeout armed");
}

static void skip_unless_calibrated(void)
{
	assert_clock_not_stuck();

	if (calibration != CALIBRATION_OK) {
		TC_PRINT("CPU ran %lu work iterations in under 4 ticks; "
			 "too fast for tick-granularity measurement\n",
			 WORK_ITERATIONS_MAX);
		ztest_test_skip();
	}
}

/*
 * ticks_for_work() on the calibrated loop, asserting that a tick edge was
 * found: a clock that stops advancing mid-test must fail the measurement
 * rather than feed -1 into a budget comparison it would trivially satisfy.
 */
static k_ticks_t measure_work(void)
{
	k_ticks_t taken = ticks_for_work(work_iterations);

	zassert_true(taken >= 0, "clock stopped advancing during the measurement");

	return taken;
}

ZTEST_SUITE(system_clock_driver, NULL, system_clock_setup, NULL, NULL, NULL);

/**
 * @brief System clock driver contract tests
 * @defgroup drivers_timer_system_clock_tests System clock driver
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify that the tick keeps advancing while no timeout is armed.
 *
 * @details
 * A tickless driver is told "no deadline" once the kernel timeout queue
 * drains. It must still account for the passage of time, so that uptime read
 * after a purely cycle-counter-driven busy wait matches the wall time that
 * actually elapsed. A driver that stops the clock, or that announces a
 * bogus number of ticks in this state, shows up here as uptime that has
 * drifted away from the busy wait.
 *
 * Test steps:
 * - Let the timeout queue drain (the test thread is cooperative and arms
 *   nothing).
 * - Sample k_uptime_ticks(), busy wait a fixed interval, sample again.
 *
 * Expected result:
 * - The uptime delta matches the busy wait to within one tick either way.
 *
 * @see k_uptime_ticks()
 * @see k_busy_wait()
 */
ZTEST(system_clock_driver, test_system_clock_uptime_advances_without_timeouts)
{
	k_ticks_t expected = k_us_to_ticks_floor64(UPTIME_WINDOW_US);
	k_ticks_t start;
	k_ticks_t delta;

	/*
	 * A stuck clock would make the k_busy_wait() below spin forever on
	 * some platforms; fail with a verdict instead of hanging.
	 */
	assert_clock_not_stuck();

	start = k_uptime_ticks();
	k_busy_wait(UPTIME_WINDOW_US);
	delta = k_uptime_ticks() - start;

	zassert_true(delta >= (expected - 1),
		     "uptime advanced %lld ticks over a %u us busy wait, expected ~%lld",
		     (long long)delta, UPTIME_WINDOW_US, (long long)expected);
	zassert_true(delta <= (expected + 1),
		     "uptime advanced %lld ticks over a %u us busy wait, expected ~%lld",
		     (long long)delta, UPTIME_WINDOW_US, (long long)expected);
}

/**
 * @brief Verify that an empty timeout queue does not starve the CPU.
 *
 * @details
 * Compares how long a fixed amount of work takes with a periodic timeout
 * armed against how long the same work takes once the queue is empty. The
 * empty-queue case exercises less driver work per tick, so it can only be
 * faster; a large slowdown means the driver is spending the CPU on something
 * else, the classic case being a compare register programmed such that the
 * tick interrupt re-fires immediately and the CPU never leaves the ISR.
 *
 * Test steps:
 * - Time the calibrated work loop with a 1-tick periodic timeout armed.
 * - Stop the timer so the timeout queue drains.
 * - Time the same work loop again.
 *
 * Expected result:
 * - The empty-queue run is not dramatically slower than the armed run.
 *
 * @see sys_clock_set_timeout()
 * @see k_timer_stop()
 */
ZTEST(system_clock_driver, test_system_clock_empty_queue_does_not_starve_cpu)
{
	struct k_timer timer;
	k_ticks_t armed;
	k_ticks_t idle;

	skip_unless_calibrated();

	k_timer_init(&timer, NULL, NULL);

	k_timer_start(&timer, K_TICKS(1), K_TICKS(1));
	armed = measure_work();
	k_timer_stop(&timer);

	idle = measure_work();

	zassert_true(idle <= (armed * SLOWDOWN_TOLERANCE),
		     "work took %lld ticks with an empty timeout queue but only "
		     "%lld ticks with a timeout armed",
		     (long long)idle, (long long)armed);
}

/**
 * @brief Verify that aborting the last timeout leaves a usable deadline.
 *
 * @details
 * Aborting the head of the timeout queue makes the kernel reprogram the
 * driver (z_try_abort_timeout()), and when that was the only pending timeout
 * the queue is empty immediately afterwards -- so whatever the driver
 * programs at that moment is the last value it will write until something
 * arms a timeout again. A driver that computes its compare value from the
 * current counter can land on a degenerate value here, and the sub-tick
 * phase at which the abort happens decides whether it does.
 *
 * The abort is therefore swept across a whole tick period, and after each
 * abort the CPU is checked to still be making progress.
 *
 * Test steps:
 * - For each sub-tick offset in the sweep: arm a distant timeout, busy wait
 *   that offset, then abort the timeout, leaving the queue empty.
 * - Time the calibrated work loop after each abort.
 *
 * Expected result:
 * - Every post-abort run completes in a comparable number of ticks.
 *
 * @see k_timer_start()
 * @see k_timer_stop()
 * @see sys_clock_set_timeout()
 */
ZTEST(system_clock_driver, test_system_clock_last_timeout_abort_phase_sweep)
{
	const uint32_t tick_us = k_ticks_to_us_ceil32(1);
	const uint32_t step_us = MAX(tick_us / SWEEP_STEPS, 1U);
	k_ticks_t budget;
	k_ticks_t baseline;

	skip_unless_calibrated();

	baseline = measure_work();
	budget = baseline * SLOWDOWN_TOLERANCE;

	for (uint32_t i = 0; i < SWEEP_STEPS; i++) {
		k_ticks_t taken;

		k_timer_start(&sweep_timer, K_MSEC(SWEEP_TIMEOUT_MS), K_NO_WAIT);

		/* Land the abort at a different phase within the tick each time. */
		k_busy_wait(i * step_us);

		k_timer_stop(&sweep_timer);

		taken = measure_work();

		zassert_true(taken <= budget,
			     "work took %lld ticks (budget %lld) after aborting the "
			     "last timeout at sub-tick offset %u us",
			     (long long)taken, (long long)budget, i * step_us);
	}
}

/**
 * @}
 */
