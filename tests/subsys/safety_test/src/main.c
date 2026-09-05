/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/safety_test/safety_test.h>
#include <zephyr/ztest.h>

/* Comfortably over the 100 us budget on any target. */
#define SLOW_US	    5000
#define TIGHT_BUDGET 100
#define LOOSE_BUDGET 10000000

/* Test functions */

static int fn_pass(const struct safety_test_context *ctx)
{
	ARG_UNUSED(ctx);
	return 0;
}

static int fn_fail(const struct safety_test_context *ctx)
{
	ARG_UNUSED(ctx);
	return -EIO;
}

static int fn_slow(const struct safety_test_context *ctx)
{
	ARG_UNUSED(ctx);
	k_busy_wait(SLOW_US);
	return 0;
}

static int fn_slow_fail(const struct safety_test_context *ctx)
{
	ARG_UNUSED(ctx);
	k_busy_wait(SLOW_US);
	return -EIO;
}

/* Records the order boot tests actually ran in, via the context. */
#define BOOT_ORDER_MAX 4
static uint8_t boot_order[BOOT_ORDER_MAX];
static uint8_t boot_order_n;

static int fn_record(const struct safety_test_context *ctx)
{
	if (boot_order_n < BOOT_ORDER_MAX) {
		boot_order[boot_order_n++] = ctx->test->priority;
	}
	return 0;
}

/*
 * Registrations. One runtime test per category, so a category sweep has an
 * unambiguous expected membership.
 */

SAFETY_TEST_DEFINE(rt_cpu_ok, SAFETY_TEST_CAT_CPU, SAFETY_TEST_LEVEL_APPLICATION, 10,
		   SAFETY_TEST_FLAG_RUNTIME_OK, fn_pass, "runtime, passes");

SAFETY_TEST_DEFINE(rt_ram_fail, SAFETY_TEST_CAT_RAM, SAFETY_TEST_LEVEL_APPLICATION, 10,
		   SAFETY_TEST_FLAG_RUNTIME_OK, fn_fail, "runtime, fails");

SAFETY_TEST_DEFINE(rt_flash_ok, SAFETY_TEST_CAT_FLASH, SAFETY_TEST_LEVEL_APPLICATION, 10,
		   SAFETY_TEST_FLAG_RUNTIME_OK, fn_pass, "runtime, passes");

SAFETY_TEST_DEFINE(rt_destructive, SAFETY_TEST_CAT_CLOCK, SAFETY_TEST_LEVEL_APPLICATION, 10,
		   SAFETY_TEST_FLAG_RUNTIME_OK | SAFETY_TEST_FLAG_DESTRUCTIVE, fn_pass,
		   "runtime, destructive");

SAFETY_TEST_DEFINE_EX(rt_slow, SAFETY_TEST_CAT_ADC, SAFETY_TEST_LEVEL_APPLICATION, 10,
		      SAFETY_TEST_FLAG_RUNTIME_OK, fn_slow, "over budget, sound", NULL,
		      TIGHT_BUDGET);

SAFETY_TEST_DEFINE_EX(rt_slow_fail, SAFETY_TEST_CAT_WATCHDOG, SAFETY_TEST_LEVEL_APPLICATION, 10,
		      SAFETY_TEST_FLAG_RUNTIME_OK, fn_slow_fail, "over budget and broken", NULL,
		      TIGHT_BUDGET);

SAFETY_TEST_DEFINE_EX(rt_quick, SAFETY_TEST_CAT_COMM, SAFETY_TEST_LEVEL_APPLICATION, 10,
		      SAFETY_TEST_FLAG_RUNTIME_OK, fn_pass, "within budget", NULL, LOOSE_BUDGET);

SAFETY_TEST_DEFINE(rt_crit_fail, SAFETY_TEST_CAT_GPIO, SAFETY_TEST_LEVEL_APPLICATION, 10,
		   SAFETY_TEST_FLAG_RUNTIME_OK | SAFETY_TEST_FLAG_CRITICAL, fn_fail,
		   "runtime, critical, fails");

