/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/esmf.h>
#include <zephyr/ztest.h>

enum hier_state {
	STATE_PARENT,
	STATE_CHILD_A,
	STATE_CHILD_B,
	STATE_ROOT,
	STATE_COUNT,
};

enum hier_event {
	EVENT_TO_CHILD_B = 1,
	EVENT_TO_ROOT,
};

static const struct smf_state hier_states[STATE_COUNT];

struct hier_test_object {
	struct esmf_ctx esmf;
	bool allow_parent_transition;
	uint32_t action_count;
};

static struct hier_test_object hier_test_obj;

static bool guard_parent_to_child_b(struct esmf_ctx *ctx, uint32_t trigger)
{
	struct hier_test_object *obj = (struct hier_test_object *)ctx;

	ARG_UNUSED(trigger);

	return obj->allow_parent_transition;
}

static void action_count_transition(struct esmf_ctx *ctx, uint32_t trigger)
{
	struct hier_test_object *obj = (struct hier_test_object *)ctx;

	ARG_UNUSED(trigger);

	obj->action_count++;
}

/*
 * Test state machine diagram (PlantUML):
 *
 * @startuml
 * state PARENT {
 *   [*] --> CHILD_A
 *   CHILD_A --> CHILD_B : EVENT_TO_CHILD_B\n[allow_parent_transition]\naction_count_transition
 *   CHILD_B --> [*]
 * }
 *
 * PARENT --> ROOT : EVENT_TO_ROOT\naction_count_transition
 * @enduml
 */

static const struct esmf_transition hier_transitions[] = {
	/*
	 * Match from parent while current state is CHILD_A.
	 * This validates ancestor matching in ESMF with SMF ancestor support.
	 */
	ESMF_CREATE_TRANSITION(&hier_states[STATE_PARENT], EVENT_TO_CHILD_B,
			       guard_parent_to_child_b, action_count_transition,
			       &hier_states[STATE_CHILD_B]),
	ESMF_CREATE_TRANSITION(&hier_states[STATE_CHILD_B], EVENT_TO_ROOT, NULL,
			       action_count_transition, &hier_states[STATE_ROOT]),
};

/* clang-format off */
static const struct smf_state hier_states[STATE_COUNT] = {
	[STATE_PARENT]  = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
	[STATE_CHILD_A] = SMF_CREATE_STATE(NULL, NULL, NULL, &hier_states[STATE_PARENT], NULL),
	[STATE_CHILD_B] = SMF_CREATE_STATE(NULL, NULL, NULL, &hier_states[STATE_PARENT], NULL),
	[STATE_ROOT]    = SMF_CREATE_STATE(NULL, NULL, NULL, NULL, NULL),
};
/* clang-format on */

static void *esmf_hierarchical_setup(void)
{
	memset(&hier_test_obj, 0, sizeof(hier_test_obj));
	esmf_init(ESMF_CTX(&hier_test_obj), hier_transitions, ARRAY_SIZE(hier_transitions));

	return NULL;
}

static void esmf_hierarchical_before(void *fixture)
{
	ARG_UNUSED(fixture);

	hier_test_obj.allow_parent_transition = true;
	hier_test_obj.action_count = 0U;
	smf_set_initial(SMF_CTX(&hier_test_obj), &hier_states[STATE_CHILD_A]);
}

ZTEST(esmf_hierarchical_tests, test_ancestor_match_and_transition)
{
	int rc;

	rc = esmf_handle_event(ESMF_CTX(&hier_test_obj), EVENT_TO_CHILD_B);
	zassert_equal(rc, 0, "parent-scope transition should match from child state");
	zassert_equal(hier_test_obj.action_count, 1, "transition action should run once");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&hier_test_obj)),
		      &hier_states[STATE_CHILD_B], "current state should be CHILD_B");

	rc = esmf_handle_event(ESMF_CTX(&hier_test_obj), EVENT_TO_ROOT);
	zassert_equal(rc, 0, "transition from CHILD_B to ROOT should succeed");
	zassert_equal(hier_test_obj.action_count, 2, "second transition action should run");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&hier_test_obj)), &hier_states[STATE_ROOT],
		      "current state should be ROOT");
}

ZTEST(esmf_hierarchical_tests, test_ancestor_guard_reject)
{
	int rc;

	hier_test_obj.allow_parent_transition = false;

	rc = esmf_handle_event(ESMF_CTX(&hier_test_obj), EVENT_TO_CHILD_B);
	zassert_equal(rc, -EACCES, "guard rejection should return -EACCES");
	zassert_equal(hier_test_obj.action_count, 0, "action must not run when guard rejects");
	zassert_equal(smf_get_current_leaf_state(SMF_CTX(&hier_test_obj)),
		      &hier_states[STATE_CHILD_A], "state should remain CHILD_A");
}

ZTEST_SUITE(esmf_hierarchical_tests, NULL, esmf_hierarchical_setup, esmf_hierarchical_before, NULL,
	    NULL);
