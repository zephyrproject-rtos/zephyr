/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include "common.h"

#define num_registered_suites (_ztest_suite_node_list_end - _ztest_suite_node_list_start)

/** The current state of the test application. */
static struct global_test_state global_state;

/**
 * Copies of the test entry's snapshot, used to test assertions. There's no good way to get the
 * number of registered test suites at compile time so this is set to an arbitrary large size that
 * should be enough even if the number of tests grows. There's a runtime check for this in
 * test_verify_execution.
 */
static struct ztest_suite_stats stats_snapshot[128];

/** The results of a single execution */
struct execution_results {
	/** The test phase that was run */
	enum phase test_phase;
	/** The number of tests that ran */
	int test_run_count;
} execution_results;

/**
 * Helper function used to find a test entry by name.
 *
 * @param name The name of the test entry.
 * @return Pointer to the struct unit_test_node or NULL if not found.
 */
static struct ztest_suite_node *find_test_node(const char *name)
{
	struct ztest_suite_node *ptr;

	for (ptr = _ztest_suite_node_list_start; ptr != _ztest_suite_node_list_end; ++ptr) {
		if (strcmp(ptr->name, name) == 0) {
			return ptr;
		}
	}

	return NULL;
}

/**
 * @brief Find a snapshot in the stats_snapshot array
 *
 * Lookup a test case by name and find the matching ztest_suite_stats.
 *
 * @param name The name of the test entry.
 * @return Pointer to the stats snapshot.
 */
static struct ztest_suite_stats *find_snapshot(const char *name)
{
	int index = find_test_node(name) - _ztest_suite_node_list_start;

	return stats_snapshot + index;
}

/**
 * Reset the global state between phases. This function can be thought of similarly to making a
 * change affecting the state of the application being tested.
 *
 * @param phase The new phase of the application.
 */
static void reset_state(enum phase phase)
{
	execution_results.test_phase = phase;
	execution_results.test_run_count = 0;
	global_state.phase = phase;

	for (int i = 0; i < num_registered_suites; ++i) {
		stats_snapshot[i] = *_ztest_suite_node_list_start[i].stats;
	}
}

/**
 * Create a snapshot of the tests' stats. This function should be called after each run in order
 * to assert on only the changes in the stats.
 */
static void take_stats_snapshot(void)
{
	for (int i = 0; i < num_registered_suites; ++i) {
		struct ztest_suite_stats *snapshot = stats_snapshot + i;
		struct ztest_suite_stats *current = _ztest_suite_node_list_start[i].stats;

		snapshot->run_count = current->run_count - snapshot->run_count;
		snapshot->skip_count = current->skip_count - snapshot->skip_count;
		snapshot->fail_count = current->fail_count - snapshot->fail_count;
	}
}

static void test_verify_execution(void)
{
	const struct ztest_suite_stats *stats;

	zassert_true(ARRAY_SIZE(stats_snapshot) >= num_registered_suites,
		     "Not enough stats snapshots, please allocate more.");
	switch (execution_results.test_phase) {
	case PHASE_NULL_PREDICATE_0:
		/* Verify that only remove_first_node suite was run and removed. */
		stats = find_snapshot("run_null_predicate_once");
		zassert_equal(1, execution_results.test_run_count);
		zassert_equal(1, stats->run_count);
		zassert_equal(0, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		break;
	case PHASE_NULL_PREDICATE_1:
		/* Verify that only remove_first_two_nodes_* were run. */
		zassert_equal(0, execution_results.test_run_count);
		stats = find_snapshot("run_null_predicate_once");
		zassert_equal(0, stats->run_count);
		zassert_equal(1, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		break;
	case PHASE_STEPS_0:
		/* Verify that steps_0 and steps_all suites were run. */
		zassert_equal(2, execution_results.test_run_count);
		stats = find_snapshot("test_step_0");
		zassert_equal(1, stats->run_count);
		zassert_equal(0, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		stats = find_snapshot("test_step_1");
		zassert_equal(0, stats->run_count);
		zassert_equal(1, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		stats = find_snapshot("test_step_all");
		zassert_equal(1, stats->run_count);
		zassert_equal(0, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		break;
	case PHASE_STEPS_1:
		/* Verify that steps_1 and steps_all suites were run. */
		zassert_equal(2, execution_results.test_run_count);
		stats = find_snapshot("test_step_0");
		zassert_equal(0, stats->run_count);
		zassert_equal(1, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		stats = find_snapshot("test_step_1");
		zassert_equal(1, stats->run_count);
		zassert_equal(0, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		stats = find_snapshot("test_step_all");
		zassert_equal(1, stats->run_count);
		zassert_equal(0, stats->skip_count);
		zassert_equal(0, stats->fail_count);
		break;
	default:
		ztest_test_fail();
	}
}

static bool verify_predicate(const void *state)
{
	const struct global_test_state *s = state;

	return s->phase == PHASE_VERIFY;
}

ztest_register_test_suite(verify, verify_predicate,
			  ztest_unit_test(test_verify_execution));

void test_main(void)
{
	/* Make sure that when predicate is set to NULL, the test is run. */
	reset_state(PHASE_NULL_PREDICATE_0);
	execution_results.test_run_count = ztest_run_registered_test_suites(&global_state);
	take_stats_snapshot();
	global_state.phase = PHASE_VERIFY;
	ztest_run_registered_test_suites(&global_state);

	/* Try running the tests again, nothing should run. */
	reset_state(PHASE_NULL_PREDICATE_1);
	execution_results.test_run_count = ztest_run_registered_test_suites(&global_state);
	take_stats_snapshot();
	global_state.phase = PHASE_VERIFY;
	ztest_run_registered_test_suites(&global_state);

	/* Run filter tests for step 0. */
	reset_state(PHASE_STEPS_0);
	execution_results.test_run_count = ztest_run_registered_test_suites(&global_state);
	global_state.phase = PHASE_VERIFY;
	take_stats_snapshot();
	ztest_run_registered_test_suites(&global_state);

	/* Run filter tests for step 1. */
	reset_state(PHASE_STEPS_1);
	execution_results.test_run_count = ztest_run_registered_test_suites(&global_state);
	global_state.phase = PHASE_VERIFY;
	take_stats_snapshot();
	ztest_run_registered_test_suites(&global_state);
}