/*
 * Boot-order probes. The names are deliberately in a different order to the
 * priorities: the linker sorts the section by name, so a suite that named them
 * in priority order would pass even if the priority sort were removed.
 */
SAFETY_TEST_DEFINE(boot_alpha, SAFETY_TEST_CAT_STACK, SAFETY_TEST_LEVEL_POST_KERNEL, 30,
		   SAFETY_TEST_FLAG_BOOT_OK, fn_record, "third");
SAFETY_TEST_DEFINE(boot_bravo, SAFETY_TEST_CAT_STACK, SAFETY_TEST_LEVEL_POST_KERNEL, 10,
		   SAFETY_TEST_FLAG_BOOT_OK, fn_record, "first");
SAFETY_TEST_DEFINE(boot_charlie, SAFETY_TEST_CAT_STACK, SAFETY_TEST_LEVEL_POST_KERNEL, 20,
		   SAFETY_TEST_FLAG_BOOT_OK, fn_record, "second");

#ifdef CONFIG_SAFETY_TEST_TESTS_BOOT_PASS
SAFETY_TEST_DEFINE(boot_crit, SAFETY_TEST_CAT_OTHER, SAFETY_TEST_LEVEL_POST_KERNEL, 40,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_CRITICAL, fn_pass,
		   "critical boot, passes");
#endif
#ifdef CONFIG_SAFETY_TEST_TESTS_BOOT_FAIL
SAFETY_TEST_DEFINE(boot_crit, SAFETY_TEST_CAT_OTHER, SAFETY_TEST_LEVEL_POST_KERNEL, 40,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_CRITICAL, fn_fail,
		   "critical boot, fails");
#endif

/* Hooks */

static atomic_t hook_calls;
static int hook_nested_rc;

static void failure_hook(const struct safety_test *test,
			 const struct safety_test_result_record *rec)
{
	ARG_UNUSED(test);
	ARG_UNUSED(rec);

	atomic_inc(&hook_calls);
	hook_nested_rc = safety_test_run_by_name("rt_cpu_ok", 0, NULL);
}
SAFETY_TEST_FAILURE_HOOK_DEFINE(suite_hook, failure_hook);

static atomic_t safe_state_calls;
static int safe_state_nested_rc;
static int safe_state_get_rc;

enum safety_test_action safety_test_safe_state(const struct safety_test *test,
					       const struct safety_test_result_record *rec)
{
	struct safety_test_result_record tmp;

	ARG_UNUSED(test);
	ARG_UNUSED(rec);

	atomic_inc(&safe_state_calls);
	safe_state_nested_rc = safety_test_run_by_name("rt_cpu_ok", 0, NULL);
	safe_state_get_rc = safety_test_get_result(safety_test_get("rt_cpu_ok"), &tmp);

	return SAFETY_TEST_ACTION_CONTINUE;
}

/* Helpers */

struct cat_count {
	enum safety_test_category cat;
	uint32_t n;
};

static bool count_in_category(const struct safety_test *test, void *user_data)
{
	struct cat_count *c = user_data;

	if (test->category == c->cat) {
		c->n++;
	}
	return true;
}

static uint32_t registered_in_category(enum safety_test_category cat)
{
	struct cat_count c = {.cat = cat, .n = 0};

	safety_test_foreach(count_in_category, &c);
	return c.n;
}

static const struct safety_test *must_get(const char *name)
{
	const struct safety_test *t = safety_test_get(name);

	zassert_not_null(t, "test '%s' is not registered", name);
	return t;
}

static struct safety_test_result_record result_of(const char *name)
{
	struct safety_test_result_record rec;

	zassert_ok(safety_test_get_result(must_get(name), &rec));
	return rec;
}

/*
 * Boot-time observations, captured once before any test perturbs the stored
 * records.
 */

static uint8_t captured_order[BOOT_ORDER_MAX];
static uint8_t captured_order_n;
static int captured_boot_rc;
static bool captured_boot_passed;

