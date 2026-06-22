/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework _test.
 */

#ifndef ZEPHYR_TESTSUITE_ZTEST_TEST_H_
#define ZEPHYR_TESTSUITE_ZTEST_TEST_H_

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <stdbool.h>


/**
 * @defgroup ztest_test Ztest testing macros
 * @ingroup ztest
 *
 * This module eases the testing process by providing helpful macros and other
 * testing structures.
 *
 * @{
 */

#if defined(CONFIG_USERSPACE)
#define __USERSPACE_FLAGS (K_USER)
#else
#define __USERSPACE_FLAGS (0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The expected result of a test.
 *
 * @see ZTEST_EXPECT_FAIL
 * @see ZTEST_EXPECT_SKIP
 */
enum ztest_expected_result {
	ZTEST_EXPECTED_RESULT_FAIL = 0, /**< Expect a test to fail */
	ZTEST_EXPECTED_RESULT_SKIP,	/**< Expect a test to pass */
};

/**
 * @brief A single expectation entry allowing tests to fail/skip and be considered passing.
 *
 * @see ZTEST_EXPECT_FAIL
 * @see ZTEST_EXPECT_SKIP
 */
struct ztest_expected_result_entry {
	const char *test_suite_name; /**< The test suite's name for the expectation */
	const char *test_name;	     /**< The test's name for the expectation */
	enum ztest_expected_result expected_result; /**< The expectation */
};

extern struct ztest_expected_result_entry _ztest_expected_result_entry_list_start[];
extern struct ztest_expected_result_entry _ztest_expected_result_entry_list_end[];

#define __ZTEST_EXPECT(_suite_name, _test_name, expectation)                                       \
	static const STRUCT_SECTION_ITERABLE(                                                      \
		ztest_expected_result_entry,                                                       \
		UTIL_CAT(UTIL_CAT(z_ztest_expected_result_, _suite_name), _test_name)) = {         \
			.test_suite_name = STRINGIFY(_suite_name),                                 \
			.test_name = STRINGIFY(_test_name),                                        \
			.expected_result = expectation,                                            \
	}

/**
 * @brief Expect a test to fail (mark it passing if it failed)
 *
 * Adding this macro to your logic will allow the failing test to be considered passing, example:
 *
 *     ZTEST_EXPECT_FAIL(my_suite, test_x);
 *     ZTEST(my_suite, text_x) {
 *       zassert_true(false, NULL);
 *     }
 *
 * @param _suite_name The name of the suite
 * @param _test_name The name of the test
 */
#define ZTEST_EXPECT_FAIL(_suite_name, _test_name)                                                 \
	__ZTEST_EXPECT(_suite_name, _test_name, ZTEST_EXPECTED_RESULT_FAIL)

/**
 * @brief Expect a test to skip (mark it passing if it failed)
 *
 * Adding this macro to your logic will allow the failing test to be considered passing, example:
 *
 *     ZTEST_EXPECT_SKIP(my_suite, test_x);
 *     ZTEST(my_suite, text_x) {
 *       zassume_true(false, NULL);
 *     }
 *
 * @param _suite_name The name of the suite
 * @param _test_name The name of the test
 */
#define ZTEST_EXPECT_SKIP(_suite_name, _test_name)                                                 \
	__ZTEST_EXPECT(_suite_name, _test_name, ZTEST_EXPECTED_RESULT_SKIP)

struct ztest_unit_test {
	const char *test_suite_name;
	const char *name;
	void (*test)(void *data);
	uint32_t thread_options;

	/** Stats */
	struct ztest_unit_test_stats *const stats;
};

/**
 * @brief Parameter value set for value-parameterized tests.
 */
