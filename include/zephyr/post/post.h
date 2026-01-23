/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Power On Self Test (POST) subsystem API
 *
 * This subsystem provides a framework for hardware and software self-testing
 * at boot time and runtime. Tests can be registered at different initialization
 * levels and optionally executed from userspace.
 */

#ifndef ZEPHYR_INCLUDE_POST_H_
#define ZEPHYR_INCLUDE_POST_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup post_api Power On Self Test (POST)
 * @ingroup os_services
 * @{
 */

/**
 * @brief POST test result codes
 */
enum post_result {
	/** Test passed successfully */
	POST_RESULT_PASS = 0,
	/** Test failed - fault detected */
	POST_RESULT_FAIL = 1,
	/** Test skipped (precondition not met) */
	POST_RESULT_SKIP = 2,
	/** Test error (test itself failed to execute) */
	POST_RESULT_ERROR = 3,
	/** Test has not been executed yet */
	POST_RESULT_NOT_RUN = 0xFF,
};

/**
 * @brief POST test categories
 */
enum post_category {
	/** CPU register and instruction tests */
	POST_CAT_CPU = BIT(0),
	/** RAM integrity tests */
	POST_CAT_RAM = BIT(1),
	/** Flash/ROM integrity tests */
	POST_CAT_FLASH = BIT(2),
	/** Clock and timing tests */
	POST_CAT_CLOCK = BIT(3),
	/** Stack integrity tests */
	POST_CAT_STACK = BIT(4),
	/** Watchdog tests */
	POST_CAT_WATCHDOG = BIT(5),
	/** GPIO tests */
	POST_CAT_GPIO = BIT(6),
	/** Communication peripheral tests */
	POST_CAT_COMM = BIT(7),
	/** ADC tests */
	POST_CAT_ADC = BIT(8),
	/** UART loop back tests */
	POST_CAT_UART_LOOPBACK = BIT(9),
	/** Start-Up registers tests */
	POST_CAT_STARTUP_CFG_REG = BIT(10),
	/** DMA tests */
	POST_CAT_DMA = BIT(11),
	/** Interrupt tests */
	POST_CAT_INTERRUPT = BIT(12),
	/** Counter test */
	POST_CAT_COUNTER = BIT(9),
	/** PWM test */
	POST_CAT_PWM = BIT(10),
	/** PWM_Gatekill test */
	POST_CAT_PWM_GATEKILL = BIT(13),
	/** User-defined tests */
	POST_CAT_CUSTOM = BIT(15),
};

/**
 * @brief POST test flags
 */
enum post_test_flags {
	/** Test should only run at boot time */
	POST_FLAG_BOOT_ONLY = BIT(0),
	/** Test is safe to run at runtime */
	POST_FLAG_RUNTIME_OK = BIT(1),
	/** Test may affect system state (destructive) */
	POST_FLAG_DESTRUCTIVE = BIT(2),
	/** Test can be called from userspace */
	POST_FLAG_USERSPACE_OK = BIT(3),
	/** Failure of this test should halt boot */
	POST_FLAG_CRITICAL = BIT(4),
	/** Test is from a vendor library */
	POST_FLAG_VENDOR = BIT(5),
};

/**
 * @brief POST initialization levels (matches Zephyr init levels)
 */
enum post_init_level {
	POST_LEVEL_EARLY = 0,
	POST_LEVEL_PRE_KERNEL_1 = 1,
	POST_LEVEL_PRE_KERNEL_2 = 2,
	POST_LEVEL_POST_KERNEL = 3,
	POST_LEVEL_APPLICATION = 4,
};

/* Forward declaration */
struct post_test;

/**
 * @brief POST test execution context
 *
 * Provides information to the test function about its execution environment.
 */
struct post_context {
	/** Current initialization level */
	uint32_t init_level;
	/** Unique test identifier */
	uint32_t test_id;
	/** Test start timestamp (cycles) */
	uint64_t start_time;
	/** Optional user data */
	void *user_data;
};

/**
 * @brief POST test function signature
 *
 * @param ctx Execution context
 * @return Test result
 */
typedef enum post_result (*post_test_fn)(const struct post_context *ctx);

/**
 * @brief POST test descriptor
 *
 * Describes a single POST test including metadata and the test function.
 */
struct post_test {
	/** Human-readable test name */
	const char *name;
	/** Detailed description */
	const char *description;
	/** Unique test ID (for traceability) */
	uint32_t id;
	/** Test category (POST_CAT_*) */
	uint32_t category;
	/** Earliest init level this test can run */
	uint8_t init_level;
	/** Execution priority within level (0 = highest) */
	uint8_t priority;
	/** Test flags (POST_FLAG_*) */
	uint16_t flags;
	/** Test function */
	post_test_fn test_fn;
	/** Test timeout in milliseconds (0 = no timeout) */
	uint32_t timeout_ms;
};

/**
 * @brief POST test result record
 *
 * Stores the result of a test execution.
 */
struct post_result_record {
	/** Test ID */
	uint32_t test_id;
	/** Test result */
	enum post_result result;
	/** Execution time in microseconds */
	uint64_t duration_us;
	/** Detailed error code (test-specific) */
	uint32_t error_code;
	/** Additional error context data */
	uint32_t error_data;
};

/**
 * @brief POST failure callback signature
 */
typedef void (*post_failure_cb)(const struct post_test *test,
				const struct post_result_record *result,
				void *user_data);

/** @cond INTERNAL_HIDDEN */

/* Internal macro to generate test entry name */
#define Z_POST_TEST_NAME(name) _CONCAT(__post_test_, name)

/* Internal macro for section placement */
#define Z_POST_TEST_SECTION \
	__attribute__((__section__(".post_tests")))

/** @endcond */

/**
 * @brief Define a POST test
 *
 * Registers a test function to be executed at the specified initialization
 * level. Tests are automatically collected into an iterable section.
 * Test IDs are automatically assigned sequentially at link time (0, 1, 2, ...)
 * based on the test's position in the sorted array.
 *
 * @param _name    Unique C identifier for the test
 * @param _cat     Test category (POST_CAT_*)
 * @param _level   Earliest init level (POST_LEVEL_*)
 * @param _prio    Priority within level (0-255, lower = earlier)
 * @param _flags   Test flags (POST_FLAG_*)
 * @param _fn      Test function (post_test_fn)
 * @param _desc    Human-readable description string
 */
#define POST_TEST_DEFINE(_name, _cat, _level, _prio, _flags, _fn, _desc) \
	static const STRUCT_SECTION_ITERABLE(post_test, Z_POST_TEST_NAME(_name)) = { \
		.name = STRINGIFY(_name), \
		.description = _desc, \
		.id = 0, /* Computed from array position at runtime */ \
		.category = _cat, \
		.init_level = _level, \
		.priority = _prio, \
		.flags = _flags, \
		.test_fn = _fn, \
		.timeout_ms = 0, \
	}

/**
 * @brief Define a POST test with timeout
 *
 * Same as POST_TEST_DEFINE but with a timeout value.
 * Test IDs are automatically assigned sequentially at link time (0, 1, 2, ...).
 */
#define POST_TEST_DEFINE_TIMEOUT(_name, _cat, _level, _prio, _flags, \
				 _fn, _desc, _timeout_ms) \
	static const STRUCT_SECTION_ITERABLE(post_test, Z_POST_TEST_NAME(_name)) = { \
		.name = STRINGIFY(_name), \
		.description = _desc, \
		.id = 0, /* Computed from array position at runtime */ \
		.category = _cat, \
		.init_level = _level, \
		.priority = _prio, \
		.flags = _flags, \
		.test_fn = _fn, \
		.timeout_ms = _timeout_ms, \
	}