static void *suite_setup(void)
{
	captured_order_n = boot_order_n;
	memcpy(captured_order, boot_order, sizeof(captured_order));
	captured_boot_rc = safety_test_boot_passed(&captured_boot_passed);

	return NULL;
}

ZTEST_SUITE(safety_test_api, NULL, suite_setup, NULL, NULL, NULL);

/* Registration and lookup */

ZTEST(safety_test_api, test_get_by_name)
{
	const struct safety_test *t = must_get("rt_cpu_ok");

	zassert_str_equal(t->name, "rt_cpu_ok");
	zassert_equal(t->category, SAFETY_TEST_CAT_CPU);
	zassert_equal(t->init_level, SAFETY_TEST_LEVEL_APPLICATION);
	zassert_is_null(safety_test_get("no_such_test"));
	zassert_is_null(safety_test_get(NULL));
}

static bool count_all(const struct safety_test *test, void *user_data)
{
	ARG_UNUSED(test);
	(*(uint32_t *)user_data)++;
	return true;
}

static bool stop_after_first(const struct safety_test *test, void *user_data)
{
	ARG_UNUSED(test);
	(*(uint32_t *)user_data)++;
	return false;
}

ZTEST(safety_test_api, test_foreach)
{
	uint32_t all = 0;
	uint32_t one = 0;

	safety_test_foreach(count_all, &all);
	zassert_true(all >= 8, "expected at least the suite's own tests, saw %u", all);

	safety_test_foreach(stop_after_first, &one);
	zassert_equal(one, 1, "returning false must stop iteration");

	safety_test_foreach(NULL, NULL);
}

/* Argument validation */

ZTEST(safety_test_api, test_invalid_arguments)
{
	struct safety_test_result_record rec;
	struct safety_test_summary summary;
	struct safety_test_stats stats;
	bool passed;

	zassert_equal(safety_test_run(NULL, 0, NULL), -EINVAL);
	zassert_equal(safety_test_run_by_name(NULL, 0, NULL), -EINVAL);
	zassert_equal(safety_test_run_by_name("no_such_test", 0, NULL), -ENOENT);
	zassert_equal(safety_test_get_result(NULL, &rec), -EINVAL);
	zassert_equal(safety_test_get_result(must_get("rt_cpu_ok"), NULL), -EINVAL);
	zassert_equal(safety_test_get_summary(NULL), -EINVAL);
	zassert_equal(safety_test_boot_passed(NULL), -EINVAL);
	zassert_equal(safety_test_run_category(0, &stats), -EINVAL);
	zassert_equal(safety_test_run_level(SAFETY_TEST_LEVEL_APPLICATION + 1, NULL), -EINVAL);

	ARG_UNUSED(summary);
	ARG_UNUSED(passed);
}

/* Execution and policy */

ZTEST(safety_test_api, test_run_pass_and_fail)
{
	enum safety_test_result r;
	struct safety_test_result_record rec;

	zassert_ok(safety_test_run_by_name("rt_cpu_ok", 0, &r));
	zassert_equal(r, SAFETY_TEST_RESULT_PASS);
	rec = result_of("rt_cpu_ok");
	zassert_equal(rec.result, SAFETY_TEST_RESULT_PASS);
	zassert_equal(rec.error_code, 0);

	zassert_ok(safety_test_run_by_name("rt_ram_fail", 0, &r));
	zassert_equal(r, SAFETY_TEST_RESULT_FAIL);
	rec = result_of("rt_ram_fail");
	zassert_equal(rec.result, SAFETY_TEST_RESULT_FAIL);
	zassert_equal(rec.error_code, -EIO, "the test's own return value must be preserved");
}

ZTEST(safety_test_api, test_boot_only_test_is_skipped_at_runtime)
{
	struct safety_test_result_record before = result_of("boot_bravo");
	struct safety_test_result_record after;
	enum safety_test_result r;

	zassert_ok(safety_test_run_by_name("boot_bravo", 0, &r));
	zassert_equal(r, SAFETY_TEST_RESULT_SKIP, "no RUNTIME_OK flag");

	after = result_of("boot_bravo");
	zassert_equal(after.result, before.result, "a skip must not overwrite the record");
	zassert_equal(after.duration_us, before.duration_us);
}

