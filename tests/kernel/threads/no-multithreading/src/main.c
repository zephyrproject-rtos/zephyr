/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

/**
 * @brief Verify that k_busy_wait() delays for the requested time when
 *        multithreading is disabled.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * With CONFIG_MULTITHREADING=n there is no scheduler and no other thread to
 * switch to, so a busy wait has to be served entirely by the caller spinning
 * while the system clock keeps advancing. Uptime is sampled around a busy wait
 * of a known length to confirm both that the clock runs and that the delay is
 * of the requested duration.
 *
 * Test steps:
 * - Spin until the uptime changes, so the measurement starts on a tick
 *   boundary, failing if it never does.
 * - Sample the uptime, call k_busy_wait() for 10 ms and sample it again.
 * - Compare the elapsed uptime against the requested delay.
 *
 * Expected result:
 * - The uptime advances, and the elapsed time is within 2 ms of the 10 ms
 *   that were requested.
 *
 * @see k_busy_wait()
 * @see k_uptime_get()
 */
ZTEST(no_multithreading, test_no_multithreading_busy_wait)
{
	int64_t now = k_uptime_get();
	uint32_t watchdog = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	while (k_uptime_get() != now) {
		/* Wait until uptime progresses */
		watchdog--;
		if (watchdog == 0) {
			zassert_false(true, "No progress in uptime");
		}
	}

	now = k_uptime_get();
	/* Check that k_busy_wait is working as expected. */
	k_busy_wait(10000);

	int64_t diff = k_uptime_get() - now;

	zassert_within(diff, 10, 2);
}

static void timeout_handler(struct k_timer *timer)
{
	bool *flag = k_timer_user_data_get(timer);

	*flag = true;
}

K_TIMER_DEFINE(timer, timeout_handler, NULL);

/**
 * @brief Verify that locking interrupts defers a timer expiry handler.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Interrupt locking is the only mutual exclusion mechanism left once
 * multithreading is disabled, so it has to hold off the timer ISR: a timer
 * that expires while interrupts are locked must not run its handler until
 * they are unlocked. The handler sets a flag through the timer user data,
 * which is sampled on both sides of the unlock.
 *
 * Test steps:
 * - Start a 10 ms timer whose handler raises a flag.
 * - Lock interrupts, busy-wait past the timer deadline with a tick of slack
 *   and check the flag is still clear.
 * - Unlock interrupts, allowing the pending timer interrupt to be delivered.
 * - On a tickful kernel, busy-wait one more tick so a second announce reaches
 *   the timer deadline, then check the flag.
 *
 * Expected result:
 * - The handler does not run while interrupts are locked.
 * - The handler runs once interrupts are unlocked.
 *
 * @see irq_lock()
 * @see irq_unlock()
 * @see k_timer_start()
 */
ZTEST(no_multithreading, test_no_multithreading_irq_lock)
{
	volatile bool timeout_run = false;

	k_timer_user_data_set(&timer, (void *)&timeout_run);
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);

	unsigned int key = irq_lock();

	/* Wait long enough to cover the 10 ms timeout plus one full
	 * tick of slack for z_add_timeout()'s "at least N" round-up
	 * plus a few ms of measurement margin.
	 */
	k_busy_wait(10000 + k_ticks_to_us_ceil32(1) + 5000);
	zassert_false(timeout_run, "Timeout should not expire because irq is locked");

	irq_unlock(key);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * On a tickful kernel, the timer ISR announces exactly one
		 * tick per invocation. While IRQs were masked, multiple tick
		 * interrupts fired but the IRQ controller's pending bit
		 * coalesces them, so only a single ISR runs when IRQs are
		 * unlocked. Our K_MSEC(10) timer needs two announces to
		 * reach its deadline under the "at least N ticks" contract,
		 * so the first pending delivery only brings dticks from 2
		 * down to 1. Wait for the next tick so a second ISR fires
		 * and the timer actually expires.
		 */
		k_busy_wait(k_ticks_to_us_ceil32(1) + 1000);
	}

	zassert_true(timeout_run, "Timeout should expire because irq got unlocked");
}

/**
 * @brief Verify that k_cpu_idle() sleeps until a timer interrupt wakes it.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Without multithreading there is no idle thread, so the application itself
 * calls k_cpu_idle() to wait. The CPU must suspend until an interrupt arrives
 * and must resume with the timer's expiry handler having run, which is how a
 * single-threaded application waits for time to pass without spinning.
 *
 * Test steps:
 * - Start a 10 ms timer whose handler raises a flag, sampling the uptime.
 * - Call k_cpu_idle(); on a tickful kernel, where every tick wakes the CPU,
 *   idle again until the handler has actually run.
 * - Check the flag and measure the elapsed uptime.
 *
 * Expected result:
 * - The CPU resumes with the timer handler having run.
 * - At least the requested 10 ms elapsed, and no more than one tick plus a
 *   small measurement margin beyond it.
 *
 * @see k_cpu_idle()
 * @see k_timer_start()
 */
