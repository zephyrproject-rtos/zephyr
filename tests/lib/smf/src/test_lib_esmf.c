/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/esmf.h>
#include <zephyr/ztest.h>

enum test_state {
	STATE_A,
	STATE_B,
	STATE_C,
	STATE_COUNT,
};

enum test_event {
	EVENT_TO_B = 1,
	EVENT_TO_C,
	EVENT_RESET,
	EVENT_INTERNAL,
	EVENT_SELF,
	EVENT_PRIORITY_LOWEST,
	EVENT_PRIORITY_TIE,
};

static const struct smf_state test_states[STATE_COUNT];

struct test_object {
	struct esmf_ctx esmf;
	bool allow_to_c;
	uint32_t action_count;
	uint32_t entry_count[STATE_COUNT];
	uint32_t exit_count[STATE_COUNT];
};

static struct test_object test_obj;

static void state_a_entry(void *obj)
{
	struct test_object *o = obj;

	o->entry_count[STATE_A]++;
}

static void state_a_exit(void *obj)
{
	struct test_object *o = obj;

	o->exit_count[STATE_A]++;
}

static void state_b_entry(void *obj)
{
	struct test_object *o = obj;

	o->entry_count[STATE_B]++;
}

static void state_b_exit(void *obj)
{
	struct test_object *o = obj;

	o->exit_count[STATE_B]++;
}

static void state_c_entry(void *obj)
{
	struct test_object *o = obj;

	o->entry_count[STATE_C]++;
}

static void state_c_exit(void *obj)
{
	struct test_object *o = obj;

	o->exit_count[STATE_C]++;
}

static bool guard_allow_to_c(struct esmf_ctx *ctx, uint32_t trigger)
{
	struct test_object *o = (struct test_object *)ctx;

	ARG_UNUSED(trigger);

	return o->allow_to_c;
}

static void action_increment(struct esmf_ctx *ctx, uint32_t trigger)
{
	struct test_object *o = (struct test_object *)ctx;

	ARG_UNUSED(trigger);

	o->action_count++;
}

/*
 * Test state machine diagram (PlantUML):
 *
 * @startuml
 * [*] --> A
 *
 * A --> B : EVENT_TO_B
 * B --> C : EVENT_TO_C\n[guard_allow_to_c]\naction_increment
 * A --> B : EVENT_PRIORITY_LOWEST\nprio=5
 * A --> C : EVENT_PRIORITY_LOWEST\nprio=1 (selected)
 * A --> B : EVENT_PRIORITY_TIE\nprio=3 (first found, selected)
 * A --> C : EVENT_PRIORITY_TIE\nprio=3
 *
 * A --> A : EVENT_RESET
 * B --> A : EVENT_RESET
 * C --> A : EVENT_RESET
 *
 * A : EVENT_INTERNAL / action_increment
 * B : EVENT_INTERNAL / action_increment
 * C : EVENT_INTERNAL / action_increment
 *
 * B --> B : EVENT_SELF / action_increment
 * @enduml
 */

static const struct esmf_transition transitions[] = {
	ESMF_CREATE_TRANSITION(&test_states[STATE_A], EVENT_TO_B, NULL, NULL,
			       &test_states[STATE_B]),
	ESMF_CREATE_TRANSITION(&test_states[STATE_B], EVENT_TO_C, guard_allow_to_c,
			       action_increment, &test_states[STATE_C]),
	ESMF_CREATE_TRANSITION(NULL, EVENT_RESET, NULL, NULL, &test_states[STATE_A]),
	ESMF_CREATE_TRANSITION(NULL, EVENT_INTERNAL, NULL, action_increment, NULL),
	ESMF_CREATE_TRANSITION(&test_states[STATE_B], EVENT_SELF, NULL, action_increment,
			       &test_states[STATE_B]),
	ESMF_CREATE_TRANSITION_PRIO(&test_states[STATE_A], EVENT_PRIORITY_LOWEST, NULL, NULL,
				    &test_states[STATE_B], 5),
	ESMF_CREATE_TRANSITION_PRIO(&test_states[STATE_A], EVENT_PRIORITY_LOWEST, NULL, NULL,
				    &test_states[STATE_C], 1),
	ESMF_CREATE_TRANSITION_PRIO(&test_states[STATE_A], EVENT_PRIORITY_TIE, NULL, NULL,
				    &test_states[STATE_B], 3),
	ESMF_CREATE_TRANSITION_PRIO(&test_states[STATE_A], EVENT_PRIORITY_TIE, NULL, NULL,
				    &test_states[STATE_C], 3),
};

