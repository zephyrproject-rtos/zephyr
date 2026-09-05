/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Safety Test subsystem core
 */

#include <string.h>

#include <zephyr/fatal.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/safety_test/safety_test.h>
#ifdef CONFIG_REBOOT
#include <zephyr/sys/reboot.h>
#endif

LOG_MODULE_REGISTER(safety_test, CONFIG_SAFETY_TEST_LOG_LEVEL);

BUILD_ASSERT((SAFETY_TEST_OPT_MASK & 0xFFU) == 0U,
	     "safety test options must not overlap the flag range");

/*
 * safety_test_run_lock serializes test execution. It is held across application code
 * (test functions, hooks, safe-state), so it must be a mutex and those paths are thread
 * context only. safety_test_rec_lock protects only the result records and is never held
 * across application code, so the read-only queries stay callable from an ISR.
 */
static K_MUTEX_DEFINE(safety_test_run_lock);
static struct k_spinlock safety_test_rec_lock;

/* Guarded by safety_test_run_lock. True while a test function or a failure hook is
 * executing, so a hook that calls back into the run API is rejected with -EALREADY
 * instead of silently nesting a test execution inside a failing test's own reporting
 * path. The run lock is recursive, so nothing else would catch it.
 */
static bool test_in_flight;

/*
 * At the EARLY level the log buffer does not exist yet, so a failing test prints nothing
 * and the board just stops. This record is the only trace left: a debugger can read it on
 * the halted board, and if RAM survives the reset it is logged on the next boot. RAM
 * retention is not guaranteed, so treat it as best effort.
 */
#define SAFETY_TEST_FAILURE_MAGIC 0x5afe7e57U

struct safety_test_failure_record {
	uint32_t magic;
	const struct safety_test *test;
	int error_code;
};

static __noinit struct safety_test_failure_record safety_test_failure;

static void failure_record_set(const struct safety_test *test,
			       const struct safety_test_result_record *rec)
{
	safety_test_failure.test = test;
	safety_test_failure.error_code = rec->error_code;
	safety_test_failure.magic = SAFETY_TEST_FAILURE_MAGIC;
}

static void failure_record_clear(void)
{
	safety_test_failure.magic = 0;
}

/* Trust the pointer only if it still matches a registered test. That rejects a stale
 * record left behind by a different image.
 */
static const struct safety_test *failure_record_get(void)
{
	if (safety_test_failure.magic != SAFETY_TEST_FAILURE_MAGIC) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if (test == safety_test_failure.test) {
			return test;
		}
	}

	return NULL;
}

/*
 * k_is_pre_kernel() is true exactly for the EARLY, PRE_KERNEL_1 and PRE_KERNEL_2 init
 * levels: z_sys_post_kernel is set in kernel/init.c before POST_KERNEL device init runs.
 * At those levels nothing else is running and there is no kernel to take a mutex with, so
 * the run lock is both unnecessary and unavailable. Everywhere else it is required.
 *
 * The record spinlock needs no such guard: it is valid at every init level.
 */
static void lock(void)
{
	if (!k_is_pre_kernel()) {
		k_mutex_lock(&safety_test_run_lock, K_FOREVER);
	}
}

static void unlock(void)
{
	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&safety_test_run_lock);
	}
}

/*
 * k_cycle_get_32() wraps every 2^32 cycles (8.9 s at 480 MHz). Use the 64-bit counter
 * wherever the target has one.
 */
static uint64_t cycles_get(void)
{
	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return k_cycle_get_64();
	}

	return k_cycle_get_32();
}

static uint64_t cycles_delta(uint64_t start, uint64_t end)
{
	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return end - start;
	}

	/*
	 * Truncating to 32 bits makes the unsigned subtraction correct across one wrap of
	 * the 32-bit counter. More than one is undetectable, and needs a test to run for
	 * a whole lap, which is seconds. A diagnostic that blocks for that long has
	 * already failed its purpose.
	 */
	return (uint32_t)(end - start);
}