/**
 * @brief Define a CPU test (convenience macro)
 */
#define POST_CPU_TEST(_name, _level, _fn) \
	POST_TEST_DEFINE(_name, POST_CAT_CPU, _level, 0, \
			 POST_FLAG_RUNTIME_OK | POST_FLAG_CRITICAL, \
			 _fn, "CPU test: " STRINGIFY(_name))

/**
 * @brief Define a RAM test (convenience macro)
 *
 * RAM tests are typically destructive and boot-only.
 */
#define POST_RAM_TEST(_name, _level, _fn) \
	POST_TEST_DEFINE(_name, POST_CAT_RAM, _level, 10, \
			 POST_FLAG_BOOT_ONLY | POST_FLAG_DESTRUCTIVE | \
			 POST_FLAG_CRITICAL, \
			 _fn, "RAM test: " STRINGIFY(_name))

/**
 * @brief Define a Stack test (convenience macro)
 */
#define POST_STACK_TEST(_name, _level, _fn) \
	POST_TEST_DEFINE(_name, POST_CAT_STACK, _level, 20, \
			 POST_FLAG_RUNTIME_OK | POST_FLAG_USERSPACE_OK, \
			 _fn, "Stack test: " STRINGIFY(_name))

/**
 * @brief Define a Flash test (convenience macro)
 */