ZTEST(safety_test_api, test_destructive_needs_opt_in)
{
	enum safety_test_result r;

	zassert_ok(safety_test_run_by_name("rt_destructive", 0, &r));
	zassert_equal(r, SAFETY_TEST_RESULT_SKIP, "destructive without the opt-in");

	zassert_ok(safety_test_run_by_name("rt_destructive",
					   SAFETY_TEST_OPT_ALLOW_DESTRUCTIVE, &r));
	zassert_equal(r, SAFETY_TEST_RESULT_PASS, "destructive with the opt-in");
}

/* Category mask encoding */

ZTEST(safety_test_api, test_category_mask_selects_that_category)
{
	struct safety_test_stats stats;

	zassert_ok(safety_test_run_category(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_FLASH), &stats));
	zassert_equal(stats.total, registered_in_category(SAFETY_TEST_CAT_FLASH));
	zassert_equal(stats.passed, 1, "rt_flash_ok is the only FLASH test and it passes");
	zassert_equal(stats.failed, 0);
}

ZTEST(safety_test_api, test_category_mask_combines)
{
	struct safety_test_stats stats;
	uint32_t expect = registered_in_category(SAFETY_TEST_CAT_CPU) +
			  registered_in_category(SAFETY_TEST_CAT_FLASH);

	zassert_ok(safety_test_run_category(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_CPU) |
					    SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_FLASH), &stats));
	zassert_equal(stats.total, expect);
	zassert_equal(stats.passed, expect);
}

/*
 * Regression guard for the original defect: the mask is bit-encoded, so a raw
 * category value selects whatever category sits at that bit position, not the
 * category itself. SAFETY_TEST_CAT_RAM is 1, which is BIT(SAFETY_TEST_CAT_CPU).
 */
ZTEST(safety_test_api, test_category_mask_is_bit_encoded_not_value_encoded)
{
	struct safety_test_stats by_raw_value;
	struct safety_test_stats by_cpu_bit;

	BUILD_ASSERT(SAFETY_TEST_CAT_RAM == 1, "this guard assumes CAT_RAM is enum value 1");
	BUILD_ASSERT(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_CPU) == 1);

	zassert_ok(safety_test_run_category(SAFETY_TEST_CAT_RAM, &by_raw_value));
	zassert_ok(safety_test_run_category(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_CPU),
					    &by_cpu_bit));

	zassert_equal(by_raw_value.total, by_cpu_bit.total);
	zassert_equal(by_raw_value.passed, by_cpu_bit.passed);
	zassert_equal(by_raw_value.total, registered_in_category(SAFETY_TEST_CAT_CPU),
		      "a raw category value must select by bit position, not by value");
}

ZTEST(safety_test_api, test_category_mask_all)
{
	struct safety_test_stats stats;
	uint32_t all = 0;

	safety_test_foreach(count_all, &all);

	zassert_ok(safety_test_run_category(UINT32_MAX, &stats));
	zassert_equal(stats.total, all, "UINT32_MAX must consider every registered test");
	zassert_true(stats.skipped > 0, "boot-only tests must be counted as skipped");
}

/* Duration budget */

ZTEST(safety_test_api, test_budget_overrun_is_recorded)
{
	struct safety_test_result_record rec;

	zassert_ok(safety_test_run_by_name("rt_slow", 0, NULL));
	rec = result_of("rt_slow");

	zassert_true(rec.over_budget, "a %u us test must exceed a %u us budget", SLOW_US,
		     TIGHT_BUDGET);
	zassert_true(rec.duration_us >= SLOW_US, "duration %u us is implausibly short",
		     rec.duration_us);
	zassert_equal(rec.error_code, 0, "the test returned 0 and that must survive");

	if (IS_ENABLED(CONFIG_SAFETY_TEST_BUDGET_FAILS)) {
		zassert_equal(rec.result, SAFETY_TEST_RESULT_FAIL,
			      "with BUDGET_FAILS the overrun becomes the verdict");
	} else {
		zassert_equal(rec.result, SAFETY_TEST_RESULT_PASS,
			      "by default an overrun must not fail a sound test");
	}
}

