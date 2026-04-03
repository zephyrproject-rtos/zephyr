/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/smf.h>
#include "test_instrumentation_common.h"

/*
 * Hierarchical Test Transition:
 *
 * Hierarchy: PARENT_AB{A, B}, PARENT_C{C}, D (all parents are roots)
 *
 * Normal flow after set_initial(A) (PARENT_AB_ENTRY, A_ENTRY during init,
 * not captured — hooks installed after init):
 *
 * Run 0: A_RUN (propagate) --> PARENT_AB_RUN (transitions to B)
 *         --> A_EXIT, transition(A,B), B_ENTRY
 * Run 1: B_RUN (transitions to C)
 *         --> B_EXIT, PARENT_AB_EXIT, transition(B,C), PARENT_C_ENTRY, C_ENTRY
 * Run 2: C_RUN (transitions to D, returns HANDLED so PARENT_C_RUN not called)
 *         --> C_EXIT, PARENT_C_EXIT, transition(C,D), D_ENTRY
 *
 * This version uses instrumentation hooks to verify action ordering.
 */

#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN 3

/* Forward declaration of test_states */
static const struct smf_state test_states[];

enum test_state {
	PARENT_AB,
	PARENT_C,
	STATE_A,
	STATE_B,
	STATE_C,
	STATE_D
};

enum terminate_action {
	NONE,
	PARENT_ENTRY,
	PARENT_RUN,
	PARENT_EXIT,
	ENTRY,
	RUN,
	EXIT
};

static struct test_object {
	struct smf_ctx ctx;
	enum terminate_action terminate;
} test_obj;

/* Parent AB handlers */
static void parent_ab_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == PARENT_ENTRY) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

static enum smf_state_result parent_ab_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == PARENT_RUN) {
		smf_set_terminate(SMF_CTX(obj), -1);
		return SMF_EVENT_PROPAGATE;
	}

	smf_set_state(SMF_CTX(obj), &test_states[STATE_B]);
	return SMF_EVENT_PROPAGATE;
}

static void parent_ab_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == PARENT_EXIT) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

/* Parent C handlers */
static void parent_c_entry(void *obj)
{
	(void)obj;
}

static enum smf_state_result parent_c_run(void *obj)
{
	(void)obj;
	/* Should not be reached in the normal flow since C handles event */
	return SMF_EVENT_PROPAGATE;
}

static void parent_c_exit(void *obj)
{
	(void)obj;
}

/* State A handlers */
static void state_a_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == ENTRY) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

static enum smf_state_result state_a_run(void *obj)
{
	(void)obj;
	/* Propagate to parent (PARENT_AB_RUN will handle) */
	return SMF_EVENT_PROPAGATE;
}

static void state_a_exit(void *obj)
{
	(void)obj;
}

/* State B handlers */
static void state_b_entry(void *obj)
{
	(void)obj;
}

static enum smf_state_result state_b_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == RUN) {
		smf_set_terminate(SMF_CTX(obj), -1);
		return SMF_EVENT_PROPAGATE;
	}

	smf_set_state(SMF_CTX(obj), &test_states[STATE_C]);
	return SMF_EVENT_PROPAGATE;
}

static void state_b_exit(void *obj)
{
	(void)obj;
}

/* State C handlers */
static void state_c_entry(void *obj)
{
	(void)obj;
}

static enum smf_state_result state_c_run(void *obj)
{
	smf_set_state(SMF_CTX(obj), &test_states[STATE_D]);
	return SMF_EVENT_HANDLED;
}

static void state_c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == EXIT) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

/* State D handlers */
static void state_d_entry(void *obj)
{
	(void)obj;
}

static enum smf_state_result state_d_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}

static void state_d_exit(void *obj)
{
	(void)obj;
}

/* clang-format off */
static const struct smf_state test_states[] = {
	[PARENT_AB] = SMF_CREATE_STATE(parent_ab_entry, parent_ab_run, parent_ab_exit,
				       NULL, NULL),
	[PARENT_C]  = SMF_CREATE_STATE(parent_c_entry, parent_c_run, parent_c_exit,
				       NULL, NULL),
	[STATE_A]   = SMF_CREATE_STATE(state_a_entry, state_a_run, state_a_exit,
				       &test_states[PARENT_AB], NULL),
	[STATE_B]   = SMF_CREATE_STATE(state_b_entry, state_b_run, state_b_exit,
				       &test_states[PARENT_AB], NULL),
	[STATE_C]   = SMF_CREATE_STATE(state_c_entry, state_c_run, state_c_exit,
				       &test_states[PARENT_C], NULL),
	[STATE_D]   = SMF_CREATE_STATE(state_d_entry, state_d_run, state_d_exit,
				       NULL, NULL),
};
/* clang-format on */

static void run_fsm(enum test_state initial, enum terminate_action term)
{
	reset_logs();
	test_obj.terminate = term;
	smf_set_initial(SMF_CTX(&test_obj), &test_states[initial]);
	smf_set_hooks(SMF_CTX(&test_obj), &test_hooks);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state(SMF_CTX(&test_obj)) < 0) {
			break;
		}
	}
}

