/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extended sequence-range tests for 64-bit builds.
 *
 * This file is compiled only when CONFIG_64BIT is set (i.e. on 64-bit Zephyr
 * targets such as native_sim).	 It is included by CMakeLists.txt
 * conditionally so that the common test suite in zfence_test.c does not need
 * to be re-run for these platform-specific cases.
 *
 * Tests cover:
 *   - zfence_next()	   returns UINT32_MAX + 1 when signal_seq is at
 *			   UINT32_MAX, confirming that the 64-bit counter does
 *			   not wrap at the 32-bit boundary.
 *   - zfence_signal_seq() succeeds for a seq value beyond UINT32_MAX and
 *			   zfence_current() reflects the new value.
 *   - zfence_result()	   correctly classifies sequence values around the
 *			   UINT32_MAX boundary (OK / OVERWRITTEN / UNSIGNALED).
 */

#include <zephyr/kernel.h>
#include <zephyr/zfence/zfence.h>
#include <zephyr/ztest.h>
#include <stdint.h>

/* Shared state */

static struct zfence g_fence_64;

static void zfence_overflow_64bit_before(void *data)
{
	ARG_UNUSED(data);
	zfence_init(&g_fence_64);
}

/* Tests */

/*
 * Force signal_seq and reserved_seq to UINT32_MAX.  On a 64-bit build the
 * next reservation must be UINT32_MAX + 1, not a wrapped-around value of 0.
 */
ZTEST(zfence_overflow_64bit_suite, test_next_beyond_uint32)
{
	atomic_set(&g_fence_64.signal_seq,
	       (atomic_val_t)(unsigned long)UINT32_MAX);
	atomic_set(&g_fence_64.reserved_seq,
	       (atomic_val_t)(unsigned long)UINT32_MAX);

	uint64_t seq = zfence_next(&g_fence_64);

	zassert_equal(seq, (uint64_t)UINT32_MAX + 1ULL,
		  "zfence_next must return UINT32_MAX+1 on 64-bit");
	zassert_equal((unsigned long)atomic_get(&g_fence_64.reserved_seq),
		  (unsigned long)((uint64_t)UINT32_MAX + 1ULL),
		  "reserved_seq must be UINT32_MAX+1 after reservation on 64-bit");
}

/*
 * Signalling a sequence value beyond UINT32_MAX must succeed on a 64-bit
 * build and be reflected by zfence_current().
 */
ZTEST(zfence_overflow_64bit_suite, test_signal_seq_beyond_uint32)
{
	uint64_t big_seq = (uint64_t)UINT32_MAX + 1ULL;

	/* Position signal_seq just below big_seq so the CAS can advance. */
	atomic_set(&g_fence_64.signal_seq,
	       (atomic_val_t)(unsigned long)UINT32_MAX);
	atomic_set(&g_fence_64.reserved_seq,
	       (atomic_val_t)(unsigned long)big_seq);

	int ret = zfence_signal_seq(&g_fence_64, big_seq, ZFENCE_RESULT_OK);

	zassert_equal(ret, 0,
		  "zfence_signal_seq with seq > UINT32_MAX must succeed on 64-bit");
	zassert_equal(zfence_current(&g_fence_64), big_seq,
		  "zfence_current must return UINT32_MAX+1 after signal on 64-bit");
}

/*
 * After signalling big_seq (UINT32_MAX + 1) via a jump-ahead:
 *   - big_seq itself	       -> ZFENCE_RESULT_OK
 *   - UINT32_MAX	       -> ZFENCE_RESULT_OVERWRITTEN (jumped over)
 *   - big_seq + 1	       -> ZFENCE_RESULT_UNSIGNALED
 */
ZTEST(zfence_overflow_64bit_suite, test_result_beyond_uint32)
{
	uint64_t big_seq = (uint64_t)UINT32_MAX + 1ULL;

	atomic_set(&g_fence_64.signal_seq,
	       (atomic_val_t)(unsigned long)UINT32_MAX);
	atomic_set(&g_fence_64.reserved_seq,
	       (atomic_val_t)(unsigned long)big_seq);

	zfence_signal_seq(&g_fence_64, big_seq, ZFENCE_RESULT_OK);

	zassert_equal(zfence_result(&g_fence_64, big_seq), ZFENCE_RESULT_OK,
		  "zfence_result for UINT32_MAX+1 must be OK on 64-bit");
	zassert_equal(zfence_result(&g_fence_64, (uint64_t)UINT32_MAX),
		  ZFENCE_RESULT_OVERWRITTEN,
		  "zfence_result for UINT32_MAX must be OVERWRITTEN on 64-bit");
	zassert_equal(zfence_result(&g_fence_64, big_seq + 1ULL),
		  ZFENCE_RESULT_UNSIGNALED,
		  "zfence_result for UINT32_MAX+2 must be UNSIGNALED on 64-bit");
}

/* Suite definition */

ZTEST_SUITE(zfence_overflow_64bit_suite, NULL, NULL,
	    zfence_overflow_64bit_before, NULL, NULL);
