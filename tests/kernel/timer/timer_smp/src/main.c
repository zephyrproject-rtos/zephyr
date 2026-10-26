/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define TIMER_DELAY  K_MSEC(250)
#define WAKE_TIMEOUT K_MSEC(100)
#define STACK_SIZE   (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

struct test_threads {
	k_tid_t waiter;
	k_tid_t stoppers[2];
};

static struct k_timer test_timer;
static struct k_thread waiter_thread;
static struct k_thread stopper_threads[2];
static struct k_thread second_stopper_thread;
static K_THREAD_STACK_DEFINE(waiter_stack, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(stopper_stacks, 2, STACK_SIZE);
static K_THREAD_STACK_DEFINE(second_stopper_stack, STACK_SIZE);
static K_SEM_DEFINE(waiter_started, 0, 1);
static K_SEM_DEFINE(waiter_done, 0, 1);
static K_SEM_DEFINE(stoppers_done, 0, 2);
static K_SEM_DEFINE(second_stop_start, 0, 1);
static K_SEM_DEFINE(second_stop_done, 0, 1);
static atomic_t handler_entered;
static atomic_t handler_released;
static atomic_t stop_claimed;
static atomic_t stop_calls;
static atomic_t expiry_calls;

static void test_stop(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	atomic_inc(&stop_calls);
}

static void test_blocking_stop(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (atomic_inc(&stop_calls) == 0) {
		k_sem_give(&second_stop_start);
		zassert_ok(k_sem_take(&second_stop_done, K_FOREVER));
	}
}

static void test_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	atomic_inc(&expiry_calls);
}

static void test_blocking_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	atomic_inc(&expiry_calls);
	atomic_set(&handler_entered, 1);
	while (atomic_get(&handler_released) == 0) {
	}
}

static void test_preclaim_handler(struct _timeout *timeout)
{
	atomic_set(&handler_entered, 1);
	while (atomic_get(&handler_released) == 0) {
	}

	z_timer_expiration_handler(timeout);
}

static void test_same_cpu_stop_handler(struct _timeout *timeout)
{
	k_timer_stop(&test_timer);
	z_timer_expiration_handler(timeout);
}

static void timer_waiter(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&waiter_started);
	(void)k_timer_status_sync(&test_timer);
	k_sem_give(&waiter_done);
}

static void timer_stopper(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (atomic_get(&handler_entered) == 0) {
	}

	if (atomic_cas(&stop_claimed, 0, 1)) {
		k_timer_stop(&test_timer);
		atomic_set(&handler_released, 1);
	}

	k_sem_give(&stoppers_done);
}

static void second_timer_stopper(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_ok(k_sem_take(&second_stop_start, K_FOREVER));
	k_timer_stop(&test_timer);
	k_sem_give(&second_stop_done);
}

static void reset_test_state(void)
{
	k_sem_reset(&waiter_started);
	k_sem_reset(&waiter_done);
	k_sem_reset(&stoppers_done);
	k_sem_reset(&second_stop_start);
	k_sem_reset(&second_stop_done);
	atomic_clear(&handler_entered);
	atomic_clear(&handler_released);
	atomic_clear(&stop_claimed);
	atomic_clear(&stop_calls);
	atomic_clear(&expiry_calls);
}

static k_tid_t start_waiter_thread(void)
{
	return k_thread_create(&waiter_thread, waiter_stack, K_THREAD_STACK_SIZEOF(waiter_stack),
			       timer_waiter, NULL, NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0,
			       K_NO_WAIT);
}

static k_tid_t create_stopper_thread(size_t index)
{
	return k_thread_create(&stopper_threads[index], stopper_stacks[index],
			       K_THREAD_STACK_SIZEOF(stopper_stacks[index]), timer_stopper, NULL,
			       NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);
}

static struct test_threads start_test_threads(void)
{
	struct test_threads threads;

	threads.waiter = start_waiter_thread();

	zassert_ok(k_sem_take(&waiter_started, K_FOREVER));