static void record_store(const struct safety_test *test,
			 const struct safety_test_result_record *rec)
{
	K_SPINLOCK(&safety_test_rec_lock) {
		*test->result = *rec;
	}
}

static void invoke_failure_hooks(const struct safety_test *test,
				 const struct safety_test_result_record *rec)
{
	STRUCT_SECTION_FOREACH(safety_test_failure_hook, hook) {
		if (hook->fn != NULL) {
			hook->fn(test, rec);
		}
	}
}

__weak enum safety_test_action safety_test_safe_state(const struct safety_test *test,
						      const struct safety_test_result_record *rec)
{
	ARG_UNUSED(test);
	ARG_UNUSED(rec);

	if (IS_ENABLED(CONFIG_SAFETY_TEST_DEFAULT_ACTION_RESET)) {
		return SAFETY_TEST_ACTION_RESET;
	}

	return SAFETY_TEST_ACTION_HALT;
}

static void handle_critical_failure(const struct safety_test *test,
				    const struct safety_test_result_record *rec)
{
	enum safety_test_action action;

	/* Before the handler runs, so the record survives a handler that hangs or faults. */
	failure_record_set(test, rec);

	LOG_ERR("critical test '%s' failed, entering safe state", test->name);

	action = safety_test_safe_state(test, rec);

	if (IS_ENABLED(CONFIG_SAFETY_TEST_STRICT_CRITICAL) &&
	    action == SAFETY_TEST_ACTION_CONTINUE) {
		LOG_ERR("CONTINUE refused: CONFIG_SAFETY_TEST_STRICT_CRITICAL is set");
		action = SAFETY_TEST_ACTION_HALT;
	}

	if (action == SAFETY_TEST_ACTION_CONTINUE) {
		LOG_WRN("continuing after a critical failure");
		failure_record_clear();
		return;
	}

	if (action == SAFETY_TEST_ACTION_RESET) {
#ifdef CONFIG_REBOOT
		LOG_ERR("rebooting");
		LOG_PANIC();
		sys_reboot(SYS_REBOOT_COLD);
#else
		LOG_ERR("RESET refused: CONFIG_REBOOT is not enabled");
#endif
	}

	LOG_ERR("halting");
	LOG_PANIC();
	k_fatal_halt(K_ERR_KERNEL_PANIC);
}

/**
 * @brief Execute a test and record the outcome. Applies no policy.
 */
static enum safety_test_result run_one(const struct safety_test *test,
				       enum safety_test_init_level level,
				       struct safety_test_result_record *out)
{
	struct safety_test_result_record rec = {0};
	const struct safety_test_context ctx = {
		.init_level = level,
		.test = test,
		.user_data = test->user_data,
	};
	uint64_t start;
	uint64_t end;
	uint64_t elapsed_us;
	bool timed;
	int rc;

	/*
	 * Below PRE_KERNEL_2 the system timer has not initialised and reading it
	 * faults. That rules out two things: taking a duration, and logging,
	 * because the log core timestamps every message from that same timer.
	 */
	timed = level >= SAFETY_TEST_LEVEL_PRE_KERNEL_2;

	if (timed) {
		LOG_INF("running %s", test->name);
	}

	/* No lock is held across the test function: it is application code that may
	 * block, and the record lock is a spinlock.
	 */

	start = timed ? cycles_get() : 0U;
	rc = test->test_fn(&ctx);
	end = timed ? cycles_get() : 0U;

	/* Saturate rather than truncate: a wrapped-small duration would silently satisfy a
	 * budget the test blew through.
	 */
	elapsed_us = k_cyc_to_us_floor64(cycles_delta(start, end));
	rec.duration_us = (elapsed_us > UINT32_MAX) ? UINT32_MAX : (uint32_t)elapsed_us;
	rec.error_code = rc;
	rec.result = (rc == 0) ? SAFETY_TEST_RESULT_PASS : SAFETY_TEST_RESULT_FAIL;
	/* Below PRE_KERNEL_2 the cycle counter may not be running yet, so duration_us is
	 * not a number worth judging anything against.
	 */
	rec.over_budget = level >= SAFETY_TEST_LEVEL_PRE_KERNEL_2 &&
			  test->max_duration_us != 0 &&
			  rec.duration_us > test->max_duration_us;

