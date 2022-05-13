/*
 * Copyright 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _AP_PWRSEQ_SM_H_
#define _AP_PWRSEQ_SM_H_
#include <zephyr/device.h>
#include <zephyr/ap_pwrseq/ap_pwrseq_sm_defs.h>

/**
 * @brief Set current `run` iteration as handled.
 *
 * This will inform all parents that current iteration had been handled by
 * child when executing `run` function.
 *
 * @param data State machine data pointer.
 *
 * @retval None
 **/
#define SET_HANDLED(data)                                                     \
	((struct ap_pwrseq_sm_data *)(data))->cur_state_handled = true

/**
 * @brief Check if current `run` iteration has been handled.
 *
 * @param data State machine data pointer.
 *
 * @retval True This means child `run` function has already handled this
 * iteration, return false otherwise.
 **/
#define IS_HANDLED(data)                                                      \
	(!!((struct ap_pwrseq_sm_data *)(data))->cur_state_handled)

/**
 * @brief Return if current `run` iteration has been handled.
 *
 * @param data State machine data pointer.
 *
 * @retval None.
 **/
#define RETURN_IF_HANDLED(data)      do { if (IS_HANDLED(data))               \
					{return;} }while(0)

/**
 * @brief Check if event is set.
 *
 * @param data State machine data pointer.
 *
 * @param event Event to be check if set.
 *
 * @retval True Event has been set, return False otherwise.
 **/
#define IS_EVENT_SET(data, event)                                             \
	(((struct ap_pwrseq_sm_data *)(data))->events & BIT(event))

/**
 * @brief Define architecture level state action handlers.
 *
 * @param name Valid enumaration value of state.
 *
 * @param entry Function to be called when entrying state.
 *
 * @param run Function to be called when executing `run` operation.
 *
 * @param exit Function to be called when exiting state.
 *
 * @retval Defines global structure with action handlers to be used by AP
 * power seuqence state machine.
 **/
#define AP_POWER_ARCH_STATE_DEFINE(name, entry, run, exit)                    \
	const struct smf_state arch_##name##_actions =                        \
		SMF_CREATE_STATE(entry, run, exit, NULL);

/**
 * @brief Define chipset level state action handlers.
 *
 * @param name Valid enumaration value of state.
 *
 * @param entry Function to be called when entrying state.
 *
 * @param run Function to be called when executing `run` operation.
 *
 * @param exit Function to be called when exiting state.
 *
 * @retval Defines global structure with action handlers to be used by AP
 * power seuqence state machine.
 **/