	for (size_t i = 0; i < ARRAY_SIZE(threads.stoppers); i++) {
		threads.stoppers[i] = create_stopper_thread(i);
		zassert_ok(k_thread_cpu_pin(threads.stoppers[i], (int)i));
	}

	for (size_t i = 0; i < ARRAY_SIZE(threads.stoppers); i++) {
		k_thread_start(threads.stoppers[i]);
	}

	return threads;
}

static bool finish_test_threads(const struct test_threads *threads, int *stop_count,
				int *expiry_count)
{
	bool waiter_woke;

	zassert_ok(k_sem_take(&stoppers_done, K_FOREVER));
	zassert_ok(k_sem_take(&stoppers_done, K_FOREVER));
	waiter_woke = k_sem_take(&waiter_done, WAKE_TIMEOUT) == 0;
	*stop_count = atomic_get(&stop_calls);
	*expiry_count = atomic_get(&expiry_calls);

	if (!waiter_woke) {
		k_timer_start(&test_timer, K_SECONDS(1), K_NO_WAIT);
		k_timer_stop(&test_timer);
		zassert_ok(k_sem_take(&waiter_done, K_FOREVER));
	}

	zassert_ok(k_thread_join(threads->waiter, K_FOREVER));
	for (size_t i = 0; i < ARRAY_SIZE(threads->stoppers); i++) {
		zassert_ok(k_thread_join(threads->stoppers[i], K_FOREVER));
	}

	return waiter_woke;
}

/**
 * @brief Verify stopping an unclaimed expiration completes the stop
 *
 * @details The timeout callback is paused after the timeout dispatcher marks
 * it in flight but before the timer expiration handler takes its lock. A
 * thread on the other CPU stops the timer while another thread is blocked in
 * k_timer_status_sync(). The stop must invoke the stop callback and wake the
 * waiter; the superseded expiration handler must then return without firing.
 *
 * @see k_timer_stop()
 * @see k_timer_status_sync()
 */
ZTEST(timer_smp, test_stop_inflight_wakes_status_waiter)
{
	int stop_count;
	int expiry_count;

	reset_test_state();
	k_timer_init(&test_timer, test_expiry, test_stop);
	k_timer_start(&test_timer, TIMER_DELAY, K_NO_WAIT);
	/* Pause the real handler after the timeout dispatcher marks it in flight. */
	test_timer.timeout.fn = test_preclaim_handler;

	struct test_threads threads = start_test_threads();
	bool waiter_woke = finish_test_threads(&threads, &stop_count, &expiry_count);

	zassert_true(waiter_woke, "k_timer_stop() did not wake the status waiter");
	zassert_equal(stop_count, 1, "stop callback called %d times", stop_count);
	zassert_equal(expiry_count, 0, "expiry callback was called");
}

/**
 * @brief Verify a same-CPU interrupt can stop an unclaimed expiration
 *
 * @details The timeout callback stops its own timer before entering the real
 * expiration handler. The stop must complete even though the dispatcher and
 * aborter are running on the same CPU.
 *
 * @see k_timer_stop()
 * @see k_timer_status_sync()
 */
ZTEST(timer_smp, test_same_cpu_stop_inflight_wakes_status_waiter)
{
	reset_test_state();
	k_timer_init(&test_timer, test_expiry, test_stop);
	k_timer_start(&test_timer, TIMER_DELAY, K_NO_WAIT);
	test_timer.timeout.fn = test_same_cpu_stop_handler;

	k_tid_t waiter = start_waiter_thread();

	zassert_ok(k_sem_take(&waiter_started, K_FOREVER));
	bool waiter_woke = k_sem_take(&waiter_done, K_SECONDS(1)) == 0;
	int stop_count = atomic_get(&stop_calls);
	int expiry_count = atomic_get(&expiry_calls);

	if (!waiter_woke) {
		k_timer_start(&test_timer, K_SECONDS(1), K_NO_WAIT);
		k_timer_stop(&test_timer);
		zassert_ok(k_sem_take(&waiter_done, K_FOREVER));
	}

	zassert_ok(k_thread_join(waiter, K_FOREVER));
	zassert_true(waiter_woke, "k_timer_stop() did not wake the status waiter");
	zassert_equal(stop_count, 1, "stop callback called %d times", stop_count);
	zassert_equal(expiry_count, 0, "expiry callback was called");
}

