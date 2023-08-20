/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_PM_H_
#define ZEPHYR_INCLUDE_PM_PM_H_

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>
#include <zephyr/pm/state.h>
#include <zephyr/toolchain.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System and device power management
 * @defgroup subsys_pm Power Management (PM)
 * @ingroup os_services
 * @{
 * @}
 */

/**
 * @brief System Power Management API
 * @defgroup subsys_pm_sys System
 * @ingroup subsys_pm
 * @{
 */

/**
 * Power management notifier struct
 *
 * This struct contains callbacks that are called when the target enters and
 * exits power states.
 *
 * As currently implemented the entry callback is invoked when
 * transitioning from PM_STATE_ACTIVE to another state, and the exit
 * callback is invoked when transitioning from a non-active state to
 * PM_STATE_ACTIVE. This behavior may change in the future.
 *
 * @note These callbacks can be called from the ISR of the event
 *       that caused the kernel exit from idling.
 *
 * @note It is not allowed to call @ref pm_notifier_unregister or
 *       @ref pm_notifier_register from these callbacks because they are called
 *       with the spin locked in those functions.
 */
struct pm_notifier {
	sys_snode_t _node;

	/**
	 * Bit field indicating whether the callback should trigger on power state entry, exit, or
	 * both.
	 *
	 * @note See PM_STATE_ENTRY and PM_STATE_EXIT
	 */
	uint8_t direction;

	/**
	 * Application or driver defined function for doing any target specific operations during
	 * power state transition.
	 *
	 * @param direction Bit field indicator specifying whether the callback was triggered during
	 *		    state entry or exit
	 * @param ctx Optional callback context, e.g. driver instance, application data, etc.
	 */
	void (*callback)(uint8_t direction, void *ctx);

#ifdef CONFIG_PM
	/**
	 * Workqueue item to be called in place of above callback. Runs via dedicated PM workqueue.
	 * Used for logic that needs to block.
	 *
	 * @note State direction comparison is not supported in PM workqueue handler.
	 */
	struct k_work work;
#endif

	/**
	 * Optional callback context
	 */
	const void *ctx;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Helper macro for building a pm_notifiers list name.
 *
 * @param node_id zephyr,power-state node identifer
 */
#define Z_PM_NOTIFIERS_DT_NAME(node_id) UTIL_CAT(pm_notifiers_, DT_NODE_CHILD_IDX(node_id))

/**
 * @brief Generate a pm_notifiers list for the passed power-states child node
 *	  identifier.
 *
 * @param node_id zephyr,power-state node identifier
 */
#define Z_PM_NOTIFIERS_CREATE_DT_STATES(node_id)			\
	static sys_slist_t Z_PM_NOTIFIERS_DT_NAME(node_id) =		\
		SYS_SLIST_STATIC_INIT(&Z_PM_NOTIFIERS_DT_NAME(node_id))

/**
 * @brief Provides a pm_notifiers list pointer for a power-states child node
 *	  identifier.
 *
 * @param node_id zephyr,power-state node identifier
 */
#define Z_PM_NOTIFIERS_DT_STATES(node_id) &Z_PM_NOTIFIERS_DT_NAME(node_id)

/** @endcond */

#ifdef CONFIG_PM
/**
 * @brief Generic helper for defining a pm_notifier struct.
 *
 * @param id Unique alphanumeric identifier, specific to usage context
 *	     (avoid using only numbers here to prevent collisions)
 * @param state_direction Transition direction: state entry, exit, or both
 * @param callback_fn callback function
 */
#define PM_NOTIFIER_DEFINE(id, state_direction, callback_fn, context)	\
	static struct pm_notifier UTIL_CAT(pm_notifier_, id) = {	\
		.direction = state_direction,				\
		.callback = callback_fn,				\
		.ctx = context,						\
	}

/**
 * @brief Retrieve the name of a pm_notifier struct instance.
 *
 * @param id Unique alphanumeric identifier, specific to usage context
 */
#define PM_NOTIFIER(id) UTIL_CAT(pm_notifier_, id)
#else
#define PM_NOTIFIER_DEFINE(id, state_direction, callback_fn, context)
#define PM_NOTIFIER(id) NULL
#endif

#if defined(CONFIG_PM) || defined(__DOXYGEN__)
/**
 * @brief Force usage of given power state.
 *
 * This function overrides decision made by PM policy forcing
 * usage of given power state upon next entry of the idle thread.
 *
 * @note This function can only run in thread context
 *
 * @param cpu CPU index.
 * @param info Power state which should be used in the ongoing
 *	suspend operation.
 */
bool pm_state_force(uint8_t cpu, const struct pm_state_info *info);

/**
 * @brief Register a power management notifier
 *
 * Register the given notifier from the power management notification
 * list.
 *
 * @param notifier pm_notifier object to be registered.
 * @param state PM state for which the notifier should be registered
 * @param substate_id PM substate for which the notifier should be registered.
 *
 * @return 0 if the notifier was successfully registered, a negative value
 * otherwise.
 */
int pm_notifier_register(struct pm_notifier *notifier, enum pm_state state, uint8_t substate_id);

/**
 * @brief Unregister a power management notifier
 *
 * Remove the given notifier from the power management notification
 * list. After that this object callbacks will not be called.
 *
 * @param notifier pm_notifier object to be unregistered.
 * @param state PM state for which the notifier should be registered
 * @param substate_id PM substate for which the notifier should be registered.
 *
 * @return 0 if the notifier was successfully removed, a negative value
 * otherwise.
 */
int pm_notifier_unregister(struct pm_notifier *notifier, enum pm_state state, uint8_t substate_id);

/**
 * @brief Gets the next power state that will be used.
 *
 * This function returns the next power state that will be used by the
 * SoC.
 *
 * @param cpu CPU index.
 * @return next pm_state_info that will be used
 */
const struct pm_state_info *pm_state_next_get(uint8_t cpu);

/**
 * @}
 */

/**
 * @brief System Power Management Hooks
 * @defgroup subsys_pm_sys_hooks Hooks
 * @ingroup subsys_pm_sys
 * @{
 */

/**
 * @brief Put processor into a power state.
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 *
 * @param state Power state.
 * @param substate_id Power substate id.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id);

/**
 * @brief Do any SoC or architecture specific post ops after sleep state exits.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after sleep state exits. Currently it enables
 * interrupts after resuming from sleep state. In future, the enabling
 * of interrupts may be moved into the kernel.
 *
 * @param state Power state.
 * @param substate_id Power substate id.
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id);

/**
 * @}
 */

#else  /* CONFIG_PM */

static inline int pm_notifier_register(struct pm_notifier *notifier,
				       enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(notifier);
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	return -ENOSYS;
}

static inline int pm_notifier_unregister(struct pm_notifier *notifier,
					 enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(notifier);
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	return -ENOSYS;
}

static inline const struct pm_state_info *pm_state_next_get(uint8_t cpu)
{
	ARG_UNUSED(cpu);

	return NULL;
}
#endif /* CONFIG_PM */

void z_pm_save_idle_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_PM_H_ */