struct ztest_param_values {
	/** Pointer to the array of parameter values (NULL when @p value_at_cb is set). */
	const void *values;
	/** Number of parameter values (or invocations when @p value_at_cb is set). */
	size_t count;
	/** Size in bytes of each individual parameter value. */
	size_t elem_size;
	/** Optional callback returning a human-readable name for the value at @p index. */
	const char *(*name_cb)(size_t index, const void *value);
	/**
	 * @brief Optional value generator for range-based or runtime parameters.
	 *
	 * When non-NULL, called once per invocation with the 0-based index
	 * and a pointer to an aligned scratch buffer of at least @p elem_size
	 * bytes.  The callback must write exactly @p elem_size bytes into
	 * @p out.  @p values must be NULL when this callback is provided.
	 *
	 * Use ZTEST_DEFINE_PARAM_RANGE() or ZTEST_DEFINE_PARAM_GENERATOR()
	 * instead of setting this directly.
	 */
	void (*value_at_cb)(size_t index, void *out);
	/**
	 * @brief Optional one-time setup called before the dispatch loop.
	 *
	 * When non-NULL, invoked exactly once per parameterized test dispatch
	 * before the first value is produced.  Useful for seeding a PRNG,
	 * resetting stateful generators, or any other per-test-run
	 * initialisation.  May be NULL.
	 *
	 * Use ZTEST_DEFINE_PARAM_GENERATOR_WITH_SETUP() to set this field.
	 */
	void (*setup_cb)(void);
};

/**
 * @brief Registration entry mapping a test to a parameter value set.
 */
struct ztest_param_inst {
	/** Name of the test suite this instantiation belongs to. */
	const char *test_suite_name;
	/** Name of the parameterized test function. */
	const char *test_name;
	/** Human-readable label for this particular instantiation. */
	const char *instance_name;
	/** Parameter value set to iterate over for this instantiation. */
	const struct ztest_param_values *values;
};

extern struct ztest_unit_test _ztest_unit_test_list_start[];
extern struct ztest_unit_test _ztest_unit_test_list_end[];
extern struct ztest_param_inst _ztest_param_inst_list_start[];
extern struct ztest_param_inst _ztest_param_inst_list_end[];
/** Number of registered unit tests */
#define ZTEST_TEST_COUNT (_ztest_unit_test_list_end - _ztest_unit_test_list_start)
/** Number of registered value-parameterized test instantiations */
#define ZTEST_PARAM_INST_COUNT (_ztest_param_inst_list_end - _ztest_param_inst_list_start)

/**
 * Stats about a ztest suite
 */
struct ztest_suite_stats {
	/** The number of times that the suite ran */
	uint32_t run_count;
	/** The number of times that the suite was skipped */
	uint32_t skip_count;
	/** The number of times that the suite failed */
	uint32_t fail_count;
};

struct ztest_unit_test_stats {
	/** The number of times that the test ran */
	uint32_t run_count;
	/** The number of times that the test was skipped */
	uint32_t skip_count;
	/** The number of times that the test failed */
	uint32_t fail_count;
	/** The number of times that the test passed */
	uint32_t pass_count;
	/** The longest duration of the test across multiple times */
	uint32_t duration_worst_ms;
};

/**
 * Setup function to run before running this suite
 *
 * @return Pointer to the data structure that will be used throughout this test suite
 */
typedef void *(*ztest_suite_setup_t)(void);

/**
 * Function to run before each test in this suite
 *
 * @param fixture The test suite's fixture returned from setup()
 */
typedef void (*ztest_suite_before_t)(void *fixture);

/**
 * Function to run after each test in this suite
 *
 * @param fixture The test suite's fixture returned from setup()
 */
typedef void (*ztest_suite_after_t)(void *fixture);

/**
 * Teardown function to run after running this suite
 *
 * @param fixture The test suite's data returned from setup()
 */
typedef void (*ztest_suite_teardown_t)(void *fixture);

/**
 * An optional predicate function to determine if the test should run. If NULL, then the
 * test will only run once on the first attempt.
 *
 * @param global_state The current state of the test application.
 * @return True if the suite should be run; false to skip.
 */
typedef bool (*ztest_suite_predicate_t)(const void *global_state);

/**
 * A single node of test suite. Each node should be added to a single linker section which will
 * allow ztest_run_test_suites() to iterate over the various nodes.
 */
struct ztest_suite_node {
	/** The name of the test suite. */
	const char *const name;

	/** Setup function */
	const ztest_suite_setup_t setup;

	/** Before function */
	const ztest_suite_before_t before;

	/** After function */
	const ztest_suite_after_t after;

	/** Teardown function */
	const ztest_suite_teardown_t teardown;

	/** Optional predicate filter */
	const ztest_suite_predicate_t predicate;

	/** Stats */
	struct ztest_suite_stats *const stats;
};

extern struct ztest_suite_node _ztest_suite_node_list_start[];
extern struct ztest_suite_node _ztest_suite_node_list_end[];

/** Number of registered test suites */
#define ZTEST_SUITE_COUNT (_ztest_suite_node_list_end - _ztest_suite_node_list_start)