#define AP_POWER_CHIPSET_STATE_DEFINE(name, entry, run, exit)                 \
	const struct smf_state chipset_##name##_actions =                     \
		SMF_CREATE_STATE(entry, run, exit, &arch_##name##_actions);

/**
 * @brief Define application level state action handlers.
 *
 * @param name Valid enumaration value of state.
 *
 * @param entry Function to be called when entrying state.
 *
 * @param run Function to be called when executing `run` operation.
 *
 * @param exit Function to be called when exiting state.
 *
 * @retval Defines global structure with action handlers to be used by AP
 * power seuqence state machine.
 **/
#define AP_POWER_APP_STATE_DEFINE(name, entry, run, exit)                     \
	const struct ap_pwrseq_smf app_state_##name = {                       \
		.actions = SMF_CREATE_STATE(entry, run, exit,                 \
			&chipset_##name##_actions),                           \
		.state = name};

/**
 * @brief Define chipset level substate action handlers.
 *
 * @param name Valid enumaration value of state, as provided by devicetree
 * compatible with "ap-pwrseq-sub-states".
 *
 * @param entry Function to be called when entrying state.
 *
 * @param run Function to be called when executing `run` operation.
 *
 * @param exit Function to be called when exiting state.
 *
 * @param parent Valid enumaration value of parent state,
 *
 * @retval Defines global structure with action handlers to be used by AP
 * power seuqence state machine.
 **/
#define AP_POWER_CHIPSET_SUB_STATE_DEFINE(name, entry, run, exit, parent)     \
	const struct ap_pwrseq_smf chipset_##name##_actions = {               \
		.actions = SMF_CREATE_STATE(entry, run, exit,                 \
			&arch_##parent##_actions),                            \
		.state = name};

/**
 * @brief Define application level substate action handlers.
 *
 * @param name Valid enumaration value of state, as provided by devicetree
 * compatible with "ap-pwrseq-sub-states".
 *
 * @param entry Function to be called when entrying state.
 *
 * @param run Function to be called when executing `run` operation.
 *
 * @param exit Function to be called when exiting state.
 *
 * @param parent Valid enumaration value of parent state,
 *
 * @retval Defines global structure with action handlers to be used by AP
 * power seuqence state machine.
 **/
#define AP_POWER_APP_SUB_STATE_DEFINE(name, entry, run, exit, parent)         \
	const struct ap_pwrseq_smf app_state_##name = {                       \
		.actions = SMF_CREATE_STATE(entry, run, exit,                 \
			 (struct smf_state *) &chipset_##parent##_actions),   \
		.state = name};

typedef const void (*ap_pwrseq_sm_init_func)(void *dev);

typedef const enum ap_pwrseq_state (*ap_pwrseq_sm_get_init_state)(void *dev);

extern const ap_pwrseq_sm_init_func init_func;

extern const ap_pwrseq_sm_get_init_state get_init_state_func;

#define AP_POWER_INIT_FUNC(func) init_func = func

#define AP_POWER_INIT_STATE_FUNC(func) get_init_state_func = func

struct ap_pwrseq_sm_data {
	/* Zephyr SMF context */
	struct smf_ctx smf;
	/* Pointer to array of states structures */
	const struct ap_pwrseq_smf** states;
	/* Bitfiled of events */
	uint32_t events;
	/* Flag to inform if current state has been handled */
	bool cur_state_handled;
};

/**
 * @brief Obtain AP power sequence state machine instance.
 *
 * @param None.
 *
 * @retval Return instance data of the state machine, only one instance is
 * allowed per application.
 **/
struct ap_pwrseq_sm_data * ap_pwrseq_sm_get_instance(void);

/**
 * @brief Sets AP power sequence state machine initial state.
 *
 * @param data Pointer to AP power sequence state machine instance data.
 *
 * @retval SUCCESS Upon success, state ‘entry’ action handlers on all
 * implemented levels will be invoked.
 * @retval -EINVAL State provided is invalid.
 * @retval -EPERM  State machine is already initialized.
 **/
int ap_pwrseq_sm_init(struct ap_pwrseq_sm_data *const data);

/**
 * @brief Sets AP power sequence state machine to provided state.
 *
 * @param data Pointer to AP power sequence state machine instance data.
 *
 * @param state Enum value of next state to be executed.
 *
 * @retval SUCCESS Upon success, current state `exit` action handler and next
 * state `entry` action handler will be executed.
 * @retval -EINVAL State provided is invalid.
 **/
int ap_pwrseq_sm_set_state(struct ap_pwrseq_sm_data *const data,
			   enum ap_pwrseq_state state);

/**
 * @brief Sets events to be processed by AP power sequence state machine.
 *
 * @param data Pointer to AP power sequence state machine instance data.
 *
 * @param events Bitfield of events bein triggrered to be processed.
 *
 * @retval SUCCESS Events have been set.
 **/
int ap_pwrseq_sm_set_events(struct ap_pwrseq_sm_data *const data,
			    uint32_t events);

/**
 * @brief Execute current state `run` action handlers.
 *
 * @param data Pointer to AP power sequence state machine instance data.
 *
 * @retval SUCCESS Upon success, provided `run` action handlers will be executed
 * for all levels in current state.
 * @retval -EINVAL State machine has not been initialized.
 **/
int ap_pwrseq_sm_run_state(struct ap_pwrseq_sm_data *const data);

/**
 * @brief Get current state enumeration value.
 *
 * @param data Pointer to AP power sequence state machine instance data.
 *
 * @retval Enum value Upon success
 * @retval AP_POWER_STATE_UNDEF If state machine has not been initialized.
 **/
enum ap_pwrseq_state ap_pwrseq_sm_get_cur_state(struct ap_pwrseq_sm_data
						*const data);
/**
 * @brief Get previous state enumeration value.
 *
 * @param data Pointer to AP power sequence state machine instance data.
 *
 * @retval Enum value Upon success
 * @retval AP_POWER_STATE_UNDEF If state machine is at initial state and no
 * previous state is available.
 **/
enum ap_pwrseq_state ap_pwrseq_sm_get_prev_state(struct ap_pwrseq_sm_data
						 *const data);

/**
 * @brief Get enumeration state value corresponding string value.
 *
 * @param state Enumeration value of state.
 *
 * @retval String value containing the state name.
 * @retval NULL If state provided is invalid.
 **/
const char *const ap_pwrseq_sm_get_state_str(enum ap_pwrseq_state state);

#endif //_AP_PWRSEQ_SM_H_
