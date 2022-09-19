/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "common.h"

static void test(void)
{
}

static bool predicate(const void *state)
{
	const struct global_test_state *s = state;

	return s->phase == PHASE_STEPS_0;
}

ztest_register_test_suite(test_step_0, predicate, ztest_unit_test(test));
