/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <smf.h>

/*
 * Flat Test Transtion:
 *
 *	A_ENTRY --> A_RUN --> A_EXIT --> B_ENTRY --> B_RUN --|
 *	                                                     |
 *	|----------------------------------------------------|
 *	|
 *	|--> B_EXIT --> C_ENTRY --> C_RUN --> C_EXIT
 *
 */


#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN                         5

#define STATE_A_ENTRY_BIT       (1 << 0)
#define STATE_A_RUN_BIT         (1 << 1)
#define STATE_A_EXIT_BIT        (1 << 2)

#define STATE_B_ENTRY_BIT       (1 << 3)
#define STATE_B_RUN_BIT         (1 << 4)
#define STATE_B_EXIT_BIT        (1 << 5)

#define STATE_C_ENTRY_BIT       (1 << 6)
#define STATE_C_RUN_BIT         (1 << 7)
#define STATE_C_EXIT_BIT        (1 << 8)

#define TEST_VALUE_NUM          10
static uint32_t test_value[] = {
	0x00,  /* STATE_A_ENTRY */
	0x01,  /* STATE_A_RUN */
	0x03,  /* STATE_A_EXIT */
	0x07,  /* STATE_B_ENTRY */
	0x0f,  /* STATE_B_RUN */
	0x1f,  /* STATE_B_EXIT */
	0x3f,  /* STATE_C_ENTRY */
	0x7f,  /* STATE_C_RUN */
	0xff,  /* STATE_C_EXIT */
	0x1ff, /* FINAL VALUE */
};

/* Forward declaration of test_states */
static const struct smf_state test_states[];

/* List of all TypeC-level states */
enum test_state {
	STATE_A,
	STATE_B,
	STATE_C,
	STATE_D
};

static struct test_object {
	struct smf_ctx ctx;
	uint32_t transition_bits;
	uint32_t tv_idx;
} test_obj;

static void state_a_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx = 0;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State A entry failed");
	o->transition_bits |= STATE_A_ENTRY_BIT;
}

static void state_a_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State A run failed");
	o->transition_bits |= STATE_A_RUN_BIT;

	set_state((struct smf_ctx *)obj, &test_states[STATE_B]);
}

static void state_a_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State A exit failed");
	o->transition_bits |= STATE_A_EXIT_BIT;
}

static void state_b_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State B entry failed");
	o->transition_bits |= STATE_B_ENTRY_BIT;
}

static void state_b_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State B run failed");
	o->transition_bits |= STATE_B_RUN_BIT;

	set_state((struct smf_ctx *)obj, &test_states[STATE_C]);
}

static void state_b_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State B exit failed");
	o->transition_bits |= STATE_B_EXIT_BIT;
}

static void state_c_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State C entry failed");
	o->transition_bits |= STATE_C_ENTRY_BIT;
}

static void state_c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State C run failed");
	o->transition_bits |= STATE_C_RUN_BIT;

	set_state((struct smf_ctx *)obj, &test_states[STATE_D]);
}

static void state_c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State C exit failed");
	o->transition_bits |= STATE_C_EXIT_BIT;
}

static void state_d_entry(void *obj)
{
	/* Do nothing */
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
	[STATE_A] = FSM_CREATE_STATE(state_a_entry, state_a_run, state_a_exit),
	[STATE_B] = FSM_CREATE_STATE(state_b_entry, state_b_run, state_b_exit),
	[STATE_C] = FSM_CREATE_STATE(state_c_entry, state_c_run, state_c_exit),
	[STATE_D] = FSM_CREATE_STATE(state_d_entry, state_d_run, state_d_exit),
};

void test_smf_transitions(void)
{
	set_state((struct smf_ctx *)&test_obj, &test_states[STATE_A]);

	for (int i = 0; i < SMF_RUN; i++) {
		run_state((struct smf_ctx *)&test_obj);
	}

	test_obj.tv_idx++;

	zassert_equal(TEST_VALUE_NUM, test_obj.tv_idx + 1,
		      "Incorrect test value index");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final state not reached");
}
