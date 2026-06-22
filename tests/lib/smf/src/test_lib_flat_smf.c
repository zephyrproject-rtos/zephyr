/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/smf.h>
#include "test_instrumentation_common.h"

/*
 * Flat Test Transition:
 *
 * A_ENTRY --> A_RUN --> A_EXIT --> B_ENTRY --> B_RUN --|
 *                                                      |
 * |----------------------------------------------------|
 * |
 * |--> B_EXIT --> C_ENTRY --> C_RUN --> C_EXIT --> D_ENTRY
 *
 * This version uses the instrumentation hooks to verify the exact
 * sequence of actions and transitions, replacing the bitmask pattern.
 *
 * Note: smf_set_initial() clears ctx->hooks, so hooks are installed
 * after initialisation.  Initial entry actions are therefore NOT
 * captured in the trace.
 */

#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN 3

/* Forward declaration of test_states */
static const struct smf_state test_states[];

/* List of all TypeC-level states */
enum test_state {
	STATE_A,
	STATE_B,
	STATE_C,
	STATE_D
};

enum terminate_action {
	NONE,
	ENTRY,
	RUN,
	EXIT
};

static struct test_object {
	struct smf_ctx ctx;
	enum terminate_action terminate;
} test_obj;

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
	smf_set_state(SMF_CTX(obj), &test_states[STATE_B]);
	return SMF_EVENT_HANDLED;
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
		return SMF_EVENT_HANDLED;
	}

	smf_set_state(SMF_CTX(obj), &test_states[STATE_C]);
	return SMF_EVENT_HANDLED;
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
	return SMF_EVENT_HANDLED;
}

static void state_d_exit(void *obj)
{
	(void)obj;
}

/* clang-format off */
static const struct smf_state test_states[] = {
	[STATE_A] = SMF_CREATE_STATE(state_a_entry, state_a_run, state_a_exit, NULL, NULL),
	[STATE_B] = SMF_CREATE_STATE(state_b_entry, state_b_run, state_b_exit, NULL, NULL),
	[STATE_C] = SMF_CREATE_STATE(state_c_entry, state_c_run, state_c_exit, NULL, NULL),
	[STATE_D] = SMF_CREATE_STATE(state_d_entry, state_d_run, state_d_exit, NULL, NULL),
};
/* clang-format on */

/*
 * Helper to run the FSM with hooks installed after smf_set_initial().
 * Because smf_set_initial() clears ctx->hooks, hooks must be set
 * afterwards.  Initial entry actions are therefore NOT captured.
 */
static void run_fsm(enum test_state initial, enum terminate_action term)
{
	reset_logs();
	test_obj.terminate = term;
	smf_set_initial(SMF_CTX(&test_obj), &test_states[initial]);
	smf_set_hooks(SMF_CTX(&test_obj), &test_hooks);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state(SMF_CTX(&test_obj))) {
			break;
		}
	}
}

/*
 * Flat test: normal transitions A -> B -> C -> D
 *
 * set_initial(A): A_ENTRY (not captured — hooks installed after init)
 *
 * Run 0: A_RUN -> [A transitions to B] -> A_EXIT, transition(A,B), B_ENTRY
 * Run 1: B_RUN -> [B transitions to C] -> B_EXIT, transition(B,C), C_ENTRY
 * Run 2: C_RUN -> [C transitions to D] -> C_EXIT, transition(C,D), D_ENTRY
 */
ZTEST(smf_tests, test_smf_flat_instrumented_normal)
{
	run_fsm(STATE_A, NONE);

	/* Verify action sequence (initial A_ENTRY not captured) */
	zassert_equal(action_log_count, 9, "Expected 9 action hooks");

	/* Run 0: A_RUN, A_EXIT, B_ENTRY */
	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);

	zassert_equal(action_log[1].state, &test_states[STATE_A]);
	zassert_equal(action_log[1].action, SMF_ACTION_EXIT);

	zassert_equal(action_log[2].state, &test_states[STATE_B]);
	zassert_equal(action_log[2].action, SMF_ACTION_ENTRY);

	/* Run 1: B_RUN, B_EXIT, C_ENTRY */
	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_RUN);

	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_EXIT);

	zassert_equal(action_log[5].state, &test_states[STATE_C]);
	zassert_equal(action_log[5].action, SMF_ACTION_ENTRY);

	/* Run 2: C_RUN, C_EXIT, D_ENTRY */
	zassert_equal(action_log[6].state, &test_states[STATE_C]);
	zassert_equal(action_log[6].action, SMF_ACTION_RUN);

	zassert_equal(action_log[7].state, &test_states[STATE_C]);
	zassert_equal(action_log[7].action, SMF_ACTION_EXIT);

	zassert_equal(action_log[8].state, &test_states[STATE_D]);
	zassert_equal(action_log[8].action, SMF_ACTION_ENTRY);

	/* Verify transition sequence */
	zassert_equal(transition_log_count, 3, "Expected 3 transitions");

	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(transition_log[1].source, &test_states[STATE_B]);
	zassert_equal(transition_log[1].dest, &test_states[STATE_C]);

	zassert_equal(transition_log[2].source, &test_states[STATE_C]);
	zassert_equal(transition_log[2].dest, &test_states[STATE_D]);

	/* Verify no errors */
	zassert_equal(error_log_count, 0);

	/* Verify final state */
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_D]);
}

