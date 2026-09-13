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
std::atomic<uint64_t> atomic_u64;
std::atomic_flag atomic_flag = ATOMIC_FLAG_INIT;
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
	atomic_u64.store(0);
	atomic_flag.clear();
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

/**
 * @brief Tests the 8-byte (uint64_t) atomic compare-exchange implementation.
 */
ZTEST(cxx_atomic, test_u64_compare_exchange_weak)
{
	uint64_t expected = 0;
	const uint64_t desired = 0x1122334455667788ULL;

	while (!atomic_u64.compare_exchange_weak(expected, desired)) {
	}
	zassert_equal(atomic_u64.load(), desired, "Value should have been updated");

	expected = 0;
	bool success = atomic_u64.compare_exchange_weak(expected, 0ULL);

	zassert_false(success, "Exchange should have failed");
	zassert_equal(atomic_u64.load(), desired, "Value should remain unchanged");
	zassert_equal(expected, desired, "Expected should be updated");
}

/**
 * @brief Tests load, store and exchange across the supported widths.
 */
ZTEST(cxx_atomic, test_load_store_exchange)
{
	atomic_u8.store(0x12);
	zassert_equal(atomic_u8.load(), 0x12, "u8 store/load mismatch");
	zassert_equal(atomic_u8.exchange(0x34), 0x12, "u8 exchange should return old value");
	zassert_equal(atomic_u8.load(), 0x34, "u8 exchange should update value");

	atomic_u64.store(0xAABBCCDDEEFF0011ULL);
	zassert_equal(atomic_u64.load(), 0xAABBCCDDEEFF0011ULL, "u64 store/load mismatch");
	zassert_equal(atomic_u64.exchange(1ULL), 0xAABBCCDDEEFF0011ULL,
		      "u64 exchange should return old value");
	zassert_equal(atomic_u64.load(), 1ULL, "u64 exchange should update value");
}

/**
 * @brief Tests the read-modify-write fetch operations.
 */
ZTEST(cxx_atomic, test_fetch_ops)
{
	atomic_u32.store(0x00FF00FF);
	zassert_equal(atomic_u32.fetch_add(1), 0x00FF00FF, "fetch_add should return old value");
	zassert_equal(atomic_u32.load(), 0x00FF0100, "fetch_add should update value");
	zassert_equal(atomic_u32.fetch_sub(0x100), 0x00FF0100, "fetch_sub should return old value");
	zassert_equal(atomic_u32.fetch_or(0xFF000000), 0x00FF00FF, "fetch_or should return old");
	zassert_equal(atomic_u32.fetch_and(0x0000FFFF), 0xFFFF00FF, "fetch_and should return old");
	zassert_equal(atomic_u32.load(), 0x000000FF, "fetch_and should update value");
	zassert_equal(atomic_u32.fetch_xor(0x000000FF), 0x000000FF, "fetch_xor should return old");
	zassert_equal(atomic_u32.load(), 0x0U, "fetch_xor should update value");

	atomic_u64.store(0);
	zassert_equal(atomic_u64.fetch_add(0x1'0000'0000ULL), 0ULL, "u64 fetch_add old value");
	zassert_equal(atomic_u64.load(), 0x1'0000'0000ULL, "u64 fetch_add should update value");
}

/**
 * @brief Tests std::atomic_flag (test_and_set / clear).
 */
ZTEST(cxx_atomic, test_atomic_flag)
{
	zassert_false(atomic_flag.test_and_set(), "First test_and_set should return false");
	zassert_true(atomic_flag.test_and_set(), "Second test_and_set should return true");
	atomic_flag.clear();
	zassert_false(atomic_flag.test_and_set(), "test_and_set after clear should return false");
	atomic_flag.clear();
}

ZTEST_SUITE(cxx_atomic, nullptr, nullptr, cxx_atomic_before, nullptr, nullptr);
