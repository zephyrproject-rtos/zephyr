/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "intc_gic_cpu_targets.h"

ZTEST(gic_cpu_targets, test_zero_affinity_after_unregistered_slot)
{
	struct gic_cpu_target targets[3] = {0};

	gic_cpu_target_register(&targets[0], 0x100U, BIT(3));
	gic_cpu_target_register(&targets[2], 0U, BIT(6));

	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 0U), BIT(6));
}

ZTEST(gic_cpu_targets, test_non_linear_affinities)
{
	struct gic_cpu_target targets[3] = {0};

	gic_cpu_target_register(&targets[0], 0U, BIT(3));
	gic_cpu_target_register(&targets[1], 0x101U, BIT(0));
	gic_cpu_target_register(&targets[2], 0x202U, BIT(6));

	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 0U), BIT(3));
	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 0x101U), BIT(0));
	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 0x202U), BIT(6));
	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 1U), 0U);
}

ZTEST(gic_cpu_targets, test_unpublished_affinity)
{
	struct gic_cpu_target targets[2] = {0};

	/* A writer interrupted before publishing its mask must not block readers. */
	targets[0].mpidr = 0x101U;
	gic_cpu_target_register(&targets[1], 0x202U, BIT(5));

	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 0x101U), 0U);
	zassert_equal(gic_cpu_target_mask(targets, ARRAY_SIZE(targets), 0x202U), BIT(5));
}

ZTEST(gic_cpu_targets, test_published_mapping_is_immutable)
{
	struct gic_cpu_target target = {0};

	gic_cpu_target_register(&target, 0x101U, BIT(4));
	gic_cpu_target_register(&target, 0x202U, BIT(1));

	zassert_equal(gic_cpu_target_mask(&target, 1U, 0x101U), BIT(4));
	zassert_equal(gic_cpu_target_mask(&target, 1U, 0x202U), 0U);
}

ZTEST(gic_cpu_targets, test_zero_mask_is_not_published)
{
	struct gic_cpu_target target = {0};

	gic_cpu_target_register(&target, 0x101U, 0U);
	zassert_equal(gic_cpu_target_mask(&target, 1U, 0x101U), 0U);

	gic_cpu_target_register(&target, 0x101U, BIT(2));
	zassert_equal(gic_cpu_target_mask(&target, 1U, 0x101U), BIT(2));
}

ZTEST_SUITE(gic_cpu_targets, NULL, NULL, NULL, NULL, NULL);
