/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Safety Test subsystem core implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/safety_test/safety_test.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(safety_test, CONFIG_SAFETY_TEST_LOG_LEVEL);

/* Iterable section bounds */
STRUCT_SECTION_START_EXTERN(safety_test);
STRUCT_SECTION_END_EXTERN(safety_test);

/* Result storage */
static struct safety_test_result_record results[CONFIG_SAFETY_TEST_RESULT_BUFFER_SIZE];
static uint32_t result_count;

/* Failure hooks */
struct failure_hook {
	safety_test_failure_cb cb;
	void *user_data;
};

static struct failure_hook failure_hooks[CONFIG_SAFETY_TEST_RESULT_BUFFER_SIZE];
static uint32_t hook_count;

/* Safe state callback (for critical test failures) */
static safety_test_safe_state_cb_t safe_state_cb;
static void *safe_state_user_data;

/* Category critical flags (bitmask of critical categories) */
static uint32_t critical_categories;

/* Statistics */
static uint32_t total_passed;
static uint32_t total_failed;
static uint32_t total_skipped;

/*
 * Internal helpers
 */

/**
 * @brief Compute test ID from test pointer position
 *
 * Test IDs are computed from the test's position in the sorted array.
 * This provides sequential IDs based on linker sort order
 */
static inline uint32_t safety_test_compute_test_id(const struct safety_test *test)
{
	extern struct safety_test _safety_test_list_start[];

	return (uint32_t)(test - _safety_test_list_start);
}

static struct safety_test_result_record *find_result(uint32_t test_id)
{
	for (uint32_t i = 0; i < result_count; i++) {
		if (results[i].test_id == test_id) {
			return results + i;
		}
	}

	return NULL;
}

static struct safety_test_result_record *alloc_result(const struct safety_test *test)
{
	uint32_t test_id = safety_test_compute_test_id(test);

	struct safety_test_result_record *rec = find_result(test_id);

	if (rec != NULL) {
		return rec;
	}

	if (result_count < CONFIG_SAFETY_TEST_RESULT_BUFFER_SIZE) {
		rec = &results[result_count++];
		rec->test_id = test_id;
		rec->result = SAFETY_TEST_RESULT_NOT_RUN;
		return rec;
	}

	LOG_WRN("Result buffer full, cannot store result for test %u", test_id);

	return NULL;
}

static void invoke_failure_hooks(const struct safety_test *test,
				 const struct safety_test_result_record *result)
{
	for (uint32_t i = 0; i < hook_count; i++) {
		if (failure_hooks[i].cb != NULL) {
			failure_hooks[i].cb(test, result, failure_hooks[i].user_data);
		}
	}
}

static enum safety_test_result execute_test(const struct safety_test *test)
{
	uint32_t test_id = safety_test_compute_test_id(test);

	struct safety_test_context ctx = {
		.init_level = test->init_level,
		.test_id = test_id,
		.user_data = NULL,
		.error_code = 0,
	};

	uint64_t start_us = 0;
	uint64_t duration = 0;
	enum safety_test_result result;

	/* Only use timing APIs after POST_KERNEL level */
	if (test->init_level >= SAFETY_TEST_LEVEL_POST_KERNEL) {
		start_us = k_ticks_to_us_floor64(k_uptime_ticks());
	}

	LOG_INF("Running test: %s (ID=%u)", test->name, test_id);

	/* Execute the test - pass non-const context */
	result = test->test_fn(&ctx);

	if (test->init_level >= SAFETY_TEST_LEVEL_POST_KERNEL) {
		uint64_t end_us = k_ticks_to_us_floor64(k_uptime_ticks());

		duration = end_us - start_us;
	}

	if ((test->timeout_us != 0) && (duration > test->timeout_us)) {
		result = SAFETY_TEST_RESULT_FAIL;
		LOG_ERR("Test timed out");
	}

	struct safety_test_result_record *rec = alloc_result(test);