/*
 * Flat test: terminate in entry action of State A
 *
 * set_initial(A): state_a_entry terminates (hooks not installed yet).
 * run_state returns immediately due to terminate flag.
 * No actions or transitions are captured.
 */
ZTEST(smf_tests, test_smf_flat_instrumented_entry_terminate)
{
	run_fsm(STATE_A, ENTRY);

	zassert_equal(action_log_count, 0, "No actions captured — terminate during init");

	zassert_equal(transition_log_count, 0, "No transitions should occur");

	zassert_equal(error_log_count, 0);
}

/*
 * Flat test: terminate in run action of State B
 *
 * set_initial(A): A_ENTRY (not captured)
 * Run 0: A_RUN -> [A transitions to B] -> A_EXIT, transition(A,B), B_ENTRY
 * Run 1: B_RUN (terminates)
 */
ZTEST(smf_tests, test_smf_flat_instrumented_run_terminate)
{
	run_fsm(STATE_A, RUN);

	/* A_RUN, A_EXIT, B_ENTRY, B_RUN = 4 actions (initial A_ENTRY not captured) */
	zassert_equal(action_log_count, 4, "Expected 4 action hooks before termination");

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
 * Flat test: terminate in exit action of State C
 *
 * set_initial(A): A_ENTRY (not captured)
 * Run 0: A_RUN, A_EXIT, transition(A,B), B_ENTRY
 * Run 1: B_RUN, B_EXIT, transition(B,C), C_ENTRY
 * Run 2: C_RUN, C_EXIT (terminates during exit)
 */
ZTEST(smf_tests, test_smf_flat_instrumented_exit_terminate)
{
	run_fsm(STATE_A, EXIT);

	zassert_equal(action_log_count, 8, "Expected 8 action hooks before termination");

	zassert_equal(action_log[0].state, &test_states[STATE_A]);
	zassert_equal(action_log[0].action, SMF_ACTION_RUN);

	zassert_equal(action_log[1].state, &test_states[STATE_A]);
	zassert_equal(action_log[1].action, SMF_ACTION_EXIT);

	zassert_equal(action_log[2].state, &test_states[STATE_B]);
	zassert_equal(action_log[2].action, SMF_ACTION_ENTRY);

	zassert_equal(action_log[3].state, &test_states[STATE_B]);
	zassert_equal(action_log[3].action, SMF_ACTION_RUN);

	zassert_equal(action_log[4].state, &test_states[STATE_B]);
	zassert_equal(action_log[4].action, SMF_ACTION_EXIT);

	zassert_equal(action_log[5].state, &test_states[STATE_C]);
	zassert_equal(action_log[5].action, SMF_ACTION_ENTRY);

	zassert_equal(action_log[6].state, &test_states[STATE_C]);
	zassert_equal(action_log[6].action, SMF_ACTION_RUN);

	zassert_equal(action_log[7].state, &test_states[STATE_C]);
	zassert_equal(action_log[7].action, SMF_ACTION_EXIT);

	/* Two transitions completed before the third was aborted by terminate in exit */
	zassert_equal(transition_log_count, 2);
	zassert_equal(transition_log[0].source, &test_states[STATE_A]);
	zassert_equal(transition_log[0].dest, &test_states[STATE_B]);

	zassert_equal(transition_log[1].source, &test_states[STATE_B]);
	zassert_equal(transition_log[1].dest, &test_states[STATE_C]);

	zassert_equal(error_log_count, 0);
}
