/*
 * Copyright 2024 Glenn Andrews
 * based on test_lib_hierarchical_smf.c
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/smf.h>

/*
 *  Hierarchical Test Transition to self:
 *
 * This implements a hierarchical state machine using UML rules and demonstrates
 * initial transitions, transitions to self (in PARENT_C) and smf_set_handled (in STATE_B)
 *
 * The order of entry, exit and run actions is given in the ordering of the test_value[] array.
 */

#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN 5

/* Number of state transitions for each test: */
#define TEST_VALUE_NUM              22
#define TEST_PARENT_ENTRY_VALUE_NUM 1
#define TEST_PARENT_RUN_VALUE_NUM   8
#define TEST_PARENT_EXIT_VALUE_NUM  10
#define TEST_ENTRY_VALUE_NUM        2
#define TEST_RUN_VALUE_NUM          6
#define TEST_EXIT_VALUE_NUM         15

enum test_steps {
	/* Initial Setup: Testing initial transitions */
	ROOT_ENTRY = 0,
	PARENT_AB_ENTRY,
	STATE_A_ENTRY,

	/* Run 0: normal state transition */
	STATE_A_RUN,
	STATE_A_EXIT,
	STATE_B_ENTRY,

	/* Run 1: Test smf_set_handled() */
	STATE_B_1ST_RUN,

	/* Run 2: Normal state transition via parent */
	STATE_B_2ND_RUN,
	PARENT_AB_RUN,
	STATE_B_EXIT,
	PARENT_AB_EXIT,
	PARENT_C_1ST_ENTRY,
	STATE_C_1ST_ENTRY,

	/* Run 3: PARENT_C executes transition to self  */
	STATE_C_1ST_RUN,
	PARENT_C_RUN,
	STATE_C_1ST_EXIT,
	PARENT_C_1ST_EXIT,
	PARENT_C_2ND_ENTRY,
	STATE_C_2ND_ENTRY,

	/* Run 4: Test transition from parent state */
	STATE_C_2ND_RUN,
	STATE_C_2ND_EXIT,
	PARENT_C_2ND_EXIT,

	/* End of run */
	FINAL_VALUE,

	/* Unused functions: Error checks if set */
	ROOT_RUN,
	ROOT_EXIT,
};

/*
 * Note: Test values are taken before the appropriate test bit for that state is set i.e. if
 * ROOT_ENTRY_BIT is BIT(0), test_value for root_entry() will be BIT_MASK(0) not BIT_MASK(1)
 */
static uint32_t test_value[] = {
	/* Initial Setup */
	BIT_MASK(ROOT_ENTRY),
	BIT_MASK(PARENT_AB_ENTRY),
	BIT_MASK(STATE_A_ENTRY),
	/* Run 0 */
	BIT_MASK(STATE_A_RUN),
	BIT_MASK(STATE_A_EXIT),
	BIT_MASK(STATE_B_ENTRY),
	/* Run 1 */
	BIT_MASK(STATE_B_1ST_RUN),
	/* Run 2 */
	BIT_MASK(STATE_B_2ND_RUN),
	BIT_MASK(PARENT_AB_RUN),
	BIT_MASK(STATE_B_EXIT),
	BIT_MASK(PARENT_AB_EXIT),
	BIT_MASK(PARENT_C_1ST_ENTRY),
	BIT_MASK(STATE_C_1ST_ENTRY),
	/* Run 3 */
	BIT_MASK(STATE_C_1ST_RUN),
	BIT_MASK(PARENT_C_RUN),
	BIT_MASK(STATE_C_1ST_EXIT),
	BIT_MASK(PARENT_C_1ST_EXIT),
	BIT_MASK(PARENT_C_2ND_ENTRY),
	BIT_MASK(STATE_C_2ND_ENTRY),
	/* Run 4 */
	BIT_MASK(STATE_C_2ND_RUN),
	BIT_MASK(STATE_C_2ND_EXIT),
	BIT_MASK(PARENT_C_2ND_EXIT),
	/* Post-run Check */
	BIT_MASK(FINAL_VALUE),
};

/* Forward declaration of test_states */
static const struct smf_state test_states[];

/* List of all TypeC-level states */
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

#define B_ENTRY_FIRST_TIME        BIT(0)
#define B_RUN_FIRST_TIME          BIT(1)
#define PARENT_C_ENTRY_FIRST_TIME BIT(2)
#define C_RUN_FIRST_TIME          BIT(3)
#define C_ENTRY_FIRST_TIME        BIT(4)
#define C_EXIT_FIRST_TIME         BIT(5)

#define FIRST_TIME_BITS                                                                            \
	(B_ENTRY_FIRST_TIME | B_RUN_FIRST_TIME | PARENT_C_ENTRY_FIRST_TIME | C_RUN_FIRST_TIME |    \
	 C_ENTRY_FIRST_TIME | C_EXIT_FIRST_TIME)

static struct test_object {
	struct smf_ctx ctx;
	uint32_t transition_bits;
	uint32_t tv_idx;
	enum terminate_action terminate;
	uint32_t first_time;
} test_obj;

static void root_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx = 0;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Root entry failed");

	o->transition_bits |= BIT(ROOT_ENTRY);
}

static void root_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Root run failed");

	o->transition_bits |= BIT(ROOT_RUN);

	/* Return to parent run state */
}

static void root_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Root exit failed");
	o->transition_bits |= BIT(ROOT_EXIT);
}

static void parent_ab_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent AB entry failed");

	if (o->terminate == PARENT_ENTRY) {
		smf_set_terminate(obj, -1);
		return;
	}

	o->transition_bits |= BIT(PARENT_AB_ENTRY);
}

