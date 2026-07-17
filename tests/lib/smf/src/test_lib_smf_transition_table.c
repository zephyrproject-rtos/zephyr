/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_SMF_TRANSITION_TABLE

#include <zephyr/ztest.h>
#include <zephyr/smf.h>
#include "test_instrumentation_common.h"

/* -----------------------------------------------------------------------
 * Shared test context
 * ----------------------------------------------------------------------- */

enum tt_state { TT_A, TT_B, TT_C, TT_D, TT_STATE_COUNT };
enum tt_event { TT_EV_FOO = 1, TT_EV_BAR };

/* Sequence tags used in effect-ordering tests */
#define SEQ_EXIT_A   1
#define SEQ_EFFECT   2
#define SEQ_ENTRY_B  3

/* Terminate value used in the terminate-propagation regression test */
#define TT_TERMINATE_VAL 42

struct tt_ctx {
	struct smf_ctx ctx;
	int counter;
	int seq[16];
	int seq_n;
};

static struct tt_ctx tt;
static const struct smf_state tt_states[];

static void tt_reset(void)
{
	memset(&tt, 0, sizeof(tt));
}

/* -----------------------------------------------------------------------
 * Generic state callbacks used across multiple tests
 * ----------------------------------------------------------------------- */

static void tt_a_entry(void *obj)
{
	(void)obj;
}

static void tt_a_exit(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = SEQ_EXIT_A;
}

static void tt_b_entry(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = SEQ_ENTRY_B;
}

static void tt_effect_seq(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = SEQ_EFFECT;
}

static void tt_effect_inc(void *obj)
{
	struct tt_ctx *t = obj;

	t->counter++;
}

static void tt_effect_terminate(void *obj)
{
	smf_set_terminate(obj, TT_TERMINATE_VAL);
}

static bool tt_guard_true(void *obj)
{
	(void)obj;
	return true;
}

static bool tt_guard_false(void *obj)
{
	(void)obj;
	return false;
}

static bool tt_guard_counter_eq5(void *obj)
{
	struct tt_ctx *t = obj;

	return t->counter == 5;
}

/* clang-format off */
static const struct smf_state tt_states[] = {
	[TT_A] = SMF_CREATE_STATE(tt_a_entry, NULL, tt_a_exit, NULL, NULL),
	[TT_B] = SMF_CREATE_STATE(tt_b_entry, NULL, NULL,      NULL, NULL),
	[TT_C] = SMF_CREATE_STATE(NULL,       NULL, NULL,      NULL, NULL),
	[TT_D] = SMF_CREATE_STATE(NULL,       NULL, NULL,      NULL, NULL),
};
/* clang-format on */

/* -----------------------------------------------------------------------
 * Test 1: specific trigger fires when matching event is raised
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_trigger_fires)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      TT_EV_FOO, NULL, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	smf_raise_event(SMF_CTX(&tt), TT_EV_FOO);
	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
}

/* -----------------------------------------------------------------------
 * Test 2: specific trigger does NOT fire without a matching event
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_trigger_no_fire_without_event)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      TT_EV_FOO, NULL, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	/* No event raised — event stays SMF_TRIGGER_ANY (-1), which does not match TT_EV_FOO */
	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_A]);
}

/* -----------------------------------------------------------------------
 * Test 3: SMF_TRIGGER_ANY fires every cycle without any event
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_trigger_any_always_fires)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, NULL, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	/* No explicit event — SMF_TRIGGER_ANY still fires */
	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
}

/* -----------------------------------------------------------------------
 * Test 4: guard blocks transition when it returns false
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_guard_blocks)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, tt_guard_false, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_A]);
}

/* -----------------------------------------------------------------------
 * Test 5: guard allows transition when it returns true
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_guard_allows)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, tt_guard_true, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
}

/* -----------------------------------------------------------------------
 * Test 6: priority — lower value wins among eligible transitions
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_priority)
{
	/* Both transitions are eligible; prio 1 (A→C) should beat prio 2 (A→B) */
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, NULL, NULL, 2),
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_C],
				      SMF_TRIGGER_ANY, NULL, NULL, 1),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_C]);
}

