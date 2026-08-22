/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Safety Test subsystem API
 *
 * Framework for registering and executing hardware and software self-tests at boot time and
 * at runtime. Provision is also provided for linking external Safety test Libraries.
 */

#ifndef ZEPHYR_INCLUDE_SAFETY_TEST_SAFETY_TEST_H_
#define ZEPHYR_INCLUDE_SAFETY_TEST_SAFETY_TEST_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
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

/** @brief Outcome of a safety test. */
enum safety_test_result {
	/** Never executed. Zeroed RAM reads as this value. */
	SAFETY_TEST_RESULT_NOT_RUN = 0,
	/** Executed and detected no fault. */
	SAFETY_TEST_RESULT_PASS,
	/**
	 * Executed and detected a fault. Also set for a budget overrun, but only when
	 * @kconfig{CONFIG_SAFETY_TEST_BUDGET_FAILS} is enabled.
	 */
	SAFETY_TEST_RESULT_FAIL,
	/** Not executed because the invocation context did not permit it. */
	SAFETY_TEST_RESULT_SKIP,
};

/** @brief Safety test categories. */
enum safety_test_category {
	/** The core itself: registers, arithmetic, and instruction execution. */
	SAFETY_TEST_CAT_CPU = 0,
	/** Data memory: stuck bits, addressing faults, and coupling between cells. */
	SAFETY_TEST_CAT_RAM,
	/** Program memory, usually a checksum over the image. */
	SAFETY_TEST_CAT_FLASH,
	/** Clock sources: that they run, and at the frequency expected. */
	SAFETY_TEST_CAT_CLOCK,
	/** Stack use: overflow, and corruption of the guard area. */
	SAFETY_TEST_CAT_STACK,
	/** The watchdog itself: that it is running and can still reset the system. */
	SAFETY_TEST_CAT_WATCHDOG,
	/** Digital pins, typically driven and read back through a loopback. */
	SAFETY_TEST_CAT_GPIO,
	/** Communication links, such as a serial bus or a network interface. */
	SAFETY_TEST_CAT_COMM,
	/** Analog inputs, usually measured against a known reference. */
	SAFETY_TEST_CAT_ADC,
	/** Anything the categories above do not describe, including vendor-specific tests. */
	SAFETY_TEST_CAT_OTHER,
	/** Number of categories. Not a category. */
	SAFETY_TEST_CAT_COUNT,
};

/**
 * @brief Category bit, for building a safety_test_run_category() mask.
 *
 * @ref safety_test_category is an enumeration, not a set of bit flags, so a category
 * value cannot be OR'd into a mask directly. This macro converts one into the bit that
 * safety_test_run_category() matches against:
 *
 * @code
 * safety_test_run_category(SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_RAM) |
 *                          SAFETY_TEST_CAT_BIT(SAFETY_TEST_CAT_FLASH), &stats);
 * @endcode
 *
 * @param _cat Category (@ref safety_test_category).
 */
#define SAFETY_TEST_CAT_BIT(_cat) BIT(_cat)

/** @brief Initialization levels, mirroring Zephyr's own. */
enum safety_test_init_level {
	/** Runs before logging is usable: a failure here produces no console output. */
	SAFETY_TEST_LEVEL_EARLY = 0,
	/** Before the kernel starts. Interrupts are off and the timer is not running yet. */
	SAFETY_TEST_LEVEL_PRE_KERNEL_1,
	/** After the system timer is up. The earliest level where logging works. */
	SAFETY_TEST_LEVEL_PRE_KERNEL_2,
	/** Kernel services are available, so a test may use them. */
	SAFETY_TEST_LEVEL_POST_KERNEL,
	/** The last boot level, after drivers and services are initialised. */
	SAFETY_TEST_LEVEL_APPLICATION,
};

/** @brief What the system should do after a critical test fails. */
enum safety_test_action {
	/** Stop the CPU. Nothing runs afterwards. */
	SAFETY_TEST_ACTION_HALT = 0,
	/** Reboot the system. */
	SAFETY_TEST_ACTION_RESET,
	/** Carry on. Refused when CONFIG_SAFETY_TEST_STRICT_CRITICAL is set. */
	SAFETY_TEST_ACTION_CONTINUE,
};

/** @brief Test may run automatically at boot. */
#define SAFETY_TEST_FLAG_BOOT_OK     BIT(0)
/** @brief Test may be invoked at runtime. */
#define SAFETY_TEST_FLAG_RUNTIME_OK  BIT(1)
/** @brief Test disturbs system state. */
#define SAFETY_TEST_FLAG_DESTRUCTIVE BIT(2)
/** @brief Failure of this test must reach a safe state. */
#define SAFETY_TEST_FLAG_CRITICAL    BIT(3)