/**
 * @brief Verify stopping a committed one-shot expiration has no effect
 *
 * @details The expiry callback pauses after the expiration handler has
 * committed to running. A thread on the other CPU calls k_timer_stop(). The
 * timer has already expired, so the expiry callback must complete and the stop
 * callback must not run.
 *
 * @see k_timer_stop()
 */
ZTEST(timer_smp, test_stop_after_expiration_commit_has_no_effect)
{
	int stop_count;
	int expiry_count;

	reset_test_state();
	k_timer_init(&test_timer, test_blocking_expiry, test_stop);
	k_timer_start(&test_timer, TIMER_DELAY, K_NO_WAIT);

	struct test_threads threads = start_test_threads();
	bool waiter_woke = finish_test_threads(&threads, &stop_count, &expiry_count);

	zassert_true(waiter_woke, "expiration did not wake the status waiter");
	zassert_equal(expiry_count, 1, "expiry callback count mismatch");
	zassert_equal(stop_count, 0, "stop callback called %d times", stop_count);
}

/**
 * @brief Verify stopping the next period preserves the current expiration
 *
 * @details A periodic timer re-arms before invoking its expiry callback. A
 * concurrent stop removes the next period and invokes the stop callback, while
 * the already-committed expiry callback completes normally.
 *
 * @see k_timer_stop()
 */
ZTEST(timer_smp, test_stop_committed_periodic_expiration)
{
	int stop_count;
	int expiry_count;

	reset_test_state();
	k_timer_init(&test_timer, test_blocking_expiry, test_stop);
	k_timer_start(&test_timer, TIMER_DELAY, K_SECONDS(1));

	struct test_threads threads = start_test_threads();
	bool waiter_woke = finish_test_threads(&threads, &stop_count, &expiry_count);

	zassert_true(waiter_woke, "timer stop did not wake the status waiter");
	zassert_equal(expiry_count, 1, "expiry callback count mismatch");
	zassert_equal(stop_count, 1, "stop callback count mismatch");
}

/**
 * @brief Verify only one concurrent stop completes an unclaimed expiration
 *
 * @details The first stop callback blocks after an unclaimed expiration has
 * been superseded, allowing a second thread to stop the same timer before the
 * expiration handler resumes. Only the first stop may invoke the stop callback
 * and complete the stop operation.
 *
 * @see k_timer_stop()
 */
ZTEST(timer_smp, test_concurrent_stop_completes_once)
{
	int stop_count;
	int expiry_count;

	reset_test_state();
	k_timer_init(&test_timer, test_expiry, test_blocking_stop);
	k_timer_start(&test_timer, TIMER_DELAY, K_NO_WAIT);
	/* Pause the real handler after the timeout dispatcher marks it in flight. */
	test_timer.timeout.fn = test_preclaim_handler;

	k_tid_t second_stopper =
		k_thread_create(&second_stopper_thread, second_stopper_stack,
				K_THREAD_STACK_SIZEOF(second_stopper_stack), second_timer_stopper,
				NULL, NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	struct test_threads threads = start_test_threads();
	bool waiter_woke = finish_test_threads(&threads, &stop_count, &expiry_count);

	zassert_ok(k_thread_join(second_stopper, K_FOREVER));
	zassert_true(waiter_woke, "k_timer_stop() did not wake the status waiter");
	zassert_equal(stop_count, 1, "stop callback called %d times", stop_count);
	zassert_equal(expiry_count, 0, "expiry callback was called");
}

ZTEST_SUITE(timer_smp, NULL, NULL, NULL, NULL, NULL);
