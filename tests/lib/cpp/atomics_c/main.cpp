/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic>
#include <cstdint>
#include <zephyr/ztest.h>

namespace
{
std::atomic<uint8_t> atomic_u8;
std::atomic<uint16_t> atomic_u16;
std::atomic<uint32_t> atomic_u32;
} // namespace

/**
 * @brief This `before` function is run before each test in the suite.
 *
 * It ensures that all atomic variables are reset to a known state (0)
 * so that the tests are independent and repeatable.
 */
static void cxx_atomic_before(void *fixture)
{
	atomic_u8.store(0);
	atomic_u16.store(0);
	atomic_u32.store(0);
}

/**
 * @brief Tests the 1-byte (uint8_t) atomic implementation.
 */
ZTEST(cxx_atomic, test_u8_compare_exchange_weak)
{
	/* === Test 1: Successful exchange === */
	/* We expect the value to be 0, so this exchange should succeed. */
	uint8_t expected = 0;
	const uint8_t desired = 42;

	/* Loop until the weak exchange succeeds. */
	while (!atomic_u8.compare_exchange_weak(expected, desired)) {
	}
	zassert_equal(atomic_u8.load(), desired, "Value should have been updated to 42");

	/* === Test 2: Failed exchange === */
	/* Now, the atomic value is 42. We will set `expected` to 0, so the exchange must fail. */
	expected = 0;
	const uint8_t new_desired = 99;
	bool success = atomic_u8.compare_exchange_weak(expected, new_desired);

	zassert_false(success, "Exchange should have failed");
	zassert_equal(atomic_u8.load(), desired, "Value should remain 42");
	/* Crucially, `expected` should be updated with the value that caused the failure. */
	zassert_equal(expected, desired, "Expected should be updated to 42");
}

/**
 * @brief Tests the 2-byte (uint16_t) atomic implementation.
 */
ZTEST(cxx_atomic, test_u16_compare_exchange_weak)
{
	/* === Test 1: Successful exchange === */
	uint16_t expected = 0;
	const uint16_t desired = 1337;
	while (!atomic_u16.compare_exchange_weak(expected, desired)) {
	}
	zassert_equal(atomic_u16.load(), desired, "Value should have been updated to 1337");

	/* === Test 2: Failed exchange === */
	expected = 0;
	const uint16_t new_desired = 9999;
	bool success = atomic_u16.compare_exchange_weak(expected, new_desired);

	zassert_false(success, "Exchange should have failed");
	zassert_equal(atomic_u16.load(), desired, "Value should remain 1337");
	zassert_equal(expected, desired, "Expected should be updated to 1337");
}

/**
 * @brief Tests the 4-byte (uint32_t) atomic implementation.
 */
ZTEST(cxx_atomic, test_u32_compare_exchange_weak)
{
	/* === Test 1: Successful exchange === */
	uint32_t expected = 0;
	const uint32_t desired = 0xDEADBEEF;
	while (!atomic_u32.compare_exchange_weak(expected, desired)) {
	}
	zassert_equal(atomic_u32.load(), desired, "Value should have been updated to 0xDEADBEEF");

	/* === Test 2: Failed exchange === */
	expected = 0;
	const uint32_t new_desired = 0x12345678;
	bool success = atomic_u32.compare_exchange_weak(expected, new_desired);

	zassert_false(success, "Exchange should have failed");
	zassert_equal(atomic_u32.load(), desired, "Value should remain 0xDEADBEEF");
	zassert_equal(expected, desired, "Expected should be updated to 0xDEADBEEF");
}

ZTEST_SUITE(cxx_atomic, nullptr, nullptr, cxx_atomic_before, nullptr, nullptr);
