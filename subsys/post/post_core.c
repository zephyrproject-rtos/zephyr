/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POST subsystem core implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/post/post.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(post, CONFIG_POST_LOG_LEVEL);

/* Iterable section bounds */
STRUCT_SECTION_START_EXTERN(post_test);
STRUCT_SECTION_END_EXTERN(post_test);

/* Result storage */
static struct post_result_record results[CONFIG_POST_RESULT_BUFFER_SIZE];
static uint32_t result_count;

/* Failure hooks */
struct failure_hook {
	post_failure_cb cb;
	void *user_data;
};

static struct failure_hook failure_hooks[CONFIG_POST_MAX_FAILURE_HOOKS];
static uint32_t hook_count;

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
 * This provides sequential IDs (0, 1, 2, ...) based on linker sort order.
 */
static inline uint32_t post_compute_test_id(const struct post_test *test)
{
	extern struct post_test _post_test_list_start[];
	return (uint32_t)(test - _post_test_list_start);
}

static struct post_result_record *find_result(uint32_t test_id)
{
	for (uint32_t i = 0; i < result_count; i++) {
		if (results[i].test_id == test_id) {
			return &results[i];
		}
	}
	return NULL;
}

static struct post_result_record *alloc_result(const struct post_test *test)
{
	uint32_t test_id = post_compute_test_id(test);
	
	/* Check if already exists */
	struct post_result_record *rec = find_result(test_id);

	if (rec != NULL) {
		return rec;
	}

	/* Allocate new slot */
	if (result_count < CONFIG_POST_RESULT_BUFFER_SIZE) {
		rec = &results[result_count++];
		rec->test_id = test_id;
		rec->result = POST_RESULT_NOT_RUN;
		return rec;
	}

	LOG_WRN("Result buffer full, cannot store result for test %u", test_id);
	return NULL;
}

static void invoke_failure_hooks(const struct post_test *test,
				 const struct post_result_record *result)
{
	for (uint32_t i = 0; i < hook_count; i++) {
		if (failure_hooks[i].cb != NULL) {
			failure_hooks[i].cb(test, result,
					   failure_hooks[i].user_data);
		}
	}
}

static enum post_result execute_test(const struct post_test *test)
{
	uint32_t test_id = post_compute_test_id(test);
	
	struct post_context ctx = {
		.init_level = test->init_level,
		.test_id = test_id,
		.start_time = 0,
		.user_data = NULL,
	};

	uint64_t start_us = 0;
	uint64_t duration = 0;
	enum post_result result;

	/* Only use timing APIs after POST_KERNEL level */
	if (test->init_level >= POST_LEVEL_POST_KERNEL) {
		ctx.start_time = k_cycle_get_64();
		start_us = k_ticks_to_us_floor64(k_uptime_ticks());
	}

	LOG_INF("Running test: %s (ID=%u)", test->name, test_id);

	/* Execute the test */
	result = test->test_fn(&ctx);

	if (test->init_level >= POST_LEVEL_POST_KERNEL) {
		uint64_t end_us = k_ticks_to_us_floor64(k_uptime_ticks());
		duration = end_us - start_us;
	}

	/* Store result */
	struct post_result_record *rec = alloc_result(test);

	if (rec != NULL) {
		rec->result = result;
		rec->duration_us = duration;
		rec->error_code = 0;
		rec->error_data = 0;
	}

	/* Update statistics */
	switch (result) {
	case POST_RESULT_PASS:
		total_passed++;
		LOG_INF("  PASS (%u us)", (uint32_t)duration);
		break;
	case POST_RESULT_FAIL:
		total_failed++;
		LOG_ERR("  FAIL (%u us)", (uint32_t)duration);
		invoke_failure_hooks(test, rec);
		break;
	case POST_RESULT_SKIP:
		total_skipped++;
		LOG_WRN("  SKIP");
		break;
	case POST_RESULT_ERROR:
		total_failed++;
		LOG_ERR("  ERROR");
		invoke_failure_hooks(test, rec);
		break;
	default:
		break;
	}

	return result;
}

/*
 * Public API
 */

int post_run_level(enum post_init_level level)
{
	int failures = 0;

	LOG_INF("POST: Running level %d tests", level);

	STRUCT_SECTION_FOREACH(post_test, test) {
		if (test->init_level != level) {
			continue;
		}

		enum post_result result = execute_test(test);

		if (result == POST_RESULT_FAIL || result == POST_RESULT_ERROR) {
			failures++;

			if ((test->flags & POST_FLAG_CRITICAL) &&
			    IS_ENABLED(CONFIG_POST_HALT_ON_FAILURE)) {
				LOG_ERR("Critical test failed, halting!");
				k_panic();
			}
		}
	}

	return failures;
}