/* -----------------------------------------------------------------------
 * Test 7: effect fires between exit and entry (UML order)
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_effect_ordering)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, NULL, tt_effect_seq, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));

	zassert_equal(tt.seq_n, 3, "Expected exit, effect, entry");
	zassert_equal(tt.seq[0], SEQ_EXIT_A,  "First: exit A");
	zassert_equal(tt.seq[1], SEQ_EFFECT,  "Second: effect");
	zassert_equal(tt.seq[2], SEQ_ENTRY_B, "Third: entry B");
}

/* -----------------------------------------------------------------------
 * Test 8: event is auto-cleared after smf_run_state()
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_event_auto_clear)
{
	static const struct smf_transition table[] = {
		/* A→B on TT_EV_FOO */
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      TT_EV_FOO, NULL, NULL, 0),
		/* B→C on TT_EV_FOO — must NOT fire on second run */
		SMF_CREATE_TRANSITION(&tt_states[TT_B], &tt_states[TT_C],
				      TT_EV_FOO, NULL, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	/* Raise event once, run once → A→B */
	smf_raise_event(SMF_CTX(&tt), TT_EV_FOO);
	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);

	/* No new event — event was consumed; B→C must NOT fire */
	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
}

/* -----------------------------------------------------------------------
 * Test 9: smf_raise_event() is equivalent to direct ctx->event assignment
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_raise_event_helper)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      TT_EV_BAR, NULL, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	smf_raise_event(SMF_CTX(&tt), TT_EV_BAR);
	zassert_equal(SMF_CTX(&tt)->event, TT_EV_BAR);

	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
}

/* -----------------------------------------------------------------------
 * Test 10: self-transition (A→A) runs exit → effect → entry
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_self_transition)
{
	static const struct smf_transition table[] = {
		/* A→A: increment counter until 5, then A→B */
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, tt_guard_counter_eq5, NULL,         1),
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_A],
				      SMF_TRIGGER_ANY, NULL,                 tt_effect_inc, 2),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	/* Run until we leave A */
	while (smf_get_current_leaf_state(SMF_CTX(&tt)) == &tt_states[TT_A]) {
		zassert_ok(smf_run_state(SMF_CTX(&tt)));
	}

	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
	zassert_equal(tt.counter, 5);
}

/* -----------------------------------------------------------------------
 * Test 11 (HSM only): transition from parent fires when current is child
 * ----------------------------------------------------------------------- */

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT

enum tt_hsm_state { TT_HSM_PARENT, TT_HSM_CHILD, TT_HSM_OTHER, TT_HSM_STATE_COUNT };

static const struct smf_state tt_hsm_states[];

/* clang-format off */
static const struct smf_state tt_hsm_states[] = {
	[TT_HSM_PARENT] = SMF_CREATE_STATE(NULL, NULL, NULL, NULL,              NULL),
	[TT_HSM_CHILD]  = SMF_CREATE_STATE(NULL, NULL, NULL, &tt_hsm_states[TT_HSM_PARENT], NULL),
	[TT_HSM_OTHER]  = SMF_CREATE_STATE(NULL, NULL, NULL, NULL,              NULL),
};
/* clang-format on */

ZTEST(smf_tests, test_smf_tt_hsm_ancestor_transition)
{
	static struct tt_ctx hsm_tt;
	/* Transition defined on PARENT, but current state will be CHILD */
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_hsm_states[TT_HSM_PARENT], &tt_hsm_states[TT_HSM_OTHER],
				      SMF_TRIGGER_ANY, NULL, NULL, 0),
	};

	memset(&hsm_tt, 0, sizeof(hsm_tt));

	smf_set_initial(SMF_CTX(&hsm_tt), &tt_hsm_states[TT_HSM_CHILD]);
	smf_set_transitions(SMF_CTX(&hsm_tt), table, ARRAY_SIZE(table));

	/* Current is CHILD (descendant of PARENT) — PARENT's transition must fire */
	zassert_ok(smf_run_state(SMF_CTX(&hsm_tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&hsm_tt)),
		      &tt_hsm_states[TT_HSM_OTHER]);
}

#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */

