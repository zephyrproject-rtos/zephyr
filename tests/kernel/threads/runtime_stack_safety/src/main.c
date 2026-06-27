/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <errno.h>

/*
 * These tests exercise the kernel thread runtime stack safety API
 * (CONFIG_THREAD_RUNTIME_STACK_SAFETY):
 *
 *  - k_thread_runtime_stack_unused_threshold_pct_set()
 *  - k_thread_runtime_stack_unused_threshold_set()
 *  - k_thread_runtime_stack_unused_threshold_get()
 *  - k_thread_runtime_stack_safety_full_check()
 *  - k_thread_runtime_stack_safety_threshold_check()
 *
 * A helper thread is spawned and made to consume a deterministic amount of
 * its stack so that there is a meaningful, stable high-water mark to measure.
 * The thread then parks on a semaphore while the test thread inspects it.
 */

#define CHILD_STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define CHILD_CONSUME   256

static K_THREAD_STACK_DEFINE(child_stack, CHILD_STACKSIZE);
static struct k_thread child_thread;

static K_SEM_DEFINE(child_ready, 0, 1);
static K_SEM_DEFINE(child_park, 0, 1);

/* State captured by the stack safety handler */
static unsigned int handler_calls;
static const struct k_thread *handler_thread;
static size_t handler_unused;
static void *handler_arg;

static void reset_handler_state(void)
{
	handler_calls = 0;
	handler_thread = NULL;
	handler_unused = 0;
	handler_arg = NULL;
}

static void safety_handler(const struct k_thread *thread, size_t unused_space, void *arg)
{
	handler_calls++;
	handler_thread = thread;
	handler_unused = unused_space;
	handler_arg = arg;
}

static void child_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Consume a known chunk of stack and touch it so the compiler cannot
	 * optimize it away. This overwrites the 0xaa fill pattern and so
	 * establishes a stable stack high-water mark for the checks below.
	 */
	volatile uint8_t buf[CHILD_CONSUME];

	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = (uint8_t)i;
	}
	zassert_equal(buf[0], 0);

	k_sem_give(&child_ready);

	/* Stay alive and off the CPU while the test thread inspects us. */
	k_sem_take(&child_park, K_FOREVER);
}

static void *suite_setup(void)
{
	k_thread_create(&child_thread, child_stack, CHILD_STACKSIZE, child_entry, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* Wait for the child to reach its high-water mark and park. */
	zassert_equal(k_sem_take(&child_ready, K_SECONDS(1)), 0, "child thread did not start");

	return NULL;
}

static void suite_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_give(&child_park);
	k_thread_join(&child_thread, K_FOREVER);
}

/**
 * @brief Thread runtime stack safety tests
 * @defgroup tests_kernel_runtime_stack_safety Thread runtime stack safety
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify the percentage threshold setter stores bytes and rejects >= 100%.
 *
 * @details
 * The percentage form of the threshold setter must convert the requested
 * percentage of the thread's total stack size into an absolute byte count
 * (the unit the kernel stores and reports), treat 0% as "disabled", and
 * reject any percentage of 100% or more.
 *
 * Test steps:
 * - Set the threshold to 50% and 99% and confirm the byte value read back
 *   equals the matching fraction of the stack size.
 * - Set the threshold to 0% and confirm it reads back as 0 (disabled).
 * - Set the threshold to 100% and 200% and confirm both are rejected.
 *
 * Expected result:
 * - Valid percentages return 0 and round-trip to the equivalent byte count.
 * - Percentages of 100% and above return -EINVAL.
 *
 * @see k_thread_runtime_stack_unused_threshold_pct_set()
 * @see k_thread_runtime_stack_unused_threshold_get()
 */
ZTEST(runtime_stack_safety, test_threshold_pct_set_get)
{
	size_t size = child_thread.stack_info.size;

	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(&child_thread, 50), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(&child_thread),
		      (size * 50) / 100);

	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(&child_thread, 99), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(&child_thread),
		      (size * 99) / 100);

	/* 0% disables the check */
	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(&child_thread, 0), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(&child_thread), 0);

	/* 100% and above are invalid */
	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(&child_thread, 100), -EINVAL);
	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(&child_thread, 200), -EINVAL);
}