	if (rec != NULL) {
		rec->result = result;
		rec->error_code = ctx.error_code;
		rec->duration_us = duration;

		/* If test failed but didn't set error code, set a default one */
		if ((result == SAFETY_TEST_RESULT_FAIL) && (ctx.error_code == 0)) {
			/* Set default error code based on result type */
			rec->error_code = -(int)(0x10000 | (test_id & 0xFFFF));
		}
	}

	/* Update statistics */
	switch (result) {
	case SAFETY_TEST_RESULT_PASS:
		total_passed++;
		LOG_INF("  PASS (%u us)", (uint32_t)duration);
		break;
	case SAFETY_TEST_RESULT_FAIL:
		total_failed++;
		LOG_ERR("  FAIL (%u us)", (uint32_t)duration);
		invoke_failure_hooks(test, rec);
		break;
	case SAFETY_TEST_RESULT_SKIP:
		total_skipped++;
		LOG_WRN("  SKIP");
		break;
	default:
		break;
	}

	return result;
}

/**
 * @brief Check if the test is critical
 *
 * @param test Test descriptor
 * @return true if critical, false otherwise
 */
static bool is_test_critical(const struct safety_test *test)
{
	if (test->flags & SAFETY_TEST_FLAG_CRITICAL) {
		return true;
	}

	if (critical_categories & test->category) {
		return true;
	}

	return false;
}

/**
 * @brief Handle critical test failure
 *
 * Invokes safe state callback (if registered) and then halts the system.
 *
 * @param test Failed test
 * @param result Test result record
 */
static void handle_critical_failure(const struct safety_test *test,
				    const struct safety_test_result_record *result)
{
	LOG_ERR("Critical test '%s' (ID=%u) failed, transitioning to safe state", test->name,
		test->id);

	/* Invoke safe state callback if registered */
	if (safe_state_cb != NULL) {
		safe_state_cb(test, result, safe_state_user_data);
	}

	/* Halt the system */
	LOG_ERR("System halted due to critical test failure");
	k_panic();
}

/*
 * Public API
 */

int safety_test_run_level(enum safety_test_init_level level)
{
	int failures = 0;

	LOG_INF("Safety test: Running level %d tests", level);

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if (test->init_level != level) {
			continue;
		}

		if (test->flags & SAFETY_TEST_FLAG_ON_DEMAND_ONLY) {
			continue;
		}

		if (test->flags & SAFETY_TEST_FLAG_BOOT_OK) {
			enum safety_test_result result = execute_test(test);

			if (result == SAFETY_TEST_RESULT_FAIL) {
				failures++;
				if (is_test_critical(test)) {
					struct safety_test_result_record *rec =
						find_result(safety_test_compute_test_id(test));

					handle_critical_failure(test, rec);
				}
			}
		}
	}

	return failures;
}

/* Runtime test execution */
enum safety_test_result safety_test_run_test(uint32_t test_id)
{
	extern struct safety_test _safety_test_list_start[];

	enum safety_test_result result;
	uint32_t test_count = safety_test_get_test_count();

	if (test_id >= test_count) {
		LOG_ERR("Test ID %u not found", test_id);
		return SAFETY_TEST_RESULT_FAIL;
	}

	const struct safety_test *test = &_safety_test_list_start[test_id];

	/* Execute on demand flags */
	if (test->flags & SAFETY_TEST_FLAG_ON_DEMAND_ONLY) {
		return execute_test(test);
	}

	/* If flag is destructive, skip execution */
	if (test->flags & SAFETY_TEST_FLAG_DESTRUCTIVE) {
		LOG_WRN("Test %u is destructive may affect system state", test_id);
		return SAFETY_TEST_RESULT_SKIP;
	}

