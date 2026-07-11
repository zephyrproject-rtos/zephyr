/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests build-time anchored init ordering (SYS_INIT_ANCHORED() in
 * <zephyr/init.h>): services are run at boot in the dependency order encoded
 * in their anchor keys, regardless of source/link order. Ordering is resolved
 * entirely by the linker's section sort; there is no runtime dependency
 * tracking.
 *
 * Dependency chain (arrows = "depends on"):
 *
 *   PRE_KERNEL:   charlie -> bravo -> alpha
 *   POST_KERNEL:  delta   -> charlie   (cross-level; charlie already ran)
 *
 * The services are declared in an order that does not match their run order,
 * so only the anchor keys can produce the expected sequence. A plain numeric
 * SYS_INIT() at the highest priority of the same level is also registered to
 * verify that anchored entries run after every numeric-priority entry.
 */

#include <zephyr/ztest.h>
#include <zephyr/init.h>

#define MAX_ENTRIES 8

static const char *run_order[MAX_ENTRIES];
static int run_count;

static void record(const char *name)
{
	if (run_count < MAX_ENTRIES) {
		run_order[run_count++] = name;
	}
}

/* Anchor keys. Conventionally each service defines its own key next to its
 * API (in its header); they are grouped here for the test.
 */
#define SYS_ANCHOR_alpha   SYS_ANCHOR(alpha)
#define SYS_ANCHOR_bravo   SYS_ANCHOR_AFTER(SYS_ANCHOR_alpha, bravo)
#define SYS_ANCHOR_charlie SYS_ANCHOR_AFTER(SYS_ANCHOR_bravo, charlie)
#define SYS_ANCHOR_delta   SYS_ANCHOR_AFTER(SYS_ANCHOR_charlie, delta)

/* Declared charlie -> bravo -> alpha (reverse of run order) on purpose. */
static int charlie_init(void)
{
	record("charlie");
	return 0;
}
SYS_INIT_ANCHORED(charlie, charlie_init, PRE_KERNEL);

static int bravo_init(void)
{
	record("bravo");
	return 0;
}
SYS_INIT_ANCHORED(bravo, bravo_init, PRE_KERNEL);

static int alpha_init(void)
{
	record("alpha");
	return 0;
}
SYS_INIT_ANCHORED(alpha, alpha_init, PRE_KERNEL);

/* Cross-level: a POST_KERNEL service depending on a PRE_KERNEL one. */
static int delta_init(void)
{
	record("delta");
	return 0;
}
SYS_INIT_ANCHORED(delta, delta_init, POST_KERNEL);

/* Conditional dependency with the condition enabled: orders after delta. */
#define SYS_ANCHOR_echo SYS_ANCHOR_AFTER_IF(CONFIG_ZTEST, SYS_ANCHOR_delta, echo)
static int echo_init(void)
{
	record("echo");
	return 0;
}
SYS_INIT_ANCHORED(echo, echo_init, POST_KERNEL);

/* Conditional dependency with the condition disabled: no dependency, the
 * entry behaves like a dependency-free anchored service.
 */
#define SYS_ANCHOR_foxtrot                                                     \
	SYS_ANCHOR_AFTER_IF(CONFIG_THIS_OPTION_DOES_NOT_EXIST, SYS_ANCHOR_delta, foxtrot)
static int foxtrot_init(void)
{
	record("foxtrot");
	return 0;
}
SYS_INIT_ANCHORED(foxtrot, foxtrot_init, POST_KERNEL);

/* Numeric-priority entry at the lowest possible priority of the same level:
 * must still run before every anchored entry of the level.
 */
static int fixed_init(void)
{
	record("fixed");
	return 0;
}
SYS_INIT(fixed_init, PRE_KERNEL, 999);

ZTEST(service, test_anchored_ordering)
{
	static const char *const expected[] = {
		"fixed", "alpha", "bravo", "charlie", "delta", "echo", "foxtrot",
	};

	zassert_equal(run_count, ARRAY_SIZE(expected),
		      "expected %zu init calls, got %d",
		      ARRAY_SIZE(expected), run_count);

	for (int i = 0; i < run_count; i++) {
		zassert_str_equal(run_order[i], expected[i],
				  "entry %d ran out of order: %s (expected %s)",
				  i, run_order[i], expected[i]);
	}
}

ZTEST_SUITE(service, NULL, NULL, NULL, NULL, NULL);
