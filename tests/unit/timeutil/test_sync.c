/*
 * Copyright 2020 Peter Bigot Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests for the time_sync data structures */

#include <string.h>
#include <ztest.h>
#include "timeutil_test.h"

static const struct timeutil_sync_config cfg1 = {
	.ref_Hz = USEC_PER_SEC,
	.local_Hz = 32768,
};

static const struct timeutil_sync_config cfg2 = {
	.ref_Hz = NSEC_PER_SEC,
	.local_Hz = 100,
};

static inline uint64_t scale_ref(uint32_t factor,
			      const struct timeutil_sync_config *cfg)
{
	return (uint64_t)factor * (uint64_t)cfg->ref_Hz;
}

static inline uint64_t scale_local(uint32_t factor,
				   const struct timeutil_sync_config *cfg)
{
	return (uint64_t)factor * (uint64_t)cfg->local_Hz;
}

static inline int64_t scale_local_signed(int32_t factor,
					 const struct timeutil_sync_config *cfg)
{
	return (int64_t)factor * (int64_t)cfg->local_Hz;
}


static void test_state_update(void)
{
	struct timeutil_sync_instant si = { 0 };
	struct timeutil_sync_state ss = { 0 };
	int rv = timeutil_sync_state_update(&ss, &si);

	zassert_equal(rv, -EINVAL,
		      "invalid init got: %d", rv);
	zassert_equal(ss.base.ref, 0,
		      "unexpected base ref");
	zassert_equal(ss.skew, 0,
		      "unexpected skew");

	si.ref = 1;
	rv = timeutil_sync_state_update(&ss, &si);
	zassert_equal(rv, 0,
		      "valid first init got: %d", rv);
	zassert_equal(ss.base.ref, 1,
		      "base not updated");
	zassert_equal(ss.latest.ref, 0,
		      "unexpected latest ref");
	zassert_equal(ss.skew, 1.0,
		      "unexpected skew");

	rv = timeutil_sync_state_update(&ss, &si);
	zassert_equal(rv, -EINVAL,
		      "non-increasing ref got: %d", rv);
	zassert_equal(ss.base.ref, 1,
		      "unexpected base ref");
	zassert_equal(ss.base.local, 0,
		      "unexpected base local");
	zassert_equal(ss.latest.ref, 0,
		      "unexpected latest ref");

	si.ref += 1;
	rv = timeutil_sync_state_update(&ss, &si);
	zassert_equal(rv, -EINVAL,
		      "non-increasing local got: %d", rv);
	zassert_equal(ss.latest.ref, 0,
		      "unexpected latest ref");

	si.local += 20;
	rv = timeutil_sync_state_update(&ss, &si);
	zassert_equal(rv, 1,
		      "increasing got: %d", rv);
	zassert_equal(ss.base.ref, 1,
		      "unexpected base ref");
	zassert_equal(ss.base.local, 0,
		      "unexpected base local");
	zassert_equal(ss.latest.ref, si.ref,
		      "unexpected latest ref");
	zassert_equal(ss.latest.local, si.local,
		      "unexpected latest local");
}

static void test_state_set_skew(void)
{
	struct timeutil_sync_instant si = {
		.ref = 1,
	};
	struct timeutil_sync_state ss = {
		.cfg = &cfg1,
	};
	float skew = 0.99;
	int rv = timeutil_sync_state_update(&ss, &si);

	zassert_equal(rv, 0,
		      "valid first init got: %d", rv);
	zassert_equal(ss.skew, 1.0,
		      "unexpected skew");

	rv = timeutil_sync_state_set_skew(&ss, -1.0, NULL);
	zassert_equal(rv, -EINVAL,
		      "negative skew set got: %d", rv);
	zassert_equal(ss.skew, 1.0,
		      "unexpected skew");

	rv = timeutil_sync_state_set_skew(&ss, 0.0, NULL);
	zassert_equal(rv, -EINVAL,
		      "zero skew set got: %d", rv);
	zassert_equal(ss.skew, 1.0,
		      "unexpected skew");

	rv = timeutil_sync_state_set_skew(&ss, skew, NULL);
	zassert_equal(rv, 0,
		      "valid skew set got: %d", rv);
	zassert_equal(ss.skew, skew,
		      "unexpected skew");
	zassert_equal(ss.base.ref, si.ref,
		      "unexpected base ref");
	zassert_equal(ss.base.local, si.local,
		      "unexpected base ref");

	skew = 1.01;
	si.ref += 5;
	si.local += 3;

	rv = timeutil_sync_state_set_skew(&ss, skew, &si);
	zassert_equal(rv, 0,
		      "valid skew set got: %d", rv);
	zassert_equal(ss.skew, skew,
		      "unexpected skew");
	zassert_equal(ss.base.ref, si.ref,
		      "unexpected base ref");
	zassert_equal(ss.base.local, si.local,
		      "unexpected base ref");
	zassert_equal(ss.latest.ref, 0,
		      "uncleared latest ref");
	zassert_equal(ss.latest.local, 0,
		      "uncleared latest local");
}