/* -----------------------------------------------------------------------
 * Test 12: smf_run_state() propagates terminate_val set by a transition
 * effect on the same call (regression test for the missing check after
 * smf_evaluate_transitions()).
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_terminate_propagation)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, NULL, tt_effect_terminate, 0),
	};
	int32_t rc;

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	rc = smf_run_state(SMF_CTX(&tt));
	zassert_equal(rc, TT_TERMINATE_VAL,
		      "terminate_val should propagate on the same call, got %d", rc);
}

/* -----------------------------------------------------------------------
 * Test 13: a guard that fails must not block a lower-priority (higher prio
 * value) transition whose own guard passes — guards filter candidates
 * before priority arbitrates among the survivors.
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_guard_filters_before_priority)
{
	static const struct smf_transition table[] = {
		/* Would win on priority alone, but its guard fails */
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, tt_guard_false, NULL, 1),
		/* Loses on priority, but is the only one whose guard passes */
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_C],
				      SMF_TRIGGER_ANY, tt_guard_true, NULL, 2),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_C]);
}

/* -----------------------------------------------------------------------
 * Test 14: the current event is consumed even when no transition matches
 * it (guard blocked the only candidate), so it cannot linger into a later
 * smf_run_state() cycle.
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_event_cleared_even_without_match)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      TT_EV_FOO, tt_guard_false, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	smf_raise_event(SMF_CTX(&tt), TT_EV_FOO);
	zassert_ok(smf_run_state(SMF_CTX(&tt)));

	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_A]);
	zassert_equal(SMF_CTX(&tt)->event, SMF_TRIGGER_ANY,
		      "event must be consumed even though the guard blocked the transition");
}

/* -----------------------------------------------------------------------
 * Test 15: when two eligible transitions share the same priority, the
 * first one in table order wins (implementation detail of the strict '<'
 * comparison in smf_evaluate_transitions() — locked in so a future change
 * to '<=' can't silently flip the winner without a test noticing).
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_priority_tie_break_is_table_order)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, NULL, NULL, 0),
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_C],
				      SMF_TRIGGER_ANY, NULL, NULL, 0),
	};

	tt_reset();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
}

/* -----------------------------------------------------------------------
 * Test 16: instrumentation hooks observe transitions driven by the
 * transition table, not just ones driven by explicit smf_set_state().
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_instrumentation_hooks_fire)
{
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_states[TT_A], &tt_states[TT_B],
				      SMF_TRIGGER_ANY, NULL, NULL, 0),
	};

	tt_reset();
	reset_logs();
	smf_set_initial(SMF_CTX(&tt), &tt_states[TT_A]);
	smf_set_hooks(SMF_CTX(&tt), &test_hooks);
	smf_set_transitions(SMF_CTX(&tt), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&tt)));

	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&tt)), &tt_states[TT_B]);
	zassert_equal(transition_log_count, 1, "Expected 1 transition recorded via hooks");
	zassert_equal(transition_log[0].source, &tt_states[TT_A]);
	zassert_equal(transition_log[0].dest, &tt_states[TT_B]);
}

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT

/* -----------------------------------------------------------------------
 * Test 17 (HSM only): exit/effect/entry ordering holds across a real
 * multi-level hierarchy transition (multiple exits, multiple entries),
 * not just the trivial single-level case covered by Test 7.
 * ----------------------------------------------------------------------- */

enum tt_hsm2_seq {
	HSM2_SEQ_EXIT_CHILD = 1,
	HSM2_SEQ_EXIT_PARENT,
	HSM2_SEQ_EFFECT,
	HSM2_SEQ_ENTRY_PARENT,
	HSM2_SEQ_ENTRY_CHILD,
};

enum tt_hsm2_state { TT_HSM2_PARENT_A, TT_HSM2_CHILD_A, TT_HSM2_PARENT_B, TT_HSM2_CHILD_B,
		     TT_HSM2_STATE_COUNT };

static void hsm2_exit_child(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = HSM2_SEQ_EXIT_CHILD;
}

static void hsm2_exit_parent(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = HSM2_SEQ_EXIT_PARENT;
}