/**
 * Create and register a ztest suite. Using this macro creates a new test suite.
 * It then creates a struct ztest_suite_node in a specific linker section.
 *
 * Tests can then be run by calling ztest_run_test_suites(const void *state) by passing
 * in the current state. See the documentation for ztest_run_test_suites for more info.
 *
 * @param SUITE_NAME The name of the suite
 * @param PREDICATE A function to test against the state and determine if the test should run.
 * @param setup_fn The setup function to call before running this test suite
 * @param before_fn The function to call before each unit test in this suite
 * @param after_fn The function to call after each unit test in this suite
 * @param teardown_fn The function to call after running all the tests in this suite
 */
#define ZTEST_SUITE(SUITE_NAME, PREDICATE, setup_fn, before_fn, after_fn, teardown_fn)             \
	struct ztest_suite_stats UTIL_CAT(z_ztest_suite_node_stats_, SUITE_NAME);                  \
	static const STRUCT_SECTION_ITERABLE(ztest_suite_node,                                     \
					     UTIL_CAT(z_ztest_test_node_, SUITE_NAME)) = {         \
		.name = STRINGIFY(SUITE_NAME),                                                     \
		.setup = (setup_fn),                                                               \
		.before = (before_fn),                                                             \
		.after = (after_fn),                                                               \
		.teardown = (teardown_fn),                                                         \
		.predicate = PREDICATE,                                                            \
		.stats = &UTIL_CAT(z_ztest_suite_node_stats_, SUITE_NAME),             \
	}
/**
 * Default entry point for running or listing registered unit tests.
 *
 * @param state The current state of the machine as it relates to the test executable.
 * @param shuffle Shuffle tests
 * @param suite_iter Test suite repetitions.
 * @param case_iter Test case repetitions.
 */
void ztest_run_all(const void *state, bool shuffle, int suite_iter, int case_iter);

/**
 * The result of the current running test. It's possible that the setup function sets the result
 * to ZTEST_RESULT_SUITE_* which will apply the failure/skip to every test in the suite.
 */
enum ztest_result {
	ZTEST_RESULT_PENDING,
	ZTEST_RESULT_PASS,
	ZTEST_RESULT_FAIL,
	ZTEST_RESULT_SKIP,
	ZTEST_RESULT_SUITE_SKIP,
	ZTEST_RESULT_SUITE_FAIL,
};
/**
 * Each enum member represents a distinct phase of execution for the test binary.
 * TEST_PHASE_FRAMEWORK is active when internal ztest code is executing; the rest refer to
 * corresponding phases of user test code.
 */
enum ztest_phase {
	TEST_PHASE_SETUP,
	TEST_PHASE_BEFORE,
	TEST_PHASE_TEST,
	TEST_PHASE_AFTER,
	TEST_PHASE_TEARDOWN,
	TEST_PHASE_FRAMEWORK,
};

/**
 * Run the registered unit tests which return true from their predicate function.
 *
 * @param state The current state of the machine as it relates to the test executable.
 * @param shuffle Shuffle tests
 * @param suite_iter Test suite repetitions.
 * @param case_iter Test case repetitions.
 * @return The number of tests that ran.
 */

#ifdef ZTEST_UNITTEST
int z_impl_ztest_run_test_suites(const void *state, bool shuffle,
				int suite_iter, int case_iter);

static inline int ztest_run_test_suites(const void *state, bool shuffle,
				int suite_iter, int case_iter)
{
	return z_impl_ztest_run_test_suites(state, shuffle, suite_iter, case_iter);
}

#else
__syscall int ztest_run_test_suites(const void *state, bool shuffle,
				int suite_iter, int case_iter);
#endif

#ifdef ZTEST_UNITTEST
void z_impl___ztest_set_test_result(enum ztest_result new_result);
static inline void __ztest_set_test_result(enum ztest_result new_result)
{
	z_impl___ztest_set_test_result(new_result);
}

void z_impl___ztest_set_test_phase(enum ztest_phase new_phase);
static inline void __ztest_set_test_phase(enum ztest_phase new_phase)
{
	z_impl___ztest_set_test_phase(new_phase);
}
#else
__syscall void __ztest_set_test_result(enum ztest_result new_result);
__syscall void __ztest_set_test_phase(enum ztest_phase new_phase);
#endif


