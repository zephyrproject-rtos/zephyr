/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Overflow and boundary tests for 32-bit builds.
 *
 * This file is compiled only when CONFIG_64BIT is NOT set (i.e. on 32-bit
 * Zephyr targets such as native_sim/native/32 or native_posix).  It is
 * included by CMakeLists.txt conditionally so that the common test suite
 * in zfence_test.c does not need to be re-run for these platform-specific
 * cases.
 *
 * Tests cover:
 *   - zfence_signal()	   returns -EINVAL when signal_seq is already at
 *			   UINT32_MAX (the maximum representable value on a
 *			   32-bit atomic_t).
 *   - zfence_signal_seq() returns -EINVAL when the requested seq exceeds
 *			   UINT32_MAX.
 *   - zfence_next()	   returns UINT32_MAX (the last valid value) when
 *			   signal_seq is at UINT32_MAX - 1, and does not
 *			   wrap around to 0.
 */

#include <zephyr/kernel.h>
#include <zephyr/zfence/zfence.h>
#include <zephyr/ztest.h>
#include <stdint.h>

/* Shared state */

static struct zfence g_fence_32;

static void zfence_overflow_32bit_before(void *data)
{
	ARG_UNUSED(data);
	zfence_init(&g_fence_32);
}

/* Tests */

/*
 * Force signal_seq to UINT32_MAX (the maximum representable value on a
 * 32-bit atomic_t).  A further zfence_signal() must return -EINVAL rather
 * than silently wrapping the counter back to 0.
 */
ZTEST(zfence_overflow_32bit_suite, test_signal_at_max_returns_einval)
{
	atomic_set(&g_fence_32.signal_seq,
	       (atomic_val_t)(unsigned long)UINT32_MAX);

	int ret = zfence_signal(&g_fence_32, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EINVAL,
		  "zfence_signal at UINT32_MAX must return -EINVAL on 32-bit");
}

/*
 * A seq value that exceeds UINT32_MAX cannot be stored in a 32-bit atomic_t
 * and must be rejected with -EINVAL.
 */
ZTEST(zfence_overflow_32bit_suite, test_signal_seq_overflow_returns_einval)
{
	uint64_t overflow_seq = (uint64_t)UINT32_MAX + 1ULL;

	int ret = zfence_signal_seq(&g_fence_32, overflow_seq, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EINVAL,
		  "zfence_signal_seq with seq > UINT32_MAX must return -EINVAL on 32-bit");
}

/*
 * Force signal_seq and reserved_seq to UINT32_MAX - 1.	 The next reservation
 * must be UINT32_MAX (the last representable value) and must not wrap to 0.
 */
ZTEST(zfence_overflow_32bit_suite, test_next_at_boundary)
{
	atomic_set(&g_fence_32.signal_seq,
	       (atomic_val_t)(unsigned long)(UINT32_MAX - 1U));
	atomic_set(&g_fence_32.reserved_seq,
	       (atomic_val_t)(unsigned long)(UINT32_MAX - 1U));

	uint64_t seq = zfence_next(&g_fence_32);

	zassert_equal(seq, (uint64_t)UINT32_MAX,
		  "zfence_next at UINT32_MAX-1 must return UINT32_MAX on 32-bit");
	zassert_equal((unsigned long)atomic_get(&g_fence_32.reserved_seq),
		  (unsigned long)UINT32_MAX,
		  "reserved_seq must be UINT32_MAX after boundary reservation");
}

/* Suite definition */

ZTEST_SUITE(zfence_overflow_32bit_suite, NULL, NULL,
	    zfence_overflow_32bit_before, NULL, NULL);
