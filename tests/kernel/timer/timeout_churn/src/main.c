/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/**
 * @brief Kernel timeout churn tests
 * @defgroup kernel_timeout_churn_tests Kernel timeout churn
 * @ingroup all_tests
 * @{
 *
 * Regression tests for system clock correctness under timeout-queue churn
 * and near-boundary timer reprogramming. The kernel re-evaluates its
 * earliest timeout on every add and abort, so real workloads (time-slice
 * resets, poll loops) produce dense streams of driver reprograms, and a
 * deadline may land arbitrarily close to "now" when a timeout is armed
 * just before a tick boundary. Tickless timer drivers must survive both
 * without fabricating, losing, or freezing time.
 *
 * Neither corner is exercised by the classic timer tests, whose arms are
 * paced by sleeps or driven from the timer ISR at comfortable distances.
 * Both corners harbored real driver regressions that were only caught
 * downstream by networking and gdbstub suites: a drift-compensation bug
 * fabricated a full timer period per back-to-back reprogram pair, and an
 * equality-match catch-up loop livelocked on any deadline landing within
 * its minimum-delay window.
 *
 * The suite runs in two QEMU timing regimes (see tests.yaml). Under
 * icount, virtual time is instruction-driven and the near-boundary sweep
 * is deterministic. With icount disabled (the regime every networking
 * test runs in), virtual time follows the wall clock and emulated timers
 * expose reprogramming windows (e.g. QEMU's SysTick holds VAL at zero
 * for the 10 us timer floor after a write) that only wall-clock timing
 * can reach; the churn cases reproduce the fabricated-time class there.
 */

/* Reprogram pairs for the churn cases. Restarting a running k_timer is the
 * tightest reprogram pair the kernel can produce: the abort empties the
 * timeout queue (the kernel arms the driver as far out as it can) and the
 * add re-arms it near, both within one locked operation a few dozen
 * instructions apart.
 */
#define CHURN_PAIRS 1000

/* A single start/stop pair is microseconds of work; any single-pair uptime
 * jump at or above this is fabricated time (one fabricated timer period is
 * already hundreds of milliseconds on common configs), not execution time.
 */
#define JUMP_MS 100

/* Host stalls on a loaded emulation host can also produce a large jump, so
 * tolerate a couple of isolated ones; a fabricating clock produces several
 * per churn burst.
 */
#define JUMPS_ALLOWED 2

/* Gross runaway backstop for the uptime consumed by the reprogram pairs
 * alone (the loop's periodic announce-enabling sleeps are excluded).
 */
#define CHURN_UPTIME_BOUND_MS 5000

/* Generous timeout for the expiry test: the partner's churn burst plus the
 * semaphore give complete in a small fraction of this even on a slow host.
 */
#define EXPIRY_TIMEOUT_MS 2000

/* Near-boundary sweep: number of phase-swept one-shot arms, and the uptime
 * budget for each to fire. Each iteration arms a timeout on the next tick
 * boundary from a stepped phase within the tick, so the reprogram distance
 * seen by the driver sweeps down to almost zero.
 */
#define SWEEP_ARMS 400
#define SWEEP_FIRE_BOUND_MS (5 * MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC + 10)

static K_TIMER_DEFINE(churn_timer, NULL, NULL);
static struct k_sem done_sem;
static K_THREAD_STACK_DEFINE(partner_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread partner_thread;

/* Force back-to-back driver reprograms 'pairs' times, without ever
 * blocking. Restarting a running timer is the tightest reprogram pair the
 * kernel can produce: the abort empties the timeout queue (the kernel arms
 * the driver as far out as it can) and the add re-arms it near, both
 * within one locked operation a few dozen instructions apart.
 */
static void churn(int pairs)
{
	for (int i = 0; i < pairs; i++) {
		k_timer_start(&churn_timer, K_MSEC(100), K_NO_WAIT);
	}
	k_timer_stop(&churn_timer);
}

static void partner_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	churn(CHURN_PAIRS);
	k_sem_give(&done_sem);
}

/**
 * @brief Verify that timeout add/abort churn does not fabricate time.
 *
 * @details
 * A timer restart is microseconds of non-blocking work, so it may never
 * advance uptime by a macroscopic step. The test hammers the driver with
 * restarts of one running k_timer, each an abort (the kernel arms the
 * driver far) plus an add (it re-arms near) inside one locked operation,
 * and lets an announce through every few dozen restarts, as any real
 * workload would. A driver that misaccounts its counter around
 * reprogramming credits a whole timer period per reprogram pair, which
 * shows up as discrete jumps of hundreds of milliseconds. A small number
 * of large jumps is tolerated so a preempted emulation host does not
 * fail the test.
 *
 * Test steps:
 * - Run CHURN_PAIRS restarts of a running k_timer, measuring the uptime
 *   consumed by each, with a one-tick sleep every 50 restarts.
 * - Count restarts that consumed JUMP_MS or more.
 *
 * Expected result:
 * - At most JUMPS_ALLOWED pairs show a large uptime jump, and the whole
 *   loop stays under CHURN_UPTIME_BOUND_MS.
 *
 * @see k_timer_start()
 * @see k_timer_stop()
 * @see k_uptime_get()
 */