/**
 * @brief Verify the byte threshold setter round-trips and rejects oversized values.
 *
 * @details
 * The byte form of the threshold setter must store the requested value
 * verbatim, accept a threshold equal to the full stack size, and reject any
 * value larger than the stack size.
 *
 * Test steps:
 * - Set the threshold to a small byte value and confirm it reads back exactly.
 * - Set the threshold to the full stack size and confirm it is accepted.
 * - Set the threshold to stack size + 1 and confirm it is rejected.
 *
 * Expected result:
 * - Valid byte thresholds return 0 and round-trip unchanged.
 * - A threshold larger than the stack size returns -EINVAL.
 *
 * @see k_thread_runtime_stack_unused_threshold_set()
 * @see k_thread_runtime_stack_unused_threshold_get()
 */
ZTEST(runtime_stack_safety, test_threshold_bytes_set_get)
{
	size_t size = child_thread.stack_info.size;

	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, 128), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(&child_thread), 128);

	/* Exactly the stack size is allowed */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, size), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(&child_thread), size);

	/* Exceeding the stack size is invalid */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, size + 1),
		      -EINVAL);
}

/**
 * @brief Verify the full check reports unused space and fires only below threshold.
 *
 * @details
 * A full stack safety check scans the whole stack to compute the exact unused
 * space, reports it through the output pointer, and invokes the handler only
 * when that unused space has dropped below the configured threshold. When it
 * does fire, the handler must receive the checked thread, the reported unused
 * amount and the caller-supplied argument.
 *
 * Test steps:
 * - With the threshold disabled (0), run a check and confirm the handler does
 *   not fire while a plausible unused value is still reported.
 * - Set the threshold just above the reported unused space, run a check, and
 *   confirm the handler fires once with the matching thread, unused amount and
 *   argument.
 * - Set the threshold below the unused space, run a check, and confirm the
 *   handler does not fire.
 *
 * Expected result:
 * - The unused space is reported on every successful check.
 * - The handler fires exactly when unused space is below the threshold and is
 *   passed the correct thread, unused amount and argument.
 *
 * @see k_thread_runtime_stack_safety_full_check()
 * @verifies ZEP-SRS-1-30
 */
ZTEST(runtime_stack_safety, test_full_check)
{
	size_t size = child_thread.stack_info.size;
	size_t unused = 0;
	size_t threshold;

	/* With the check disabled (threshold 0) the handler must never fire,
	 * but the unused space is still reported.
	 */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, 0), 0);
	reset_handler_state();
	zassert_equal(k_thread_runtime_stack_safety_full_check(&child_thread, &unused,
							       safety_handler, NULL),
		      0);
	zassert_equal(handler_calls, 0, "handler fired while disabled");
	zassert_true((unused > 0) && (unused < size), "implausible unused value %zu", unused);

	/* Threshold above the current unused space: the handler must fire and
	 * be passed the matching thread, unused amount and user argument.
	 */
	threshold = unused + 16;
	zassert_true(threshold <= size, "test stack too small");
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, threshold), 0);

	reset_handler_state();
	zassert_equal(k_thread_runtime_stack_safety_full_check(&child_thread, &unused,
							       safety_handler, &child_thread),
		      0);
	zassert_equal(handler_calls, 1, "handler did not fire below threshold");
	zassert_equal_ptr(handler_thread, &child_thread);
	zassert_equal(handler_unused, unused);
	zassert_equal_ptr(handler_arg, &child_thread);
	zassert_true(unused < threshold);

	/* Threshold below the current unused space: the handler must not fire. */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, unused / 2), 0);
	reset_handler_state();
	zassert_equal(
		k_thread_runtime_stack_safety_full_check(&child_thread, NULL, safety_handler, NULL),
		0);
	zassert_equal(handler_calls, 0, "handler fired above threshold");
}

/**
 * @brief Verify the full check tolerates a NULL handler when the threshold is crossed.
 *
 * @details
 * The handler argument is optional. Even when the unused space is below the
 * threshold (so a handler would normally be invoked), passing a NULL handler
 * must not fault and the check must still succeed and report the unused space.
 *
 * Test steps:
 * - Set the threshold to the full stack size so the threshold is guaranteed
 *   to be crossed.
 * - Run a full check with a NULL handler.
 *
 * Expected result:
 * - The check returns 0, reports a plausible unused value, and does not fault.
 *
 * @see k_thread_runtime_stack_safety_full_check()
 */
