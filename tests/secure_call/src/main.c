/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief Ztest suite for the secure_call infrastructure.
 *
 * Tests the runtime behaviour of the generated NS-side wrappers in the
 * single-image (non-TrustZone) configuration. In this mode the wrapper's
 * #else branch fires:
 *
 *   compiler_barrier();
 *   return z_secure_impl_*(args...);
 *
 * so we can verify argument forwarding and return-value propagation using
 * ordinary host-side test implementations of z_secure_impl_*.
 *
 * Separate Python tests (scripts/tests/build/test_parse_gen.py) cover the
 * code-generation output from parse_secure_calls.py and gen_secure_calls.py.
 */

#include <zephyr/ztest.h>
#include <zephyr/secure_call.h>
#include <zephyr/internal/secure_call_handler.h>
#include "test_calls.h"

/*
 * z_secure_impl_* implementations
 * These are the "actual TEE implementations" for the test. In a real dual-
 * image build they would live in the Secure image; here they run in-process
 * so we can inspect their arguments and control their return values.
 */

static int impl_add_last_a;
static int impl_add_last_b;

int z_secure_impl_sc_add(int a, int b)
{
	impl_add_last_a = a;
	impl_add_last_b = b;
	return a + b;
}

static uint8_t *impl_fill_last_buf;
static size_t impl_fill_last_len;

void z_secure_impl_sc_fill(uint8_t *buf, size_t len)
{
	impl_fill_last_buf = buf;
	impl_fill_last_len = len;
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)i;
	}
}

int z_secure_impl_sc_nop(void)
{
	return 42;
}

ZTEST_SUITE(secure_call, NULL, NULL, NULL, NULL, NULL);

/**
 * Calling sc_add() in single-image mode must:
 *   1. Forward both arguments to z_secure_impl_sc_add unchanged.
 *   2. Return the value produced by z_secure_impl_sc_add.
 */
ZTEST(secure_call, test_single_image_int_return)
{
	int result = sc_add(3, 4);

	zassert_equal(result, 7, "sc_add(3,4) returned %d, expected 7", result);
	zassert_equal(impl_add_last_a, 3, "arg a not forwarded correctly");
	zassert_equal(impl_add_last_b, 4, "arg b not forwarded correctly");
}

/**
 * Calling sc_fill() in single-image mode must:
 *   1. Forward the pointer and length to z_secure_impl_sc_fill unchanged.
 *   2. Not crash (void return, no value to check beyond side-effects).
 */
ZTEST(secure_call, test_single_image_void_return)
{
	uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};

	sc_fill(buf, 4);

	zassert_equal_ptr(impl_fill_last_buf, buf, "buf pointer not forwarded");
	zassert_equal(impl_fill_last_len, 4U, "len not forwarded correctly");

	/* Verify the impl actually ran (filled the buffer) */
	for (int i = 0; i < 4; i++) {
		zassert_equal(buf[i], (uint8_t)i, "buf[%d] = 0x%02x, expected 0x%02x", i, buf[i],
			      (uint8_t)i);
	}
}

/**
 * Calling sc_nop() must return the value from z_secure_impl_sc_nop.
 */
ZTEST(secure_call, test_single_image_zero_arg)
{
	int result = sc_nop();

	zassert_equal(result, 42, "sc_nop() returned %d, expected 42", result);
}

/**
 * z_secure_call_lock/unlock must not crash and must be no-ops in non-NS mode
 * (CONFIG_TRUSTED_EXECUTION_NONSECURE not set in this test build).
 */
ZTEST(secure_call, test_lock_unlock_noop)
{
	z_secure_call_lock();
	z_secure_call_unlock();
	/* If we get here without hanging or crashing, the test passes. */
}

/**
 * Z_SECURE_MEMORY_* macros must compile and be no-ops when
 * CONFIG_ARM_SECURE_FIRMWARE is not set (which is the case for this test).
 */
ZTEST(secure_call, test_handler_macros_noop)
{
	uint8_t buf[8];

	(void)buf; /* macros are no-ops in non-S builds; suppress unused warning */
	/* These expand to do { } while (0) in non-S builds */
	Z_SECURE_MEMORY_READ(buf, sizeof(buf));
	Z_SECURE_MEMORY_WRITE(buf, sizeof(buf));
	Z_SECURE_VERIFY(1);
}

/**
 * Z_SECURE_VERIFY(0) should call arch_secure_call_oops().
 * In non-S builds the macro is a no-op, so this just verifies compilation.
 * A separate on-target test would be needed to verify the oops path.
 */
ZTEST(secure_call, test_handler_verify_noop_on_non_s)
{
	/* In non-S builds Z_SECURE_VERIFY(0) is a no-op — no oops triggered */
	Z_SECURE_VERIFY(0);
}
