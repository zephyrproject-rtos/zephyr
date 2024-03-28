/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for clock management
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_H_

/**
 * @brief Clock Management Interface
 * @defgroup clock_mgmt_interface Clock management Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/devicetree/clock_mgmt.h>
#include <zephyr/sys/slist.h>
#include <errno.h>

/**
 * @name Clock Management States
 * @{
 */

/** Default state (state used when the device is in operational state). */
#define CLOCK_MGMT_STATE_DEFAULT 0U
/** Sleep state (state used when the device is in low power mode). */
#define CLOCK_MGMT_STATE_SLEEP 1U

/** This and higher values refer to custom private states. */
#define CLOCK_MGMT_STATE_PRIV_START 2U

/** @} */

/**
 * @name Clock Subsys Names
 * @{
 */

/** Default clock subsystem */
#define CLOCK_MGMT_SUBSYS_DEFAULT 0U
/** This and higher values refer to custom private clock subsystems */
#define CLOCK_MGMT_SUBSYS_PRIV_START 1U

/** @} */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Defines name for clock management setpoint function
 */
#define Z_CLOCK_MGMT_SETPOINT_FUNC_NAME(node, state_idx) \
	z_clock_setpoint_##node##_state_##state_idx

/**
 * @brief Define a clock management state
 * This macro defines the extern function prototype for a clock management
 * function. The function is implemented within the generated soc clock
 * management code.
 */
#define Z_CLOCK_MGMT_STATE_DEFINE(state_idx, node) \
	extern clock_setpoint_t Z_CLOCK_MGMT_SETPOINT_FUNC_NAME(node, state_idx);

/**
 * @brief Initialize a clock management state
 * This macro handles initialization of a clock management state for a
 * given node, by defining the function pointer to use
 */
#define Z_CLOCK_MGMT_STATE_INIT(state_idx, node) \
	&Z_CLOCK_MGMT_SETPOINT_FUNC_NAME(node, state_idx)

/**
 * @brief Define a clock management callback
 * This macro defines a clock management callback prototype. Note that the
 * callback is declared as extern, since it is implemented in the
 * clock_callbacks.c common code
 */
#define Z_CLOCK_MGMT_CALLBACK_DEFINE(node, prop, idx) \
	extern sys_slist_t _CONCAT(clock_callback_, DT_STRING_TOKEN( \
		DT_PHANDLE_BY_IDX(node, prop, idx), clock_id));

/**
 * @brief Init a clock management callback
 * This macro gets a reference to a clock management callback. This reference
 * is what results in the linker keeping this callback symbol, since it
 * is not referenced within the clock_callbacks.c common code.
 */
#define Z_CLOCK_MGMT_CALLBACK_INIT(node, prop, idx) \
	_CONCAT(&clock_callback_, DT_STRING_TOKEN( \
		DT_PHANDLE_BY_IDX(node, prop, idx), clock_id))


/**
 * @brief Defines base name for clock management structures given a node id
 * Provides a unique base name for clock management structures given a node
 * identifier. This can be pasted with a suffix to identify the specific
 * structure being initialized.
 */
#define Z_CLOCK_MGMT_NODE_NAME(node_id) clock_mgmt_##node_id

/**
 * @brief Defines name for clock management rate read function
 */
#define Z_CLOCK_MGMT_SUBSYS_FUNC_NAME(node_id, prop, idx)		\
	z_clock_rate_##node_id##_subsys_##idx


/**
 * @brief Defines a reference to clock management read function
 * Defines an extern reference to the clock management read function, which
 * is implemented in the generated SOC clock management code
 */
#define Z_CLOCK_MGMT_SUBSYS_DEFINE(node_id, prop, idx)			\
	extern clock_rate_t Z_CLOCK_MGMT_SUBSYS_FUNC_NAME(node_id, prop, idx);


/**
 * @brief Gets reference to clock management rate read function
 */
#define Z_CLOCK_MGMT_SUBSYS_INIT(node_id, prop, idx)			\
	&Z_CLOCK_MGMT_SUBSYS_FUNC_NAME(node_id, prop, idx)