ZTEST(no_multithreading, test_no_multithreading_cpu_idle)
{
	volatile bool timeout_run = false;
	int64_t now, diff;

	k_timer_user_data_set(&timer, (void *)&timeout_run);
	now = k_uptime_get();
	/* Start timer and go to idle, cpu should sleep until it is waken up
	 * by sys clock interrupt.
	 */
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * On a tickless kernel the only scheduled wakeup while
		 * we are idle is our own timer, so k_cpu_idle() returns
		 * once with the timer callback already executed.
		 */
		k_cpu_idle();
		zassert_true(timeout_run, "Timeout should expire");
	} else {
		/*
		 * On a tickful kernel periodic tick interrupts wake
		 * k_cpu_idle() on every tick regardless of our timer.
		 * Loop back to idle until the timer callback has actually
		 * run, which under the "at least N ticks" contract may
		 * take more than one tick for K_MSEC() values that round
		 * to a single tick.
		 */
		while (!timeout_run) {
			k_cpu_idle();
		}
	}

	diff = k_uptime_get() - now;
	/* Timer fires at least 10 ms from now (minimum delay), at most
	 * 10 ms plus one tick for the round-up in z_add_timeout() and
	 * a couple of ms of measurement margin.
	 */
	zassert_between_inclusive(diff, 10, 10 + k_ticks_to_ms_ceil32(1) + 2,
				  "Unexpected time passed: %d ms", (int)diff);
}

/* TODO: Remove CONFIG_ARM check once TLS is supported on all architectures
 * See https://github.com/zephyrproject-rtos/zephyr/issues/114503
 */
#if defined(CONFIG_ARM)
/**
 * @brief Verify that thread-local storage is initialized without
 *        multithreading.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Thread-local variables are set up from their initializers as part of
 * starting a thread. With CONFIG_MULTITHREADING=n there is only the main
 * thread of control, and its TLS block still has to be established before the
 * application runs, so a thread-local variable must read back its initializer
 * rather than zero or garbage.
 *
 * Test steps:
 * - Declare a thread-local variable with a non-zero initializer.
 * - Read it back from the single thread of control.
 *
 * Expected result:
 * - The variable holds the value it was initialized with.
 *
 * @see Z_THREAD_LOCAL
 */
ZTEST(no_multithreading, test_no_multithreading_tls)
{
	static volatile Z_THREAD_LOCAL int i = 42;

	zassert_equal(i, 42, "TLS variable was not initialized");
}
#endif

#define IDX_PRE_KERNEL_1 0
#define IDX_PRE_KERNEL_2 1
#define IDX_POST_KERNEL 2

#define SYS_INIT_CREATE(level) \
	static int pre_kernel_##level##_init_func(void) \
	{ \
		if (init_order != IDX_##level && sys_init_result == 0) { \
			sys_init_result = -1; \
			return -EIO; \
		} \
		init_order++; \
		return 0;\
	} \
	SYS_INIT(pre_kernel_##level##_init_func, level, 0)

static int init_order;
static int sys_init_result;

FOR_EACH(SYS_INIT_CREATE, (;), PRE_KERNEL_1, PRE_KERNEL_2, POST_KERNEL);

/**
 * @brief Verify that SYS_INIT functions run in order without multithreading.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The initialization levels are a property of the boot sequence rather than of
 * the scheduler, so disabling multithreading must not change them. Each
 * registered init function checks that it is entered at its own position in
 * the sequence and then advances a counter, so a wrong order is recorded as it
 * happens rather than being inferred afterwards.
 *
 * Test steps:
 * - Register one init function at PRE_KERNEL_1, PRE_KERNEL_2 and POST_KERNEL,
 *   each asserting its position and incrementing a shared counter.
 * - After boot, read the counter.
 *
 * Expected result:
 * - All three init functions ran, in level order, leaving the counter at 3.
 *
 * @see SYS_INIT()
 */
ZTEST(no_multithreading, test_no_multithreading_sys_init)
{
	zassert_equal(init_order, 3, "SYS_INIT failed: %d", init_order);
}

ZTEST_SUITE(no_multithreading, NULL, NULL, NULL, NULL, NULL);