	/* If flag is runtime ok then execute test */
	if (test->flags & SAFETY_TEST_FLAG_RUNTIME_OK) {
		/* Warn user that test is critical */
		if (is_test_critical(test)) {
			LOG_WRN("Test %u is Critical", test_id);
		}

		/* Warn user that test is executed from user space */
		if (test->flags & SAFETY_TEST_FLAG_USERSPACE_OK) {
			LOG_WRN("Test %u is called from user space", test_id);
		}

		/* Execute test */
		result = execute_test(test);

		/* Handle if critical test fails */
		if ((result == SAFETY_TEST_RESULT_FAIL) && is_test_critical(test)) {
			struct safety_test_result_record *rec = find_result(test_id);

			handle_critical_failure(test, rec);
		}

		return result;
	}

	LOG_WRN("Test can only be executed at boot time");
	return SAFETY_TEST_RESULT_SKIP;
}

int safety_test_run_category(uint32_t category)
{
	int failures = 0;

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if (!(test->category & category)) {
			continue;
		}

		if (!(test->flags & SAFETY_TEST_FLAG_RUNTIME_OK) &&
		    !(test->flags & SAFETY_TEST_FLAG_ON_DEMAND_ONLY)) {
			continue;
		}

		enum safety_test_result result = execute_test(test);

		if (result == SAFETY_TEST_RESULT_FAIL) {
			failures++;

			if (is_test_critical(test)) {
				struct safety_test_result_record *rec;

				test_id = safety_test_compute_test_id(test);
				rec = find_result(test_id);
				handle_critical_failure(test, rec);
			}
		}
	}

	return failures;
}

int safety_test_get_result(uint32_t test_id, struct safety_test_result_record *record)
{
	if (record == NULL) {
		return -EINVAL;
	}

	struct safety_test_result_record *rec = find_result(test_id);

	if (rec == NULL) {
		return -ENOENT;
	}

	*record = *rec;

	return 0;
}

static void update_category_summary(const struct safety_test *test, enum safety_test_result result,
				    struct safety_test_category_summary *cat_summary)
{
	cat_summary->total++;

	switch (result) {
	case SAFETY_TEST_RESULT_PASS:
		cat_summary->passed++;
		break;
	case SAFETY_TEST_RESULT_FAIL:
		cat_summary->failed++;
		break;
	case SAFETY_TEST_RESULT_SKIP:
		cat_summary->skipped++;
		break;
	default:
		break;
	}
}

int safety_test_get_summary(struct safety_test_summary *summary)
{
	if (summary == NULL) {
		return -EINVAL;
	}

	memset(summary, 0, sizeof(*summary));

	STRUCT_SECTION_FOREACH(safety_test, test) {
		uint32_t test_id = safety_test_compute_test_id(test);
		struct safety_test_result_record *rec = find_result(test_id);
		enum safety_test_result result;

		result = (rec != NULL) ? rec->result : SAFETY_TEST_RESULT_NOT_RUN;
		summary->global.total++;

		switch (result) {
		case SAFETY_TEST_RESULT_PASS:
			summary->global.passed++;
			break;
		case SAFETY_TEST_RESULT_FAIL:
			summary->global.failed++;
			break;
		case SAFETY_TEST_RESULT_SKIP:
			summary->global.skipped++;
			break;
		default:
			break;
		}

		for (int i = 0; i < SAFETY_TEST_CAT_MAX; i++) {
			uint32_t cat_bit = BIT(i);

			if (test->category & cat_bit) {
				update_category_summary(test, result, &summary->categories[i]);
			}
		}
	}

	return 0;
}