/** @endcond */

/**
 * Definition for clock rate read functions. This is internal to the clock
 * management subsystem
 */
typedef int (*clock_rate_t)(void);
/**
 * Definition for clock setpoint functions. This is internal to the clock
 * management subsystem
 */
typedef int (*clock_setpoint_t)(void);

/** Clock management configuration */
struct clock_mgmt {
	/** Functions to read rate for each clock subsystem */
	const clock_rate_t *rates;
	/** Array of callback lists, one per subsystem */
	sys_slist_t **callbacks;
	/** Count of clock subsystems for this instance */
	uint8_t clocks_cnt;
	/** Clock states. Each state corresponds to a clock tree setting */
	const clock_setpoint_t *states;
	/** Count of clock states for this instance */
	uint8_t state_cnt;
};

/**
 * @brief Clock callback reasons
 *
 * Reasons for a clock callback to occur. One of these will be passed to the
 * driver when a clock callback is issued, so that the driver can decide
 * what action to take
 */
enum clock_mgmt_cb_reason {
	/** Rate of the clock changed */
	CLOCK_MGMT_RATE_CHANGED,
	/** Clock started */
	CLOCK_MGMT_STARTED,
	/** Clock stopped */
	CLOCK_MGMT_STOPPED,
};

struct clock_mgmt_callback;

/**
 * @typedef clock_mgmt_callback_handler_t
 * @brief Define the application clock callback handler function signature
 *
 * @param cb Original struct clock_mgmt_callback owning this handler
 * @param reason Reason callback occurred
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct clock_mgmt_callback is stored in
 * another private structure.
 */
typedef void (*clock_mgmt_callback_handler_t)(struct clock_mgmt_callback *cb,
					      enum clock_mgmt_cb_reason reason);
/**
 * @brief Clock management callback structure
 *
 * Used to register a callback for clock change events in the driver consuming
 * the clock. As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct clock_mgmt_callback.
 * Beware such pointers must not be allocated on the stack.
 *
 * Note: to help setting the callback, clock_mgmt_init_callback() below
 */
struct clock_mgmt_callback {
	/** This is meant to be used in the driver and the user should not
	 * mess with it
	 */
	sys_snode_t node;

	/** Actual callback function being called when relevant */
	clock_mgmt_callback_handler_t handler;
};


/**
 * @brief Fire clock callback for a clock ID
 *
 * This macro issues a clock callback for a given clock identifier. If no
 * macros are registered for the clock identifier, the macro will simply
 * return
 * @param clock_id clock identifier to fire callback for
 * @param reason Reason clock callback has fired
 */
#define CLOCK_MGMT_FIRE_CALLBACK(clock_id, reason)                              \
	/* Extern symbols- defined by linker */                                 \
	extern sys_slist_t clock_callback_##clock_id##_start;			\
	extern sys_slist_t clock_callback_##clock_id##_end;			\
										\
	if (&clock_callback_##clock_id##_start !=                               \
	    &clock_callback_##clock_id##_end) {                                 \
		struct clock_mgmt_callback *cb, *tmp;                           \
										\
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&clock_callback_##clock_id##_start, \
				cb, tmp, node) {				\
			__ASSERT(cb->handler, "No callback handler!");          \
			cb->handler(cb, reason);                                \
		}                                                               \
	}

/**
 * @brief Fire clock callback for all clock IDs
 *
 * This macro issues a clock callback for all clock identifiers in the
 * system. It may be useful if a root clock source is changed.
 * @param reason Reason clock callback has fired
 */
#define CLOCK_MGMT_FIRE_ALL_CALLBACKS(reason)					\
	/* Extern symbols- defined by linker */					\
	extern sys_slist_t clock_callbacks_start;				\
	extern sys_slist_t clock_callbacks_end;					\
	sys_slist_t *curr = &clock_callbacks_start;				\
										\
	while (curr != &clock_callbacks_end) {					\
		struct clock_mgmt_callback *cb, *tmp;                           \
										\
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(curr,				\
				cb, tmp, node) {				\
			__ASSERT(cb->handler, "No callback handler!");          \
			cb->handler(cb, reason);                                \
		}                                                               \
		curr++;								\
	}