ZTEST(timeout_churn, test_timeout_churn_uptime_bounded)
{
	int64_t total = 0;
	int jumps = 0;

	k_timer_start(&churn_timer, K_MSEC(500), K_NO_WAIT);

	for (int i = 0; i < CHURN_PAIRS && jumps <= 2 * JUMPS_ALLOWED; i++) {
		int64_t t0 = k_uptime_get();

		/* Restart of a running timer: abort + add in one operation. */
		k_timer_start(&churn_timer, K_MSEC(500), K_NO_WAIT);

		int64_t delta = k_uptime_get() - t0;

		if (delta >= JUMP_MS) {
			jumps++;
		}
		total += delta;

		/* Let an announce through periodically: fabrication paths in
		 * several drivers are self-limiting until the next announce
		 * resets their accounting, and real workloads always announce
		 * eventually. The sleep itself is excluded from the budget.
		 */
		if ((i % 50) == 49) {
			k_sleep(K_TICKS(1));
		}
	}
	k_timer_stop(&churn_timer);

	zassert_true(jumps <= JUMPS_ALLOWED,
		     "%d churn pairs fabricated >= %d ms of uptime each", jumps,
		     JUMP_MS);
	zassert_true(total < CHURN_UPTIME_BOUND_MS,
		     "non-blocking churn consumed %lld ms of uptime", total);
}

/**
 * @brief Verify a generous timeout does not expire before a ready
 *        thread's bounded work.
 *
 * @details
 * A thread blocking with a timeout must not be woken by that timeout
 * while a lower-priority thread that will satisfy the wait is runnable
 * and needs only microseconds of CPU. The partner thread runs a timeout
 * churn burst and then gives the semaphore; the churn must not fabricate
 * enough elapsed time to fire the waiter's timeout first. This is the
 * distilled form of a networking-test failure where poll() timed out in
 * milliseconds of real time because timer reprogramming during
 * time-slice churn advanced the clock by dozens of ticks.
 *
 * Test steps:
 * - Spawn a lower-priority partner thread that churns the timeout queue
 *   and then gives a semaphore.
 * - Block on the semaphore with a generous timeout, which lets the
 *   partner run.
 *
 * Expected result:
 * - k_sem_take() returns 0 (semaphore given), not -EAGAIN (timeout).
 *
 * @see k_sem_take()
 * @see k_timer_start()
 * @see k_timer_stop()
 */
ZTEST(timeout_churn, test_timeout_churn_no_spurious_expiry)
{
	int ret;

	k_sem_init(&done_sem, 0, 1);

	k_thread_create(&partner_thread, partner_stack,
			K_THREAD_STACK_SIZEOF(partner_stack), partner_fn,
			NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	ret = k_sem_take(&done_sem, K_MSEC(EXPIRY_TIMEOUT_MS));
	zassert_equal(ret, 0,
		      "timeout fired before the ready partner thread ran (%d)",
		      ret);

	k_thread_join(&partner_thread, K_FOREVER);
}

/**
 * @brief Verify timers armed arbitrarily close to a tick boundary fire.
 *
 * @details
 * A zero-duration k_timer expires on the next tick boundary, so the
 * reprogram distance the driver sees equals the remaining part of the
 * current tick at arm time. Stepping the arm phase through the tick
 * sweeps that distance down to almost nothing, deterministically driving
 * the driver's minimum-delay/catch-up handling that normal workloads hit
 * only by rare accident. A driver whose near-deadline path spins (e.g. a
 * catch-up loop that can never satisfy its own exit condition against a
 * moving counter) hangs here and is caught by the harness timeout; one
 * that silently drops or defers the arm trips the per-arm firing bound.
 *
 * Test steps:
 * - For SWEEP_ARMS iterations: busy-wait a stepped sub-tick phase, arm a
 *   zero-duration one-shot k_timer, and wait for it with
 *   k_timer_status_sync().
 * - Check the uptime consumed by each arm-to-fire cycle.
 *
 * Expected result:
 * - Every timer fires, each within SWEEP_FIRE_BOUND_MS.
 *
 * @see k_timer_start()
 * @see k_timer_status_sync()
 * @see k_busy_wait()
 */
ZTEST(timeout_churn, test_timeout_churn_near_tick_arms)
{
	const uint32_t tick_us = USEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	for (int i = 0; i < SWEEP_ARMS; i++) {
		/* Golden-ratio-ish stride: quasi-uniform phase coverage with
		 * no resonance against the tick period.
		 */
		k_busy_wait((617U * i) % tick_us);

		int64_t t0 = k_uptime_get();

		k_timer_start(&churn_timer, K_TICKS(0), K_NO_WAIT);
		k_timer_status_sync(&churn_timer);

		int64_t took = k_uptime_get() - t0;

		zassert_true(took < SWEEP_FIRE_BOUND_MS,
			     "near-boundary arm %d took %lld ms to fire", i,
			     took);
	}
}

/**
 * @}
 */

ZTEST_SUITE(timeout_churn, NULL, NULL, NULL, NULL, NULL);