enum post_result post_run_test(uint32_t test_id)
{
	extern struct post_test _post_test_list_start[];
	extern struct post_test _post_test_list_end[];
	
	/* Direct array access - O(1) lookup */
	if (test_id >= (uint32_t)(_post_test_list_end - _post_test_list_start)) {
		LOG_ERR("Test ID %u not found", test_id);
		return POST_RESULT_ERROR;
	}

	const struct post_test *test = &_post_test_list_start[test_id];

	if (!IS_ENABLED(CONFIG_POST_RUNTIME_TESTS) &&
	    !(test->flags & POST_FLAG_BOOT_ONLY)) {
		LOG_WRN("Runtime tests disabled");
		return POST_RESULT_SKIP;
	}

	if (!(test->flags & POST_FLAG_RUNTIME_OK) &&
	    !(test->flags & POST_FLAG_BOOT_ONLY)) {
		LOG_WRN("Test %u not safe for runtime", test_id);
		return POST_RESULT_SKIP;
	}

	return execute_test(test);
}

int post_run_category(uint32_t category)
{
	int failures = 0;

	STRUCT_SECTION_FOREACH(post_test, test) {
		if (!(test->category & category)) {
			continue;
		}

		if (!(test->flags & POST_FLAG_RUNTIME_OK)) {
			continue;
		}

		enum post_result result = execute_test(test);

		if (result == POST_RESULT_FAIL || result == POST_RESULT_ERROR) {
			failures++;
		}
	}

	return failures;
}

int post_get_result(uint32_t test_id, struct post_result_record *record)
{
	if (record == NULL) {
		return -EINVAL;
	}

	struct post_result_record *rec = find_result(test_id);

	if (rec == NULL) {
		return -ENOENT;
	}

	*record = *rec;
	return 0;
}

int post_get_summary(uint32_t *passed, uint32_t *failed, uint32_t *skipped)
{
	if (passed != NULL) {
		*passed = total_passed;
	}
	if (failed != NULL) {
		*failed = total_failed;
	}
	if (skipped != NULL) {
		*skipped = total_skipped;
	}

	return post_get_test_count();
}

int post_register_failure_hook(post_failure_cb cb, void *user_data)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (hook_count >= CONFIG_POST_MAX_FAILURE_HOOKS) {
		return -ENOMEM;
	}

	failure_hooks[hook_count].cb = cb;
	failure_hooks[hook_count].user_data = user_data;
	hook_count++;

	return 0;
}

const struct post_test *post_get_test(uint32_t test_id)
{
	extern struct post_test _post_test_list_start[];
	extern struct post_test _post_test_list_end[];
	
	/* Direct array access - O(1) lookup */
	if (test_id >= (uint32_t)(_post_test_list_end - _post_test_list_start)) {
		return NULL;
	}

	return &_post_test_list_start[test_id];
}

int post_get_test_count(void)
{
	int count = 0;

	STRUCT_SECTION_FOREACH(post_test, test) {
		ARG_UNUSED(test);
		count++;
	}

	return count;
}

/*
 * Boot-time initialization
 */

#ifdef CONFIG_POST_EARLY_TESTS
static int post_init_early(void)
{
	return post_run_level(POST_LEVEL_EARLY);
}

SYS_INIT(post_init_early, EARLY, 99);
#endif

static int post_init_pre_kernel_1(void)
{
	return post_run_level(POST_LEVEL_PRE_KERNEL_1);
}

SYS_INIT(post_init_pre_kernel_1, PRE_KERNEL_1, 99);

static int post_init_pre_kernel_2(void)
{
	return post_run_level(POST_LEVEL_PRE_KERNEL_2);
}

SYS_INIT(post_init_pre_kernel_2, PRE_KERNEL_2, 99);

static int post_init_post_kernel(void)
{
	return post_run_level(POST_LEVEL_POST_KERNEL);
}

SYS_INIT(post_init_post_kernel, POST_KERNEL, 99);

static int post_init_application(void)
{
	int result = post_run_level(POST_LEVEL_APPLICATION);

	LOG_INF("POST complete: %u passed, %u failed, %u skipped",
		total_passed, total_failed, total_skipped);

	return result;
}

SYS_INIT(post_init_application, APPLICATION, 99);