/**
 * @brief Fails the test if any of the registered tests did not run.
 *
 * When registering test suites, a pragma function can be provided to determine WHEN the test should
 * run. It is possible that a test suite could be registered but the pragma always prevents it from
 * running. In cases where a test should make sure that ALL suites ran at least once, this function
 * may be called at the end of test_main(). It will cause the test to fail if any suite was
 * registered but never ran.
 */
void ztest_verify_all_test_suites_ran(void);

/**
 * @brief Run a test suite.
 *
 * Internal implementation. Do not call directly. This will run the full test suite along with some
 * checks for fast failures and initialization.
 *
 * @param name The name of the suite to run.
 * @param shuffle Shuffle tests
 * @param suite_iter Test suite repetitions.
 * @param case_iter Test case repetitions.
 * @param param Parameter passing into test.
 * @return Negative value if the test suite never ran; otherwise, return the number of failures.
 */
int z_ztest_run_test_suite(const char *name, bool shuffle, int suite_iter,
			int case_iter, void *param);

/**
 * @brief Returns next test within suite.
 *
 * @param suite Name of suite to get next test from.
 * @param prev  Previous unit test acquired from suite, use NULL to return first
 *		unit test.
 * @return struct ztest_unit_test*
 */
struct ztest_unit_test *z_ztest_get_next_test(const char *suite, struct ztest_unit_test *prev);

/* definitions for use with testing application shared memory   */
#ifdef CONFIG_USERSPACE
/**
 * @brief Make data section used by Ztest userspace accessible
 */
#define ZTEST_DMEM K_APP_DMEM(ztest_mem_partition)
/**
 * @brief Make bss section used by Ztest userspace accessible
 */
#define ZTEST_BMEM K_APP_BMEM(ztest_mem_partition)
/**
 * @brief Ztest data section for accessing data from userspace
 */
#define ZTEST_SECTION K_APP_DMEM_SECTION(ztest_mem_partition)
extern struct k_mem_partition ztest_mem_partition;
#else
#define ZTEST_DMEM
#define ZTEST_BMEM
#define ZTEST_SECTION .data
#endif

/**
 * @brief Fail the currently running test.
 *
 * This is the function called from failed assertions and the like. You
 * probably don't need to call it yourself.
 */
void ztest_test_fail(void);

/**
 * @brief Pass the currently running test.
 *
 * Normally a test passes just by returning without an assertion failure.
 * However, if the success case for your test involves a fatal fault,
 * you can call this function from k_sys_fatal_error_handler to indicate that
 * the test passed before aborting the thread.
 */
void ztest_test_pass(void);

/**
 * @brief Skip the current test.
 *
 */
void ztest_test_skip(void);


void ztest_skip_failed_assumption(void);

/**
 * @brief Returns currently active value-parameterized test argument.
 *
 * For non-parameterized tests this returns NULL.
 */
const void *ztest_get_current_param(void);

/**
 * @brief Returns index of currently active value-parameterized test argument.
 *
 * For non-parameterized tests this returns 0.
 */
size_t ztest_get_current_param_index(void);

/**
 * @brief Returns size of current parameter element in bytes.
 *
 * For non-parameterized tests this returns 0.
 */
size_t ztest_get_current_param_size(void);

/**
 * @brief Returns true when currently running a value-parameterized test instance.
 */
bool ztest_has_current_param(void);

/**
 * @brief Get current parameter as a typed pointer.
 */
#define ZTEST_GET_PARAM_PTR(type) ((const type *)ztest_get_current_param())

/**
 * @brief Get current parameter as a typed value.
 */
#define ZTEST_GET_PARAM(type) (*ZTEST_GET_PARAM_PTR(type))

/**
 * @brief Declare a static value set for use with ZTEST_INSTANTIATE_TEST_SUITE_P().
 *
 * Creates a named ``static const struct ztest_param_values`` backed by the
 * supplied literal values.  The resulting name is passed directly to
 * ZTEST_INSTANTIATE_TEST_SUITE_P().
 *
 * @param name_   Identifier for the resulting variable.
 * @param type_   C type of each individual parameter value.
 * @param ...     Comma-separated list of parameter values.
 */