/* The overrun used to be dropped entirely when the test had already failed. */
ZTEST(safety_test_api, test_budget_overrun_recorded_on_a_failing_test)
{
	struct safety_test_result_record rec;

	zassert_ok(safety_test_run_by_name("rt_slow_fail", 0, NULL));
	rec = result_of("rt_slow_fail");

	zassert_equal(rec.result, SAFETY_TEST_RESULT_FAIL);
	zassert_equal(rec.error_code, -EIO, "-ETIME must not overwrite the test's own error");
	zassert_true(rec.over_budget, "an overrun on a failing test must still be recorded");
}

ZTEST(safety_test_api, test_within_budget_is_not_flagged)
{
	struct safety_test_result_record rec;

	zassert_ok(safety_test_run_by_name("rt_quick", 0, NULL));
	rec = result_of("rt_quick");

	zassert_equal(rec.result, SAFETY_TEST_RESULT_PASS);
	zassert_false(rec.over_budget);
}

ZTEST(safety_test_api, test_stats_count_overruns)
{
	struct safety_test_stats stats;

	zassert_ok(safety_test_run_category(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_ADC), &stats));
	zassert_equal(stats.total, registered_in_category(SAFETY_TEST_CAT_ADC));
	zassert_equal(stats.over_budget, 1, "an advisory overrun must stay visible in stats");

	zassert_ok(safety_test_run_category(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_COMM), &stats));
	zassert_equal(stats.over_budget, 0);
}

/* Reentrancy */

ZTEST(safety_test_api, test_failure_hook_may_not_run_tests)
{
	atomic_val_t before = atomic_get(&hook_calls);

	hook_nested_rc = 0;
	zassert_ok(safety_test_run_by_name("rt_ram_fail", 0, NULL));

	zassert_true(atomic_get(&hook_calls) > before, "the failure hook must have run");
	zassert_equal(hook_nested_rc, -EALREADY,
		      "a hook calling back into the run API must be rejected");
}

ZTEST(safety_test_api, test_safe_state_may_not_run_tests)
{
	atomic_val_t before = atomic_get(&safe_state_calls);

	safe_state_nested_rc = 0;
	safe_state_get_rc = -1;
	zassert_ok(safety_test_run_by_name("rt_crit_fail", 0, NULL));

	zassert_true(atomic_get(&safe_state_calls) > before, "safe_state must have run");
	zassert_equal(safe_state_nested_rc, -EALREADY,
		      "safe_state calling back into the run API must be rejected");
	zassert_ok(safe_state_get_rc, "safe_state must still be able to read results");
}

/* Interrupt context */

static volatile bool isr_done;
static bool isr_was_isr;
static int isr_get_rc;
static int isr_summary_rc;
static int isr_boot_rc;

static void isr_probe(struct k_timer *timer)
{
	struct safety_test_result_record rec;
	struct safety_test_summary summary;
	bool passed;

	ARG_UNUSED(timer);

	isr_was_isr = k_is_in_isr();
	isr_get_rc = safety_test_get_result(safety_test_get("rt_cpu_ok"), &rec);
	isr_summary_rc = safety_test_get_summary(&summary);
	isr_boot_rc = safety_test_boot_passed(&passed);
	isr_done = true;
}
static K_TIMER_DEFINE(isr_timer, isr_probe, NULL);