int safety_test_get_category_summary(uint32_t category, struct safety_test_summary *summary)
{
	if (summary == NULL) {
		return -EINVAL;
	}

	memset(summary, 0, sizeof(*summary));

	STRUCT_SECTION_FOREACH(safety_test, test) {
		if ((test->category & category) == 0) {
			continue;
		}

		uint32_t test_id = safety_test_compute_test_id(test);
		struct safety_test_result_record *rec = find_result(test_id);
		enum safety_test_result result;

		result = (rec != NULL) ? rec->result : SAFETY_TEST_RESULT_NOT_RUN;
		summary->global.total++;

		switch (result) {
		case SAFETY_TEST_RESULT_PASS:
			summary->global.passed++;
			break;
		case SAFETY_TEST_RESULT_FAIL:
			summary->global.failed++;
			break;
		case SAFETY_TEST_RESULT_SKIP:
			summary->global.skipped++;
			break;
		default:
			break;
		}

		for (int i = 0; i < SAFETY_TEST_CAT_MAX; i++) {
			uint32_t cat_bit = BIT(i);

			if ((category & cat_bit) && (test->category & cat_bit)) {
				update_category_summary(test, result, &summary->categories[i]);
			}
		}
	}

	return 0;
}

int safety_test_register_failure_hook(safety_test_failure_cb cb, void *user_data)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (hook_count >= CONFIG_SAFETY_TEST_MAX_FAILURE_HOOKS) {
		return -ENOMEM;
	}

	failure_hooks[hook_count].cb = cb;
	failure_hooks[hook_count].user_data = user_data;
	hook_count++;

	return 0;
}

int safety_test_register_safe_state_cb(safety_test_safe_state_cb_t cb, void *user_data)
{
	safe_state_cb = cb;
	safe_state_user_data = user_data;

	return 0;
}

void safety_test_set_category_critical(uint32_t category, bool critical)
{
	if (critical) {
		critical_categories |= category;
		LOG_INF("Category 0x%x marked as critical", category);
	} else {
		critical_categories &= ~category;
		LOG_INF("Category 0x%x no longer critical", category);
	}
}

const struct safety_test *safety_test_get_test(uint32_t test_id)
{
	extern struct safety_test _safety_test_list_start[];

	uint32_t test_count = safety_test_get_test_count();

	if (test_id >= test_count) {
		return NULL;
	}

	return &_safety_test_list_start[test_id];
}

uint32_t safety_test_get_test_count(void)
{
	uint32_t count = 0;

	STRUCT_SECTION_FOREACH(safety_test, test) {
		ARG_UNUSED(test);
		count++;
	}

	return count;
}

/*
 * Boot-time init
 */

#ifdef CONFIG_SAFETY_TEST_EARLY_TESTS
static int safety_test_init_early(void)
{
	return safety_test_run_level(SAFETY_TEST_LEVEL_EARLY);
}

/*
 * TODO: Make the priority a configuration
 */
SYS_INIT(safety_test_init_early, EARLY, 199);
#endif

static int safety_test_init_pre_kernel_1(void)
{
	return safety_test_run_level(SAFETY_TEST_LEVEL_PRE_KERNEL_1);
}

/*
 * TODO: Make the priority a configuration
 */
SYS_INIT(safety_test_init_pre_kernel_1, PRE_KERNEL_1, 199);

static int safety_test_init_pre_kernel_2(void)
{
	return safety_test_run_level(SAFETY_TEST_LEVEL_PRE_KERNEL_2);
}

/*
 * TODO: Make the priority a configuration
 */
SYS_INIT(safety_test_init_pre_kernel_2, PRE_KERNEL_2, 199);

static int safety_test_init_post_kernel(void)
{
	return safety_test_run_level(SAFETY_TEST_LEVEL_POST_KERNEL);
}

/*
 * TODO: Make the priority a configuration
 */
SYS_INIT(safety_test_init_post_kernel, POST_KERNEL, 199);

static int safety_test_init_application(void)
{
	int result = safety_test_run_level(SAFETY_TEST_LEVEL_APPLICATION);
	struct safety_test_summary summary;

	safety_test_get_summary(&summary);

	LOG_INF("Safety Test complete: %u passed, %u failed, %u skipped", summary.global.passed,
		summary.global.failed, summary.global.skipped);

	return result;
}

/*
 * TODO: Make the priority a configuration
 */
SYS_INIT(safety_test_init_application, APPLICATION, 199);