/**
 * @brief Permit a SAFETY_TEST_FLAG_DESTRUCTIVE test to execute.
 *
 * Options deliberately start above the SAFETY_TEST_FLAG_* range. The flags live
 * in a uint8_t field, so no flag value can ever be a valid option, and passing
 * one here by mistake is rejected with -EINVAL instead of quietly granting
 * permission to run a destructive test.
 */
#define SAFETY_TEST_OPT_ALLOW_DESTRUCTIVE BIT(16)

/** @brief Every defined SAFETY_TEST_OPT_*. Anything else is rejected. */
#define SAFETY_TEST_OPT_MASK (SAFETY_TEST_OPT_ALLOW_DESTRUCTIVE)

struct safety_test;

/** @brief Execution context handed to a test function. */
struct safety_test_context {
	/**
	 * Level at which safety test is invoked at. Runtime invocations report
	 * SAFETY_TEST_LEVEL_APPLICATION regardless of the level the test was registered for.
	 */
	enum safety_test_init_level init_level;
	/** The descriptor being executed. */
	const struct safety_test *test;
	/** The descriptor's @ref safety_test.user_data. */
	const void *user_data;
};

/**
 * @brief Safety test entry point.
 *
 * A certified vendor library function that returns 0 on success and non-zero on failure
 * already matches this signature and needs no wrapper.
 *
 * @param ctx Execution context.
 * @return 0 if no fault was detected, a negative errno otherwise.
 */
typedef int (*safety_test_fn)(const struct safety_test_context *ctx);

/** @brief Outcome of the most recent execution of a test. */
struct safety_test_result_record {
	/** Outcome. */
	enum safety_test_result result;
	/** What the test function returned. Never replaced by a subsystem error. */
	int error_code;
	/**
	 * Wall time spent inside the test function, saturating at UINT32_MAX. Targets
	 * without @kconfig{CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER} measure this with a
	 * 32-bit counter, which is accurate across one wrap only.
	 */
	uint32_t duration_us;
	/** Test ran longer than its budget. Recorded whatever @ref result says. */
	bool over_budget;
};

/**
 * @brief Safety test descriptor.
 *
 * Lives in Read Only Memory. @ref name is the stable identity:
 * it will not change at rebuilds and refactors, which makes it the value to record
 * in non-volatile storage.
 */
struct safety_test {
	/** Stable identity. Generated by SAFETY_TEST_DEFINE() from the C identifier. */
	const char *name;
	/** Human-readable description. */
	const char *description;
	/** Entry point. */
	safety_test_fn test_fn;
	const void *user_data;
	/** This test result. Never NULL. */
	struct safety_test_result_record *result;
	/**
	 * Duration budget in microseconds, 0 to disable. Checked after the test returns, so
	 * it is a budget, not a timeout: a test that hangs is not caught here, that is a
	 * watchdog's job. Not checked for a run at EARLY or PRE_KERNEL_1, where the cycle
	 * counter may not be running yet; a runtime run of the same test still is.
	 *
	 * This is wall time, so interrupts and preemption during the test are charged to it.
	 * Going over sets @ref safety_test_result_record.over_budget and logs a warning. It
	 * changes the verdict only if @kconfig{CONFIG_SAFETY_TEST_BUDGET_FAILS} is set, and
	 * then a SAFETY_TEST_FLAG_CRITICAL test takes the safe-state path.
	 */
	uint32_t max_duration_us;
	/** Category. */
	enum safety_test_category category;
	/** Level at which the test runs automatically, if SAFETY_TEST_FLAG_BOOT_OK is set. */
	enum safety_test_init_level init_level;
	/** Bitwise OR of SAFETY_TEST_FLAG_*. */
	uint8_t flags;
	/** Order within a level. Lower runs first. */
	uint8_t priority;
};

/** @brief Test outcomes. */
struct safety_test_stats {
	/** Tests counted. */
	uint32_t total;
	/** Tests that passed. */
	uint32_t passed;
	/** Tests that failed. */
	uint32_t failed;
	/** Tests skipped by policy. */
	uint32_t skipped;
	/** Tests never executed. */
	uint32_t not_run;
	/** Tests that ran longer than their budget, whatever their verdict was. */
	uint32_t over_budget;
};

