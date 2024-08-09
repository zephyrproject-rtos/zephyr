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

#include <zephyr/drivers/clock_mgmt/clock_driver.h>
#include <errno.h>
#if defined(CONFIG_CLOCK_MGMT)
/* Include headers for clock management drivers */
#include <clock_mgmt_drivers.h>
#endif

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
 * @name Clock Output Names
 * @{
 */

/** Default clock output */
#define CLOCK_MGMT_OUTPUT_DEFAULT 0U
/** This and higher values refer to custom private clock outputs */
#define CLOCK_MGMT_OUTPUT_PRIV_START 1U

/** @} */

/**
 * @typedef clock_mgmt_callback_handler_t
 * @brief Define the application clock callback handler function signature
 *
 * @param output_idx Index of output that was reconfigured in clocks property
 * @param rate New rate clock will use once this callback completes
 * @param user_data User data set by consumer
 * @return 0 if consumer can accept the new parent rate
 * @return -ENOTSUP if consumer cannot accept the new parent rate
 * @return -EBUSY if the consumer does not permit clock changes at this time
 */
typedef int (*clock_mgmt_callback_handler_t)(uint8_t output_idx,
					     uint32_t rate,
					     const void *user_data);

/**
 * @brief Clock management callback data
 *
 * Describes clock management callback data. Drivers should not directly access
 * or modify these fields.
 */
struct clock_mgmt_callback {
	clock_mgmt_callback_handler_t clock_callback;
	const void *user_data;
};

/**
 * @brief Clock management state data
 *
 * Describes clock management state data. Drivers should not directly access
 * or modify these fields.
 */
struct clock_mgmt_state {
	/** Clocks to configure for this state */
	const struct clk *const *clocks;
	/** Clock data tokens to pass to each clock */
	const void **clock_config_data;
	/** Number of clocks in this state */
	const uint8_t num_clocks;
};

/**
 * @brief Clock management configuration
 *
 * Describes clock management data for a device. Drivers should not access or
 * modify these fields.
 */
struct clock_mgmt {
	/** Clock outputs to read rates from */
	const struct clk *const *outputs;
	/** States to configure */
	const struct clock_mgmt_state *const *states;
#ifdef CONFIG_CLOCK_MGMT_NOTIFY
	/** Clock callback (one per clock management instance )*/
	struct clock_mgmt_callback *callback;
#endif
	/** Reference to the clock structure for this node */
	const struct clk *clk_hw;
	/** Count of clock outputs */
	const uint8_t output_count;
	/** Count of clock states */
	const uint8_t state_count;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Defines clock management data for a specific clock
 *
 * Defines clock management data for a clock, based on the clock's compatible
 * string. Given clock nodes with compatibles like so:
 *
 * @code{.dts}
 *     a {
 *             compatible = "vnd,source";
 *     };
 *
 *     b {
 *             compatible = "vnd,mux";
 *     };
 *
 *     c {
 *             compatible = "vnd,div";
 *     };
 * @endcode
 *
 * The clock driver must provide definitions like so:
 *
 * @code{.c}
 *     #define Z_CLOCK_MGMT_VND_SOURCE_DATA_DEFINE(node_id, prop, idx)
 *     #define Z_CLOCK_MGMT_VND_MUX_DATA_DEFINE(node_id, prop, idx)
 *     #define Z_CLOCK_MGMT_VND_DIV_DATA_DEFINE(node_id, prop, idx)
 * @endcode
 *
 * All macros take the node id of the node with the clock-state-i, the name of
 * the clock-state-i property, and the index of the phandle for this clock node
 * as arguments. The _DATA_DEFINE macros should initialize any data structure
 * needed by the clock.
 *
 * @param node_id Node identifier
 * @param prop clock property name
 * @param idx property index
 */
#define Z_CLOCK_MGMT_CLK_DATA_DEFINE(node_id, prop, idx)                       \
	_CONCAT(_CONCAT(Z_CLOCK_MGMT_, DT_STRING_UPPER_TOKEN(                  \
		DT_PHANDLE_BY_IDX(node_id, prop, idx), compatible_IDX_0)),     \
		_DATA_DEFINE)(node_id, prop, idx);

/**
 * @brief Gets clock management data for a specific clock
 *
 * Reads clock management data for a clock, based on the clock's compatible
 * string. Given clock nodes with compatibles like so:
 *
 * @code{.dts}
 *     a {
 *             compatible = "vnd,source";
 *     };
 *
 *     b {
 *             compatible = "vnd,mux";
 *     };
 *
 *     c {
 *             compatible = "vnd,div";
 *     };
 * @endcode
 *
 * The clock driver must provide definitions like so:
 *
 * @code{.c}
 *     #define Z_CLOCK_MGMT_VND_SOURCE_DATA_GET(node_id, prop, idx)
 *     #define Z_CLOCK_MGMT_VND_MUX_DATA_GET(node_id, prop, idx)
 *     #define Z_CLOCK_MGMT_VND_DIV_DATA_GET(node_id, prop, idx)
 * @endcode
 *
 * All macros take the node id of the node with the clock-state-i, the name of
 * the clock-state-i property, and the index of the phandle for this clock node
 * as arguments.
 * The _DATA_GET macros should get a reference to the clock data structure
 * data structure, which will be cast to a void pointer by the clock management
 * subsystem.
 * @param node_id Node identifier
 * @param prop clock property name
 * @param idx property index
 */
#define Z_CLOCK_MGMT_CLK_DATA_GET(node_id, prop, idx)                          \
	(void *)_CONCAT(_CONCAT(Z_CLOCK_MGMT_, DT_STRING_UPPER_TOKEN(          \
		DT_PHANDLE_BY_IDX(node_id, prop, idx), compatible_IDX_0)),     \
		_DATA_GET)(node_id, prop, idx)

/**
 * @brief Gets pointer to @ref clk structure for a given node property index
 * @param node_id Node identifier
 * @param prop clock property name
 * @param idx property index
 */
#define Z_CLOCK_MGMT_GET_REF(node_id, prop, idx)                               \
	CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

/**
 * @brief Clock outputs array name
 * @param node_id Node identifier
 */
#define Z_CLOCK_MGMT_OUTPUTS_NAME(node_id)                                     \
	_CONCAT(Z_CLOCK_DT_CLK_ID(node_id), _clock_outputs)

/**
 * @brief Individual clock state name
 * @param node_id Node identifier
 * @param idx clock state index
 */
#define Z_CLOCK_MGMT_STATE_NAME(node_id, idx)                                  \
	_CONCAT(_CONCAT(Z_CLOCK_DT_CLK_ID(node_id), _clock_state_), idx)

/**
 * @brief Clock states array name
 * @param node_id Node identifier
 */
#define Z_CLOCK_MGMT_STATES_NAME(node_id)                                      \
	_CONCAT(Z_CLOCK_DT_CLK_ID(node_id), _clock_states)

/**
 * @brief Clock management structure name
 * @param node_id Node identifier
 */
#define Z_CLOCK_MGMT_NAME(node_id)                                             \
	_CONCAT(Z_CLOCK_DT_CLK_ID(node_id), _clock_mgmt)

/**
 * @brief Clock management callback name
 * @param node_id Node identifier
 */
#define Z_CLOCK_MGMT_CALLBACK_NAME(node_id)                                    \
	_CONCAT(Z_CLOCK_DT_CLK_ID(node_id), _clock_callback)
/**
 * @brief Defines a clock management state for a given index
 * @param idx State index to define
 * @param node_id node identifier state is defined on
 */
#define Z_CLOCK_MGMT_STATE_DEFINE(idx, node_id)                                \
	const struct clk *const                                                \
		_CONCAT(Z_CLOCK_MGMT_STATE_NAME(node_id, idx), _clocks)[] = {  \
			DT_FOREACH_PROP_ELEM_SEP(node_id,                      \
			_CONCAT(clock_state_, idx), Z_CLOCK_MGMT_GET_REF, (,)) \
	};                                                                     \
	DT_FOREACH_PROP_ELEM_SEP(node_id,                                      \
	_CONCAT(clock_state_, idx),                                            \
	Z_CLOCK_MGMT_CLK_DATA_DEFINE, (;))                                     \
	const void *CONCAT(Z_CLOCK_MGMT_STATE_NAME(node_id, idx),              \
		_clock_data)[] = {                                             \
			DT_FOREACH_PROP_ELEM_SEP(node_id,                      \
			_CONCAT(clock_state_, idx),                            \
			Z_CLOCK_MGMT_CLK_DATA_GET, (,))                        \
	};                                                                     \
	const struct clock_mgmt_state                                          \
		Z_CLOCK_MGMT_STATE_NAME(node_id, idx) = {                      \
		.clocks = _CONCAT(                                             \
			Z_CLOCK_MGMT_STATE_NAME(node_id, idx), _clocks),       \
		.clock_config_data = _CONCAT(                                  \
			Z_CLOCK_MGMT_STATE_NAME(node_id, idx), _clock_data),   \
		.num_clocks = DT_PROP_LEN(node_id, _CONCAT(clock_state_, idx)),\
	};

/**
 * @brief Gets a reference to a clock management state for a given index
 * @param idx State index to define
 * @param node_id node identifier state is defined on
 */
#define Z_CLOCK_MGMT_STATE_GET(idx, node_id)                                   \
		&Z_CLOCK_MGMT_STATE_NAME(node_id, idx)                         \

/**
 * @brief Internal API definition for clock consumers
 *
 * Since clock consumers only need to implement the "notify" API, we use
 * a reduced API pointer. Since the notify field is the first entry in
 * both structures, we can still receive callbacks via this API structure.
 */
struct clock_mgmt_clk_api {
	/** Notify clock consumer of rate change */
	int (*notify)(const struct clk *clk, const struct clk *parent,
		      uint32_t parent_rate);
};

/**
 * @brief Define a @ref clk structure for clock consumer
 *
 * This structure is used to receive "notify" callbacks from the parent
 * clock devices, and will pass the notifications onto the clock consumer
 * @param node_id Node identifier to define clock structure for
 * @param data: Data to use for clock structure hw_data pointer
 * @param clk_id Clock identifier for the node
 */
#define Z_CLOCK_DEFINE_OUTPUT(node_id, data, clk_id)                           \
	/* API implemented in clock_mgmt_common.c */                           \
	extern struct clock_mgmt_clk_api clock_consumer_api;                   \
	CLOCK_DT_DEFINE(node_id, data,                                         \
		COND_CODE_1(CONFIG_CLOCK_MGMT_NOTIFY,                          \
			    ((struct clock_driver_api *)&clock_consumer_api),  \
			    NULL));


/** @endcond */

/**
 * @brief Defines clock management information for a given node identifier.
 *
 * This macro should be called during the device definition. It will define
 * and initialize the clock management configuration for the device represented
 * by node_id. Clock subsystems as well as clock management states will be
 * initialized by this macro.
 * @param node_id node identifier to define clock management data for
 */
#define CLOCK_MGMT_DEFINE(node_id)					       \
	/* Define clock outputs */                                             \
	const struct clk *const Z_CLOCK_MGMT_OUTPUTS_NAME(node_id)[] =         \
		{DT_FOREACH_PROP_ELEM_SEP(node_id, clock_outputs,              \
					Z_CLOCK_MGMT_GET_REF, (,))};           \
	/* Define clock states */                                              \
	LISTIFY(DT_NUM_CLOCK_MGMT_STATES(node_id),                             \
		Z_CLOCK_MGMT_STATE_DEFINE, (;), node_id);                      \
	/* Init clock state array */                                           \
	const struct clock_mgmt_state *Z_CLOCK_MGMT_STATES_NAME(node_id)[] = { \
		LISTIFY(DT_NUM_CLOCK_MGMT_STATES(node_id),                     \
			Z_CLOCK_MGMT_STATE_GET, (,), node_id)                  \
	};                                                                     \
	/* Clock management callback structure, stored in RAM */               \
	struct clock_mgmt_callback Z_CLOCK_MGMT_CALLBACK_NAME(node_id);        \
	/* Define clock management structure */                                \
	const struct clock_mgmt Z_CLOCK_MGMT_NAME(node_id) = {                 \
		.outputs = Z_CLOCK_MGMT_OUTPUTS_NAME(node_id),                 \
		.states = Z_CLOCK_MGMT_STATES_NAME(node_id),                   \
		IF_ENABLED(CONFIG_CLOCK_MGMT_NOTIFY,                           \
		(.callback = &Z_CLOCK_MGMT_CALLBACK_NAME(node_id),))           \
		.clk_hw = CLOCK_DT_GET(node_id),                               \
		.output_count = DT_PROP_LEN(node_id, clock_outputs),           \
		.state_count = DT_NUM_CLOCK_MGMT_STATES(node_id),              \
	};                                                                     \
	/* Define clock API and structure */                                   \
	Z_CLOCK_DEFINE_OUTPUT(node_id, &Z_CLOCK_MGMT_NAME(node_id),            \
			Z_CLOCK_DT_CLK_ID(node_id))



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
 *      const struct clock_mgmt *clock_mgmt;
 *      ...
 * };
 * ...
 * #define VND_DEVICE_INIT(node_id)                               \
 *      CLOCK_MGMT_DEFINE(node_id);                               \
 *                                                                \
 *      struct vnd_device_cfg cfg_##node_id = {                   \
 *          .clock_mgmt = CLOCK_MGMT_DT_DEV_CONFIG_GET(node_id),  \
 *      }
 * ```
 * @param node_id: Node identifier to initialize clock management structure for
 */
#define CLOCK_MGMT_DT_DEV_CONFIG_GET(node_id) &Z_CLOCK_MGMT_NAME(node_id)

/**
 * @brief Initializes clock management information for a given driver instance
 *
 * Equivalent to CLOCK_MGMT_DT_DEV_CONFIG_GET(DT_DRV_INST(inst))
 * @param inst Driver instance number
 */
#define CLOCK_MGMT_DT_INST_DEV_CONFIG_GET(inst)                                \
	CLOCK_MGMT_DT_DEV_CONFIG_GET(DT_DRV_INST(inst))

/**
 * @brief Get clock rate for given subsystem
 *
 * Gets output clock rate for provided clock identifier. Clock identifiers
 * start with CLOCK_MGMT_SUBSYS_DEFAULT, and drivers can define additional
 * identifiers. Each identifier refers to the nth clock node in the clocks
 * devicetree property for the devicetree node representing this driver.
 * @param clk_cfg Clock management structure
 * @param clk_idx Output clock index in clocks property
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if output id could not be found
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return frequency of clock output in HZ
 */
static inline int clock_mgmt_get_rate(const struct clock_mgmt *clk_cfg,
				      uint8_t clk_idx)
{
	if (!clk_cfg) {
		return -EINVAL;
	}

	if (clk_cfg->output_count <= clk_idx) {
		return -ENOENT;
	}

	/* Read rate */
	return clock_get_rate(clk_cfg->outputs[clk_idx]);
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
 * @return -ENOSYS if clock does not implement configure API
 * @return -EIO if state could not be set
 * @return -EBUSY if clocks cannot be modified at this time
 * @return 0 on success
 */
int clock_mgmt_apply_state(const struct clock_mgmt *clk_cfg,
			   uint8_t state_idx);

/**
 * @brief Set callback for clock reconfiguration
 *
 * Set callback, which will fire when a clock output (or any of its parents)
 * are reconfigured. The callback will fire with an index equal to
 * the clock output that fired the callback
 * @param clk_cfg Clock management structure
 * @param callback Callback function to install
 * @param user_data User data to issue with callback (can be NULL)
 * @return -EINVAL if parameters are invalid
 * @return -ENOTSUP if callbacks are not supported
 * @return 0 on success
 */
static inline int clock_mgmt_set_callback(const struct clock_mgmt *clk_cfg,
					  clock_mgmt_callback_handler_t callback,
					  const void *user_data)
{
#ifdef CONFIG_CLOCK_MGMT_NOTIFY
	if ((!clk_cfg) || (!callback)) {
		return -EINVAL;
	}

	clk_cfg->callback->clock_callback = callback;
	clk_cfg->callback->user_data = user_data;
	return 0;
#else
	return -ENOTSUP;
#endif
}

/**
 * @brief Disable unused clocks within the system
 *
 * Disable unused clocks within the system. This API will notify all clocks
 * of a configuration event, and clocks that are no longer in use
 * will gate themselves automatically
 */
static inline void clock_mgmt_disable_unused(void)
{
#ifdef CONFIG_CLOCK_MGMT_NOTIFY
	STRUCT_SECTION_FOREACH_ALTERNATE(clk_root, clk, clk) {
		/* Call clock_notify on each root clock. Clocks can use this
		 * notification event to determine if they are able
		 * to gate themselves
		 */
		clock_notify(clk, NULL, 0);
	}
#endif
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_H_ */
