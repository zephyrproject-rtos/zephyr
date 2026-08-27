/*
 * Copyright 2024 Glenn Andrews
 * based on test_lib_hierarchical_smf.c
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/smf.h>
#include "test_instrumentation_common.h"

/*
 * Hierarchical self-transition test:
 *
 * Topology: ROOT > {PARENT_AB > {A, B}, PARENT_C > {C}, D}
 *   ROOT has initial = PARENT_AB
 *   PARENT_AB has initial = STATE_A
 *   PARENT_C has initial = STATE_C
 *
 * Normal flow:
 *   set_initial(PARENT_AB): follows initials to A, entry order ROOT,
 *   PARENT_AB, A (not captured — hooks installed after init)
 *
 *   Run 0: A_RUN (transitions to B, sibling)
 *           -> A_EXIT, transition(A,B), B_ENTRY
 *   Run 1: B_RUN (1st time, HANDLED — no propagation)
 *   Run 2: B_RUN (2nd time, PROPAGATE), PARENT_AB_RUN (transitions to STATE_C)
 *           -> B_EXIT, PARENT_AB_EXIT, transition(B,C), PARENT_C_ENTRY, C_ENTRY
 *   Run 3: C_RUN (1st time, propagate), PARENT_C_RUN (self-transition to PARENT_C)
 *           -> C_EXIT, PARENT_C_EXIT(self), PARENT_C_ENTRY(self),
 *              transition(C,C), C_ENTRY
 *   Run 4: C_RUN (2nd time, transitions to D)
 *           -> C_EXIT, PARENT_C_EXIT, transition(C,D), D_ENTRY
 */

#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN 5

/* Forward declaration of test_states */
static const struct smf_state test_states[];

enum test_state {
	ROOT,
	PARENT_AB,
	PARENT_C,
	STATE_A,
	STATE_B,
	STATE_C,
	STATE_D,
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
	int b_run_count;
	int c_run_count;
} test_obj;

/* ROOT handlers */
static void root_entry(void *obj)
{
	(void)obj;
}
static enum smf_state_result root_run(void *obj)
{
	(void)obj;
	return SMF_EVENT_PROPAGATE;
}
static void root_exit(void *obj)
{
	(void)obj;
}

/* PARENT_AB handlers */
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

	smf_set_state(SMF_CTX(obj), &test_states[STATE_C]);
	return SMF_EVENT_HANDLED;
}

static void parent_ab_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == PARENT_EXIT) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

/* PARENT_C handlers */
static void parent_c_entry(void *obj)
{
	(void)obj;
}

static enum smf_state_result parent_c_run(void *obj)
{
	/* Self-transition to PARENT_C */
	smf_set_state(SMF_CTX(obj), &test_states[PARENT_C]);
	return SMF_EVENT_PROPAGATE;
}

static void parent_c_exit(void *obj)
{
	(void)obj;
}

/* STATE_A handlers */
static void state_a_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == ENTRY) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

static enum smf_state_result state_a_run(void *obj)
{
	smf_set_state(SMF_CTX(obj), &test_states[STATE_B]);
	return SMF_EVENT_PROPAGATE;
}

static void state_a_exit(void *obj)
{
	(void)obj;
}

/* STATE_B handlers */
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

	o->b_run_count++;
	if (o->b_run_count == 1) {
		/* First time: handle event, suppress propagation */
		return SMF_EVENT_HANDLED;
	}
	/* Second time: propagate to PARENT_AB */
	return SMF_EVENT_PROPAGATE;
}

static void state_b_exit(void *obj)
{
	(void)obj;
}

/* STATE_C handlers */
static void state_c_entry(void *obj)
{
	(void)obj;
}

static enum smf_state_result state_c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->c_run_count++;
	if (o->c_run_count == 1) {
		/* First time: propagate to parent (PARENT_C does self-transition) */
		return SMF_EVENT_PROPAGATE;
	}
	/* Second time: transition to D */
	smf_set_state(SMF_CTX(obj), &test_states[STATE_D]);
	return SMF_EVENT_PROPAGATE;
}