	/* Whether the hardware is sound and whether the test fit its budget are two
	 * different questions, so they are recorded separately. Only an integrator who has
	 * measured the worst case lets the second one change the verdict.
	 */
	if (rec.over_budget) {
		LOG_WRN("%s took %u us, over its %u us budget", test->name, rec.duration_us,
			test->max_duration_us);

		if (IS_ENABLED(CONFIG_SAFETY_TEST_BUDGET_FAILS)) {
			rec.result = SAFETY_TEST_RESULT_FAIL;
		}
	}

	/* Publish the whole record in one critical section so a concurrent reader,
	 * including one in an ISR, never sees a half-updated result.
	 */
	record_store(test, &rec);

	if (rec.result == SAFETY_TEST_RESULT_FAIL) {
		if (timed) {
			LOG_ERR("%s FAIL (%d) after %u us", test->name, rec.error_code,
				rec.duration_us);
		}

		invoke_failure_hooks(test, &rec);
	} else if (timed) {
		LOG_INF("%s PASS after %u us", test->name, rec.duration_us);
	}

	if (out != NULL) {
		*out = rec;
	}

	return rec.result;
}

/**
 * @brief Decide whether policy permits a runtime invocation.
 */
static bool runtime_allowed(const struct safety_test *test, uint32_t opts)
{
	if ((test->flags & SAFETY_TEST_FLAG_RUNTIME_OK) == 0) {
		LOG_DBG("%s is not runtime-capable", test->name);
		return false;
	}

	if ((test->flags & SAFETY_TEST_FLAG_DESTRUCTIVE) != 0 &&
	    (opts & SAFETY_TEST_OPT_ALLOW_DESTRUCTIVE) == 0) {
		LOG_DBG("%s is destructive and was not opted into", test->name);
		return false;
	}

	return true;
}

static void stats_add(struct safety_test_stats *stats,
		      const struct safety_test_result_record *rec)
{
	stats->total++;

	if (rec->over_budget) {
		stats->over_budget++;
	}

	switch (rec->result) {
	case SAFETY_TEST_RESULT_PASS:
		stats->passed++;
		break;
	case SAFETY_TEST_RESULT_FAIL:
		stats->failed++;
		break;
	case SAFETY_TEST_RESULT_SKIP:
		stats->skipped++;
		break;
	case SAFETY_TEST_RESULT_NOT_RUN:
	default:
		stats->not_run++;
		break;
	}
}

const struct safety_test *safety_test_get(const char *name)
{
	if (name == NULL) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if (strcmp(test->name, name) == 0) {
			return test;
		}
	}

	return NULL;
}

void safety_test_foreach(safety_test_cb_t cb, void *user_data)
{
	if (cb == NULL) {
		return;
	}

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if (!cb(test, user_data)) {
			return;
		}
	}
}

/* @p out reports the record this call produced. A skipped call writes no record, so it
 * reports SKIP and nothing else (reading the stored record instead would return a
 * previous run's numbers).
 */
static int run_and_record(const struct safety_test *test, uint32_t opts,
			  enum safety_test_result *result,
			  struct safety_test_result_record *out)
{
	struct safety_test_result_record rec = {0};

	if (test == NULL) {
		return -EINVAL;
	}

	if ((opts & ~SAFETY_TEST_OPT_MASK) != 0U) {
		LOG_ERR("unknown option bits 0x%08x", (uint32_t)(opts & ~SAFETY_TEST_OPT_MASK));
		return -EINVAL;
	}

	lock();

	if (test_in_flight) {
		unlock();
		LOG_ERR("reentrant test execution: a failure hook must not run tests");
		return -EALREADY;
	}

