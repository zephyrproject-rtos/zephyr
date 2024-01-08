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
#include <zephyr/toolchain.h>

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

/**
 * @brief Latency change subscription.
 *
 * @note All fields in this structure are meant for private usage.
 */
struct pm_policy_latency_subscription {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	pm_policy_latency_changed_cb_t cb;
	/** @endcond */
};

/**
 * @brief Latency request.
 *
 * @note All fields in this structure are meant for private usage.
 */
struct pm_policy_latency_request {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	uint32_t value_us;
	/** @endcond */
};

/**
 * @brief Event.
 *
 * @note All fields in this structure are meant for private usage.
 */
struct pm_policy_event {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	uint32_t value_cyc;
	/** @endcond */
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
 * @param value_us Maximum allowed latency in microseconds.
 */
void pm_policy_latency_request_add(struct pm_policy_latency_request *req,
				   uint32_t value_us);

/**
 * @brief Update a latency requirement.
 *
 * @param req Latency request.
 * @param value_us New maximum allowed latency in microseconds.
 */
void pm_policy_latency_request_update(struct pm_policy_latency_request *req,
				      uint32_t value_us);

/**
 * @brief Remove a latency requirement.
 *
 * @param req Latency request.
 */
void pm_policy_latency_request_remove(struct pm_policy_latency_request *req);

/**
 * @brief Subscribe to maximum latency changes.
 *
 * @param req Subscription request.
 * @param cb Callback function (NULL to disable).
 */
void pm_policy_latency_changed_subscribe(struct pm_policy_latency_subscription *req,
					 pm_policy_latency_changed_cb_t cb);

/**
 * @brief Unsubscribe to maximum latency changes.
 *
 * @param req Subscription request.
 */
void pm_policy_latency_changed_unsubscribe(struct pm_policy_latency_subscription *req);

/**
 * @brief Register an event.
 *
 * Events in the power-management policy context are defined as any source that
 * will wake up the system at a known time in the future. By registering such
 * event, the policy manager will be able to decide whether certain power states
 * are worth entering or not.
 *
 * @note It is mandatory to unregister events once they have happened by using
 * pm_policy_event_unregister(). Not doing so is an API contract violation,
 * because the system would continue to consider them as valid events in the
 * *far* future, that is, after the cycle counter rollover.
 *
 * @param evt Event.
 * @param time_us When the event will occur, in microseconds from now.
 *
 * @see pm_policy_event_unregister
 */
void pm_policy_event_register(struct pm_policy_event *evt, uint32_t time_us);

/**
 * @brief Update an event.
 *
 * @param evt Event.
 * @param time_us When the event will occur, in microseconds from now.
 *
 * @see pm_policy_event_register
 */
void pm_policy_event_update(struct pm_policy_event *evt, uint32_t time_us);

/**
 * @brief Unregister an event.
 *
 * @param evt Event.
 *
 * @see pm_policy_event_register
 */
void pm_policy_event_unregister(struct pm_policy_event *evt);

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
	struct pm_policy_latency_request *req, uint32_t value_us)
{
	ARG_UNUSED(req);
	ARG_UNUSED(value_us);
}

static inline void pm_policy_latency_request_update(
	struct pm_policy_latency_request *req, uint32_t value_us)
{
	ARG_UNUSED(req);
	ARG_UNUSED(value_us);
}

static inline void pm_policy_latency_request_remove(
	struct pm_policy_latency_request *req)
{
	ARG_UNUSED(req);
}

static inline void pm_policy_event_register(struct pm_policy_event *evt,
					    uint32_t time_us)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(time_us);
}

static inline void pm_policy_event_update(struct pm_policy_event *evt,
					  uint32_t time_us)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(time_us);
}

static inline void pm_policy_event_unregister(struct pm_policy_event *evt)
{
	ARG_UNUSED(evt);
}
#endif /* CONFIG_PM */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_POLICY_H_ */
