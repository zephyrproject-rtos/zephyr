/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/smf.h>
#include "test_instrumentation_common.h"

/*
 * Hierarchical 5-ancestor test:
 *
 * Hierarchy: P05 > P04 > P03 > P02 > P01 > {A, B}, C (root), D (root)
 *
 * set_initial(A): P05_ENTRY .. P01_ENTRY, A_ENTRY (not captured — hooks
 * installed after init)
 *
 * Run 0: A_RUN (transitions to B, sibling under P01)
 *         -> A_EXIT, transition(A,B), B_ENTRY
 * Run 1: B_RUN (propagate), P01_RUN..P05_RUN (P05 transitions to C)
 *         -> B_EXIT, P01_EXIT..P05_EXIT, transition(B,C), C_ENTRY
 * Run 2: C_RUN (transitions to D)
 *         -> C_EXIT, transition(C,D), D_ENTRY
 */

#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN 3

/* Forward declaration of test_states */
static const struct smf_state test_states[];

enum test_state {
	P05,
	P04,
	P03,
	P02,
	P01,
	A,
	B,
	C,
	D,
};

static struct test_object {
	struct smf_ctx ctx;
} test_obj;

/* Parent handlers — all propagate, P05 transitions to C */
static void p05_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result p05_run(void *obj)
{
	smf_set_state(SMF_CTX(obj), &test_states[C]);
	return SMF_EVENT_PROPAGATE;
}
static void p05_exit(void *obj)
{
	(void)obj;
}

static void p04_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result p04_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}
static void p04_exit(void *obj)
{
	(void)obj;
}

static void p03_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result p03_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}
static void p03_exit(void *obj)
{
	(void)obj;
}

static void p02_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result p02_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}
static void p02_exit(void *obj)
{
	(void)obj;
}

static void p01_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result p01_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}
static void p01_exit(void *obj)
{
	(void)obj;
}

/* Leaf state A — transitions to sibling B */
static void a_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result a_run(void *obj)
{
	smf_set_state(SMF_CTX(obj), &test_states[B]);
	return SMF_EVENT_PROPAGATE;
}
static void a_exit(void *obj)
{
	(void)obj;
}

/* Leaf state B — propagates (ancestors handle) */
static void b_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result b_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}
static void b_exit(void *obj)
{
	(void)obj;
}

/* Root state C — transitions to D */
static void c_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result c_run(void *obj)
{
	smf_set_state(SMF_CTX(obj), &test_states[D]);
	return SMF_EVENT_PROPAGATE;
}
static void c_exit(void *obj)
{
	(void)obj;
}

/* Root state D — terminal, has entry only */
static void d_entry(void *obj)
{
	(void)obj;
}

/* clang-format off */
static const struct smf_state test_states[] = {
	[P05] = SMF_CREATE_STATE(p05_entry, p05_run, p05_exit, NULL, NULL),
	[P04] = SMF_CREATE_STATE(p04_entry, p04_run, p04_exit, &test_states[P05], NULL),
	[P03] = SMF_CREATE_STATE(p03_entry, p03_run, p03_exit, &test_states[P04], NULL),
	[P02] = SMF_CREATE_STATE(p02_entry, p02_run, p02_exit, &test_states[P03], NULL),
	[P01] = SMF_CREATE_STATE(p01_entry, p01_run, p01_exit, &test_states[P02], NULL),
	[A]   = SMF_CREATE_STATE(a_entry, a_run, a_exit, &test_states[P01], NULL),
	[B]   = SMF_CREATE_STATE(b_entry, b_run, b_exit, &test_states[P01], NULL),
	[C]   = SMF_CREATE_STATE(c_entry, c_run, c_exit, NULL, NULL),
	[D]   = SMF_CREATE_STATE(d_entry, NULL, NULL, NULL, NULL),
};
/* clang-format on */

ZTEST(smf_tests, test_smf_hierarchical_5_ancestors_instrumented)
{
	reset_logs();
	smf_set_initial(SMF_CTX(&test_obj), &test_states[A]);
	smf_set_hooks(SMF_CTX(&test_obj), &test_hooks);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state(SMF_CTX(&test_obj)) < 0) {
			break;
		}
	}

	/*
	 * Expected action trace (initial entries not captured):
	 *
	 * Run 0: A_RUN (transitions to B, sibling under P01)
	 *   A_RUN, A_EXIT, B_ENTRY
	 *
	 * Run 1: B_RUN (propagate), then ancestor runs bubble up,
	 *   P05_RUN transitions to C
	 *   B_RUN, P01_RUN, P02_RUN, P03_RUN, P04_RUN, P05_RUN
	 *   then exits: B_EXIT, P01_EXIT, P02_EXIT, P03_EXIT, P04_EXIT, P05_EXIT
	 *   then entry: C_ENTRY
	 *
	 * Run 2: C_RUN (transitions to D)
	 *   C_RUN, C_EXIT, D_ENTRY
	 */
	const struct action_record expected[] = {
		/* Run 0 */
		{&test_states[A], SMF_ACTION_RUN},
		{&test_states[A], SMF_ACTION_EXIT},
		{&test_states[B], SMF_ACTION_ENTRY},
		/* Run 1: leaf + ancestor runs */
		{&test_states[B], SMF_ACTION_RUN},
		{&test_states[P01], SMF_ACTION_RUN},
		{&test_states[P02], SMF_ACTION_RUN},
		{&test_states[P03], SMF_ACTION_RUN},
		{&test_states[P04], SMF_ACTION_RUN},
		{&test_states[P05], SMF_ACTION_RUN},
		/* Run 1: exit chain (current=B, up to NULL) */
		{&test_states[B], SMF_ACTION_EXIT},
		{&test_states[P01], SMF_ACTION_EXIT},
		{&test_states[P02], SMF_ACTION_EXIT},
		{&test_states[P03], SMF_ACTION_EXIT},
		{&test_states[P04], SMF_ACTION_EXIT},
		{&test_states[P05], SMF_ACTION_EXIT},
		/* Run 1: entry into C */
		{&test_states[C], SMF_ACTION_ENTRY},
		/* Run 2 */
		{&test_states[C], SMF_ACTION_RUN},
		{&test_states[C], SMF_ACTION_EXIT},
		{&test_states[D], SMF_ACTION_ENTRY},
	};
	int expected_count = ARRAY_SIZE(expected);

	zassert_equal(action_log_count, expected_count, "Expected %d action hooks", expected_count);

	for (int i = 0; i < expected_count; i++) {
		zassert_equal(action_log[i].state, expected[i].state, "Action %d: wrong state", i);
		zassert_equal(action_log[i].action, expected[i].action,
			      "Action %d: wrong action type", i);
	}

	/* Verify transitions */
	zassert_equal(transition_log_count, 3);

	zassert_equal(transition_log[0].source, &test_states[A]);
	zassert_equal(transition_log[0].dest, &test_states[B]);

	zassert_equal(transition_log[1].source, &test_states[B]);
	zassert_equal(transition_log[1].dest, &test_states[C]);

	zassert_equal(transition_log[2].source, &test_states[C]);
	zassert_equal(transition_log[2].dest, &test_states[D]);

	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[D]);

	zassert_equal(error_log_count, 0);
}
