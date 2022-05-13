/*
 * Copyright 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ap_pwrseq, CONFIG_AP_PWRSEQ_LOG_LEVEL);

/**
 * Declare weak `struct smf_state` definitions of all ACPI states for
 * architecture and chipset level. These are used as placeholder to keep AP
 * power sequence state machine hierarchy in case corresponding state action
 * handlers are not provided by implementation.
 **/
#define AP_POWER_ARCH_STATE_WEAK_DEFINE(name)                                 \
	const struct smf_state __weak arch_##name##_actions =                 \
		SMF_CREATE_STATE(NULL, NULL, NULL, NULL);

AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_G3)
AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_S5)
AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_S4)
AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_S3)
AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_S2)
AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_S1)
AP_POWER_ARCH_STATE_WEAK_DEFINE(AP_POWER_STATE_S0)

#define AP_POWER_CHIPSET_STATE_WEAK_DEFINE(name)                              \
	const struct smf_state __weak chipset_##name##_actions =              \
		SMF_CREATE_STATE(NULL, NULL, NULL, &arch_##name##_actions);

AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_G3)
AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_S5)
AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_S4)
AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_S3)
AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_S2)
AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_S1)
AP_POWER_CHIPSET_STATE_WEAK_DEFINE(AP_POWER_STATE_S0)

/**
 * Declare weak `struct ap_pwrseq_smf` definitions of all ACPI states for
 * application level. These are used as placeholder to keep AP
 * power sequence state machine hierarchy in case corresponding state action
 * handlers are not provided by implementation.
 **/
#define AP_POWER_APP_STATE_WEAK_DEFINE(name)                                  \
	const struct ap_pwrseq_smf __weak app_state_##name = {                \
		.actions = SMF_CREATE_STATE(NULL, NULL, NULL,                 \
					    &chipset_##name##_actions),       \
		.state = name};

AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_G3)
AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_S5)
AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_S4)
AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_S3)
AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_S2)
AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_S1)
AP_POWER_APP_STATE_WEAK_DEFINE(AP_POWER_STATE_S0)

#define AP_PWRSEQ_STATE_DEFINE(name)                                          \
	[name] = &app_state_##name

/* Sub States defines */
#define AP_PWRSEQ_APP_SUB_STATE_DEFINE(state)                                 \
	[state] = &app_state_##state,

#define AP_PWRSEQ_APP_SUB_STATE_DEFINE_(state)                                \
	AP_PWRSEQ_APP_SUB_STATE_DEFINE(state)

#define AP_PWRSEQ_EACH_APP_SUB_STATE_NODE_DEFINE__(node_id, prop, idx)        \
	AP_PWRSEQ_APP_SUB_STATE_DEFINE_(DT_CAT6(node_id, _P_, prop, _IDX_,    \
						idx, _STRING_UPPER_TOKEN))

#define AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_DEFINE(state)                   \
	[state] = &chipset_##state##_actions,

#define AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_DEFINE_(state)                  \
	AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_DEFINE(state)

#define AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_DEFINE__(node_id, prop, idx)    \
	AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_DEFINE_(                        \
			DT_CAT6(node_id, _P_, prop, _IDX_, idx,               \
				_STRING_UPPER_TOKEN))

#define AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_CHILD_DEFINE(node_id)           \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, chipset),                       \
		(DT_FOREACH_PROP_ELEM(node_id, chipset,                       \
			AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_DEFINE__)),     \
		())

#define AP_PWRSEQ_EACH_APP_SUB_STATE_NODE_CHILD_DEFINE(node_id)               \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, application),                   \
		(DT_FOREACH_PROP_ELEM(node_id, application,                   \
			AP_PWRSEQ_EACH_APP_SUB_STATE_NODE_DEFINE__)),         \
		())

/**
 * @brief Array containing power state action handlers for all state and
 * and substates available for AP power sequence state machine, these items
 * correspond to `enum ap_pwrseq_state`.
 **/
static const struct ap_pwrseq_smf *ap_pwrseq_states[AP_POWER_STATE_COUNT] = {
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_G3),
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_S5),
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_S4),
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_S3),
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_S2),
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_S1),
	AP_PWRSEQ_STATE_DEFINE(AP_POWER_STATE_S0),
	DT_FOREACH_STATUS_OKAY(ap_pwrseq_sub_states,
			AP_PWRSEQ_EACH_APP_SUB_STATE_NODE_CHILD_DEFINE)
	DT_FOREACH_STATUS_OKAY(ap_pwrseq_sub_states,
			AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_NODE_CHILD_DEFINE)
};