static void state_c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	if (o->terminate == EXIT) {
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

/* STATE_D handlers */
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

static const struct smf_state test_states[] = {
	[ROOT] = SMF_CREATE_STATE(root_entry, root_run, root_exit, NULL, &test_states[PARENT_AB]),
	[PARENT_AB] = SMF_CREATE_STATE(parent_ab_entry, parent_ab_run, parent_ab_exit,
				       &test_states[ROOT], &test_states[STATE_A]),
	[PARENT_C] = SMF_CREATE_STATE(parent_c_entry, parent_c_run, parent_c_exit,
				      &test_states[ROOT], &test_states[STATE_C]),
	[STATE_A] = SMF_CREATE_STATE(state_a_entry, state_a_run, state_a_exit,
				     &test_states[PARENT_AB], NULL),
	[STATE_B] = SMF_CREATE_STATE(state_b_entry, state_b_run, state_b_exit,
				     &test_states[PARENT_AB], NULL),
	[STATE_C] = SMF_CREATE_STATE(state_c_entry, state_c_run, state_c_exit,
				     &test_states[PARENT_C], NULL),
	[STATE_D] = SMF_CREATE_STATE(state_d_entry, state_d_run, state_d_exit, &test_states[ROOT],
				     NULL),
};

static void run_fsm(enum terminate_action term)
{
	reset_logs();
	test_obj.terminate = term;
	test_obj.b_run_count = 0;
	test_obj.c_run_count = 0;
	smf_set_initial(SMF_CTX(&test_obj), &test_states[PARENT_AB]);
	smf_set_hooks(SMF_CTX(&test_obj), &test_hooks);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state(SMF_CTX(&test_obj)) < 0) {
			break;
		}
	}
}

ZTEST(smf_tests, test_smf_self_transition_instrumented_normal)
{
	run_fsm(NONE);

	const struct action_record expected[] = {
		/* Initial entry ROOT, PARENT_AB, A not captured */

		/* Run 0: A_RUN transitions to B (sibling under PARENT_AB) */
		{&test_states[STATE_A], SMF_ACTION_RUN},
		{&test_states[STATE_A], SMF_ACTION_EXIT},
		{&test_states[STATE_B], SMF_ACTION_ENTRY},

		/* Run 1: B_RUN (1st time, HANDLED — no propagation, no transition) */
		{&test_states[STATE_B], SMF_ACTION_RUN},

		/* Run 2: B_RUN (2nd, PROPAGATE) -> PARENT_AB_RUN (transitions to C) */
		{&test_states[STATE_B], SMF_ACTION_RUN},
		{&test_states[PARENT_AB], SMF_ACTION_RUN},
		{&test_states[STATE_B], SMF_ACTION_EXIT},
		{&test_states[PARENT_AB], SMF_ACTION_EXIT},
		{&test_states[PARENT_C], SMF_ACTION_ENTRY},
		{&test_states[STATE_C], SMF_ACTION_ENTRY},

		/* Run 3: C_RUN (1st, PROPAGATE) -> PARENT_C_RUN (self-transition) */
		{&test_states[STATE_C], SMF_ACTION_RUN},
		{&test_states[PARENT_C], SMF_ACTION_RUN},
		{&test_states[STATE_C], SMF_ACTION_EXIT},
		{&test_states[PARENT_C], SMF_ACTION_EXIT},
		{&test_states[PARENT_C], SMF_ACTION_ENTRY},
		{&test_states[STATE_C], SMF_ACTION_ENTRY},

		/* Run 4: C_RUN (2nd, transitions to D) */
		{&test_states[STATE_C], SMF_ACTION_RUN},
		{&test_states[STATE_C], SMF_ACTION_EXIT},
		{&test_states[PARENT_C], SMF_ACTION_EXIT},
		{&test_states[STATE_D], SMF_ACTION_ENTRY},
	};
	int expected_count = ARRAY_SIZE(expected);

	zassert_equal(action_log_count, expected_count, "Expected %d action hooks", expected_count);

	for (int i = 0; i < expected_count; i++) {
		zassert_equal(action_log[i].state, expected[i].state, "Action %d: wrong state", i);
		zassert_equal(action_log[i].action, expected[i].action,
			      "Action %d: wrong action type", i);
	}

	/* Transitions: A->B, B->C, C->C (self via parent), C->D */
	zassert_equal(transition_log_count, 4);

	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(transition_log[1].source, &test_states[STATE_B]);
	zassert_equal(transition_log[1].dest, &test_states[STATE_C]);

	/* Self-transition: previous was C, and after following PARENT_C->initial,
	 * current is C again
	 */
	zassert_equal(transition_log[2].source, &test_states[STATE_C]);
	zassert_equal(transition_log[2].dest, &test_states[STATE_C]);

	zassert_equal(transition_log[3].source, &test_states[STATE_C]);
	zassert_equal(transition_log[3].dest, &test_states[STATE_D]);

	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_D]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in parent entry — terminates during set_initial, hooks not
 * installed yet.  No actions or transitions captured.
 */
