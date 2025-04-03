/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_POLICY_H_
#define ZEPHYR_INCLUDE_PM_POLICY_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
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
	int64_t uptime_ticks;
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
 * @brief Check if a power state is available.
 *
 * It is unavailable if locked or latency requirement cannot be fulfilled in that state.
 *
 * @param state Power state.
 * @param substate_id Power substate ID. Use PM_ALL_SUBSTATES to affect all the
 *		      substates in the given power state.
 *
 * @retval true if power state is active.
 * @retval false if power state is not active.
 */
bool pm_policy_state_is_available(enum pm_state state, uint8_t substate_id);

/**
 * @brief Check if any power state can be used.
 *
 * Function allows to quickly check if any power state is available and exit
 * suspend operation early.
 *
 * @retval true if any power state is active.
 * @retval false if all power states are unavailable.
 */
bool pm_policy_state_any_active(void);

/**
 * @brief Register an event.
 *
 * Events in the power-management policy context are defined as any source that
 * will wake up the system at a known time in the future. By registering such
 * event, the policy manager will be able to decide whether certain power states
 * are worth entering or not.
 *
 * CPU is woken up before the time passed in cycle to minimize event handling
 * latency. Once woken up, the CPU will be kept awake until the event has been
 * handled, which is signaled by pm_policy_event_unregister() or moving event
 * into the future using pm_policy_event_update().
 *
 * @param evt Event.
 * @param uptime_ticks When the event will occur, in uptime ticks.
 *
 * @see pm_policy_event_unregister()
 */
void pm_policy_event_register(struct pm_policy_event *evt, int64_t uptime_ticks);

/**
 * @brief Update an event.
 *
 * This shortcut allows for moving the time an event will occur without the
 * need for an unregister + register cycle.
 *
 * @param evt Event.
 * @param uptime_ticks When the event will occur, in uptime ticks.
 *
 * @see pm_policy_event_register
 */
void pm_policy_event_update(struct pm_policy_event *evt, int64_t uptime_ticks);

/**
 * @brief Unregister an event.
 *
 * @param evt Event.
 *
 * @see pm_policy_event_register
 */
void pm_policy_event_unregister(struct pm_policy_event *evt);

/**
 * @brief Increase power state locks.
 *
 * Set power state locks in all power states that disable power in the given
 * device.
 *
 * @param dev Device reference.
 *
 * @see pm_policy_device_power_lock_put()
 * @see pm_policy_state_lock_get()
 */
void pm_policy_device_power_lock_get(const struct device *dev);

/**
 * @brief Decrease power state locks.
 *
 * Remove power state locks in all power states that disable power in the given
 * device.
 *
 * @param dev Device reference.
 *
 * @see pm_policy_device_power_lock_get()
 * @see pm_policy_state_lock_put()
 */
void pm_policy_device_power_lock_put(const struct device *dev);

/**
 * @brief Returns the ticks until the next event
 *
 * If an event is registred, it will return the number of ticks until the next event, if the
 * "next"/"oldest" registered event is in the past, it will return 0. Otherwise it returns -1.
 *
 * @retval >0 If next registered event is in the future
 * @retval 0 If next registered event is now or in the past
 * @retval -1 Otherwise
 */
int64_t pm_policy_next_event_ticks(void);

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

static inline void pm_policy_event_register(struct pm_policy_event *evt, uint32_t cycle)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(cycle);
}

static inline void pm_policy_event_update(struct pm_policy_event *evt, uint32_t cycle)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(cycle);
}

static inline void pm_policy_event_unregister(struct pm_policy_event *evt)
{
	ARG_UNUSED(evt);
}

static inline void pm_policy_device_power_lock_get(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline void pm_policy_device_power_lock_put(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline int64_t pm_policy_next_event_ticks(void)
{
	return -1;
}

#endif /* CONFIG_PM */

#if defined(CONFIG_PM) || defined(CONFIG_PM_POLICY_LATENCY_STANDALONE) || defined(__DOXYGEN__)
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
#else
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
#endif /* CONFIG_PM CONFIG_PM_POLICY_LATENCY_STANDALONE */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_POLICY_H_ */