ZTEST(safety_test_api, test_queries_are_usable_from_an_isr)
{
	isr_done = false;
	k_timer_start(&isr_timer, K_MSEC(20), K_NO_WAIT);

	for (int i = 0; i < 100 && !isr_done; i++) {
		k_msleep(10);
	}

	zassert_true(isr_done, "the timer callback never ran");
	zassert_true(isr_was_isr, "a k_timer expiry is expected to run in ISR context");
	zassert_ok(isr_get_rc, "safety_test_get_result() must work from an ISR");
	zassert_ok(isr_summary_rc, "safety_test_get_summary() must work from an ISR");

	if (IS_ENABLED(CONFIG_SAFETY_TEST_TESTS_BOOT_NONE)) {
		zassert_equal(isr_boot_rc, -ENOENT);
	} else {
		zassert_ok(isr_boot_rc, "safety_test_boot_passed() must work from an ISR");
	}
}

/* Boot behaviour */

ZTEST(safety_test_api, test_boot_tests_ran_in_priority_order)
{
	zassert_equal(captured_order_n, 3, "all three boot probes must have run");
	zassert_equal(captured_order[0], 10);
	zassert_equal(captured_order[1], 20);
	zassert_equal(captured_order[2], 30,
		      "boot tests must run by ascending priority, not by symbol name");
}

ZTEST(safety_test_api, test_boot_passed_reports_three_distinct_answers)
{
	if (IS_ENABLED(CONFIG_SAFETY_TEST_TESTS_BOOT_NONE)) {
		zassert_equal(captured_boot_rc, -ENOENT,
			      "no critical boot test is not the same answer as a failure");
	} else if (IS_ENABLED(CONFIG_SAFETY_TEST_TESTS_BOOT_FAIL)) {
		zassert_ok(captured_boot_rc);
		zassert_false(captured_boot_passed, "a failing critical boot test must not pass");
	} else {
		zassert_ok(captured_boot_rc);
		zassert_true(captured_boot_passed, "every critical boot test passed");
	}
}

ZTEST(safety_test_api, test_boot_passed_leaves_output_alone_on_error)
{
	bool sentinel = true;
	int rc;

	Z_TEST_SKIP_IFNDEF(CONFIG_SAFETY_TEST_TESTS_BOOT_NONE);

	rc = safety_test_boot_passed(&sentinel);
	zassert_equal(rc, -ENOENT);
	zassert_true(sentinel, "the output must not be written when the call fails");
}

ZTEST(safety_test_api, test_run_level_is_idempotent_for_boot_tests)
{
	struct safety_test_stats stats;
	uint32_t boot_tests = 0;

	boot_order_n = 0;
	zassert_ok(safety_test_run_level(SAFETY_TEST_LEVEL_POST_KERNEL, &stats));

	safety_test_foreach(count_all, &boot_tests);
	zassert_true(stats.total >= 3, "the three boot probes must have run again");
	zassert_equal(boot_order_n, 3);
	zassert_equal(boot_order[0], 10);
	zassert_equal(boot_order[2], 30);
}

/* Summary */

ZTEST(safety_test_api, test_summary_totals_match_registrations)
{
	struct safety_test_summary summary;
	uint32_t all = 0;
	uint32_t sum_of_categories = 0;

	safety_test_foreach(count_all, &all);
	zassert_ok(safety_test_get_summary(&summary));

	zassert_equal(summary.global.total, all);
	zassert_equal(summary.global.total,
		      summary.global.passed + summary.global.failed +
		      summary.global.skipped + summary.global.not_run,
		      "the outcome buckets must account for every test");

	for (int i = 0; i < SAFETY_TEST_CAT_COUNT; i++) {
		sum_of_categories += summary.categories[i].total;
	}
	zassert_equal(sum_of_categories, all, "per-category totals must sum to the global");

	zassert_equal(summary.categories[SAFETY_TEST_CAT_STACK].total,
		      registered_in_category(SAFETY_TEST_CAT_STACK));
}

ZTEST(safety_test_api, test_summary_reports_stored_state)
{
	struct safety_test_summary summary;

	zassert_ok(safety_test_run_by_name("rt_ram_fail", 0, NULL));
	zassert_ok(safety_test_get_summary(&summary));

	zassert_true(summary.global.failed >= 1);
	zassert_equal(summary.categories[SAFETY_TEST_CAT_RAM].failed, 1);
}