ZTEST(smf_tests, test_smf_self_transition_instrumented_parent_entry_terminate)
{
	run_fsm(PARENT_ENTRY);

	zassert_equal(action_log_count, 0);

	zassert_equal(transition_log_count, 0);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in child entry — terminates during set_initial, hooks not
 * installed yet.  No actions or transitions captured.
 */
ZTEST(smf_tests, test_smf_self_transition_instrumented_child_entry_terminate)
{
	run_fsm(ENTRY);

	zassert_equal(action_log_count, 0);

	zassert_equal(transition_log_count, 0);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in parent run (PARENT_AB_RUN)
 * (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_self_transition_instrumented_parent_run_terminate)
{
	run_fsm(PARENT_RUN);

	zassert_equal(action_log_count, 6);

	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[STATE_A]);
	zassert_equal(action_log[1].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[2].state, &test_states[STATE_B]);
	zassert_equal(action_log[2].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_RUN);
	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_RUN);
	zassert_equal(action_log[5].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[5].action, SMF_ACTION_RUN);

	zassert_equal(transition_log_count, 1);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in parent exit (PARENT_AB_EXIT)
 * (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_self_transition_instrumented_parent_exit_terminate)
{
	run_fsm(PARENT_EXIT);

	zassert_equal(action_log_count, 8);

	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[STATE_A]);
	zassert_equal(action_log[1].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[2].state, &test_states[STATE_B]);
	zassert_equal(action_log[2].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_RUN);
	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_RUN);
	zassert_equal(action_log[5].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[5].action, SMF_ACTION_RUN);
	zassert_equal(action_log[6].state, &test_states[STATE_B]);
	zassert_equal(action_log[6].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[7].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[7].action, SMF_ACTION_EXIT);

	zassert_equal(transition_log_count, 1);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in child run (B_RUN)
 * (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_self_transition_instrumented_child_run_terminate)
{
	run_fsm(RUN);

	zassert_equal(action_log_count, 4);

	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[STATE_A]);
	zassert_equal(action_log[1].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[2].state, &test_states[STATE_B]);
	zassert_equal(action_log[2].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_RUN);

	zassert_equal(transition_log_count, 1);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(error_log_count, 0);
}

/*
 * Terminate in child exit (C_EXIT)
 * (initial entries not captured)
 */
ZTEST(smf_tests, test_smf_self_transition_instrumented_child_exit_terminate)
{
	run_fsm(EXIT);

	zassert_equal(action_log_count, 13);

	/* Run 0 */
	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);
	zassert_equal(action_log[1].state, &test_states[STATE_A]);
	zassert_equal(action_log[1].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[2].state, &test_states[STATE_B]);
	zassert_equal(action_log[2].action, SMF_ACTION_ENTRY);

	/* Run 1 */
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_RUN);

	/* Run 2 */
	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_RUN);
	zassert_equal(action_log[5].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[5].action, SMF_ACTION_RUN);
	zassert_equal(action_log[6].state, &test_states[STATE_B]);
	zassert_equal(action_log[6].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[7].state, &test_states[PARENT_AB]);
	zassert_equal(action_log[7].action, SMF_ACTION_EXIT);
	zassert_equal(action_log[8].state, &test_states[PARENT_C]);
	zassert_equal(action_log[8].action, SMF_ACTION_ENTRY);
	zassert_equal(action_log[9].state, &test_states[STATE_C]);
	zassert_equal(action_log[9].action, SMF_ACTION_ENTRY);

	/* Run 3: C_RUN propagates, PARENT_C_RUN does self-transition, C_EXIT terminates */
	zassert_equal(action_log[10].state, &test_states[STATE_C]);
	zassert_equal(action_log[10].action, SMF_ACTION_RUN);
	zassert_equal(action_log[11].state, &test_states[PARENT_C]);
	zassert_equal(action_log[11].action, SMF_ACTION_RUN);
	zassert_equal(action_log[12].state, &test_states[STATE_C]);
	zassert_equal(action_log[12].action, SMF_ACTION_EXIT);

	/* Two transitions completed, self-transition aborted during exit */
	zassert_equal(transition_log_count, 2);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);
	zassert_equal(transition_log[1].source, &test_states[STATE_B]);
	zassert_equal(transition_log[1].dest, &test_states[STATE_C]);

	zassert_equal(error_log_count, 0);
}
