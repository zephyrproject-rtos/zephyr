/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POST vendor test integration API
 *
 * Provides macros for integrating third-party self-test libraries
 * (e.g., Infineon MTB-STL, TI SafeTI) with the Zephyr POST framework.
 */

#ifndef ZEPHYR_INCLUDE_POST_VENDOR_H_
#define ZEPHYR_INCLUDE_POST_VENDOR_H_

#include <zephyr/post/post.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup post_vendor POST Vendor Integration
 * @ingroup post_api
 * @{
 */

/**
 * @brief Wrap a vendor test function for POST registration
 *
 * This macro wraps a vendor test function that returns 0 on success
 * and non-zero on failure, converting it to the POST result format.
 * Test IDs are automatically assigned sequentially at link time (0, 1, 2, ...).
 *
 * @param _name    Unique C identifier for the test
 * @param _cat     Test category (POST_CAT_*)
 * @param _level   Earliest init level (POST_LEVEL_*)
 * @param _vendor_fn Vendor test function (must return 0=pass, non-zero=fail)
 * @param _desc    Human-readable description
 */
#define POST_VENDOR_TEST_WRAP(_name, _cat, _level, _vendor_fn, _desc) \
	static enum post_result _name##_wrapper(const struct post_context *ctx) \
	{ \
		ARG_UNUSED(ctx); \
		return ((_vendor_fn)() == 0) ? POST_RESULT_PASS : POST_RESULT_FAIL; \
	} \
	POST_TEST_DEFINE(_name, _cat, _level, 50, \
			 POST_FLAG_VENDOR | POST_FLAG_RUNTIME_OK, \
			 _name##_wrapper, _desc)

/**
 * @brief Wrap a vendor test with custom flags
 *
 * Test IDs are automatically assigned sequentially at link time (0, 1, 2, ...).
 *
 * @param _name      Unique C identifier for the test
 * @param _cat       Test category (POST_CAT_*)
 * @param _level     Earliest init level (POST_LEVEL_*)
 * @param _flags     Additional flags (POST_FLAG_VENDOR is added automatically)
 * @param _vendor_fn Vendor test function (must return 0=pass, non-zero=fail)
 * @param _desc      Human-readable description
 */
#define POST_VENDOR_TEST_WRAP_FLAGS(_name, _cat, _level, _flags, \
				    _vendor_fn, _desc) \
	static enum post_result _name##_wrapper(const struct post_context *ctx) \
	{ \
		ARG_UNUSED(ctx); \
		return ((_vendor_fn)() == 0) ? POST_RESULT_PASS : POST_RESULT_FAIL; \
	} \
	POST_TEST_DEFINE(_name, _cat, _level, 50, \
			 POST_FLAG_VENDOR | (_flags), \
			 _name##_wrapper, _desc)

/**
 * @brief Wrap a vendor test with init and cleanup callbacks
 *
 * For tests that require hardware initialization before running
 * and cleanup afterward.
 * Test IDs are automatically assigned sequentially at link time (0, 1, 2, ...).
 *
 * @param _name       Unique C identifier for the test
 * @param _cat        Test category (POST_CAT_*)
 * @param _level      Earliest init level (POST_LEVEL_*)
 * @param _init_fn    Init function (called before test, can be NULL)
 * @param _vendor_fn  Vendor test function (0=pass, non-zero=fail)
 * @param _cleanup_fn Cleanup function (called after test, can be NULL)
 * @param _desc       Human-readable description
 */
#define POST_VENDOR_TEST_WRAP_EX(_name, _cat, _level, \
				 _init_fn, _vendor_fn, _cleanup_fn, _desc) \
	static enum post_result _name##_wrapper(const struct post_context *ctx) \
	{ \
		ARG_UNUSED(ctx); \
		enum post_result result; \
		if ((_init_fn) != NULL) { \
			(_init_fn)(); \
		} \
		result = ((_vendor_fn)() == 0) ? POST_RESULT_PASS : POST_RESULT_FAIL; \
		if ((_cleanup_fn) != NULL) { \
			(_cleanup_fn)(); \
		} \
		return result; \
	} \
	POST_TEST_DEFINE(_name, _cat, _level, 50, \
			 POST_FLAG_VENDOR | POST_FLAG_RUNTIME_OK, \
			 _name##_wrapper, _desc)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POST_VENDOR_H_ */
