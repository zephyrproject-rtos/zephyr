/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/power/qos.h>
#include <zephyr/ztest.h>

static const struct pstate low_pstate = {
	.frequency_hz = 12000000U,
};

static const struct pstate mid_pstate = {
	.frequency_hz = 48000000U,
};

static const struct pstate high_pstate = {
	.frequency_hz = 96000000U,
};

ZTEST(power_qos, test_empty_constraints)
{
	struct power_qos_pstate_constraints constraints;

	power_qos_pstate_constraints_get(&constraints);

	zassert_is_null(constraints.min_pstate);
	zassert_is_null(constraints.max_pstate);
	zassert_false(constraints.conflict);
	zassert_true(power_qos_pstate_is_allowed(&low_pstate, &constraints));
	zassert_true(power_qos_pstate_is_allowed(&high_pstate, &constraints));
}

ZTEST(power_qos, test_min_max_aggregation)
{
	struct power_qos_pstate_request min_low;
	struct power_qos_pstate_request min_mid;
	struct power_qos_pstate_request max_high;
	struct power_qos_pstate_request max_mid;
	struct power_qos_pstate_constraints constraints;

	power_qos_min_pstate_request_add(&min_low, &low_pstate);
	power_qos_min_pstate_request_add(&min_mid, &mid_pstate);
	power_qos_max_pstate_request_add(&max_high, &high_pstate);
	power_qos_max_pstate_request_add(&max_mid, &mid_pstate);

	power_qos_pstate_constraints_get(&constraints);

	zassert_equal(constraints.min_pstate, &mid_pstate);
	zassert_equal(constraints.max_pstate, &mid_pstate);
	zassert_false(constraints.conflict);
	zassert_false(power_qos_pstate_is_allowed(&low_pstate, &constraints));
	zassert_true(power_qos_pstate_is_allowed(&mid_pstate, &constraints));
	zassert_false(power_qos_pstate_is_allowed(&high_pstate, &constraints));

	power_qos_min_pstate_request_remove(&min_low);
	power_qos_min_pstate_request_remove(&min_mid);
	power_qos_max_pstate_request_remove(&max_high);
	power_qos_max_pstate_request_remove(&max_mid);
}

ZTEST(power_qos, test_update_and_remove)
{
	struct power_qos_pstate_request min_req;
	struct power_qos_pstate_constraints constraints;

	power_qos_min_pstate_request_add(&min_req, &high_pstate);
	power_qos_pstate_constraints_get(&constraints);
	zassert_equal(constraints.min_pstate, &high_pstate);

	power_qos_min_pstate_request_update(&min_req, &low_pstate);
	power_qos_pstate_constraints_get(&constraints);
	zassert_equal(constraints.min_pstate, &low_pstate);
	zassert_true(power_qos_pstate_is_allowed(&mid_pstate, &constraints));

	power_qos_min_pstate_request_remove(&min_req);
	power_qos_pstate_constraints_get(&constraints);
	zassert_is_null(constraints.min_pstate);
}

ZTEST(power_qos, test_conflict)
{
	struct power_qos_pstate_request min_req;
	struct power_qos_pstate_request max_req;
	struct power_qos_pstate_constraints constraints;

	power_qos_min_pstate_request_add(&min_req, &high_pstate);
	power_qos_max_pstate_request_add(&max_req, &mid_pstate);
	power_qos_pstate_constraints_get(&constraints);

	zassert_true(constraints.conflict);
	zassert_false(power_qos_pstate_is_allowed(&low_pstate, &constraints));
	zassert_false(power_qos_pstate_is_allowed(&mid_pstate, &constraints));
	zassert_false(power_qos_pstate_is_allowed(&high_pstate, &constraints));

	power_qos_min_pstate_request_remove(&min_req);
	power_qos_max_pstate_request_remove(&max_req);
}

ZTEST_SUITE(power_qos, NULL, NULL, NULL, NULL, NULL);