static void test_estimate_skew(void)
{
	struct timeutil_sync_state ss = {
		.cfg = &cfg1,
	};
	struct timeutil_sync_instant si0 = {
		.ref = cfg1.ref_Hz,
	};
	struct timeutil_sync_instant si1 = {
		.ref = si0.ref + cfg1.ref_Hz,
		.local = si0.local + cfg1.local_Hz,
	};
	float skew = 0.0;

	skew = timeutil_sync_estimate_skew(&ss);
	zassert_equal(skew, 0,
		      "unexpected uninit skew: %f", skew);

	int rv = timeutil_sync_state_update(&ss, &si0);

	zassert_equal(rv, 0,
		      "valid init got: %d", rv);

	skew = timeutil_sync_estimate_skew(&ss);
	zassert_equal(skew, 0,
		      "unexpected base-only skew: %f", skew);

	rv = timeutil_sync_state_update(&ss, &si1);
	zassert_equal(rv, 1,
		      "valid update got: %d", rv);

	zassert_equal(ss.base.ref, si0.ref,
		      "unexpected base ref");
	zassert_equal(ss.base.local, si0.local,
		      "unexpected base local");
	zassert_equal(ss.latest.ref, si1.ref,
		      "unexpected latest ref");
	zassert_equal(ss.latest.local, si1.local,
		      "unexpected latest local");

	skew = timeutil_sync_estimate_skew(&ss);
	zassert_equal(skew, 1.0,
		      "unexpected linear skew: %f", skew);

	/* Local advanced half as far as it should: scale by 2 to
	 * correct.
	 */
	ss.latest.local = scale_local(1, ss.cfg) / 2;
	skew = timeutil_sync_estimate_skew(&ss);
	zassert_equal(skew, 2.0,
		      "unexpected half skew: %f", skew);

	/* Local advanced twice as far as it should: scale by 1/2 to
	 * correct.
	 */
	ss.latest.local = scale_local(2, ss.cfg);
	skew = timeutil_sync_estimate_skew(&ss);
	zassert_equal(skew, 0.5,
		      "unexpected double skew: %f", skew);
}

static void tref_from_local(const char *tag,
			    const struct timeutil_sync_config *cfg)
{
	struct timeutil_sync_state ss = {
		.cfg = cfg,
	};
	struct timeutil_sync_instant si0 = {
		/* Absolute local 0 is 5 s ref */
		.ref = scale_ref(10, cfg),
		.local = scale_local(5, cfg),
	};
	uint64_t ref = 0;
	int rv = timeutil_sync_ref_from_local(&ss, 0, &ref);

	zassert_equal(rv, -EINVAL,
		      "%s: unexpected uninit convert: %d", tag, rv);

	rv = timeutil_sync_state_update(&ss, &si0);
	zassert_equal(rv, 0,
		      "%s: unexpected init: %d", tag, rv);
	zassert_equal(ss.skew, 1.0,
		      "%s: unexpected skew");

	rv = timeutil_sync_ref_from_local(&ss, ss.base.local, NULL);
	zassert_equal(rv, -EINVAL,
		      "%s: unexpected missing dest: %d", tag, rv);

	rv = timeutil_sync_ref_from_local(&ss, ss.base.local, &ref);
	zassert_equal(rv, 0,
		      "%s: unexpected fail", tag, rv);
	zassert_equal(ref, ss.base.ref,
		      "%s: unexpected base convert", tag);

	rv = timeutil_sync_ref_from_local(&ss, 0, &ref);
	zassert_equal(rv, 0,
		      "%s: unexpected local=0 fail", tag, rv);
	zassert_equal(ref, scale_ref(5, cfg),
		      "%s: unexpected local=0 ref", tag);

	rv = timeutil_sync_ref_from_local(&ss, ss.base.local, &ref);
	zassert_equal(rv, 0,
		      "%s: unexpected local=base fail", tag, rv);
	zassert_equal(ref, ss.base.ref,
		      "%s: unexpected local=base ref", tag);

	rv = timeutil_sync_ref_from_local(&ss, ss.base.local
					  + scale_local(2, cfg), &ref);
	zassert_equal(rv, 0,
		      "%s: unexpected local=base+2s fail %d", tag, rv);
	zassert_equal(ref, ss.base.ref + scale_ref(2, cfg),
		      "%s: unexpected local=base+2s ref", tag);

	rv = timeutil_sync_ref_from_local(&ss, (int64_t)ss.base.local
					  - scale_local(12, cfg), &ref);
	zassert_equal(rv, -ERANGE,
		      "%s: unexpected local=base-12s res %u", tag, rv);

	/* Skew of 0.5 means local runs at double speed */
	rv = timeutil_sync_state_set_skew(&ss, 0.5, NULL);
	zassert_equal(rv, 0,
		      "%s: failed set skew", tag);

	/* Local at double speed corresponds to half advance in ref */
	rv = timeutil_sync_ref_from_local(&ss, ss.base.local
					  + scale_local(2, cfg), &ref);
	zassert_equal(rv, 1,
		      "%s: unexpected skew adj fail", tag);
	zassert_equal(ref, ss.base.ref + cfg->ref_Hz,
		      "%s: unexpected skew adj convert", tag);
}

