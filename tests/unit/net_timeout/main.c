/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <inttypes.h>
#include <zephyr/net/net_timeout.h>

#include "../../../subsys/net/ip/net_timeout.c"

#define HALFMAX_S (23U + 60U * (31U + 60U * (20U + 24U * 24U)))
#define NTO_MAX_S HALFMAX_S
#define FULLMAX_S (47U + 60U * (2U + 60U * (17U + 24U * 49U)))

#if 0
static void dump_nto(const struct net_timeout *nto)
{
	uint64_t remaining = nto->timer_timeout;
	uint64_t deadline = nto->timer_start;

	remaining += NET_TIMEOUT_MAX_VALUE * nto->wrap_counter;
	deadline += remaining;

	printk("start %u, rem %u * %u + %u = %" PRIu64 ", ends %" PRIu64 "\n",
	       nto->timer_start, nto->wrap_counter,
	       NET_TIMEOUT_MAX_VALUE, nto->timer_timeout,
	       remaining, deadline);
}
#endif

ZTEST(net_timeout, test_basics)
{
	zassert_equal(NET_TIMEOUT_MAX_VALUE, INT32_MAX,
		      "Max value not as expected");
	zassert_equal(((uint32_t)(INT32_MAX / MSEC_PER_SEC)),
		      HALFMAX_S,
		      "Half-max constant is wrong");
	zassert_equal((UINT32_MAX / MSEC_PER_SEC),
		      FULLMAX_S,
		      "Full-max constant is wrong");
}

ZTEST(net_timeout, test_set)
{
	struct net_timeout nto;
	uint32_t now = 4;
	uint32_t lifetime = 0U;

	/* Zero is a special case. */
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, 0, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 0);
	zassert_equal(nto.timer_timeout, 0);

	/* Less than the max is straightforward. */
	lifetime = NTO_MAX_S / 2;
	++now;
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 0);
	zassert_equal(nto.timer_timeout, lifetime * MSEC_PER_SEC,
		      NULL);

	/* Max must not incur wrap, so fraction is not zero. */
	lifetime = NTO_MAX_S;
	++now;
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 0);
	zassert_equal(nto.timer_timeout, lifetime * MSEC_PER_SEC,
		      NULL);

	/* Next after max does wrap. */
	lifetime += 1U;
	++now;
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 1U);
	zassert_equal(nto.timer_timeout,
		      (lifetime * MSEC_PER_SEC) % NET_TIMEOUT_MAX_VALUE,
		      NULL);

	/* Fullmax should be one plus partial */
	lifetime = FULLMAX_S;
	++now;
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 1U);
	zassert_equal(nto.timer_timeout,
		      (lifetime * (uint64_t)MSEC_PER_SEC) % NET_TIMEOUT_MAX_VALUE,
		      NULL);

	/* Multiples of max must also not have a zero fraction. */
	lifetime = NET_TIMEOUT_MAX_VALUE;
	++now;
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, MSEC_PER_SEC - 1);
	zassert_equal(nto.timer_timeout, NET_TIMEOUT_MAX_VALUE);
}

ZTEST(net_timeout, test_deadline)
{
	struct net_timeout nto;
	uint64_t now = 1234;
	uint64_t rollover31 = BIT64(31);
	uint64_t rollover32 = BIT64(32);
	uint32_t lifetime = 562;

	net_timeout_set(&nto, lifetime, now);
	uint64_t expected = now + lifetime * MSEC_PER_SEC;
	zassert_equal(net_timeout_deadline(&nto, now),
		      expected,
		      NULL);

	/* Advancing now has no effect until it wraps. */
	zassert_equal(net_timeout_deadline(&nto, now + 23U),
		      expected,
		      NULL);

	/* Advancing by 2^31 is not a wrap. */
	now += rollover31;
	zassert_equal(net_timeout_deadline(&nto, now),
		      expected,
		      NULL);

	/* Advancing by 2^32 is a wrap, and should be reflected in the
	 * returned deadline
	 */
	now += rollover31;
	expected += rollover32;
	zassert_equal(net_timeout_deadline(&nto, now),
		      expected,
		      NULL);

	zassert_equal(net_timeout_deadline(&nto, now + 52),
		      expected,
		      NULL);
}

ZTEST(net_timeout, test_remaining)
{
	struct net_timeout nto;
	uint32_t now = 4;
	uint32_t lifetime = 0U;

	/* Zero is a special case. */
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, 0, now);
	zassert_equal(net_timeout_remaining(&nto, now), 0U,
		      NULL);

	/* Without wrap is straightforward. */
	lifetime = NTO_MAX_S / 2;
	memset(&nto, 0xa5, sizeof(nto));
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.wrap_counter, 0);
	zassert_equal(net_timeout_remaining(&nto, now), lifetime,
		      NULL);

	/* Estimate rounds down (legacy behavior). */
	zassert_equal(net_timeout_remaining(&nto, now + 1U),
		      lifetime - 1U,
		      NULL);
	zassert_equal(net_timeout_remaining(&nto, now + MSEC_PER_SEC - 1U),
		      lifetime - 1U,
		      NULL);
	zassert_equal(net_timeout_remaining(&nto, now + MSEC_PER_SEC),
		      lifetime - 1U,
		      NULL);
	zassert_equal(net_timeout_remaining(&nto, now + MSEC_PER_SEC + 1U),
		      lifetime - 2U,
		      NULL);

	/* Works when wrap is involved */
	lifetime = 4 * FULLMAX_S;
	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.wrap_counter, 7);
	zassert_equal(net_timeout_remaining(&nto, now), lifetime,
		      NULL);
}