static void parent_ab_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent AB run failed");

	if (o->terminate == PARENT_RUN) {
		smf_set_terminate(obj, -1);
		return;
	}

	o->transition_bits |= BIT(PARENT_AB_RUN);

	/*
	 * You should not call smf_set_handled() in the same code path as smf_set_state().
	 * There was a bug that did not reset the handled bit if both were called,
	 * so check it's still fixed:
	 */
	smf_set_handled(SMF_CTX(obj));
	smf_set_state(SMF_CTX(obj), &test_states[STATE_C]);
}

static void parent_ab_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent AB exit failed");

	if (o->terminate == PARENT_EXIT) {
		smf_set_terminate(obj, -1);
		return;
	}

	o->transition_bits |= BIT(PARENT_AB_EXIT);
}

static void parent_c_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent C entry failed");
	if (o->first_time & PARENT_C_ENTRY_FIRST_TIME) {
		o->first_time &= ~PARENT_C_ENTRY_FIRST_TIME;
		o->transition_bits |= BIT(PARENT_C_1ST_ENTRY);
	} else {
		o->transition_bits |= BIT(PARENT_C_2ND_ENTRY);
	}
}

static void parent_c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent C run failed");

	o->transition_bits |= BIT(PARENT_C_RUN);

	smf_set_state(SMF_CTX(obj), &test_states[PARENT_C]);
}

static void parent_c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent C exit failed");

	if (o->first_time & B_ENTRY_FIRST_TIME) {
		o->first_time &= ~B_ENTRY_FIRST_TIME;
		o->transition_bits |= BIT(PARENT_C_1ST_EXIT);
	} else {
		o->transition_bits |= BIT(PARENT_C_2ND_EXIT);
	}
}

static void state_a_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State A entry failed");

	if (o->terminate == ENTRY) {
		smf_set_terminate(obj, -1);
		return;
	}

	o->transition_bits |= BIT(STATE_A_ENTRY);
}

static void state_a_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State A run failed");

	o->transition_bits |= BIT(STATE_A_RUN);

	smf_set_state(SMF_CTX(obj), &test_states[STATE_B]);
}

static void state_a_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State A exit failed");
	o->transition_bits |= BIT(STATE_A_EXIT);
}

static void state_b_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State B entry failed");

	o->transition_bits |= BIT(STATE_B_ENTRY);
}

static void state_b_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State B run failed");

	if (o->terminate == RUN) {
		smf_set_terminate(obj, -1);
		return;
	}

	if (o->first_time & B_RUN_FIRST_TIME) {
		o->first_time &= ~B_RUN_FIRST_TIME;
		o->transition_bits |= BIT(STATE_B_1ST_RUN);
		smf_set_handled(SMF_CTX(obj));
	} else {
		o->transition_bits |= BIT(STATE_B_2ND_RUN);
		/* bubble up to PARENT_AB */
	}
}

static void state_b_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State B exit failed");

	o->transition_bits |= BIT(STATE_B_EXIT);
}

static void state_c_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State C entry failed");
	if (o->first_time & C_ENTRY_FIRST_TIME) {
		o->first_time &= ~C_ENTRY_FIRST_TIME;
		o->transition_bits |= BIT(STATE_C_1ST_ENTRY);
	} else {
		o->transition_bits |= BIT(STATE_C_2ND_ENTRY);
	}
}

static void state_c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State C run failed");

	if (o->first_time & C_RUN_FIRST_TIME) {
		o->first_time &= ~C_RUN_FIRST_TIME;
		o->transition_bits |= BIT(STATE_C_1ST_RUN);
		/* Do nothing, Let parent handle it */
	} else {
		o->transition_bits |= BIT(STATE_C_2ND_RUN);
		smf_set_state(SMF_CTX(obj), &test_states[STATE_D]);
	}
}

static void state_c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State C exit failed");

	if (o->terminate == EXIT) {
		smf_set_terminate(obj, -1);
		return;
	}

	if (o->first_time & C_EXIT_FIRST_TIME) {
		o->first_time &= ~C_EXIT_FIRST_TIME;
		o->transition_bits |= BIT(STATE_C_1ST_EXIT);
	} else {
		o->transition_bits |= BIT(STATE_C_2ND_EXIT);
	}
}

static void state_d_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
}

static void state_d_run(void *obj)
{
	/* Do nothing */
}

static void state_d_exit(void *obj)
{
	/* Do nothing */
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

ZTEST(smf_tests, test_smf_self_transition)
{
	/* A) Test state transitions */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = NONE;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_VALUE_NUM, test_obj.tv_idx, "Incorrect test value index");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final state not reached");

	/* B) Test termination in parent entry action */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = PARENT_ENTRY;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_PARENT_ENTRY_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index for parent entry termination");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final parent entry termination state not reached");

	/* C) Test termination in parent run action */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = PARENT_RUN;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_PARENT_RUN_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index for parent run termination");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final parent run termination state not reached");

	/* D) Test termination in parent exit action */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = PARENT_EXIT;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_PARENT_EXIT_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index for parent exit termination");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final parent exit termination state not reached");

	/* E) Test termination in child entry action */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = ENTRY;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_ENTRY_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index for entry termination");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final entry termination state not reached");

	/* F) Test termination in child run action */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = RUN;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_RUN_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index for run termination");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final run termination state not reached");

	/* G) Test termination in child exit action */

	test_obj.transition_bits = 0;
	test_obj.first_time = FIRST_TIME_BITS;
	test_obj.terminate = EXIT;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[PARENT_AB]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_EXIT_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index for exit termination");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final exit termination state not reached");
}