static void test_ref_from_local(void)
{
	tref_from_local("std", &cfg1);
	tref_from_local("ext", &cfg2);
}

static void tlocal_from_ref(const char *tag,
			    const struct timeutil_sync_config *cfg)
{
	struct timeutil_sync_state ss = {
		.cfg = cfg,
	};
	struct timeutil_sync_instant si0 = {
		/* Absolute local 0 is 5 s ref */
		.ref = scale_ref(10, cfg),
		.local = scale_local(5, cfg),
	};
	int64_t local = 0;
	int rv = timeutil_sync_local_from_ref(&ss, 0, &local);

	zassert_equal(rv, -EINVAL,
		      "%s: unexpected uninit convert: %d", tag, rv);

	rv = timeutil_sync_state_update(&ss, &si0);
	zassert_equal(rv, 0,
		      "%s: unexpected init: %d", tag, rv);
	zassert_equal(ss.skew, 1.0,
		      "%s: unexpected skew", tag);

	rv = timeutil_sync_local_from_ref(&ss, ss.base.ref, NULL);
	zassert_equal(rv, -EINVAL,
		      "%s: unexpected missing dest", tag, rv);

	rv = timeutil_sync_local_from_ref(&ss, ss.base.ref, &local);
	zassert_equal(rv, 0,
		      "%s: unexpected fail", tag, rv);
	zassert_equal(local, ss.base.local,
		      "%s: unexpected base convert", tag);

	rv = timeutil_sync_local_from_ref(&ss, ss.base.ref
					  + scale_ref(2, cfg), &local);
	zassert_equal(rv, 0,
		      "%s: unexpected base+2s fail", tag);
	zassert_equal(local, ss.base.local + scale_local(2, cfg),
		      "%s: unexpected base+2s convert", tag);

	rv = timeutil_sync_local_from_ref(&ss, ss.base.ref
					  - scale_ref(7, cfg), &local);
	zassert_equal(rv, 0,
		      "%s: unexpected base-7s fail", tag);
	zassert_equal(local, scale_local_signed(-2, cfg),
		      "%s: unexpected base-7s convert", tag);


	/* Skew of 0.5 means local runs at double speed */
	rv = timeutil_sync_state_set_skew(&ss, 0.5, NULL);
	zassert_equal(rv, 0,
		      "%s: failed set skew", tag);

	/* Local at double speed corresponds to half advance in ref */
	rv = timeutil_sync_local_from_ref(&ss, ss.base.ref
					  + scale_ref(1, cfg) / 2, &local);
	zassert_equal(rv, 1,
		      "%s: unexpected skew adj fail", tag);
	zassert_equal(local, ss.base.local + scale_local(1, cfg),
		      "%s: unexpected skew adj convert", tag);
}

static void test_local_from_ref(void)
{
	tlocal_from_ref("std", &cfg1);
	tlocal_from_ref("ext", &cfg2);
}

static void test_skew_to_ppb(void)
{
	float skew = 1.0;
	int32_t ppb = timeutil_sync_skew_to_ppb(skew);

	zassert_equal(ppb, 0,
		      "unexpected perfect: %d", ppb);

	skew = 0.999976;
	ppb = timeutil_sync_skew_to_ppb(skew);
	zassert_equal(ppb, 24020,
		      "unexpected fast: %d", ppb);

	skew = 1.000022;
	ppb = timeutil_sync_skew_to_ppb(skew);
	zassert_equal(ppb, -22053,
		      "unexpected slow: %d", ppb);

	skew = 3.147483587;
	ppb = timeutil_sync_skew_to_ppb(skew);
	zassert_equal(ppb, -2147483587,
		      "unexpected near limit: %.10g %d", skew, ppb);
	skew = 3.147483826;
	ppb = timeutil_sync_skew_to_ppb(skew);
	zassert_equal(ppb, INT32_MIN,
		      "unexpected above limit: %.10g %d", skew, ppb);
}

void test_sync(void)
{
	test_state_update();
	test_state_set_skew();
	test_estimate_skew();
	test_ref_from_local();
	test_local_from_ref();
	test_skew_to_ppb();
}