/**
 * @brief Defines clock management information for a given node identifier.
 *
 * This macro should be called during the device definition. It will define
 * and initialize the clock management configuration for the device represented
 * by node_id. Clock subsystems as well as clock management states will be
 * initialized by this macro.
 * @param node_id node identifier to define clock management data for
 */
#define CLOCK_MGMT_DEFINE(node_id)						\
	DT_FOREACH_PROP_ELEM_SEP(node_id, clocks,				\
				 Z_CLOCK_MGMT_CALLBACK_DEFINE, (;))		\
	DT_FOREACH_PROP_ELEM_SEP(node_id, clocks,				\
				Z_CLOCK_MGMT_SUBSYS_DEFINE, (;))		\
	LISTIFY(DT_NUM_CLOCK_MGMT_STATES(node_id),				\
		Z_CLOCK_MGMT_STATE_DEFINE, (;), node_id)			\
	static const clock_rate_t*						\
	_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _rates)[] = {			\
		DT_FOREACH_PROP_ELEM_SEP(node_id, clocks,			\
					 Z_CLOCK_MGMT_SUBSYS_INIT, (,))		\
	};									\
	static const clock_setpoint_t*						\
	_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _states)[] = {			\
		LISTIFY(DT_NUM_CLOCK_MGMT_STATES(node_id),			\
			Z_CLOCK_MGMT_STATE_INIT, (,), node_id)			\
	};									\
	static sys_slist_t*							\
	_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _cb)[] = {			\
		DT_FOREACH_PROP_ELEM_SEP(node_id, clocks,			\
					 Z_CLOCK_MGMT_CALLBACK_INIT, (,))	\
	};									\
	static const struct clock_mgmt						\
	_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _cfg) = {			\
		.rates = (clock_rate_t *)					\
			_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _rates),	\
		.callbacks = _CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _cb),	\
		.clocks_cnt = DT_NUM_CLOCKS(node_id),				\
		.states = (clock_setpoint_t *)					\
			_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _states),	\
		.state_cnt = DT_NUM_CLOCK_MGMT_STATES(node_id),			\
	}

/**
 * @brief Defines clock management information for a given driver instance
 * Equivalent to CLOCK_MGMT_DEFINE(DT_DRV_INST(inst))
 * @param inst Driver instance number
 */
#define CLOCK_MGMT_DT_INST_DEFINE(inst) CLOCK_MGMT_DEFINE(DT_DRV_INST(inst))

/**
 * @brief Initializes clock management information for a given node identifier.
 *
 * This macro should be used during device initialization, in combination with
 * #CLOCK_MGMT_DEFINE. It will get a reference to the clock management
 * structure defined with #CLOCK_MGMT_DEFINE
 * For example, a driver could initialize clock
 * management information like so:
 * ```
 * struct vnd_device_cfg {
 *      ...
 *      struct clock_mgmt clock_mgmt;
 *      ...
 * };
 * ...
 * #define VND_DEVICE_INIT(node_id)                         \
 *      CLOCK_MGMT_DEFINE(node_id);                         \
 *                                                         \
 *      struct vnd_device_cfg cfg_##node_id = {             \
 *          .clock_mgmt = CLOCK_MGMT_INIT(node_id),         \
 *      }
 * ```
 * @param node_id: Node identifier to initialize clock management structure for
 */
#define CLOCK_MGMT_INIT(node_id) &_CONCAT(Z_CLOCK_MGMT_NODE_NAME(node_id), _cfg)

/**
 * @brief Initializes clock management information for a given driver instance
 *
 * Equivalent to CLOCK_MGMT_INIT(DT_DRV_INST(inst))
 * @param inst Driver instance number
 */
#define CLOCK_MGMT_DT_INST_INIT(inst) CLOCK_MGMT_INIT(DT_DRV_INST(inst))