	if (runtime_allowed(test, opts)) {
		test_in_flight = true;
		(void)run_one(test, SAFETY_TEST_LEVEL_APPLICATION, &rec);

		/* Inside the lock and with the guard still set, so the safe-state handler
		 * sees exactly the same conditions as it does on the boot path.
		 */
		if (rec.result == SAFETY_TEST_RESULT_FAIL &&
		    (test->flags & SAFETY_TEST_FLAG_CRITICAL) != 0) {
			handle_critical_failure(test, &rec);
		}

		test_in_flight = false;
	} else {
		rec.result = SAFETY_TEST_RESULT_SKIP;
	}

	unlock();

	if (result != NULL) {
		*result = rec.result;
	}

	if (out != NULL) {
		*out = rec;
	}

	return 0;
}

int safety_test_run(const struct safety_test *test, uint32_t opts,
		    enum safety_test_result *result)
{
	return run_and_record(test, opts, result, NULL);
}

int safety_test_run_by_name(const char *name, uint32_t opts, enum safety_test_result *result)
{
	const struct safety_test *test;

	if (name == NULL) {
		return -EINVAL;
	}

	test = safety_test_get(name);
	if (test == NULL) {
		LOG_ERR("no test named '%s'", name);
		return -ENOENT;
	}

	return safety_test_run(test, opts, result);
}

int safety_test_get_result(const struct safety_test *test, struct safety_test_result_record *rec)
{
	if (test == NULL || rec == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&safety_test_rec_lock) {
		*rec = *test->result;
	}

	return 0;
}

int safety_test_run_category(uint32_t cat_mask, struct safety_test_stats *stats)
{
	struct safety_test_stats local = {0};

	if (cat_mask == 0) {
		return -EINVAL;
	}

	STRUCT_SECTION_FOREACH(safety_test, test) {
		struct safety_test_result_record rec;

		if ((cat_mask & SAFETY_TEST_CAT_BIT(test->category)) == 0) {
			continue;
		}

		(void)run_and_record(test, 0, NULL, &rec);
		stats_add(&local, &rec);
	}

	if (stats != NULL) {
		*stats = local;
	}

	return 0;
}

int safety_test_get_summary(struct safety_test_summary *summary)
{
	k_spinlock_key_t key;

	if (summary == NULL) {
		return -EINVAL;
	}

	memset(summary, 0, sizeof(*summary));

	/* Held across the whole walk so the summary is a consistent snapshot rather
	 * than a mix of before- and after-values from a concurrent run. Interrupts are
	 * masked for the duration, which is a few instructions per registered test.
	 */
	key = k_spin_lock(&safety_test_rec_lock);

	STRUCT_SECTION_FOREACH(safety_test, test) {
		stats_add(&summary->global, test->result);

		if (test->category < SAFETY_TEST_CAT_COUNT) {
			stats_add(&summary->categories[test->category], test->result);
		}
	}

	k_spin_unlock(&safety_test_rec_lock, key);

	return 0;
}

int safety_test_boot_passed(bool *passed)
{
	k_spinlock_key_t key;
	bool found = false;
	bool all_passed = true;

	if (passed == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&safety_test_rec_lock);

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if ((test->flags & SAFETY_TEST_FLAG_BOOT_OK) == 0) {
			continue;
		}

		if ((test->flags & SAFETY_TEST_FLAG_CRITICAL) == 0) {
			continue;
		}

		found = true;

		if (test->result->result != SAFETY_TEST_RESULT_PASS) {
			all_passed = false;
			break;
		}
	}

	k_spin_unlock(&safety_test_rec_lock, key);

	/* Nothing to judge is not the same answer as "did not pass". */
	if (!found) {
		return -ENOENT;
	}

	*passed = all_passed;

	return 0;
}