static void hsm2_effect(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = HSM2_SEQ_EFFECT;
}

static void hsm2_entry_parent(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = HSM2_SEQ_ENTRY_PARENT;
}

static void hsm2_entry_child(void *obj)
{
	struct tt_ctx *t = obj;

	t->seq[t->seq_n++] = HSM2_SEQ_ENTRY_CHILD;
}

static const struct smf_state tt_hsm2_states[];

/* clang-format off */
static const struct smf_state tt_hsm2_states[] = {
	[TT_HSM2_PARENT_A] = SMF_CREATE_STATE(NULL, NULL, hsm2_exit_parent, NULL, NULL),
	[TT_HSM2_CHILD_A]  = SMF_CREATE_STATE(NULL, NULL, hsm2_exit_child,
					       &tt_hsm2_states[TT_HSM2_PARENT_A], NULL),
	[TT_HSM2_PARENT_B] = SMF_CREATE_STATE(hsm2_entry_parent, NULL, NULL, NULL, NULL),
	[TT_HSM2_CHILD_B]  = SMF_CREATE_STATE(hsm2_entry_child, NULL, NULL,
					       &tt_hsm2_states[TT_HSM2_PARENT_B], NULL),
};
/* clang-format on */

ZTEST(smf_tests, test_smf_tt_effect_ordering_hierarchical)
{
	static struct tt_ctx h;
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_hsm2_states[TT_HSM2_CHILD_A],
				      &tt_hsm2_states[TT_HSM2_CHILD_B],
				      SMF_TRIGGER_ANY, NULL, hsm2_effect, 0),
	};

	memset(&h, 0, sizeof(h));
	smf_set_initial(SMF_CTX(&h), &tt_hsm2_states[TT_HSM2_CHILD_A]);
	smf_set_transitions(SMF_CTX(&h), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&h)));

	zassert_equal(h.seq_n, 5,
		      "Expected exit-child, exit-parent, effect, entry-parent, entry-child");
	zassert_equal(h.seq[0], HSM2_SEQ_EXIT_CHILD);
	zassert_equal(h.seq[1], HSM2_SEQ_EXIT_PARENT);
	zassert_equal(h.seq[2], HSM2_SEQ_EFFECT);
	zassert_equal(h.seq[3], HSM2_SEQ_ENTRY_PARENT);
	zassert_equal(h.seq[4], HSM2_SEQ_ENTRY_CHILD);
}

/* -----------------------------------------------------------------------
 * Test 18 (HSM only): when a parent-level and its child's own transition
 * are simultaneously eligible, priority alone decides the winner — there
 * is no automatic "more specific state wins" rule like strict UML defines.
 * This documents that as a deliberate implementation choice rather than
 * an oversight: flip the priorities below and the winner flips with them.
 * ----------------------------------------------------------------------- */

ZTEST(smf_tests, test_smf_tt_parent_vs_child_precedence_is_priority_driven)
{
	static struct tt_ctx h;
	/* PARENT's transition (prio 0) wins over CHILD's own (prio 5), even
	 * though CHILD is the more specific (current) state.
	 */
	static const struct smf_transition table[] = {
		SMF_CREATE_TRANSITION(&tt_hsm2_states[TT_HSM2_PARENT_A],
				      &tt_hsm2_states[TT_HSM2_PARENT_B],
				      SMF_TRIGGER_ANY, NULL, NULL, 0),
		SMF_CREATE_TRANSITION(&tt_hsm2_states[TT_HSM2_CHILD_A],
				      &tt_hsm2_states[TT_HSM2_CHILD_B],
				      SMF_TRIGGER_ANY, NULL, NULL, 5),
	};

	memset(&h, 0, sizeof(h));
	smf_set_initial(SMF_CTX(&h), &tt_hsm2_states[TT_HSM2_CHILD_A]);
	smf_set_transitions(SMF_CTX(&h), table, ARRAY_SIZE(table));

	zassert_ok(smf_run_state(SMF_CTX(&h)));
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&h)), &tt_hsm2_states[TT_HSM2_PARENT_B]);
}

#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */

#endif /* CONFIG_SMF_TRANSITION_TABLE */