#define AP_PWRSEQ_SUB_STATE_STR_DEFINE_WITH_COMA(state)                       \
	state,

#define AP_PWRSEQ_EACH_SUB_STATE_STR_DEFINE(node_id, prop, idx)               \
	AP_PWRSEQ_SUB_STATE_STR_DEFINE_WITH_COMA(DT_CAT5(node_id, _P_,        \
							 prop, _IDX_, idx))

#define AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_STR_DEF_NODE_CHILD_DEFINE(node_id)   \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, chipset),                       \
		(DT_FOREACH_PROP_ELEM(node_id, chipset,                       \
				      AP_PWRSEQ_EACH_SUB_STATE_STR_DEFINE)),  \
		())

#define AP_PWRSEQ_EACH_APP_SUB_STATE_STR_DEF_NODE_CHILD_DEFINE(node_id)       \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, application),                   \
		(DT_FOREACH_PROP_ELEM(node_id, application,                   \
				      AP_PWRSEQ_EACH_SUB_STATE_STR_DEFINE)),  \
		())

const ap_pwrseq_sm_init_func __weak init_func = NULL;

const ap_pwrseq_sm_get_init_state __weak get_init_state_func = NULL;

/**
 * @brief String of all states and substates availabel for AP power seqence
 * state machine.
 **/
static const char * const ap_pwrseq_state_str[AP_POWER_STATE_COUNT] = {
	"AP_POWER_STATE_G3",
	"AP_POWER_STATE_S5",
	"AP_POWER_STATE_S4",
	"AP_POWER_STATE_S3",
	"AP_POWER_STATE_S2",
	"AP_POWER_STATE_S1",
	"AP_POWER_STATE_S0",
	DT_FOREACH_STATUS_OKAY(ap_pwrseq_sub_states,
		AP_PWRSEQ_EACH_APP_SUB_STATE_STR_DEF_NODE_CHILD_DEFINE)
	DT_FOREACH_STATUS_OKAY(ap_pwrseq_sub_states,
		AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_STR_DEF_NODE_CHILD_DEFINE)
};

static struct ap_pwrseq_sm_data sm_data = {
	.states = ap_pwrseq_states,
};

struct ap_pwrseq_sm_data * ap_pwrseq_sm_get_instance(void)
{
	return &sm_data;
}

int ap_pwrseq_sm_init(struct ap_pwrseq_sm_data *const data)
{
	enum ap_pwrseq_state state = AP_POWER_STATE_G3;

	if (data->smf.current) {
		return -EPERM;
	}

	if (init_func) {
		init_func(data);
	}

	if (get_init_state_func) {
		state = get_init_state_func(data);
		if (state >= AP_POWER_STATE_COUNT) {
			return -EINVAL;
		}
	}

	smf_set_initial(&data->smf,
		        (const struct smf_state*)data->states[state]);
	return 0;
}

int ap_pwrseq_sm_set_state(struct ap_pwrseq_sm_data *const data,
			   enum ap_pwrseq_state state)
{
	if (state >= AP_POWER_STATE_COUNT) {
		return -EINVAL;
	}
	smf_set_state((struct smf_ctx *const)&data->smf,
		(const struct smf_state*)data->states[state]);
	return 0;
}

int ap_pwrseq_sm_set_events(struct ap_pwrseq_sm_data *const data,
			    uint32_t events)
{
	data->events = events;
	return 0;
}

int ap_pwrseq_sm_run_state(struct ap_pwrseq_sm_data *const data)
{
	if (data->smf.current == NULL) {
		return -EINVAL;
	}
	data->cur_state_handled = false;
	return smf_run_state((struct smf_ctx *const)data);
}

enum ap_pwrseq_state ap_pwrseq_sm_get_prev_state(struct ap_pwrseq_sm_data
					         *const data)
{
	if (data->smf.previous == NULL) {
		/* Undefined state since we are at the begining */
		return AP_POWER_STATE_UNDEF;
	}
	return ((struct ap_pwrseq_smf *)data->smf.previous)->state;
}

enum ap_pwrseq_state ap_pwrseq_sm_get_cur_state(struct ap_pwrseq_sm_data
						*const data)
{
	if (data->smf.current == NULL) {
		/* Undefined state since we have not started */
		return AP_POWER_STATE_UNDEF;
	}
	return ((struct ap_pwrseq_smf *)data->smf.current)->state;
}

const char *const ap_pwrseq_sm_get_state_str(enum ap_pwrseq_state state)
{
	if (state >=AP_POWER_STATE_COUNT) {
		return NULL;
	}
	return ap_pwrseq_state_str[state];
}