/**
 * @brief Get clock rate for given subsystem
 *
 * Gets output clock rate for provided clock identifier. Clock identifiers
 * start with CLOCK_MGMT_SUBSYS_DEFAULT, and drivers can define additional
 * identifiers. Each identifier refers to the nth clock node in the clocks
 * devicetree property for the devicetree node representing this driver.
 * @param clk_cfg Clock management structure
 * @param clk_idx subsys clock index in clocks property
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if output id could not be found
 * @return -ENOTSUP if clock is not supported
 * @return -EIO if clock could not be read
 * @return frequency of clock output in HZ
 */
static inline int clock_mgmt_get_rate(const struct clock_mgmt *clk_cfg,
				      uint8_t clk_idx)
{
	if (!clk_cfg) {
		return -EINVAL;
	}

	if (clk_cfg->clocks_cnt <= clk_idx) {
		return -ENOENT;
	}

	/* Call clock management driver function */
	return clk_cfg->rates[clk_idx]();
}

/**
 * @brief Set new clock state
 *
 * Sets new clock state. This function will apply a clock state as defined
 * in devicetree. Clock states can configure clocks systemwide, or only for
 * the relevant peripheral driver. Clock states are defined as clock-state-"n"
 * properties of the devicetree node for the given driver.
 * @param clk_cfg Clock management structure
 * @param state_idx Clock state index
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if state index could not be found
 * @return -ENOTSUP if state is not supported by hardware
 * @return -EIO if state could not be set
 * @return -EBUSY if clocks cannot be modified at this time
 * @return 0 on success
 */
static inline int clock_mgmt_apply_state(const struct clock_mgmt *clk_cfg,
					 uint8_t state_idx)
{
	if (!clk_cfg) {
		return -EINVAL;
	}

	if (clk_cfg->state_cnt <= state_idx) {
		return -ENOENT;
	}

	/* Call clock management driver function */
	return clk_cfg->states[state_idx]();
}


/** @cond INTERNAL_HIDDEN */

/**
 * @brief Helper to add or remove clock control callback
 * @param callbacks A pointer to the original list of callbacks
 * @param callback A pointer of the callback to insert or remove from the list
 * @param set A boolean indicating insertion or removal of the callback
 */
static inline int clock_mgmt_manage_callback(sys_slist_t *callbacks,
					     struct clock_mgmt_callback *callback,
					     bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	} else if (!set) {
		return -EINVAL;
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}

/** @endcond */

/**
 * @brief Helper to initialize a struct clock_mgmt_callback properly
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 */
static inline void clock_mgmt_init_callback(struct clock_mgmt_callback *callback,
					    clock_mgmt_callback_handler_t handler)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
}

/**
 * @brief Add application clock callback for a given subsystem
 * @param clk_cfg Clock management structure
 * @param clk_idx Subsystem clock index in clocks property
 * @param callback A valid Application's callback structure pointer.
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if subsystem index could not be found
 * @return 0 on success
 */
static inline int clock_mgmt_add_callback(const struct clock_mgmt *clk_cfg,
					  uint8_t clk_idx,
					  struct clock_mgmt_callback *callback)
{
	if (!clk_cfg || !callback) {
		return -EINVAL;
	}

	if (clk_cfg->clocks_cnt <= clk_idx) {
		return -ENOENT;
	}

	/* Add callback to sys_slist_t */
	return clock_mgmt_manage_callback(clk_cfg->callbacks[clk_idx],
					  callback, true);
}

/**
 * @brief Remove application clock callback for a given subsystem
 * @param clk_cfg Clock management structure
 * @param clk_idx Subsystem clock index in clocks property
 * @param callback A valid Application's callback structure pointer.
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if subsystem index could not be found
 * @return 0 on success
 */
static inline int clock_mgmt_remove_callback(const struct clock_mgmt *clk_cfg,
					     uint8_t clk_idx,
					     struct clock_mgmt_callback *callback)
{
	if (!clk_cfg || !callback) {
		return -EINVAL;
	}

	if (clk_cfg->clocks_cnt <= clk_idx) {
		return -ENOENT;
	}

	/* Remove callback from sys_slist_t */
	return clock_mgmt_manage_callback(clk_cfg->callbacks[clk_idx],
					  callback, false);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_H_ */
