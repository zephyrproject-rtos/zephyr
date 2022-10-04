/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_POLICY_H_
#define ZEPHYR_INCLUDE_PM_POLICY_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/pm/state.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System Power Management Policy API
 * @defgroup subsys_pm_sys_policy Policy
 * @ingroup subsys_pm_sys
 * @{
 */

/**
 * @brief Callback to notify when maximum latency changes.
 *
 * @param latency New maximum latency. Positive value represents latency in
 * microseconds. SYS_FOREVER_US value lifts the latency constraint. Other values
 * are forbidden.
 */
typedef void (*pm_policy_latency_changed_cb_t)(int32_t latency);

/** @brief Latency request. */
struct pm_policy_latency_request {
	sys_snode_t node;
	/** Request value. */
	uint32_t value;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Function to get the next PM state
 *
 * This function is called by the power subsystem when the system is
 * idle and returns the most appropriate state based on the number of
 * ticks to the next event.
 *
 * @param cpu CPU index.
 * @param ticks The number of ticks to the next scheduled event.
 *
 * @return The power state the system should use for the given cpu. The function
 * will return NULL if system should remain into PM_STATE_ACTIVE.
 */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks);

/** @endcond */

/** Special value for 'all substates'. */
#define PM_ALL_SUBSTATES (UINT8_MAX)

#if defined(CONFIG_PM) || defined(__DOXYGEN__)
/**
 * @brief Increase a power state lock counter.
 *
 * A power state will not be allowed on the first call of
 * pm_policy_state_lock_get(). Subsequent calls will just increase a reference
 * count, thus meaning this API can be safely used concurrently. A state will
 * be allowed again after pm_policy_state_lock_put() is called as many times as
 * pm_policy_state_lock_get().
 *
 * Note that the PM_STATE_ACTIVE state is always allowed, so calling this API
 * with PM_STATE_ACTIVE will have no effect.
 *
 * @param state Power state.
 * @param substate_id Power substate ID. Use PM_ALL_SUBSTATES to affect all the
 *		      substates in the given power state.
 *
 * @see pm_policy_state_lock_put()
 */
void pm_policy_state_lock_get(enum pm_state state, uint8_t substate_id);

/**
 * @brief Decrease a power state lock counter.
 *
 * @param state Power state.
 * @param substate_id Power substate ID. Use PM_ALL_SUBSTATES to affect all the
 *		      substates in the given power state.
 *
 * @see pm_policy_state_lock_get()
 */
void pm_policy_state_lock_put(enum pm_state state, uint8_t substate_id);

/**
 * @brief Check if a power state lock is active (not allowed).
 *
 * @param state Power state.
 * @param substate_id Power substate ID. Use PM_ALL_SUBSTATES to affect all the
 *		      substates in the given power state.
 *
 * @retval true if power state lock is active.
 * @retval false if power state lock is not active.
 */
bool pm_policy_state_lock_is_active(enum pm_state state, uint8_t substate_id);

/**
 * @brief Add a new latency requirement.
 *
 * The system will not enter any power state that would make the system to
 * exceed the given latency value.
 *
 * @param req Latency request.
 * @param value Maximum allowed latency in microseconds.
 */
void pm_policy_latency_request_add(struct pm_policy_latency_request *req,
				   uint32_t value);

/**
 * @brief Update a latency requirement.
 *
 * @param req Latency request.
 * @param value New maximum allowed latency in microseconds.
 */
void pm_policy_latency_request_update(struct pm_policy_latency_request *req,
				      uint32_t value);

/**
 * @brief Remove a latency requirement.
 *
 * @param req Latency request.
 */
void pm_policy_latency_request_remove(struct pm_policy_latency_request *req);

/**
 * @brief Set the callback to be called when maximum latency changes.
 *
 * @param cb Callback function (NULL to disable).
 */
void pm_policy_latency_changed(pm_policy_latency_changed_cb_t cb);
#else
static inline void pm_policy_state_lock_get(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

static inline void pm_policy_state_lock_put(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

static inline bool pm_policy_state_lock_is_active(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	return false;
}

static inline void pm_policy_latency_request_add(
	struct pm_policy_latency_request *req, uint32_t value)
{
	ARG_UNUSED(req);
	ARG_UNUSED(value);
}

static inline void pm_policy_latency_request_update(
	struct pm_policy_latency_request *req, uint32_t value)
{
	ARG_UNUSED(req);
	ARG_UNUSED(value);
}

static inline void pm_policy_latency_request_remove(
	struct pm_policy_latency_request *req)
{
	ARG_UNUSED(req);
}
#endif /* CONFIG_PM */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_POLICY_H_ */