ZTEST(runtime_stack_safety, test_full_check_null_handler)
{
	size_t size = child_thread.stack_info.size;
	size_t unused = 0;

	/* Guarantee the threshold is crossed. */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, size), 0);
	zassert_equal(k_thread_runtime_stack_safety_full_check(&child_thread, &unused, NULL, NULL),
		      0);
	zassert_true(unused < size);
}

/**
 * @brief Verify the abbreviated check fires like the full check and reports the same unused space.
 *
 * @details
 * The abbreviated threshold check performs a cheaper scan but must reach the
 * same fire/no-fire decision as the full check for a given threshold, and when
 * it fires it must report the same unused amount the full check measured.
 *
 * Test steps:
 * - Measure the unused space with a full check (threshold disabled).
 * - Set the threshold just above that unused space, run an abbreviated check,
 *   and confirm the handler fires and the reported unused amount matches the
 *   full check.
 * - Set the threshold below the unused space, run an abbreviated check, and
 *   confirm the handler does not fire.
 *
 * Expected result:
 * - The abbreviated check fires exactly when unused space is below the
 *   threshold and reports the same unused amount as the full check.
 *
 * @see k_thread_runtime_stack_safety_threshold_check()
 * @see k_thread_runtime_stack_safety_full_check()
 * @verifies ZEP-SRS-1-30
 */
ZTEST(runtime_stack_safety, test_threshold_check)
{
	size_t unused_full = 0;
	size_t unused_abbrev = 0;

	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, 0), 0);
	zassert_equal(
		k_thread_runtime_stack_safety_full_check(&child_thread, &unused_full, NULL, NULL),
		0);

	/* Threshold above the unused space: the abbreviated check fires and
	 * reports the same unused amount as the full check.
	 */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, unused_full + 16),
		      0);
	reset_handler_state();
	zassert_equal(k_thread_runtime_stack_safety_threshold_check(&child_thread, &unused_abbrev,
								    safety_handler, NULL),
		      0);
	zassert_equal(handler_calls, 1, "abbreviated check did not fire below threshold");
	zassert_equal(unused_abbrev, unused_full, "abbreviated unused %zu != full unused %zu",
		      unused_abbrev, unused_full);

	/* Threshold below the unused space: the abbreviated check does not fire. */
	zassert_equal(k_thread_runtime_stack_unused_threshold_set(&child_thread, unused_full / 2),
		      0);
	reset_handler_state();
	zassert_equal(k_thread_runtime_stack_safety_threshold_check(&child_thread, NULL,
								    safety_handler, NULL),
		      0);
	zassert_equal(handler_calls, 0, "abbreviated check fired above threshold");
}

/**
 * @brief Verify the threshold get/set syscalls operate correctly from user mode.
 *
 * @details
 * The threshold setters and getter are system calls. When userspace is enabled
 * this test runs in user mode and so exercises the syscall verification path
 * acting on the calling thread; when userspace is disabled it runs in
 * supervisor mode and remains a valid functional check. The byte setter must
 * round-trip, 0% must disable the threshold, and 100% must be rejected.
 *
 * Test steps:
 * - Set a byte threshold on the current thread and read it back.
 * - Set the percentage threshold to 0% and confirm it reads back as 0.
 * - Set the percentage threshold to 100% and confirm it is rejected.
 *
 * Expected result:
 * - The byte threshold round-trips, 0% disables the threshold, and 100%
 *   returns -EINVAL, all through the syscall interface.
 *
 * @see k_thread_runtime_stack_unused_threshold_set()
 * @see k_thread_runtime_stack_unused_threshold_get()
 * @see k_thread_runtime_stack_unused_threshold_pct_set()
 */
ZTEST_USER(runtime_stack_safety, test_threshold_syscalls)
{
	struct k_thread *self = k_current_get();

	zassert_equal(k_thread_runtime_stack_unused_threshold_set(self, 64), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(self), 64,
		      "byte threshold did not round-trip via syscall");

	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(self, 0), 0);
	zassert_equal(k_thread_runtime_stack_unused_threshold_get(self), 0,
		      "0%% did not disable threshold via syscall");

	zassert_equal(k_thread_runtime_stack_unused_threshold_pct_set(self, 100), -EINVAL,
		      "100%% accepted via syscall");
}

/**
 * @}
 */

ZTEST_SUITE(runtime_stack_safety, NULL, suite_setup, NULL, NULL, suite_teardown);