ZTEST(net_timeout, test_evaluate_basic)
{
	struct net_timeout nto;
	uint64_t now = 0;
	uint32_t half_max = NET_TIMEOUT_MAX_VALUE / 2U;
	uint32_t lifetime = FULLMAX_S + HALFMAX_S;
	uint32_t remainder;
	uint32_t delay;
	uint64_t deadline;

	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now,
		      NULL);
	zassert_equal(nto.wrap_counter, 2,
		      NULL);
	remainder = 2147482706;
	zassert_equal(nto.timer_timeout, remainder,
		      NULL);
	deadline = net_timeout_deadline(&nto, now);

	/* Evaluation with wrap and no advance returns max value
	 * without changing anything. */
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(delay, NET_TIMEOUT_MAX_VALUE,
		       NULL);
	zassert_equal(nto.timer_start, now,
		      NULL);
	zassert_equal(nto.wrap_counter, 2,
		      NULL);
	zassert_equal(nto.timer_timeout, remainder,
		      NULL);
	zassert_equal(net_timeout_deadline(&nto, now), deadline,
		     NULL);

	/* Advance now by half the delay should return the remainder,
	 * again without advancing anything.
	 */
	delay = net_timeout_evaluate(&nto, now + half_max);
	zassert_equal(delay, NET_TIMEOUT_MAX_VALUE - half_max,
		       NULL);
	zassert_equal(nto.timer_start, now,
		      NULL);
	zassert_equal(nto.wrap_counter, 2,
		      NULL);
	zassert_equal(nto.timer_timeout, remainder,
		      NULL);
	zassert_equal(net_timeout_deadline(&nto, now), deadline,
		     NULL);

	/* Advance now by just below delay still doesn't change
	 * anything.
	 */
	delay = net_timeout_evaluate(&nto, now + NET_TIMEOUT_MAX_VALUE - 1U);
	zassert_equal(delay, 1U,
		       NULL);
	zassert_equal(nto.timer_start, now,
		      NULL);
	zassert_equal(nto.wrap_counter, 2,
		      NULL);
	zassert_equal(nto.timer_timeout, remainder,
		      NULL);
	zassert_equal(net_timeout_deadline(&nto, now), deadline,
		     NULL);

	/* Advancing by the delay consumes the value of the delay. The
	 * deadline is unchanged.
	 */
	now += NET_TIMEOUT_MAX_VALUE;
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(delay, NET_TIMEOUT_MAX_VALUE,
		      NULL);
	zassert_equal(nto.timer_start, now,
		      NULL);
	zassert_equal(nto.wrap_counter, 1,
		      NULL);
	zassert_equal(nto.timer_timeout, remainder,
		      "remainder %u", nto.timer_timeout);
	zassert_equal(net_timeout_deadline(&nto, now), deadline,
		     NULL);

	/* Advancing by more than the delay consumes the value of the delay,
	 * with the excess reducing the remainder.  The deadline is
	 * unchanged.
	 */
	now += NET_TIMEOUT_MAX_VALUE + 1234;
	remainder -= 1234;
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(delay, remainder,
		      NULL);
	zassert_equal(nto.timer_start, (uint32_t)now,
		      NULL);
	zassert_equal(nto.wrap_counter, 0,
		      NULL);
	zassert_equal(nto.timer_timeout, remainder,
		      NULL);
	zassert_equal(net_timeout_deadline(&nto, now), deadline,
		     NULL);

	/* Final advance completes the timeout precisely */
	now += delay;
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(delay, 0,
		      NULL);
	zassert_equal(net_timeout_deadline(&nto, now), deadline,
		     NULL);
}

ZTEST(net_timeout, test_evaluate_whitebox)
{
	/* This explicitly tests the path where subtracting the excess elapsed
	 * from the fractional timeout requires reducing the wrap count a
	 * second time.
	 */
	struct net_timeout nto;
	uint64_t now = 0;
	uint32_t lifetime = 3 * HALFMAX_S + 2;

	net_timeout_set(&nto, lifetime, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 3);
	zassert_equal(nto.timer_timeout, 59);

	/* Preserve the deadline for validation */
	uint64_t deadline = net_timeout_deadline(&nto, now);

	uint32_t delay = net_timeout_evaluate(&nto, now);
	zassert_equal(delay, NET_TIMEOUT_MAX_VALUE);
	zassert_equal(net_timeout_deadline(&nto, now),
		      deadline, NULL);

	/* Simulate a late evaluation, far enough late that the counter gets
	 * wiped out.
	 */
	now += delay + 100U;
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(nto.timer_start, now);
	zassert_equal(nto.wrap_counter, 1);
	zassert_equal(nto.timer_timeout, 2147483606);
	zassert_equal(net_timeout_deadline(&nto, now),
		      deadline, NULL);
	zassert_equal(delay, NET_TIMEOUT_MAX_VALUE);

	/* Another late evaluation finishes the wrap leaving some extra.
	 */
	now += delay + 123U;
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(nto.timer_start, (uint32_t)now);
	zassert_equal(nto.wrap_counter, 0);
	zassert_equal(nto.timer_timeout, 2147483483);
	zassert_equal(net_timeout_deadline(&nto, now),
		      deadline, NULL);
	zassert_equal(delay, nto.timer_timeout);

	/* Complete the timeout.  This does *not* adjust the internal
	 * state.
	 */
	now += delay + 234U;
	delay = net_timeout_evaluate(&nto, now);
	zassert_equal(delay, 0);
	zassert_equal(net_timeout_deadline(&nto, now),
		      deadline, NULL);
}

ZTEST(net_timeout, test_nop)
{
}

ZTEST_SUITE(net_timeout, NULL, NULL, NULL, NULL, NULL);