/*
 * Normal hierarchical transitions: init(A), A propagates to PARENT_AB which
 * transitions to B, B transitions to C (cross-parent), C transitions to D.
 *
 * Initial PARENT_AB_ENTRY, A_ENTRY not captured (hooks installed after init).
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_normal)
{
	run_fsm(STATE_A, NONE);

	/* Expected action sequence (initial entries not captured) */
	const struct action_record expected_actions[] = {
		/* Run 0 */
		{&test_states[STATE_A], SMF_ACTION_RUN},
		{&test_states[PARENT_AB], SMF_ACTION_RUN},
		{&test_states[STATE_A], SMF_ACTION_EXIT},
		{&test_states[STATE_B], SMF_ACTION_ENTRY},
		/* Run 1 */
		{&test_states[STATE_B], SMF_ACTION_RUN},
		{&test_states[STATE_B], SMF_ACTION_EXIT},
		{&test_states[PARENT_AB], SMF_ACTION_EXIT},
		{&test_states[PARENT_C], SMF_ACTION_ENTRY},
		{&test_states[STATE_C], SMF_ACTION_ENTRY},
		/* Run 2 */
		{&test_states[STATE_C], SMF_ACTION_RUN},
		{&test_states[STATE_C], SMF_ACTION_EXIT},
		{&test_states[PARENT_C], SMF_ACTION_EXIT},
		{&test_states[STATE_D], SMF_ACTION_ENTRY},
	};
	int expected_count = ARRAY_SIZE(expected_actions);

	zassert_equal(action_log_count, expected_count, "Expected %d action hooks", expected_count);

	for (int i = 0; i < expected_count; i++) {
		zassert_equal(action_log[i].state, expected_actions[i].state,
			      "Action %d: wrong state", i);
		zassert_equal(action_log[i].action, expected_actions[i].action,
			      "Action %d: wrong action type", i);
	}

	/* Verify transitions */
	zassert_equal(transition_log_count, 3);

	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(transition_log[1].source, &test_states[STATE_B]);
	zassert_equal(transition_log[1].dest, &test_states[STATE_C]);

	zassert_equal(transition_log[2].source, &test_states[STATE_C]);
	zassert_equal(transition_log[2].dest, &test_states[STATE_D]);

	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_D]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in parent entry — terminates during set_initial, hooks not
 * installed yet.  No actions or transitions captured.
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_parent_entry_terminate)
{
	run_fsm(STATE_A, PARENT_ENTRY);

	zassert_equal(action_log_count, 0);

	zassert_equal(transition_log_count, 0);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in child entry — terminates during set_initial, hooks not
 * installed yet.  No actions or transitions captured.
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_child_entry_terminate)
{
	run_fsm(STATE_A, ENTRY);

	zassert_equal(action_log_count, 0);

	zassert_equal(transition_log_count, 0);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in parent run: A_RUN propagates, PARENT_AB_RUN terminates
 * (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_parent_run_terminate)
{
	run_fsm(STATE_A, PARENT_RUN);

	zassert_equal(action_log_count, 2);
	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[1].action, SMF_ACTION_RUN);

	zassert_equal(transition_log_count, 0);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in parent exit (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_parent_exit_terminate)
{
	run_fsm(STATE_A, PARENT_EXIT);

	zassert_equal(action_log_count, 7);

	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[1].action, SMF_ACTION_RUN);
	zassert_equal(action_log[2].state, &test_states[STATE_A]);
	zassert_equal(action_log[2].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_RUN);
	zassert_equal(action_log[5].state, &test_states[STATE_B]);
	zassert_equal(action_log[5].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[6].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[6].action, SMF_ACTION_EXIT);

	/* First transition (A->B) completed, second aborted during exit */
	zassert_equal(transition_log_count, 1);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in child run (B_RUN) (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_child_run_terminate)
{
	run_fsm(STATE_A, RUN);

	zassert_equal(action_log_count, 5);

	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[1].action, SMF_ACTION_RUN);
	zassert_equal(action_log[2].state, &test_states[STATE_A]);
	zassert_equal(action_log[2].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_RUN);

	zassert_equal(transition_log_count, 1);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in child exit (C_EXIT) (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_hierarchical_instrumented_child_exit_terminate)
{
	run_fsm(STATE_A, EXIT);

	zassert_equal(action_log_count, 11);

	/* Run 0 */
	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[1].action, SMF_ACTION_RUN);
	zassert_equal(action_log[2].state, &test_states[STATE_A]);
	zassert_equal(action_log[2].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_ENTRY);

	/* Run 1 */
	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_RUN);
	zassert_equal(action_log[5].state, &test_states[STATE_B]);
	zassert_equal(action_log[5].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[6].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[6].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[7].state, &test_states[PARENT_C]);
	zassert_equal(action_log[7].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[8].state, &test_states[STATE_C]);
	zassert_equal(action_log[8].action, SMF_ACTION_ENTRY);

	/* Run 2 */
	zassert_equal(action_log[9].state, &test_states[STATE_C]);
	zassert_equal(action_log[9].action, SMF_ACTION_RUN);
	zassert_equal(action_log[10].state, &test_states[STATE_C]);
	zassert_equal(action_log[10].action, SMF_ACTION_EXIT);

	/* Two transitions completed, third aborted */
	zassert_equal(transition_log_count, 2);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);
	zassert_equal(transition_log[1].source, &test_states[STATE_B]);
	zassert_equal(transition_log[1].dest, &test_states[STATE_C]);

	zassert_equal(error_log_count, 0);
}
