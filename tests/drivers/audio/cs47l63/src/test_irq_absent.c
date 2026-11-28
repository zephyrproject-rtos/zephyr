/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "polled_only.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_noirq))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_noirq))

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	polled_setup(CODEC, EMUL);
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	polled_teardown(CODEC);
}

ZTEST_SUITE(cs47l63_noirq, NULL, NULL, before_each, after_each, NULL);

static ZTEST(cs47l63_noirq, test_configure_leaves_the_interrupt_masks_alone)
{
	polled_check_masks_untouched(EMUL);
}

static ZTEST(cs47l63_noirq, test_a_latch_is_reported_by_the_poll_and_never_by_a_service)
{
	polled_check_reported_by_the_poll(CODEC, EMUL);
}
