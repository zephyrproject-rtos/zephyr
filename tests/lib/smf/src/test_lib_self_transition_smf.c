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

/* Initial Setup: Testing initial transitions */
#define ROOT_ENTRY_BIT      BIT(0)
#define PARENT_AB_ENTRY_BIT BIT(1)
#define STATE_A_ENTRY_BIT   BIT(2)

/* Run 0: normal state transition */
#define STATE_A_RUN_BIT   BIT(3)
#define STATE_A_EXIT_BIT  BIT(4)
#define STATE_B_ENTRY_BIT BIT(5)

/* Run 1: Test smf_set_handled() */
#define STATE_B_1ST_RUN_BIT BIT(6)

/* Run 2: Normal state transition via parent */
#define STATE_B_2ND_RUN_BIT    BIT(7)
#define PARENT_AB_RUN_BIT      BIT(8)
#define STATE_B_EXIT_BIT       BIT(9)
#define PARENT_AB_EXIT_BIT     BIT(10)
#define PARENT_C_1ST_ENTRY_BIT BIT(11)
#define STATE_C_1ST_ENTRY_BIT  BIT(12)

/* Run 3: PARENT_C executes transition to self  */
#define STATE_C_1ST_RUN_BIT    BIT(13)
#define PARENT_C_RUN_BIT       BIT(14)
#define STATE_C_1ST_EXIT_BIT   BIT(15)
#define PARENT_C_1ST_EXIT_BIT  BIT(16)
#define PARENT_C_2ND_ENTRY_BIT BIT(17)
#define STATE_C_2ND_ENTRY_BIT  BIT(18)

/* Run 4: Test transition from parent state */
#define STATE_C_2ND_RUN_BIT   BIT(19)
#define STATE_C_2ND_EXIT_BIT  BIT(20)
#define PARENT_C_2ND_EXIT_BIT BIT(21)

/* Unused functions: Error checks if set */
#define ROOT_RUN_BIT  BIT(22)
#define ROOT_EXIT_BIT BIT(23)

/* Number of state transitions for each test: */
#define TEST_VALUE_NUM              22
#define TEST_PARENT_ENTRY_VALUE_NUM 1
#define TEST_PARENT_RUN_VALUE_NUM   8
#define TEST_PARENT_EXIT_VALUE_NUM  10
#define TEST_ENTRY_VALUE_NUM        2
#define TEST_RUN_VALUE_NUM          6
#define TEST_EXIT_VALUE_NUM         15

/*
 * Note: Test values are taken before the appropriate test bit for that state is set i.e. if
 * ROOT_ENTRY_BIT is BIT(0), test_value for root_entry() will be BIT_MASK(0) not BIT_MASK(1)
 */
static uint32_t test_value[] = {
	/* Initial Setup */
	BIT_MASK(0), /* ROOT_ENTRY_BIT */
	BIT_MASK(1), /* PARENT_AB_ENTRY_BIT */
	BIT_MASK(2), /* STATE_A_ENTRY_BIT */
	/* Run 0 */
	BIT_MASK(3), /* STATE_A_RUN_BIT */
	BIT_MASK(4), /* STATE_A_EXIT_BIT */
	BIT_MASK(5), /* STATE_B_ENTRY_BIT */
	/* Run 1 */
	BIT_MASK(6), /* STATE_B_1ST_RUN_BIT */
	/* Run 2 */
	BIT_MASK(7), /* STATE_B_2ND_RUN_BIT */
	BIT_MASK(8), /* PARENT_AB_RUN_BIT */
	BIT_MASK(9), /* STATE_B_EXIT_BIT */
	BIT_MASK(10), /* PARENT_AB_EXIT_BIT */
	BIT_MASK(11), /* PARENT_C_1ST_ENTRY_BIT */
	BIT_MASK(12), /* STATE_C_1ST_ENTRY_BIT */
	/* Run 3 */
	BIT_MASK(13), /* STATE_C_1ST_RUN_BIT */
	BIT_MASK(14), /* PARENT_C_RUN_BIT */
	BIT_MASK(15), /* STATE_C_1ST_EXIT_BIT */
	BIT_MASK(16), /* PARENT_C_1ST_EXIT_BIT */
	BIT_MASK(17), /* PARENT_C_2ND_ENTRY_BIT */
	BIT_MASK(18), /* STATE_C_2ND_ENTRY_BIT */
	/* Run 4 */
	BIT_MASK(19), /* STATE_C_2ND_RUN_BIT */
	BIT_MASK(20), /* STATE_C_2ND_EXIT_BIT */
	BIT_MASK(21), /* PARENT_C_2ND_EXIT_BIT */
	/* Post-run Check */
	BIT_MASK(22), /* FINAL_VALUE */
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

	o->transition_bits |= ROOT_ENTRY_BIT;
}