/* clang-format off */
static const struct smf_state test_states[STATE_COUNT] = {
	[STATE_A] = SMF_CREATE_STATE(state_a_entry, NULL, state_a_exit, NULL, NULL),
	[STATE_B] = SMF_CREATE_STATE(state_b_entry, NULL, state_b_exit, NULL, NULL),
	[STATE_C] = SMF_CREATE_STATE(state_c_entry, NULL, state_c_exit, NULL, NULL),
};
/* clang-format on */

static void *esmf_setup(void)
{
	memset(&test_obj, 0, sizeof(test_obj));
	esmf_init(ESMF_CTX(&test_obj), transitions, ARRAY_SIZE(transitions));

	return NULL;
}

static void esmf_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(test_obj.entry_count, 0, sizeof(test_obj.entry_count));
	memset(test_obj.exit_count, 0, sizeof(test_obj.exit_count));
	test_obj.allow_to_c = false;
	test_obj.action_count = 0U;
	smf_set_initial(SMF_CTX(&test_obj), &test_states[STATE_A]);
}

ZTEST(esmf_tests, test_transition_and_state_tracking)
{
	int rc;

	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_TO_B);
	zassert_equal(rc, 0, "transition to B should succeed");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_B],
		      "current state should be B");
	zassert_equal(test_obj.exit_count[STATE_A], 1, "A exit must run once");
	zassert_equal(test_obj.entry_count[STATE_B], 1, "B entry must run once");
}

ZTEST(esmf_tests, test_guard_block_and_then_allow)
{
	int rc;

	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_TO_B);
	zassert_equal(rc, 0, "transition to B should succeed");

	test_obj.allow_to_c = false;
	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_TO_C);
	zassert_equal(rc, -EACCES, "guard should block transition");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_B],
		      "state should remain B");

	test_obj.allow_to_c = true;
	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_TO_C);
	zassert_equal(rc, 0, "guard should allow transition");
	zassert_equal(test_obj.action_count, 1, "action should run once");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_C],
		      "current state should be C");
}

ZTEST(esmf_tests, test_wildcard_and_internal_transition)
{
	int rc;
	const struct smf_state *before;

	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_TO_B);
	zassert_equal(rc, 0, "transition to B should succeed");

	before = smf_get_current_leaf_state(SMF_CTX(&test_obj));
	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_INTERNAL);
	zassert_equal(rc, 0, "internal transition event should be handled");
	zassert_equal(test_obj.action_count, 1, "action should run once");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), before,
		      "state should not change for internal transition");

	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_RESET);
	zassert_equal(rc, 0, "wildcard reset should be handled");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_A],
		      "reset should return to A");
}

ZTEST(esmf_tests, test_no_match)
{
	int rc;

	rc = esmf_handle_event(ESMF_CTX(&test_obj), 999U);
	zassert_equal(rc, -ENOENT, "unknown event should not match");
}

ZTEST(esmf_tests, test_self_transition)
{
	int rc;

	/* Move to state B first */
	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_TO_B);
	zassert_equal(rc, 0, "transition to B should succeed");

	/* Check exit and entry counts */
	zassert_equal(test_obj.exit_count[STATE_A], 1, "A exit must run once");
	zassert_equal(test_obj.entry_count[STATE_B], 1, "B entry must run once");

	/* Self-transition: smf_set_state is called, so exit and entry re-run */
	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_SELF);
	zassert_equal(rc, 0, "self-transition event should be handled");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_B],
		      "state should remain B after self-transition");
	zassert_equal(test_obj.action_count, 1, "action should run once");
	zassert_equal(test_obj.entry_count[STATE_B], 2, "entry must re-run on self-transition");
	zassert_equal(test_obj.exit_count[STATE_B], 1, "exit must re-run on self-transition");
}

ZTEST(esmf_tests, test_priority_lowest_value_wins)
{
	int rc;

	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_PRIORITY_LOWEST);
	zassert_equal(rc, 0, "priority event should be handled");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_C],
		      "transition with lower priority value should be selected");
}

ZTEST(esmf_tests, test_priority_tie_keeps_first_found)
{
	int rc;

	rc = esmf_handle_event(ESMF_CTX(&test_obj), EVENT_PRIORITY_TIE);
	zassert_equal(rc, 0, "priority tie event should be handled");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&test_obj)), &test_states[STATE_B],
		      "first matching transition should win when priorities are equal");
}

ZTEST_SUITE(esmf_tests, NULL, esmf_setup, esmf_before, NULL, NULL);