/** @brief Global and per-category stats. */
struct safety_test_summary {
	/** Every registered test. */
	struct safety_test_stats global;
	/** Indexed by @ref safety_test_category. */
	struct safety_test_stats categories[SAFETY_TEST_CAT_COUNT];
};

/**
 * @brief Failure notification callback.
 *
 * Invoked for every failing test, critical or not, before any safe-state transition.
 *
 * Runs in thread context, in the thread that ran the test, which for a boot-level test may
 * happen before the kernel is up. No lock protecting the result records is held, so a hook
 * may block: writing the failure to flash or to a console is allowed. It does hold the
 * lock that serializes test execution, so any other caller trying to run a test waits for
 * the hook to finish. Readers are not affected.
 *
 * A hook must not call safety_test_run(), safety_test_run_by_name(),
 * safety_test_run_category() or safety_test_run_level(). Such a call is rejected with
 * -EALREADY rather than nesting a test execution inside a failing test's own reporting
 * path.
 *
 * safety_test_get_result() carries no such restriction: it is safe to call from a hook at
 * any level.
 */
typedef void (*safety_test_failure_fn)(const struct safety_test *test,
				       const struct safety_test_result_record *rec);

/** @brief Failure hook descriptor. Registered with SAFETY_TEST_FAILURE_HOOK_DEFINE(). */
struct safety_test_failure_hook {
	/** Callback. */
	safety_test_failure_fn fn;
};

/**
 * @brief Iteration callback.
 *
 * @param test Current descriptor.
 * @param user_data Opaque value passed to safety_test_foreach().
 * @return true to continue iterating, false to stop.
 */
typedef bool (*safety_test_cb_t)(const struct safety_test *test, void *user_data);

/** @cond INTERNAL_HIDDEN */
#define Z_SAFETY_TEST_NAME(_name)   _CONCAT(__safety_test_, _name)
#define Z_SAFETY_TEST_RESULT(_name) _CONCAT(__safety_test_result_, _name)
/** @endcond */

/**
 * @brief Register a safety test, specifying every field.
 *
 * @param _name C identifier. Also becomes the test's stable name string.
 * @param _cat Category (@ref safety_test_category).
 * @param _level Init level (@ref safety_test_init_level).
 * @param _prio Order within the level, 0-255, lower first.
 * @param _flags Bitwise OR of SAFETY_TEST_FLAG_*. At least one of SAFETY_TEST_FLAG_BOOT_OK
 *               and SAFETY_TEST_FLAG_RUNTIME_OK is required; a test with neither could
 *               never execute.
 * @param _fn Entry point (@ref safety_test_fn).
 * @param _desc Description string.
 * @param _user_data Pointer handed to the test through its context, or NULL.
 * @param _max_duration_us Duration budget in microseconds, or 0 to disable.
 */
#define SAFETY_TEST_DEFINE_EX(_name, _cat, _level, _prio, _flags, _fn, _desc, _user_data,          \
			      _max_duration_us)                                                    \
	BUILD_ASSERT((unsigned int)(_cat) < SAFETY_TEST_CAT_COUNT,                                 \
		     "safety test: category out of range");                                        \
	BUILD_ASSERT((unsigned int)(_level) <= SAFETY_TEST_LEVEL_APPLICATION,                      \
		     "safety test: init level out of range");                                      \
	BUILD_ASSERT((unsigned int)(_prio) <= UINT8_MAX,                                           \
		     "safety test: priority must be 0-255");                                       \
	BUILD_ASSERT(((_flags) & (SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK)) != 0,   \
		     "safety test can never run: set BOOT_OK and/or RUNTIME_OK");                  \
	static struct safety_test_result_record Z_SAFETY_TEST_RESULT(_name);                       \
	extern const struct safety_test Z_SAFETY_TEST_NAME(_name);                                 \
	const STRUCT_SECTION_ITERABLE(safety_test, Z_SAFETY_TEST_NAME(_name)) = {                  \
		.name = STRINGIFY(_name),                                                          \
		.description = _desc,                                                              \
		.test_fn = (_fn),                                                                  \
		.user_data = (_user_data),                                                         \
		.result = &Z_SAFETY_TEST_RESULT(_name),                                            \
		.max_duration_us = (_max_duration_us),                                             \
		.category = (_cat),                                                                \
		.init_level = (_level),                                                            \
		.flags = (_flags),                                                                 \
		.priority = (_prio),                                                               \
	}

