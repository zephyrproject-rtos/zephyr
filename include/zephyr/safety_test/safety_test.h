/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Safety Test subsystem API
 *
 * This subsystem provides a framework for hardware and software self-testing at boot time and
 * runtime. Tests can be registered at different initialization levels and optionally executed
 * from userspace.
 */

#ifndef ZEPHYR_INCLUDE_SAFETY_TEST_H_
#define ZEPHYR_INCLUDE_SAFETY_TEST_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup safety_test_api Safety Test
 * @ingroup os_services
 * @{
 */

/**
 * @brief Safety test result codes
 */
enum safety_test_result {
	/** Test passed successfully */
	SAFETY_TEST_RESULT_PASS = 0,
	/** Test failed - fault detected */
	SAFETY_TEST_RESULT_FAIL = 1,
	/** Test skipped (precondition not met) */
	SAFETY_TEST_RESULT_SKIP = 2,
	/** Test error (test itself failed to execute) */
	SAFETY_TEST_RESULT_ERROR = 3,
	/** Test has not been executed yet */
	SAFETY_TEST_RESULT_NOT_RUN = 0xFF,
};

/**
 * @brief Safety test categories
 */
enum safety_test_category {
	/** CPU register and instruction tests */
	SAFETY_TEST_CAT_CPU = BIT(0),
	/** RAM integrity tests */
	SAFETY_TEST_CAT_RAM = BIT(1),
	/** Flash/ROM integrity tests */
	SAFETY_TEST_CAT_FLASH = BIT(2),
	/** Clock and timing tests */
	SAFETY_TEST_CAT_CLOCK = BIT(3),
	/** Stack integrity tests */
	SAFETY_TEST_CAT_STACK = BIT(4),
	/** Watchdog tests */
	SAFETY_TEST_CAT_WATCHDOG = BIT(5),
	/** GPIO tests */
	SAFETY_TEST_CAT_GPIO = BIT(6),
	/** Communication peripheral tests */
	SAFETY_TEST_CAT_COMM = BIT(7),
	/** ADC tests */
	SAFETY_TEST_CAT_ADC = BIT(8),
	/** User-defined tests */
	SAFETY_TEST_CAT_CUSTOM = BIT(15),
};

/**
 * @brief Safety test flags
 */
enum safety_test_flags {
	/** Test should only run at boot time */
	SAFETY_TEST_FLAG_BOOT_ONLY = BIT(0),
	/** Test is safe to run at runtime */
	SAFETY_TEST_FLAG_RUNTIME_OK = BIT(1),
	/** Test may affect system state */
	SAFETY_TEST_FLAG_DESTRUCTIVE = BIT(2),
	/** Test can be called from userspace */
	SAFETY_TEST_FLAG_USERSPACE_OK = BIT(3),
	/** Failure of this test should halt boot */
	SAFETY_TEST_FLAG_CRITICAL = BIT(4),
	/** Test is from a vendor library */
	SAFETY_TEST_FLAG_VENDOR = BIT(5),
	/** Test only runs on-demand, never automatically at boot */
	SAFETY_TEST_FLAG_ON_DEMAND_ONLY = BIT(6),
};

/**
 * @brief Safety test initialization levels (matches Zephyr init levels)
 *
 * @note Safety tests execute **after** their corresponding SYS_INIT level completes:
 * - SAFETY_TEST_LEVEL_EARLY: After all SYS_INIT_LEVEL(0, ...) callbacks
 * - SAFETY_TEST_LEVEL_PRE_KERNEL_1: After all SYS_INIT_LEVEL(1, ...) callbacks
 * - SAFETY_TEST_LEVEL_PRE_KERNEL_2: After all SYS_INIT_LEVEL(2, ...) callbacks
 * - SAFETY_TEST_LEVEL_POST_KERNEL: After all SYS_INIT_LEVEL(3, ...) callbacks
 * - SAFETY_TEST_LEVEL_APPLICATION: After all SYS_INIT_LEVEL(4, ...) callbacks
 *
 * This ensures tests run when the system is in the expected state for that init level.
 */
enum safety_test_init_level {
	SAFETT_TEST_LEVEL_EARLY = 0,
	SAFETY_TEST_LEVEL_PRE_KERNEL_1 = 1,
	SAFETY_TEST_LEVEL_PRE_KERNEL_2 = 2,
	SAFETY_TEST_LEVEL_POST_KERNEL = 3,
	SAFETY_TEST_LEVEL_APPLICATION = 4,
};

/* Forward declaration */
struct safety_test;

/**
 * @brief Safety test execution context
 *
 * Provides information to the test function about its execution environment.
 * Test functions can set error_code and error_data before returing to provide
 * additional failure information.
 */
struct safety_test_context {
	/** Current initialization level */
	uint32_t init_level;
	/** Unique test identifier */
	uint32_t test_id;
	/** Optional user data */
	void *user_data;
	/** Test-specific error code (set by test function) */
	uint32_t error_code;
	/** Test-specific error data (set by test function) */
	uint32_t error_data;
};

/**
 * @brief Safety test function signature
 *
 * Test functions receive a non-const context pointer so they casn set error_code and
 * error_data before returning.
 *
 * @param ctx Execution context (non-const to allow setting error fields)
 * @return Test result
 */
typedef enum safety_test_result (*safety_test_fn)(struct safety_test_context *ctx);

/**
 * @brief Safety test descriptor
 *
 * Describes a single safety test including metadata and the test function.
 */
struct safety_test {
	/** Readable test name */
	const char *name;
	/** Detailed description */
	const char *description;
	/** Unique test ID (for traceability) */
	uint32_t id;
	/** Test category (SAFETY_TEST_CAT_*) */
	uint32_t category;
	/** Earliest init level this test can run */
	uint8_t init_level;
	/** Execution priority within level (0 = highest) */
	uint8_t priority;
	/** Test flags (SAFETY_TEST_FLAG_*) */
	uint16_t flags;
	/** Test function */
	safety_test_fn test_fn;
	/** Test timeout in milliseconds (0 = no timeout) */
	uint32_t timeout_ms;
};

/**
 * @brief Safety test reult record
 *
 * Stores the rsult of a test execution.
 */
struct safety_test_result_record {
	/** Test ID */
	uint32_t test_id;
	/** Test result */
	enum safety_test_result result;
	/** Detailed error code (test-specific) */
	uint32_t error_code;
	/** Additional error context data */
	uint32_t error_data;
};

/**
 * @brief Safety test category summary statistics
 *
 * Statistics for a single category.
 */
struct safety_test_category_summary {
	/** Total number of tests in this category */
	uint32_t total;
	/** Number of passed tests */
	uint32_t passed;
	/** Number of failed tests */
	uint32_t failed;
	/** Number of skipped tests */
	uint32_t skipped;
};

/**
 * @brief Safety test summary statistics
 *
 * Contains global statistics and per-category breakdown.
 */
struct safety_test_summary {
	/** Global statistics */
	struct {
		/** Total number of tests */
		uint32_t total;
		/** Number of passed tests */
		uint32_t passed;
		/** Number of failed tests */
		uint32_t failed;
		/** Number of skipped tests */
		uint32_t skipped;
	} global;

	/** Per-category statistics (indexed by category bit position) */
	struct safety_test_category_summary categories[16];
};

/**
 * @brief Safety test failure callback signature
 *
 * Called whenever any test fails (critical or non-critical).
 */
typedef void (*safety_test_failure_cb)(const struct safety_test *test,
				       const struct safety_test_result_record *result,
				       void *user_data);

/**
 * @brief Safe state transition callback signature
 *
 * Called when a critical test fails, before system halt.
 * Allows application to define its safe state transition:
 * - Disable peripherals
 * - Set GPIOs to safe states
 * - Log to non-volatile storage
 * - Notify external systems
 *
 * @param test The failed test
 * @param result Test result with error details
 * @param user_data Application-specific data
 */
typedef void (*safety_test_safe_state_cb_t)(const struct safety_test *test,
					    const struct safety_test_result_record *result,
					    void *user_data);

/** @cond INTERNAL_HIDDEN */

/* Internal macro to generate test entry name */
#define Z_SAFETY_TEST_NAME(name) _CONCAT(__safety_test_, name)

/* Internal macro for section placement */
#define Z_SAFETY_TEST_SECTION __attribute__((__section__(".safety_test_tests")))

/** @endcond */

/**
 * @brief Define a safety test
 *
 * Registers a test function to be executed at the specified init level. Tests are
 * automatically collected into an iterable section. Test IDs are automatically assigned at
 * link time (0, 1, 2, ...) based on the test's position in the sorted array.
 *
 * @param _name Unique C identifier for the test
 * @param _cat  Test category (SAFETY_TEST_CAT_*)
 * @param _level Earliest init level (SAFETY_TEST_LEVEL_*)
 * @param _prio Priority within level (0-255, lower = earlier)
 * @param _flags Test flags (SAFETY_TEST_FLAG_*)
 * @param _fn Test function (safety_test_fn)
 * @param _desc Human-readable description string
 */
#define SAFETY_TEST_DEFINE(_name, _cat, _level, _prio, _flags, _fn, _desc)                         \
	static const STRUCT_SECTION_ITERABLE(safety_test, Z_SAFETY_TEST_NAME(_name)) = {           \
		.name = STRINGIFY(_name), .description = _desc,                                    \
				  .id = 0, /* This gets computed at runtime */                     \
				  .category = _cat, .init_level = _level, .priority = _prio,       \
				  .flags = _flags, .test_fn = _fn, .timeout_ms = 0,                \
	}

/**
 * @brief Define a safety test with timeout
 *
 * Same as SAFETY_TEST_DEFINE but with a timeout value.
 * Test IDs are automatically assigned sequentially at link time (0, 1, 2, ...).
 */
#define SAFETY_TEST_DEFINE_TIMEOUT(_name, _cat, _level, _prio, _flags, _fn, _desc, _timeout_ms)    \
	static const STRUCT_SECTION_ITERABLE(safety_test, Z_SAFETY_TEST(_name)) = {                \
		.name = STRINGIFY(_name), .description = _desc, .id = 0, .category = _cat,         \
				  .init_level = _level, .priority = _prio, .flags = _flags,        \
				  .test_fn = _fn, .timeout_ms = _timeout_ms,                       \
	}

/**
 * @brief CPU test (convenience macro)
 */
#define SAFETY_TEST_CPU(_name, _level, _fn)                                                        \
	SAFETY_TEST_DEFINE(_name, SAFETY_TEST_CAT_CPU, _level, 0,                                  \
			   SAFETY_TEST_FLAG_RUNTIME_OK | SAFETY_TEST_FLAG_CRITICAL, _fn,           \
			   "CPU test: " STRINGIFY(_name))

/**
 * @brief RAM test (convenience macro)
 */
#define SAFETY_TEST_RAM(_name, _level, _fn)                                                        \
	SAFETY_TEST_DEFINE(_name, SAFETY_TEST_CAT_RAM, _level, 10,                                 \
			   SAFETY_TEST_FLAG_BOOT_ONLY | SAFETY_TEST_FLAG_DESTRUCTIVE |             \
				   SAFETY_TEST_FLAG_CRITICAL,                                      \
			   _fn, "Stack test: " STRINGIFY(_name))

#define SAFETY_TEST_FLASH(_name, _level, _fn)                                                      \
	SAFETY_TEST_DEFINE(_name, SAFETY_TEST_CAT_FLASH, _level, 30,                               \
			   SAFETY_TEST_FLAG_RUNTIME_OK | SAFETY_TEST_FLAG_USERSPACE_OK, _fn,       \
			   "Flash test: " STRINGIFY(_name))

/*
 * Kernel API
 */

/**
 * @brief Run all safety tests for a given init level
 *
 * This is called automatically during boot. Tests are executed in priority order within
 * the level.
 *
 * @param level Initialization to run
 * @return Number of failed tests
 */
int safety_test_result safety_test_run_level(enum safety_test_init_level level);

/**
 * @brief Run a specific test by ID
 *
 * @param test_id Test ID to execute
 * @return Number of failed tests
 */
enum safety_test_result safety_test_run_test(uint32_t test_id);

/**
 * @brief Run all tests in a category
 *
 * @param category test category bitmask (SAFETY_TEST_CAT_*)
 * @return Number of failed tests
 */
int safety_test_run_category(uint32_t category);

/**
 * @brief Get test result record
 *
 * @param test_id test ID to query
 * @param record Pointer to result record to fill
 * @return 0 on success, -ENOENT if not found, -EINVAL if record is NULL
 */
int safety_test_get_result(uint32_t test_id, struct safety_test_result_record *record);

/**
 * @brief get summary of all safety test results
 *
 * Populates the summary structure with global statistics and per-category breakdown.
 *
 * @param summary Pointer to summary structure to populate
 * @return 0 on success, negative errno on error
 */
int safety_test_get_summary(struct safety_test_summary *summary);

/**
 * @brief get summary for specific category
 *
 * Populates the summary structure with statistics for the specified category only.
 * The global statistics will reflect only tests in the specified category.
 *
 * @param category Category bitmask (SAFETY_TEST_CAT_*)
 * @param summary Pointer to summary structure to populate
 * @return 0 on success, negative error on error
 */
int safety_test_get_category_summary(uint32_t category, struct safety_test_summary *summary);

/**
 * @brief Register a failure callback
 *
 * The callback is invoked whenever a test fails (critical or non-critical).
 *
 * @param cb Callback function
 * @param user_data User data passed to callback
 * @return 0 on success, negative errno on error
 */
int safety_test_register_failure_hook(safety_test_failure_cb cb, void *user_data);

/**
 * @brief Register safe state callback
 *
 * This callback is invoked when a critical test fails, before system halt.
 * This allows the application to transition to a defined safe state.
 *
 * @param cb Callback function
 * @param user_data User data passed to callback
 * @return 0 on success, negative errno on error
 */
int safety_test_register_safe_state_cb(safety_test_safe_state_cb cb, void *user_data);

/**
 * @brief Mark a category as safety-critical
 *
 * If any test in a critical category fails, the system will halt.
 *
 * @param category Category bitmask (SAFETY_TEST_CAT_*)
 * @param critical true to mark as critical, false to clear.
 */
void safety_test_category_critical(uint32_t category, bool critical);

/**
 * @brief Get test descriptor by ID
 *
 * @param test_id Test ID to find
 * @return Pointer to the test descriptor, or NULL if not found
 */
const struct safety_test *safety_test_get_test(uint32_t test_id);

/**
 * @brief Get total number of registered tests
 *
 * @return Number of tests
 */
int safety_test_get_test_count(void);

#ifdef CONFIG_USERSPACE
/*
 * Userspace API (syscalls)
 */

/**
 * @brief Run a test from userspace
 *
 * Only tests with SAFETY_TEST_FLAG_USERSPACE_OK can be executed.
 *
 * @param test_id Test ID to execute
 * @return Test result, or negative errno on error
 */
__syscall int safety_test_run_test_user(uint32_t test_d);

/**
 * @brief Get test result from userspace
 *
 * @param test_id Test ID to query
 * @param result Pointer to store result
 * @return 0 on success, negative errno on error
 */
__syscall int safety_test_get_result_user(uint32_t test_id, enum safety_test_result *result);

/**
 * @brief Get safety test summary from userspace
 *
 * @param summary Pointer to summary structure to populate
 * @return 0 on success, negative errno on error
 */
__syscall int safety_test_get_summary_user(struct safety_test_summary *summary);

/**
 * @brief Get category summary from userspace
 *
 * @param category Category bitmask (SAFETY_TEST_CAT_*)
 * @param summary Pointer to summary structure to populate
 * @return 0 on success, negative errno on error
 */
__syscall int safety_test_get_category_summary_user(uint32_t category,
						    struct safety_test_summary *summary);

#include <zephyr/syscalls/safety_test.h>
#endif /* CONFIG_USERSPACE */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SAFETY_TEST_H_ */