#define POST_FLASH_TEST(_name, _level, _fn) \
	POST_TEST_DEFINE(_name, POST_CAT_FLASH, _level, 30, \
			 POST_FLAG_RUNTIME_OK | POST_FLAG_USERSPACE_OK, \
			 _fn, "Flash test: " STRINGIFY(_name))

/*
 * Kernel API
 */

/**
 * @brief Run all POST tests for a given initialization level
 *
 * This is called automatically during boot. Tests are executed in
 * priority order within the level.
 *
 * @param level Initialization level to run
 * @return Number of failed tests
 */
int post_run_level(enum post_init_level level);

/**
 * @brief Run a specific test by ID
 *
 * @param test_id Test ID to execute
 * @return Test result
 */
enum post_result post_run_test(uint32_t test_id);

/**
 * @brief Run all tests in a category
 *
 * @param category Test category bitmask (POST_CAT_*)
 * @return Number of failed tests
 */
int post_run_category(uint32_t category);

/**
 * @brief Get test result record
 *
 * @param test_id Test ID to query
 * @param record Pointer to result record to fill
 * @return 0 on success, -ENOENT if not found, -EINVAL if record is NULL
 */
int post_get_result(uint32_t test_id, struct post_result_record *record);

/**
 * @brief Get summary of all POST results
 *
 * @param passed Pointer to store pass count (can be NULL)
 * @param failed Pointer to store fail count (can be NULL)
 * @param skipped Pointer to store skip count (can be NULL)
 * @return Total number of tests
 */
int post_get_summary(uint32_t *passed, uint32_t *failed, uint32_t *skipped);

/**
 * @brief Register a failure callback
 *
 * The callback is invoked whenever a test fails.
 *
 * @param cb Callback function
 * @param user_data User data passed to callback
 * @return 0 on success, negative errno on error
 */
int post_register_failure_hook(post_failure_cb cb, void *user_data);

/**
 * @brief Get test descriptor by ID
 *
 * @param test_id Test ID to find
 * @return Pointer to test descriptor, or NULL if not found
 */
const struct post_test *post_get_test(uint32_t test_id);

/**
 * @brief Get total number of registered tests
 *
 * @return Number of tests
 */
int post_get_test_count(void);

#ifdef CONFIG_USERSPACE
/*
 * Userspace API (syscalls)
 */

/**
 * @brief Run a test from userspace
 *
 * Only tests with POST_FLAG_USERSPACE_OK can be executed.
 *
 * @param test_id Test ID to execute
 * @return Test result, or negative errno on error
 */
__syscall int post_run_test_user(uint32_t test_id);

/**
 * @brief Get test result from userspace
 *
 * @param test_id Test ID to query
 * @param result Pointer to store result
 * @return 0 on success, negative errno on error
 */
__syscall int post_get_result_user(uint32_t test_id, enum post_result *result);

/**
 * @brief Get POST summary from userspace
 *
 * @param passed Pointer to store pass count
 * @param failed Pointer to store fail count
 * @param skipped Pointer to store skip count
 * @return Total number of tests, or negative errno on error
 */
__syscall int post_get_summary_user(uint32_t *passed, uint32_t *failed,
				    uint32_t *skipped);

#include <zephyr/syscalls/post.h>
#endif /* CONFIG_USERSPACE */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POST_H_ */