/**
 * @brief Register a safety test.
 *
 * Equivalent to SAFETY_TEST_DEFINE_EX() with no user data and no duration budget.
 */
#define SAFETY_TEST_DEFINE(_name, _cat, _level, _prio, _flags, _fn, _desc)                         \
	SAFETY_TEST_DEFINE_EX(_name, _cat, _level, _prio, _flags, _fn, _desc, NULL, 0)

/**
 * @brief Register a failure notification hook.
 *
 * @param _name Unique C identifier for the hook.
 * @param _fn Callback (@ref safety_test_failure_fn).
 */
#define SAFETY_TEST_FAILURE_HOOK_DEFINE(_name, _fn)                                                \
	static const STRUCT_SECTION_ITERABLE(safety_test_failure_hook,                             \
					     _CONCAT(__safety_test_hook_, _name)) = {              \
		.fn = (_fn),                                                                       \
	}

/**
 * @brief Look a test up by its stable name.
 *
 * @param name Test name, as given to SAFETY_TEST_DEFINE().
 * @return Descriptor, or NULL if no test has that name or @p name is NULL.
 */
const struct safety_test *safety_test_get(const char *name);

/**
 * @brief Loop over every registered test.
 *
 * @param cb Callback. Iteration stops early if it returns false. NULL is a no-op.
 * @param user_data Passed through to @p cb.
 */
void safety_test_foreach(safety_test_cb_t cb, void *user_data);

/**
 * @brief Execute a test.
 *
 * The return value reports whether the call was valid; @p result reports what the hardware
 * did.
 *
 * A test without SAFETY_TEST_FLAG_RUNTIME_OK, or one with SAFETY_TEST_FLAG_DESTRUCTIVE
 * invoked without SAFETY_TEST_OPT_ALLOW_DESTRUCTIVE, yields SAFETY_TEST_RESULT_SKIP and
 * returns 0. A skipped invocation leaves the stored result record unchanged.
 *
 * Boot-time execution is expected to go through the subsystem's own SYS_INIT entries. A call
 * from an earlier init level works too: the lock is simply not taken there, because nothing
 * else can be running yet. But timing figures from the cycle counter are not meaningful that
 * early, so treat @ref safety_test_result_record.duration_us as unreliable for a test run
 * before the cycle counter is up.
 *
 * @note Must not be called from an ISR. Not because of the subsystem's own locking, but
 *       because the test function, the failure hooks and the safe-state handler are all
 *       application code that may block.
 *
 * @param test Descriptor from safety_test_get().
 * @param opts Bitwise OR of SAFETY_TEST_OPT_*, or 0. Any other bit is -EINVAL.
 * @param result Outcome, may be NULL.
 * @return 0 on success, -EINVAL if @p test is NULL, -EALREADY if called from a failure
 *         hook or the safe-state handler.
 */
int safety_test_run(const struct safety_test *test, uint32_t opts,
		    enum safety_test_result *result);

/**
 * @brief Look a test up by name and execute it.
 *
 * @note Must not be called from an ISR. See safety_test_run().
 *
 * @param name Test name.
 * @param opts Bitwise OR of SAFETY_TEST_OPT_*, or 0. Any other bit is -EINVAL.
 * @param result Outcome, may be NULL. Untouched if the lookup fails.
 * @return 0 on success, -ENOENT if no test has that name, -EINVAL if @p name is NULL,
 *         -EALREADY if called from a failure hook or the safe-state handler.
 */
int safety_test_run_by_name(const char *name, uint32_t opts, enum safety_test_result *result);

/**
 * @brief Read a test's most recent result.
 *
 * Callable from any context and at any boot level, including from an ISR: the record is
 * copied under a spinlock that is never held across application code.
 *
 * @param test Descriptor.
 * @param rec Filled in on success.
 * @return 0 on success, -EINVAL if either argument is NULL.
 */
int safety_test_get_result(const struct safety_test *test,
			   struct safety_test_result_record *rec);

/**
 * @brief Execute every runtime-capable test in the given categories.
 *
 * SAFETY_TEST_OPT_ALLOW_DESTRUCTIVE is never applied, so destructive tests are reported as
 * skipped rather than executed. Run those individually with safety_test_run().
 *
 * A failing test carrying SAFETY_TEST_FLAG_CRITICAL takes the safe-state path from inside
 * this sweep, exactly as safety_test_run() does for a single test. That can halt or reset
 * the system partway through the sweep and never return, in which case @p stats is never
 * written.
 *
 * @note Must not be called from an ISR. See safety_test_run().
 *
 * @param cat_mask Bitwise OR of SAFETY_TEST_CAT_BIT() values. Passing a raw
 *                 @ref safety_test_category value selects the wrong categories.
 *                 UINT32_MAX selects every category.
 * @param stats Context to store stats for this invocation, may be NULL.
 * @return 0 on success, -EINVAL if @p cat_mask is 0.
 */