int safety_test_run_level(enum safety_test_init_level level, struct safety_test_stats *stats)
{
	struct safety_test_stats local = {0};

	if (level > SAFETY_TEST_LEVEL_APPLICATION) {
		return -EINVAL;
	}

	lock();

	if (test_in_flight) {
		unlock();
		LOG_ERR("reentrant test execution: a failure hook must not run tests");
		return -EALREADY;
	}

	test_in_flight = true;

	/* The linker sorts the section by name, so priority has to be applied here. Walking
	 * the priority space keeps this allocation-free and usable before the kernel exists.
	 */
	for (unsigned int prio = 0; prio <= UINT8_MAX; prio++) {
		STRUCT_SECTION_FOREACH(safety_test, test) {
			struct safety_test_result_record rec;
			enum safety_test_result r;

			if (test->init_level != level || test->priority != prio) {
				continue;
			}

			if ((test->flags & SAFETY_TEST_FLAG_BOOT_OK) == 0) {
				continue;
			}

			r = run_one(test, level, &rec);
			stats_add(&local, &rec);

			if (r == SAFETY_TEST_RESULT_FAIL &&
			    (test->flags & SAFETY_TEST_FLAG_CRITICAL) != 0) {
				handle_critical_failure(test, &rec);
			}
		}
	}

	test_in_flight = false;

	unlock();

	if (stats != NULL) {
		*stats = local;
	}

	return 0;
}

/*
 * PRE_KERNEL_2 is the earliest level where logging is safe: the log core
 * timestamps from the system timer, which is not up before then. Priority 1
 * runs after the timer driver at 0 and still before any level sweep at 99, so
 * the record is read before a sweep could clear it.
 *
 * If CONFIG_SYSTEM_CLOCK_INIT_PRIORITY is changed, make sure the SYS_INIT priority
 * for safety_test_report_prior_failure is after CONFIG_SYSTEM_CLOCK_INIT_PRIORITY.
 */
static int safety_test_report_prior_failure(void)
{
	const struct safety_test *test = failure_record_get();

	if (test != NULL) {
		LOG_ERR("previous boot entered the safe state: critical test '%s' failed (%d)",
			test->name, safety_test_failure.error_code);
		failure_record_clear();
	}

	return 0;
}
SYS_INIT(safety_test_report_prior_failure, PRE_KERNEL_2, 1);

#ifdef CONFIG_SAFETY_TEST_EARLY_TESTS
static int safety_test_boot_early(void)
{
	(void)safety_test_run_level(SAFETY_TEST_LEVEL_EARLY, NULL);

	return 0;
}
SYS_INIT(safety_test_boot_early, EARLY, CONFIG_SAFETY_TEST_INIT_PRIORITY);
#endif

static int safety_test_boot_pre_kernel_1(void)
{
	(void)safety_test_run_level(SAFETY_TEST_LEVEL_PRE_KERNEL_1, NULL);

	return 0;
}
SYS_INIT(safety_test_boot_pre_kernel_1, PRE_KERNEL_1, CONFIG_SAFETY_TEST_INIT_PRIORITY);

static int safety_test_boot_pre_kernel_2(void)
{
	(void)safety_test_run_level(SAFETY_TEST_LEVEL_PRE_KERNEL_2, NULL);

	return 0;
}
SYS_INIT(safety_test_boot_pre_kernel_2, PRE_KERNEL_2, CONFIG_SAFETY_TEST_INIT_PRIORITY);

static int safety_test_boot_post_kernel(void)
{
	(void)safety_test_run_level(SAFETY_TEST_LEVEL_POST_KERNEL, NULL);

	return 0;
}
SYS_INIT(safety_test_boot_post_kernel, POST_KERNEL, CONFIG_SAFETY_TEST_INIT_PRIORITY);

static int safety_test_boot_application(void)
{
	(void)safety_test_run_level(SAFETY_TEST_LEVEL_APPLICATION, NULL);

	return 0;
}
SYS_INIT(safety_test_boot_application, APPLICATION, CONFIG_SAFETY_TEST_INIT_PRIORITY);
