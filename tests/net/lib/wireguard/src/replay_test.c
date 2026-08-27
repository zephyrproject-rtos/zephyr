/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "wg_test_helpers.h"

ZTEST(wireguard_replay, test_first_packet)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;

	zassert_true(wg_replay_check(&bitmap, &counter, 0));
	zassert_equal(counter, 1);
	zassert_equal(bitmap, 1);
}

ZTEST(wireguard_replay, test_duplicate)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;

	zassert_true(wg_replay_check(&bitmap, &counter, 5));
	zassert_false(wg_replay_check(&bitmap, &counter, 5));
}

ZTEST(wireguard_replay, test_out_of_order)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;

	zassert_true(wg_replay_check(&bitmap, &counter, 10));
	zassert_true(wg_replay_check(&bitmap, &counter, 8));
}

ZTEST(wireguard_replay, test_too_old)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;

	zassert_true(wg_replay_check(&bitmap, &counter, 100));
	zassert_false(wg_replay_check(&bitmap, &counter, 50));
}

ZTEST(wireguard_replay, test_large_jump)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;

	zassert_true(wg_replay_check(&bitmap, &counter, 5));
	zassert_true(wg_replay_check(&bitmap, &counter, 1000));
	zassert_equal(counter, 1001);
	zassert_equal(bitmap, 1);
}

/* A forward jump larger than 2^32 must reset the window rather than be
 * truncated into a small in-window delta (which would keep stale bits and make
 * a never-seen counter look like a replay).
 */
ZTEST(wireguard_replay, test_forward_jump_over_2_32)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;
	uint64_t base = 3;

	zassert_true(wg_replay_check(&bitmap, &counter, base));

	zassert_true(wg_replay_check(&bitmap, &counter, base + (1ULL << 32) + 5));
	zassert_equal(bitmap, 1);

	/* Never seen before, so must be accepted (a truncated diff would treat
	 * it as already seen via a stale bit).
	 */
	zassert_true(wg_replay_check(&bitmap, &counter, base + (1ULL << 32)));
}

/* A counter roughly 2^32 behind the window must be rejected as too old, not
 * truncated into an in-window delta and accepted.
 */
ZTEST(wireguard_replay, test_backward_gap_over_2_32)
{
	uint32_t bitmap = 0;
	uint64_t counter = 0;

	zassert_true(wg_replay_check(&bitmap, &counter, (1ULL << 32) + 100));
	zassert_false(wg_replay_check(&bitmap, &counter, 95));
}

ZTEST_SUITE(wireguard_replay, NULL, NULL, NULL, NULL, NULL);
