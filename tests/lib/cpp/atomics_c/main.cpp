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

template <typename T> static void check_fetch_ops(std::atomic<T> &a)
{
	a.store(10);
	zassert_equal(a.fetch_add(5), 10);
	zassert_equal(a.load(), 15);
	zassert_equal(a.fetch_sub(3), 15);
	zassert_equal(a.load(), 12);
	zassert_equal(a.fetch_and(0x0A), 12);
	zassert_equal(a.load(), 0x08);
	zassert_equal(a.fetch_or(0x05), 0x08);
	zassert_equal(a.load(), 0x0D);
	zassert_equal(a.fetch_xor(0x0F), 0x0D);
	zassert_equal(a.load(), 0x02);
	zassert_equal(a.exchange(0x46), 0x02);
	zassert_equal(a.load(), 0x46);

	zassert_equal(a += 3, 0x49);
	zassert_equal(a -= 3, 0x46);
	zassert_equal(a &= 0x0F, 0x06);
	zassert_equal(a |= 0x30, 0x36);
	zassert_equal(a ^= 0x06, 0x30);

	a.store(static_cast<T>(-1));
	zassert_equal(a.fetch_add(2), static_cast<T>(-1));
	zassert_equal(a.load(), 1, "Addition should wrap around");
}

template <typename T> static void check_nand_ops(void)
{
	T v = 0x0F;

	zassert_equal(__atomic_fetch_nand(&v, 0x05, __ATOMIC_SEQ_CST), 0x0F);
	zassert_equal(v, static_cast<T>(~0x05));
	zassert_equal(__atomic_nand_fetch(&v, 0x0F, __ATOMIC_SEQ_CST), static_cast<T>(~0x0A));
	zassert_equal(v, static_cast<T>(~0x0A));
}

ZTEST(cxx_atomic, test_u8_fetch_ops)
{
	check_fetch_ops(atomic_u8);
	check_nand_ops<uint8_t>();
}

ZTEST(cxx_atomic, test_u16_fetch_ops)
{
	check_fetch_ops(atomic_u16);
	check_nand_ops<uint16_t>();
}

ZTEST(cxx_atomic, test_u32_fetch_ops)
{
	check_fetch_ops(atomic_u32);
	check_nand_ops<uint32_t>();
}

ZTEST_SUITE(cxx_atomic, nullptr, nullptr, cxx_atomic_before, nullptr, nullptr);