int safety_test_run_category(uint32_t cat_mask, struct safety_test_stats *stats);

/**
 * @brief Summarize the stored results of every registered test.
 *
 * Reports stored state rather than a single invocation. A skipped invocation does not
 * write a record.
 *
 * Callable from any context, including an ISR. Interrupts are masked for the duration of
 * the walk so the summary is a consistent snapshot, which costs a few instructions per
 * registered test.
 *
 * @param summary Filled in on success.
 * @return 0 on success, -EINVAL if @p summary is NULL.
 */
int safety_test_get_summary(struct safety_test_summary *summary);

/**
 * @brief Report whether every critical boot test passed.
 *
 * A test that was never executed counts as not passed. Having no critical boot test at
 * all is -ENOENT, not a failure: treating the two alike halts a healthy system.
 *
 * Callable from any context, including an ISR.
 *
 * @param passed Written only when 0 is returned.
 * @return 0 on success, -EINVAL if @p passed is NULL, -ENOENT if no test carries both
 *         SAFETY_TEST_FLAG_BOOT_OK and SAFETY_TEST_FLAG_CRITICAL.
 */
int safety_test_boot_passed(bool *passed);

/**
 * @brief Transition the system to a safe state after a critical test failed.
 *
 * Bound at link time rather than registered at runtime, for three reasons: it works at
 * PRE_KERNEL_1, where no application code has run yet and a registration-based hook would
 * still be NULL; it lives in flash, so there is no RAM function pointer to corrupt in a
 * subsystem whose purpose is detecting corruption; and it is statically analyzable.
 *
 * Applications override it by defining a function with this exact signature. The built-in
 * implementation is weak and returns the action selected by
 * @kconfig{CONFIG_SAFETY_TEST_DEFAULT_ACTION_HALT} or
 * @kconfig{CONFIG_SAFETY_TEST_DEFAULT_ACTION_RESET}.
 *
 * Invoked after all failure hooks, and only for tests carrying SAFETY_TEST_FLAG_CRITICAL.
 * May be called before the kernel is running, so it must not block or use kernel services.
 *
 * Always invoked with the lock that serializes test execution held, from boot and from a
 * runtime call alike, so no other test can start while the system is deciding what to do
 * and two threads can never be inside this function at once. It must not call
 * safety_test_run(), safety_test_run_by_name(), safety_test_run_category() or
 * safety_test_run_level(); such a call returns -EALREADY.
 * safety_test_get_result() is safe to call from it.
 *
 * @param test The failed test.
 * @param rec Its result record.
 * @return What the subsystem should do next. SAFETY_TEST_ACTION_CONTINUE is refused when
 *         @kconfig{CONFIG_SAFETY_TEST_STRICT_CRITICAL} is set. SAFETY_TEST_ACTION_RESET
 *         downgrades to halt when @kconfig{CONFIG_REBOOT} is not enabled, logging the
 *         refusal.
 */
enum safety_test_action safety_test_safe_state(const struct safety_test *test,
					       const struct safety_test_result_record *rec);

/**
 * @brief Execute every boot-capable test registered for a level.
 *
 * Called automatically by the subsystem's SYS_INIT entries; applications rarely call it.
 * Tests run in ascending @ref safety_test.priority order. Tests without
 * SAFETY_TEST_FLAG_BOOT_OK are ignored. SAFETY_TEST_FLAG_DESTRUCTIVE tests do run: boot is
 * where a destructive test belongs.
 *
 * Callable at any time. Below POST_KERNEL there is no kernel to lock with and nothing else
 * is running; from POST_KERNEL onwards the whole walk is serialized against the other test
 * execution paths, which wait for it. Readers are not blocked by the walk.
 *
 * @note Must not be called from an ISR. See safety_test_run().
 *
 * @param level Level to run.
 * @param stats Context for storing stats for this invocation, may be NULL.
 * @return 0 on success, -EINVAL if @p level is not a valid level, -EALREADY if called
 *         from a failure hook or the safe-state handler.
 */
int safety_test_run_level(enum safety_test_init_level level, struct safety_test_stats *stats);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SAFETY_TEST_SAFETY_TEST_H_ */