static void root_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Root run failed");

	o->transition_bits |= ROOT_RUN_BIT;

	/* Return to parent run state */
}

static void root_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Root exit failed");
	o->transition_bits |= ROOT_EXIT_BIT;
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

	o->transition_bits |= PARENT_AB_ENTRY_BIT;
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

	o->transition_bits |= PARENT_AB_RUN_BIT;

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

	o->transition_bits |= PARENT_AB_EXIT_BIT;
}

static void parent_c_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent C entry failed");
	if (o->first_time & PARENT_C_ENTRY_FIRST_TIME) {
		o->first_time &= ~PARENT_C_ENTRY_FIRST_TIME;
		o->transition_bits |= PARENT_C_1ST_ENTRY_BIT;
	} else {
		o->transition_bits |= PARENT_C_2ND_ENTRY_BIT;
	}
}

static void parent_c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent C run failed");

	o->transition_bits |= PARENT_C_RUN_BIT;

	smf_set_state(SMF_CTX(obj), &test_states[PARENT_C]);
}

static void parent_c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test Parent C exit failed");

	if (o->first_time & B_ENTRY_FIRST_TIME) {
		o->first_time &= ~B_ENTRY_FIRST_TIME;
		o->transition_bits |= PARENT_C_1ST_EXIT_BIT;
	} else {
		o->transition_bits |= PARENT_C_2ND_EXIT_BIT;
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

	o->transition_bits |= STATE_A_ENTRY_BIT;
}

static void state_a_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State A run failed");

	o->transition_bits |= STATE_A_RUN_BIT;

	smf_set_state(SMF_CTX(obj), &test_states[STATE_B]);
}

static void state_a_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State A exit failed");
	o->transition_bits |= STATE_A_EXIT_BIT;
}

static void state_b_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State B entry failed");

	o->transition_bits |= STATE_B_ENTRY_BIT;
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
		o->transition_bits |= STATE_B_1ST_RUN_BIT;
		smf_set_handled(SMF_CTX(obj));
	} else {
		o->transition_bits |= STATE_B_2ND_RUN_BIT;
		/* bubble up to PARENT_AB */
	}
}

static void state_b_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State B exit failed");

	o->transition_bits |= STATE_B_EXIT_BIT;
}

static void state_c_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State C entry failed");
	if (o->first_time & C_ENTRY_FIRST_TIME) {
		o->first_time &= ~C_ENTRY_FIRST_TIME;
		o->transition_bits |= STATE_C_1ST_ENTRY_BIT;
	} else {
		o->transition_bits |= STATE_C_2ND_ENTRY_BIT;
	}
}

static void state_c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx], "Test State C run failed");

	if (o->first_time & C_RUN_FIRST_TIME) {
		o->first_time &= ~C_RUN_FIRST_TIME;
		o->transition_bits |= STATE_C_1ST_RUN_BIT;
		/* Do nothing, Let parent handle it */
	} else {
		o->transition_bits |= STATE_C_2ND_RUN_BIT;
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
		o->transition_bits |= STATE_C_1ST_EXIT_BIT;
	} else {
		o->transition_bits |= STATE_C_2ND_EXIT_BIT;
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