#define ZTEST_DEFINE_PARAM_VALUES(name_, type_, ...)                                         \
	static const type_ z_ztest_param_arr_##name_[] = {__VA_ARGS__};                     \
	static const struct ztest_param_values name_ = {                                     \
		.values       = z_ztest_param_arr_##name_,                                   \
		.count        = ARRAY_SIZE(z_ztest_param_arr_##name_),                       \
		.elem_size    = sizeof(type_),                                                \
		.name_cb      = NULL,                                                        \
		.value_at_cb  = NULL,                                                        \
		.setup_cb     = NULL,                                                        \
	}

/**
 * @brief Declare a static value set from an existing array.
 *
 * @param name_    Identifier for the resulting variable.
 * @param array_   Array of parameter values (must be in scope at call site).
 */
#define ZTEST_DEFINE_PARAM_VALUES_ARRAY(name_, array_)                                       \
	static const struct ztest_param_values name_ = {                                     \
		.values      = (array_),                                                     \
		.count       = ARRAY_SIZE(array_),                                           \
		.elem_size   = sizeof((array_)[0]),                                          \
		.name_cb     = NULL,                                                         \
		.value_at_cb = NULL,                                                         \
		.setup_cb    = NULL,                                                         \
	}

/**
 * @brief Declare a numeric range for use with ZTEST_INSTANTIATE_TEST_SUITE_P().
 *
 * Modelled on GoogleTest's ``testing::Range(begin, end [, step])``.  The range
 * is half-open: values are ``{begin, begin+step, begin+2*step, ...}`` up to
 * (but not including) @p end_.  @p step_ defaults to 1 when omitted.
 *
 * Values are computed on demand via a generated callback — no array is
 * allocated, so arbitrarily large ranges have zero RAM cost.
 *
 * Constraints (checked at compile time):
 * - @p end_ > @p begin_
 * - @p step_ > 0
 * - ``sizeof(type_)`` must not exceed 16 bytes
 *
 * @param name_   Identifier for the resulting variable.
 * @param type_   Scalar C type (e.g. ``int``, ``uint32_t``).
 * @param begin_  First value in the range (inclusive).
 * @param end_    Upper bound of the range (exclusive).
 * @param step_   Increment between successive values (default 1).
 */
#define ZTEST_DEFINE_PARAM_RANGE(name_, type_, begin_, end_, step_)                          \
	BUILD_ASSERT((end_) > (begin_), "ZTEST_DEFINE_PARAM_RANGE: end must be > begin"); \
	BUILD_ASSERT((step_) > 0, "ZTEST_DEFINE_PARAM_RANGE: step must be > 0");          \
	BUILD_ASSERT(sizeof(type_) <= 16U,                                                   \
		     "ZTEST_DEFINE_PARAM_RANGE: type too large for scratch buffer");        \
	static void z_ztest_range_value_at_##name_(size_t z_idx_, void *z_out_)             \
	{                                                                                    \
		type_ z_val_ = (type_)(begin_) + (type_)(step_) * (type_)(z_idx_);         \
		*(type_ *)z_out_ = z_val_;                                                   \
	}                                                                                    \
	static const struct ztest_param_values name_ = {                                     \
		.values      = NULL,                                                         \
		.count       = (size_t)(((end_) - (begin_) - 1) / (step_) + 1),             \
		.elem_size   = sizeof(type_),                                                \
		.name_cb     = NULL,                                                         \
		.value_at_cb = z_ztest_range_value_at_##name_,                              \
		.setup_cb    = NULL,                                                         \
	}

/**
 * @brief Declare a runtime-generated parameter set.
 *
 * @p gen_cb_ is called once per invocation with the 0-based iteration index
 * and a pointer to an aligned scratch buffer; it must write exactly
 * ``sizeof(type_)`` bytes into that buffer.  The callback may read any
 * runtime state — random numbers, hardware registers, computed values, etc.
 *
 * Values are never stored in an array; @p count_ iterations cost no extra RAM
 * regardless of type size.
 *
 * For generators that require one-time initialisation (e.g. seeding a PRNG)
 * use ZTEST_DEFINE_PARAM_GENERATOR_WITH_SETUP() instead.
 *
 * Constraint (checked at compile time): ``sizeof(type_)`` must not exceed
 * 16 bytes.
 *
 * @param name_    Identifier for the resulting variable.
 * @param type_    C type of each generated value.
 * @param count_   Number of invocations (constant expression).
 * @param gen_cb_  Generator: ``void (*)(size_t index, void *out)``.
 */
#define ZTEST_DEFINE_PARAM_GENERATOR(name_, type_, count_, gen_cb_)                          \
	BUILD_ASSERT(sizeof(type_) <= 16U,                                                   \
		     "ZTEST_DEFINE_PARAM_GENERATOR: type too large for scratch buffer");     \
	static const struct ztest_param_values name_ = {                                     \
		.values      = NULL,                                                         \
		.count       = (count_),                                                     \
		.elem_size   = sizeof(type_),                                                \
		.name_cb     = NULL,                                                         \
		.value_at_cb = (gen_cb_),                                                    \
		.setup_cb    = NULL,                                                         \
	}

/**
 * @brief Declare a runtime-generated parameter set with one-time setup.
 *
 * Identical to ZTEST_DEFINE_PARAM_GENERATOR() but also registers
 * @p setup_cb_, which is called **once** before the first iteration of the
 * dispatch loop.  Use this to seed a PRNG, reset a stateful counter, or
 * perform any other per-test-run initialisation that must happen before
 * values are generated.
 *
 * Example — reproducible fuzz testing:
 *
 * @code{.c}
 * static void seed_rng(void) { sys_rand_seed(MY_FUZZ_SEED); }
 * static void rand_u32(size_t idx, void *out)
 * {
 *     ARG_UNUSED(idx);
 *     *(uint32_t *)out = sys_rand32_get();
 * }
 * ZTEST_DEFINE_PARAM_GENERATOR_WITH_SETUP(fuzz_vals, uint32_t,
 *                                         MY_FUZZ_ITERATIONS,
 *                                         seed_rng, rand_u32);
 * @endcode
 *
 * @param name_      Identifier for the resulting variable.
 * @param type_      C type of each generated value.
 * @param count_     Number of invocations (constant expression).
 * @param setup_cb_  Setup: ``void (*)(void)`` — called once before iteration 0.
 * @param gen_cb_    Generator: ``void (*)(size_t index, void *out)``.
 */
#define ZTEST_DEFINE_PARAM_GENERATOR_WITH_SETUP(name_, type_, count_, setup_cb_, gen_cb_)    \
	BUILD_ASSERT(sizeof(type_) <= 16U,                                                   \
		     "ZTEST_DEFINE_PARAM_GENERATOR_WITH_SETUP: type too large");             \
	static const struct ztest_param_values name_ = {                                     \
		.values      = NULL,                                                         \
		.count       = (count_),                                                     \
		.elem_size   = sizeof(type_),                                                \
		.name_cb     = NULL,                                                         \
		.value_at_cb = (gen_cb_),                                                    \
		.setup_cb    = (setup_cb_),                                                  \
	}

/**
 * @brief Instantiate a ZTEST_P test with a set of parameter values.
 *
 * Registers a ``struct ztest_param_inst`` entry in the ``ztest_param_inst``
 * linker section.  During suite execution the framework calls @p fn once per
 * value in @p param_values_name, setting the per-invocation parameter context
 * before each call.  Inside the test body use ztest_get_current_param() or the
 * typed helper ZTEST_GET_PARAM() to retrieve the current value.
 *
 * The suite fixture returned by ``setup()`` is passed as the ``data`` argument
 * of the ZTEST_P function and is **never** overwritten by the parameter.
 *
 * @param inst_             Unique identifier for this instantiation.
 * @param suite_            Suite name (must match the ZTEST_SUITE declaration).
 * @param fn_               Test name (must be declared with ZTEST_P).
 * @param param_values_name_ Name of a variable declared with
 *                           ZTEST_DEFINE_PARAM_VALUES() or
 *                           ZTEST_DEFINE_PARAM_VALUES_ARRAY().
 */
/* Note: all parameters carry a trailing underscore to avoid name collisions
 * with the struct fields of ztest_param_inst (e.g. a parameter named
 * 'instance_name' would cause the preprocessor to replace the token inside
 * the '.instance_name' designator, producing the invalid '.ints').
 */
#define ZTEST_INSTANTIATE_TEST_SUITE_P(inst_, suite_, fn_, param_values_name_)               \
	static const STRUCT_SECTION_ITERABLE(                                                \
		ztest_param_inst,                                                            \
		UTIL_CAT(UTIL_CAT(z_ztest_param_inst_, suite_),                              \
			 UTIL_CAT(_, UTIL_CAT(fn_, UTIL_CAT(_, inst_))))) = {               \
		.test_suite_name = STRINGIFY(suite_),                                        \
		.test_name       = STRINGIFY(fn_),                                           \
		.instance_name   = STRINGIFY(inst_),                                         \
		.values          = &(param_values_name_),                                    \
	}

#define Z_TEST_P(suite, fn, t_options) \
	struct ztest_unit_test_stats z_ztest_unit_test_stats_##suite##_##fn; \
	static void _##suite##_##fn##_wrapper(void *data); \
	/* data carries the suite fixture from setup(); use             */ \
	/* ztest_get_current_param() / ZTEST_GET_PARAM() for the value. */ \
	static void suite##_##fn(void *data); \
	static STRUCT_SECTION_ITERABLE(ztest_unit_test, z_ztest_unit_test__##suite##__##fn) = { \
		.test_suite_name = STRINGIFY(suite), \
		.name = STRINGIFY(fn), \
		.test = (_##suite##_##fn##_wrapper), \
		.thread_options = t_options, \
		.stats = &z_ztest_unit_test_stats_##suite##_##fn \
	}; \
	static void _##suite##_##fn##_wrapper(void *wrapper_data) \
	{ \
		 suite##_##fn(wrapper_data); \
	} \
	static inline void suite##_##fn(void *data)


#define ZTEST_P(suite, fn) Z_TEST_P(suite, fn, 0)

/**
 * @brief Define a value-parameterized test function that runs as a user thread.
 *
 * Behaves exactly like ZTEST_P() but spawns the test thread with the @c K_USER
 * flag so the test body executes in user space when @c CONFIG_USERSPACE is
 * enabled.  Use this instead of ZTEST_P() when the code under test is a
 * syscall or must otherwise be exercised from an unprivileged context.
 *
 * @param suite The name of the test suite to attach this test.
 * @param fn    The test function to call (must be paired with
 *              ZTEST_INSTANTIATE_TEST_SUITE_P()).
 */
#define ZTEST_USER_P(suite, fn) Z_TEST_P(suite, fn, K_USER)

#define Z_TEST(suite, fn, t_options, use_fixture)                                                  \
	struct ztest_unit_test_stats z_ztest_unit_test_stats_##suite##_##fn;                       \
	static void _##suite##_##fn##_wrapper(void *data);                                         \
	static void suite##_##fn(                                                                  \
		COND_CODE_1(use_fixture, (struct suite##_fixture *fixture), (void)));              \
	static STRUCT_SECTION_ITERABLE(ztest_unit_test, z_ztest_unit_test__##suite##__##fn) = {    \
		.test_suite_name = STRINGIFY(suite),                                               \
		.name = STRINGIFY(fn),                                                             \
		.test = (_##suite##_##fn##_wrapper),                                               \
		.thread_options = t_options,                                                       \
		.stats = &z_ztest_unit_test_stats_##suite##_##fn                                   \
	};                                                                                         \
	static void _##suite##_##fn##_wrapper(void *wrapper_data)                                  \
	{                                                                                          \
		COND_CODE_1(use_fixture, (suite##_##fn((struct suite##_fixture *)wrapper_data);),  \
			    (ARG_UNUSED(wrapper_data); suite##_##fn();))                           \
	}                                                                                          \
	static inline void suite##_##fn(                                                           \
		COND_CODE_1(use_fixture, (struct suite##_fixture *fixture), (void)))

#define Z_ZTEST(suite, fn, t_options) Z_TEST(suite, fn, t_options, 0)
#define Z_ZTEST_F(suite, fn, t_options) Z_TEST(suite, fn, t_options, 1)

/**
 * @brief Skips the test if config is enabled
 *
 * Use this macro at the start of your test case, to skip it when
 * config is enabled.  Useful when your test is still under development.
 *
 * @param config The Kconfig option used to skip the test.
 */
#define Z_TEST_SKIP_IFDEF(config) COND_CODE_1(config, (ztest_test_skip()), ())

/**
 * @brief Skips the test if config is not enabled
 *
 * Use this macro at the start of your test case, to skip it when
 * config is not enabled.  Useful when your need to skip test if some
 * configuration option is not enabled.
 *
 * @param config The Kconfig option used to skip the test (if not enabled).
 */
#define Z_TEST_SKIP_IFNDEF(config) COND_CODE_1(config, (), (ztest_test_skip()))

/**
 * @brief Create and register a new unit test.
 *
 * Calling this macro will create a new unit test and attach it to the declared `suite`. The `suite`
 * does not need to be defined in the same compilation unit.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST(suite, fn) Z_ZTEST(suite, fn, 0)

/**
 * @brief Define a test function that should run as a user thread
 *
 * This macro behaves exactly the same as ZTEST, but calls the test function in user space if
 * `CONFIG_USERSPACE` was enabled.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST_USER(suite, fn) Z_ZTEST(suite, fn, K_USER)

/**
 * @brief Define a test function
 *
 * This macro behaves exactly the same as ZTEST(), but the function takes an argument for the
 * fixture of type `struct suite##_fixture*` named `fixture`.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST_F(suite, fn) Z_ZTEST_F(suite, fn, 0)

/**
 * @brief Define a test function that should run as a user thread
 *
 * If CONFIG_USERSPACE is not enabled, this is functionally identical to ZTEST_F(). The test
 * function takes a single fixture argument of type `struct suite##_fixture*` named `fixture`.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST_USER_F(suite, fn) Z_ZTEST_F(suite, fn, K_USER)

/**
 * @brief Test rule callback function signature
 *
 * The function signature that can be used to register a test rule's before/after callback. This
 * provides access to the test and the fixture data (if provided).
 *
 * @param test Pointer to the unit test in context
 * @param data Pointer to the test's fixture data (may be NULL)
 */
typedef void (*ztest_rule_cb)(const struct ztest_unit_test *test, void *data);

/** @private */
struct ztest_test_rule {
	ztest_rule_cb before_each;
	ztest_rule_cb after_each;
};

/**
 * @brief Define a test rule that will run before/after each unit test.
 *
 * Functions defined here will run before/after each unit test for every test suite. Along with the
 * callback, the test functions are provided a pointer to the test being run, and the data. This
 * provides a mechanism for tests to perform custom operations depending on the specific test or
 * the data (for example logging may use the test's name).
 *
 * Ordering:
 * - Test rule's `before` function will run before the suite's `before` function. This is done to
 * allow the test suite's customization to take precedence over the rule which is applied to all
 * suites.
 * - Test rule's `after` function is not guaranteed to run in any particular order.
 *
 * @param name The name for the test rule (must be unique within the compilation unit)
 * @param before_each_fn The callback function (ztest_rule_cb) to call before each test
 *        (may be NULL)
 * @param after_each_fn The callback function (ztest_rule_cb) to call after each test (may be NULL)
 */
#define ZTEST_RULE(name, before_each_fn, after_each_fn)                                            \
	static STRUCT_SECTION_ITERABLE(ztest_test_rule, z_ztest_test_rule_##name) = {              \
		.before_each = (before_each_fn),                                                   \
		.after_each = (after_each_fn),                                                     \
	}

extern struct ztest_test_rule _ztest_test_rule_list_start[];
extern struct ztest_test_rule _ztest_test_rule_list_end[];

/**
 * @brief A 'before' function to use in test suites that just need to start 1cpu
 *
 * Ignores data, and calls z_test_1cpu_start()
 *
 * @param data The test suite's data
 */
void ztest_simple_1cpu_before(void *data);

/**
 * @brief A 'after' function to use in test suites that just need to stop 1cpu
 *
 * Ignores data, and calls z_test_1cpu_stop()
 *
 * @param data The test suite's data
 */
void ztest_simple_1cpu_after(void *data);

/**
 * @brief Run the specified test suite.
 *
 * @param suite Test suite to run.
 * @param shuffle Shuffle tests
 * @param suite_iter Test suite repetitions.
 * @param case_iter Test case repetitions.
 * @param param Test parameter
 */
#define ztest_run_test_suite(suite, shuffle, suite_iter, case_iter, param) \
	z_ztest_run_test_suite(STRINGIFY(suite), shuffle, suite_iter, case_iter, param)

/**
 * @brief Structure for architecture specific APIs
 *
 */
struct ztest_arch_api {
	void (*run_all)(const void *state, bool shuffle, int suite_iter, int case_iter);
	bool (*should_suite_run)(const void *state, struct ztest_suite_node *suite);
	bool (*should_test_run)(const char *suite, const char *test);
};

/**
 * @}
 */

__syscall void z_test_1cpu_start(void);
__syscall void z_test_1cpu_stop(void);

__syscall void sys_clock_tick_set(uint64_t tick);

#ifdef __cplusplus
}
#endif

#ifndef ZTEST_UNITTEST
#include <zephyr/syscalls/ztest_test.h>
#endif

#endif /* ZEPHYR_TESTSUITE_ZTEST_TEST_H_ */
