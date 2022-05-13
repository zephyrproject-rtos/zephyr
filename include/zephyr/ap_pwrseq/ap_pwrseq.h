/*
 * Copyright 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _AP_PWRSEQ_H_
#define _AP_PWRSEQ_H_
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AP_POWER_SUB_STATE_ENUM_DEF_WITH_COMMA(node_id, prop, idx)            \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _STRING_UPPER_TOKEN),

#define AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_ENUM_DEF(node_id)                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, chipset),                       \
		(DT_FOREACH_PROP_ELEM(node_id, chipset,                       \
			AP_POWER_SUB_STATE_ENUM_DEF_WITH_COMMA)),             \
		())

#define AP_PWRSEQ_EACH_APP_SUB_STATE_ENUM_DEF(node_id)                        \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, application),                   \
		(DT_FOREACH_PROP_ELEM(node_id, application,                   \
			AP_POWER_SUB_STATE_ENUM_DEF_WITH_COMMA)),             \
		())

enum ap_pwrseq_action {
	AP_POWER_STATE_ACTION_ENTRY,
	AP_POWER_STATE_ACTION_EXIT,
};

/** @brief AP power sequence valid power states. */
enum ap_pwrseq_state {
	AP_POWER_STATE_G3,  /* AP is OFF */
	AP_POWER_STATE_S5,  /* AP is on soft off state */
	AP_POWER_STATE_S4,  /* AP is suspended to Non-volatile disk */
	AP_POWER_STATE_S3,  /* AP is suspended to RAM */
	AP_POWER_STATE_S2,  /* AP is low wake-latency sleep */
	AP_POWER_STATE_S1,  /* AP is low wake-latency sleep */
	AP_POWER_STATE_S0,  /* AP is in active state */
	DT_FOREACH_STATUS_OKAY(ap_pwrseq_sub_states,
			       AP_PWRSEQ_EACH_APP_SUB_STATE_ENUM_DEF)
	DT_FOREACH_STATUS_OKAY(ap_pwrseq_sub_states,
			       AP_PWRSEQ_EACH_CHIPSET_SUB_STATE_ENUM_DEF)
	AP_POWER_STATE_COUNT,
	AP_POWER_STATE_UNDEF = 0xFFFE,
	AP_POWER_STATE_ERROR = 0xFFFF,
};

/** @brief AP power sequence events. */
enum ap_pwrseq_event {
	AP_PWRSEQ_EVENT_POWER_BUTTON,
	AP_PWRSEQ_EVENT_POWER_STARTUP,
	AP_PWRSEQ_EVENT_POWER_SIGNAL,
	AP_PWRSEQ_EVENT_POWER_TIMEOUT,
	AP_PWRSEQ_EVENT_HOST,
	AP_PWRSEQ_EVENT_COUNT,
};

/** @brief The signature for callback notification from AP power seqeuce driver.
 *
 * The function will be invoked by AP power sequence when `state` and one of
 * `actions` are performed by state machine.
 *
 * @param dev Pointer of AP power sequence device driver.
 *
 * @param state State that triggered this callback.
 *
 * @param action Action that triggered this callback.
 *
 * @retval None.
 */
typedef void (*ap_pwrseq_callback)(const struct device *dev,
				   enum ap_pwrseq_state state,
				   enum ap_pwrseq_action action);

struct ap_pwrseq_state_callback {
	/* Node used to link notifications */
	sys_snode_t node;
	/**
	 * Callback function, this will be called when AP power sequence
	 * performs the registered `action` on `state` provided.
	 **/
	ap_pwrseq_callback cb;
	/* Bitfiled of states to invoke callback */
	uint32_t states_bit_mask;
	/* Action done by any of the `states` that will invoke callback */
	enum ap_pwrseq_action action;
};

/**
 * @brief Get AP power sequence device driver pointer.
 *
 * @param None.
 *
 * @retval AP power sequence device driver pointer.
 **/
const struct device * ap_pwrseq_get_instance(void);

/**
 * @brief Starts AP power sequence driver thread execution.
 *
 * @param dev Pointer of AP power sequence device driver.
 *
 * @retval None.
 **/
void ap_pwrseq_start(const struct device *dev);

/**
 * @brief Post event for AP power sequence driver.
 *
 * @param dev Pointer of AP power sequence device driver.
 *
 * @param event Event posted to AP power seuqence driver.
 *
 * @retval None.
 **/
void ap_pwrseq_post_event(const struct device *dev,
			 enum ap_pwrseq_event event);

/**
 * @brief Get enumeration value of current state of AP power sequence driver.
 *
 * @param dev Pointer of AP power sequence device driver.
 *
 * @param event Pointer of variable to hold state result.
 *
 * @retval SUCCESS state is holding valid enumeration state value.
 * @retval -EIO Driver has not started and no valid state is available.
 **/
int ap_pwrseq_get_current_state(const struct device *dev,
				enum ap_pwrseq_state *state);

const char *const ap_pwrseq_get_state_str(enum ap_pwrseq_state state);
/**
 * @brief Register callback into AP power sequence driver.
 *
 * Callback funciotn will be called by AP power sequence driver when power
 * state and action selected match.
 *
 * @param dev Pointer of AP power sequence device driver.
 *
 * @param state_cb Porinter of `ap_pwrseq_state_callback` structure.
 *
 * @retval SUCCESS Callback was successfully registered.
 * @retval -EINVAL On error.
 **/
int ap_pwrseq_register_state_callback(const struct device *dev,
				      struct ap_pwrseq_state_callback
				      *state_cb);

#ifdef __cplusplus
}
#endif
#endif //_AP_PWRSEQ_H_
