/*
 * Copyright 2024 NXP
 * Copyright (c) 2025 Tenstorrent AI ULC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for clock management
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_H_

/**
 * @brief Clock Management Interface
 * @defgroup clock_management_interface Clock management Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <zephyr/kernel.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clock management event types
 *
 * Types of events the clock management framework can generate for consumers.
 */
enum clock_management_event_type {
	/**
	 * Clock is about to change from frequency given by
	 * `old_rate` to `new_rate`
	 */
	CLOCK_MANAGEMENT_PRE_RATE_CHANGE,
	/**
	 * Clock has just changed from frequency given by
	 * `old_rate` to `new_rate`
	 */
	CLOCK_MANAGEMENT_POST_RATE_CHANGE,
	/**
	 * Used to query if a clock can support a new rate.
	 * Consumers should return 0 if they can support the new rate, or
	 * -ENOTSUP if they cannot support the new rate, or -EBUSY if they
	 * cannot support the new rate at this time.
	 */
	CLOCK_MANAGEMENT_QUERY_RATE_CHANGE
};

/**
 * @brief Clock notification event structure
 *
 * Notification of clock rate change event. Consumers may examine this
 * structure to determine what rate a clock will change to, as
 * well as to determine if a clock is about to change rate or has already
 */
struct clock_management_event {
	/** Type of event */
	enum clock_management_event_type type;
	/** Old clock rate */
	clock_freq_t old_rate;
	/** New clock rate */
	clock_freq_t new_rate;
};

/**
 * @typedef clock_management_callback_handler_t
 * @brief Define the application clock callback handler function signature
 *
 * @param ev Clock management event
 * @param user_data User data set by consumer
 * @return 0 if consumer can accept the new parent rate
 * @return -ENOTSUP if consumer cannot accept the new parent rate
 * @return -EBUSY if the consumer does not permit clock changes at this time
 */
typedef int (*clock_management_callback_handler_t)(const struct clock_management_event *ev,
					     const void *user_data);

/**
 * @brief Request for a clock node
 *
 * Request for a specific clock node. Defines the clock node, and
 * the allowed states from the request
 */
struct clk_request {
	/** Clock node to request */
	const struct clk *clk;
	/** Bitmask of acceptable state indices */
	uint32_t allowed_states;
};

/**
 * @brief Clock Management Request
 *
 * Clock management request structure, used for passing a request for a new
 * state to clock framework. This structure is opaque to consumers, and should
 * only be used with the @ref clock_management_request_state API.
 */
struct clock_management_request {
	/** Does this request lock the clocks it affects? */
	bool locking;
	/** Number of clocks in the request */
	uint8_t num_clks;
	/** Array of requests for clocks */
	const struct clk_request clk_reqs[];
};

/**
 * @brief Clock management callback data
 *
 * Describes clock management callback data. Drivers should not directly access
 * or modify these fields.
 */
struct clock_management_callback {
	/** Callback handler function */
	clock_management_callback_handler_t clock_callback;
	/** User data to issue with callback */
	const void *user_data;
};

/**
 * @brief Clock output structure
 *
 * This structure describes a clock output node. The user should
 * not initialize a clock output directly, but instead reference it
 * using @ref clock_output_t
 */
struct clock_output {
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Clock management callback */
	struct clock_management_callback *cb;
#endif
	/** Internal clock structure for output clock */
	const struct clk *clk_core;
};

/**
 * @brief Clock output identifier
 */
typedef uint8_t clock_output_t;

/**
 * @brief Clock request identifier
 */
typedef uint8_t clock_request_t;

/**
 * @brief Clock Management Data Structure
 *
 * Data structure used by the clock management subsystem. This structure
 * is defined per-consumer, and records pointers to all clock outputs
 * as well a clock requests. It is also used by the clock management
 * subsystem for state tracking.
 */
struct clock_management_data {
	/** Array of clock outputs declared by the consumer */
	const struct clock_output *const *clock_outputs;
	/** Array of clock requests declared by the consumer */
	const struct clock_management_request *const *clock_requests;
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Index of the active clock request */
	int8_t *active_request;
#endif
	/** Number of clock outputs */
	uint8_t num_outputs;
	/** Number of clock requests */
	uint8_t num_requests;
};


/** @cond INTERNAL_HIDDEN */

/**
 * @brief Clock management callback name
 * @param symname Base symbol name for variables related to this clock output
 */
#define Z_CLOCK_MANAGEMENT_CALLBACK_NAME(symname)                                    \
	_CONCAT(symname, _clock_callback)

/**
 * @brief Provides symbol name for clock output object
 * @param symname Base symbol name for variables related to this clock output
 */
#define Z_CLOCK_MANAGEMENT_OUTPUT_NAME(symname) _CONCAT(symname, _clock_management_output)

/**
 * @brief Provides a section name for clock outputs
 *
 * In order to support callbacks with clock outputs, we must provide a method to
 * define place clock outputs in a section with a standard name based on the
 * node ID of the clock producer this clock output wishes to subscribe to.
 * @param node_id Node identifier for the clock node to define an output for
 */
#define Z_CLOCK_OUTPUT_SECTION_NAME(node_id)                                   \
	_CONCAT(.clock_output_, DT_DEP_ORD(node_id))

/**
 * @brief Provides a symbol name for clock outputs
 * @param node_id Node identifier for the clock node to define an output for
 * @param suffix Unique (within scope of file) suffix for symbol name
 */
#define Z_CLOCK_OUTPUT_SYMBOL_NAME(node_id, suffix)                            \
	CONCAT(clock_output_, DT_DEP_ORD(node_id), _, suffix)

/**
 * @brief Define clock output structure
 *
 * Defines a clock output structure, given a section and symbol base name to use
 * for the clock output
 * @param node_id Node identifier for the clock node to define an output for
 * @param secname Section name to place clock output structure into
 * @param symname Base symbol name for variables related to this clock output
 */
#define Z_CLOCK_MANAGEMENT_DEFINE_OUTPUT(node_id, secname, symname)                  \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(node_id, clock_output),                \
	"Nodes used as a clock output must have the clock-output compatible"); \
	/* We only actually need to define clock output objects if runtime */  \
	/* features are enabled */                                             \
	IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME, (                                \
	/* Clock management callback structure, stored in RAM */               \
	struct clock_management_callback Z_CLOCK_MANAGEMENT_CALLBACK_NAME(symname);        \
	/* Define output clock structure */                                    \
	static const Z_DECL_ALIGN(struct clock_output)                         \
	Z_GENERIC_SECTION(secname) Z_CLOCK_MANAGEMENT_OUTPUT_NAME(symname) = {       \
		.clk_core = CLOCK_DT_GET(node_id),                             \
		.cb = &Z_CLOCK_MANAGEMENT_CALLBACK_NAME(symname),                    \
	};))

/**
 * @brief Clock management request name
 *
 * @param node_id identifier of the node that defines the clock-request-n property
 * @param request_name Name of the clock request property
 */
#define Z_CLOCK_MANAGEMENT_REQ_NAME(node_id, request_name) \
	CONCAT(clk_req_, DT_DEP_ORD(node_id), _, request_name)

/**
 * @brief Define a clock request structure from a "clock-request-n" property
 *
 * Defines the C structures associated with a "clock-request-n" property
 * @param node_id Node identifier for the device defining the clock request
 * @param prop Name of the clock request property
 * @param idx Index of the clock request name in the clock-request-names property
 */
#define Z_CLOCK_MANAGEMENT_DT_DEFINE_REQUEST(node_id, prop, idx) \
	const struct clock_management_request \
	Z_CLOCK_MANAGEMENT_REQ_NAME(node_id, DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)) = { \
		.locking = DT_PROP(node_id, CONCAT(clock_request_, \
				DT_CLOCK_REQUEST_NAME_IDX(node_id, \
				DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)), \
				_locking)), \
		.num_clks = DT_CLOCK_REQUEST_LEN_BY_NAME(node_id, \
			DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)), \
		.clk_reqs = { DT_CLOCK_REQUEST_FOREACH_BY_NAME(node_id, \
			DT_STRING_TOKEN_BY_IDX(node_id, prop, idx), \
			DT_GET_CLOCK_REQUEST) }, \
	}

/**
 * @brief Get a clock request structure from a "clock-request-n" property
 *
 * Gets the C structures associated with a "clock-request-n" property
 * @param node_id Node identifier for the device defining the clock request
 * @param prop Name of the clock request property
 * @param idx Index of the clock request name in the clock-request-names property
 */
#define Z_CLOCK_MANAGEMENT_DT_GET_REQUEST(node_id, prop, idx) \
	&Z_CLOCK_MANAGEMENT_REQ_NAME(node_id, DT_STRING_TOKEN_BY_IDX(node_id, prop, idx))

/**
 * @brief Gets the phandle for a clock-output given the clock output name
 *
 * Helper to get the phandle for a clock-output given the node and clock output
 * name
 * @param node_id Node identifier for the device defining the clock output
 * @param prop Name of the clock output property
 * @param idx Index of the clock output name in the clock-output-names property
 */
#define Z_CLOCK_MANAGEMENT_CLK_OUTPUT_PHANDLE(node_id, prop, idx) \
	DT_PHANDLE_BY_IDX(node_id, clock_outputs, \
		DT_CLOCK_OUTPUT_NAME_IDX(node_id, \
		DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)))

/**
 * @brief Define clock output from clock-outputs property
 *
 * Defines a clock output using the clock-outputs devicetree property
 * for a consumer
 * @param node_id Node identifier for the device defining the clock output
 * @param prop Name of the clock output property
 * @param idx Index of the clock output name in the clock-output-names property
 */
#define Z_CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT(node_id, prop, idx) \
		Z_CLOCK_MANAGEMENT_DEFINE_OUTPUT( \
			Z_CLOCK_MANAGEMENT_CLK_OUTPUT_PHANDLE(node_id, prop, idx), \
			Z_CLOCK_OUTPUT_SECTION_NAME( \
				Z_CLOCK_MANAGEMENT_CLK_OUTPUT_PHANDLE(node_id, prop, idx)), \
			Z_CLOCK_OUTPUT_SYMBOL_NAME( \
				Z_CLOCK_MANAGEMENT_CLK_OUTPUT_PHANDLE(node_id, prop, idx), \
				DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)))

/**
 * @brief Gets a clock output for a clock node within the system clock tree
 *
 * Gets a previously defined clock output for a clock node. This macro should be
 * used when defining a clock output for access outside of device drivers,
 * devices described in devicetree should use @ref CLOCK_MANAGEMENT_DT_GET_OUTPUT.
 * Before using this macro, @ref CLOCK_MANAGEMENT_DEFINE_OUTPUT should be used to
 * define the output clock, with the same value for @p name
 * @param node_id Node identifier for the clock node to get the output for
 * @param name Software defined name for this clock output
 */
#define Z_CLOCK_MANAGEMENT_GET_OUTPUT(node_id, name)                                   \
	/* We only actually define output objects if runtime clocking is on */ \
	COND_CODE_1(CONFIG_CLOCK_MANAGEMENT_RUNTIME, (                               \
	&Z_CLOCK_MANAGEMENT_OUTPUT_NAME(Z_CLOCK_OUTPUT_SYMBOL_NAME(node_id, name))), \
	((const struct clock_output *)CLOCK_DT_GET(node_id)))

/**
 * @brief Get clock output from clock-outputs property
 *
 * Gets a clock output using the clock-outputs devicetree property
 * for a consumer
 * @param node_id Node identifier for the device defining the clock output
 * @param prop Name of the clock output property
 * @param idx Index of the clock output name in the clock-output-names property
 */
#define Z_CLOCK_MANAGEMENT_DT_GET_OUTPUT(node_id, prop, idx) \
		Z_CLOCK_MANAGEMENT_GET_OUTPUT( \
			Z_CLOCK_MANAGEMENT_CLK_OUTPUT_PHANDLE(node_id, prop, idx), \
			DT_STRING_TOKEN_BY_IDX(node_id, prop, idx))

/** @endcond */

/** Indicates there is no active request for the clock consumer */
#define CLOCK_MANAGEMENT_NO_REQUEST (-1)

/**
 * @brief Define data structures associated with a clock management consumer
 *
 * This macro should be called by any device consumer before utilizing the
 * clock management subsystem, in order to define the data structures required
 * to use it
 *
 * @param dev_node Node identifier for the device consumer
 */
#define CLOCK_MANAGEMENT_DT_DEFINE(dev_node)                                   \
		IF_ENABLED(DT_NODE_HAS_PROP(dev_node, clock_output_names), \
		(DT_FOREACH_PROP_ELEM_SEP(dev_node, clock_output_names, \
			Z_CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT, (;)))); \
		IF_ENABLED(DT_NODE_HAS_PROP(dev_node, clock_request_names), \
		(DT_FOREACH_PROP_ELEM_SEP(dev_node, clock_request_names, \
			Z_CLOCK_MANAGEMENT_DT_DEFINE_REQUEST, (;)))); \
		const struct clock_output *const \
		CONCAT(clock_outputs_, DT_DEP_ORD(dev_node))[] = { \
			IF_ENABLED(DT_NODE_HAS_PROP(dev_node, clock_output_names), \
			(DT_FOREACH_PROP_ELEM_SEP(dev_node, clock_output_names, \
				Z_CLOCK_MANAGEMENT_DT_GET_OUTPUT, (,)))) \
		}; \
		const struct clock_management_request *const \
		CONCAT(clock_requests_, DT_DEP_ORD(dev_node))[] = { \
			IF_ENABLED(DT_NODE_HAS_PROP(dev_node, clock_request_names), \
			(DT_FOREACH_PROP_ELEM_SEP(dev_node, clock_request_names, \
				Z_CLOCK_MANAGEMENT_DT_GET_REQUEST, (,)))) \
		}; \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME, ( \
			static int8_t CONCAT(active_request_, DT_DEP_ORD(dev_node)) = \
				CLOCK_MANAGEMENT_NO_REQUEST; \
		)); \
		const struct clock_management_data \
		CONCAT(clock_mgmt_data_, DT_DEP_ORD(dev_node)) = { \
			.clock_outputs = CONCAT(clock_outputs_, DT_DEP_ORD(dev_node)), \
			.clock_requests = CONCAT(clock_requests_, DT_DEP_ORD(dev_node)), \
			.num_outputs = DT_PROP_LEN_OR(dev_node, clock_output_names, 0), \
			.num_requests = DT_PROP_LEN_OR(dev_node, clock_request_names, 0), \
			IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME, ( \
				.active_request = &CONCAT(active_request_, DT_DEP_ORD(dev_node)), \
			)) \
		};
/**
 * @brief Define data structures for an instance of a clock management consumer
 *
 * This macro should be called by any device consumer before utilizing the
 * clock management subsystem. It is equivalent to
 * CLOCK_MANAGEMENT_DT_DEFINE(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 */
#define CLOCK_MANAGEMENT_DT_INST_DEFINE(inst)                                   \
		CLOCK_MANAGEMENT_DT_DEFINE(DT_DRV_INST(inst))

/**
 * @brief Get data structure for a clock management consumer
 *
 * Gets a pointer to the clock management data structure previously defined
 * using CLOCK_MANAGEMENT_DT_DEFINE
 * @param dev_node Node identifier for the clock management consumer
 */
#define CLOCK_MANAGEMENT_DT_GET(dev_node) \
		&CONCAT(clock_mgmt_data_, DT_DEP_ORD(dev_node))

/**
 * @brief Get data structure for a clock management consumer at DT_DRV_COMPAT instance
 *
 * Gets a pointer to the clock management data structure previously defined
 * using CLOCK_MANAGEMENT_DT_INST_DEFINE
 * @param inst DT_DRV_COMPAT instance number
 */
#define CLOCK_MANAGEMENT_DT_INST_GET(inst) \
		CLOCK_MANAGEMENT_DT_GET(DT_DRV_INST(inst))

/**
 * @brief Gets a clock output for system clock node at with name @p name in
 * "clock-outputs" property on device with node ID @p dev_node
 *
 * Gets a clock output for the system clock node with name @p name in the
 * device's "clock-outputs" property.
 * @param dev_node Device node with a clock-outputs property.
 * @param name Name of the clock output
 */
#define CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(dev_node, name)                       \
	DT_CLOCK_OUTPUT_NAME_IDX(dev_node, name)

/**
 * @brief Gets a clock output for system clock node at with name @p name in
 * "clock-outputs" property on instance @p inst of DT_DRV_COMPAT
 *
 * Gets a clock output for the system clock node with name @p name in the
 * device's "clock-outputs" property.
 * @param inst DT_DRV_COMPAT instance number
 * @param name Name of the clock output
 */
#define CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_NAME(inst, name)                      \
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Gets a clock output for system clock node at index @p idx in
 * "clock-outputs" property on device with node ID @p dev_node
 *
 * Gets a clock output for the system clock node with index @p idx in the
 * device's "clock-outputs" property.
 * @param dev_node Device node with a clock-outputs property.
 * @param idx Index within the "clock-outputs" property
 */
#define CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_IDX(dev_node, idx) (idx)

/**
 * @brief Gets a clock output for system clock node at index @p idx in
 * "clock-outputs" property on instance @p inst of DT_DRV_COMPAT
 *
 * Gets a clock output for the system clock node with index @p idx in the
 * device's "clock-outputs" property.
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Index within the "clock-outputs" property
 */
#define CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_IDX(inst, idx)                        \
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a clock output for a device described in devicetree by @p dev_node
 *
 * Gets a clock output for device described in devicetree. The output will be
 * retrievd from the first phandle in the node's "clock-outputs" property.
 * Note this is equivalent to
 * CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_IDX(dev_node, 0)
 * @param dev_node Device node with a clock-outputs property.
 */
#define CLOCK_MANAGEMENT_DT_GET_OUTPUT(dev_node)                                     \
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_IDX(dev_node, 0)

/**
 * @brief Get clock output for instance @p inst of a DT_DRV_COMPAT
 *
 * Gets a clock output for device described in devicetree. The output will be
 * retrievd from the first phandle in the node's "clock-outputs" property.
 * Note this is equivalent to
 * CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 */
#define CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT(inst)                                    \
	CLOCK_MANAGEMENT_DT_GET_OUTPUT(DT_DRV_INST(inst))

/**
 * @brief Get a clock request from a "clock-request-n" property
 *
 * Gets a clock request from a "clock-request-n" property, given
 * the name of the state.
 *
 * For example, for the following devicetree definition:
 * @code{.dts}
 * &hs_clock {
 *      hsclk_state0: state0 {
 *              compatible = "clock-state";
 *              clocks = <...>;
 *              clock-frequency = <...>;
 *      };
 *      hsclk_state1: state1 {
 *              compatible = "clock-state";
 *              clocks = <...>;
 *              clock-frequency = <...>;
 *      };
 * };
 *
 * &lp_clock {
 *      lpclk_state0: state0 {
 *              compatible = "clock-state";
 *              clocks = <...>;
 *              clock-frequency = <...>;
 *      };
 *      lpclk_state1: state1 {
 *              compatible = "clock-state";
 *              clocks = <...>;
 *              clock-frequency = <...>;
 *      };
 * };
 * my_dev: mydev@0 {
 *         compatible = "vnd,device";
 *         reg = <0>;
 *         clock-outputs = <&hs_clock> <&lp_clock>;
 *         clock-output-names = "highspeed", "low-power"
 *         clock-request-0 = <&hsclk_state0 &lpclk_state0>;
 *         clock-request-1  = <&hsclk_state1 &lpclk_state1>;
 *         clock-state-names = "active", "sleep";
 * };
 * @endcode
 * The clock state identifiers could be accessed like so:
 * @code{.c}
 *     // Get identifier to apply sleep request
 *     CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(my_dev), sleep)
 *     // Get identifier to apply active request
 *     CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(my_dev), active)
 * @endcode
 * @param dev_id Node identifier for device with "clock-outputs" property
 * @param request_name Name of clock request to get for this clock output
 */
#define CLOCK_MANAGEMENT_DT_GET_REQUEST(dev_id, request_name) \
	DT_CLOCK_REQUEST_NAME_IDX(dev_id, request_name)

/**
 * @brief Get a clock request from a "clock-request-n" property
 *
 * Gets a clock request from a "clock-request-n" property, given the name
 * of the request. Note this is equivalent
 * to CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_DRV_INST(inst), request_name)
 * @param inst DT_DRV_COMPAT instance number
 * @param request_name Name of clock request to get for this clock output
 */
#define CLOCK_MANAGEMENT_DT_INST_GET_REQUEST(inst, request_name)                     \
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_DRV_INST(inst), request_name)

/**
 * @brief Get clock rate for given output
 *
 * Gets output clock rate in Hz for provided clock output.
 * @param data Clock management data structure for the device
 * @param clk Clock output to read rate of
 * @return -EINVAL if parameters are invalid
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return frequency of clock output in HZ
 */
int clock_management_get_rate(const struct clock_management_data *data, clock_output_t clk);

/**
 * @brief Request a frequency for the clock output
 *
 * Requests a new rate for a clock output. The clock will configure to
 * the closest available state to the requested frequency. Requires
 * `CONFIG_CLOCK_MANAGEMENT_SET_RATE` to be set.
 * @param data Clock management data structure for the device
 * @param clk Clock output to request rate for
 * @param freq Rate request for clock output
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if request could not be satisfied
 * @return -EPERM if clock is not configurable
 * @return -EIO if configuration of a clock failed
 * @return -ENOTSUP if clock management set rate is not supported
 * @return frequency of clock output in HZ on success
 */
int clock_management_req_rate(const struct clock_management_data *data,
			      clock_output_t clk, clock_freq_t freq);

/**
 * @brief Request a clock state
 *
 * Request a clock state. This request may apply to multiple clock outputs,
 * and is defined in devicetree using the "clock-request-n" property.
 * @param data Clock management data structure for the device
 * @param request Clock management request to apply
 * @return -EIO if configuration of a clock failed
 * @return -EINVAL if parameters are invalid
 * @return -EPERM if clock is not configurable
 * @return 0 on success
 */
int clock_management_request_state(const struct clock_management_data *data,
				   clock_request_t request);

/**
 * @brief Set callback for clock output reconfiguration
 *
 * Set callback, which will fire when a clock output (or any of its parents) are
 * reconfigured. A negative return value from this callback will prevent the
 * clock from being reconfigured.
 * @param data Clock management data structure for the device
 * @param clk Clock output to add callback for
 * @param callback Callback function to install
 * @param user_data User data to issue with callback (can be NULL)
 * @return -EINVAL if parameters are invalid
 * @return -ENOTSUP if callbacks are not supported
 * @return 0 on success
 */
static inline int clock_management_set_callback(const struct clock_management_data *data,
					clock_output_t clk,
					clock_management_callback_handler_t callback,
					const void *user_data)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if (!callback) {
		return -EINVAL;
	}

	if (clk >= data->num_outputs) {
		return -EINVAL;
	}

	extern k_spinlock_key_t spinlock_key;
	extern struct k_spinlock clock_mgmt_spinlock;

	spinlock_key = k_spin_lock(&clock_mgmt_spinlock);

	data->clock_outputs[clk]->cb->clock_callback = callback;
	data->clock_outputs[clk]->cb->user_data = user_data;
	k_spin_unlock(&clock_mgmt_spinlock, spinlock_key);
	return 0;
#else
	return -ENOTSUP;
#endif
}

/**
 * @brief Disable unused clocks within the system
 *
 * Disable unused clocks within the system. This API will gate all clocks in
 * the system with a usage count of zero, when CONFIG_CLOCK_MANAGEMENT_RUNTIME
 * is enabled.
 */
void clock_management_disable_unused(void);

/**
 * @brief Enable a clock output and its sources
 *
 * Turns a clock output and its sources on. This function will
 * unconditionally enable the clock and its sources.
 * @param data Clock management data structure for the device
 * @param clk clock output to turn on
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned on
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
int clock_management_on(const struct clock_management_data *data, clock_output_t clk);

/**
 * @brief Disable a clock output and its sources
 *
 * Turns a clock output and its sources off. This function will
 * unconditionally disable the output and its sources.
 * @param data Clock management data structure for the device
 * @param clk clock output to turn off
 * @return -ENOSYS if clock does not implement on_off API
 * @return -EIO if clock could not be turned off
 * @return -EBUSY if clock cannot be modified at this time
 * @return negative errno for other error turning clock on or off
 * @return 0 on success
 */
int clock_management_off(const struct clock_management_data *data, clock_output_t clk);

/**
 * @brief Lock a clock output to block further reconfiguration
 *
 * Locks a clock output, preventing any reconfiguration from occurring to the
 * clock until @ref clock_management_unlock is called by the same clock consumer.
 * @param data Clock management data structure for the device
 * @param clk clock output to lock
 * @return 0 on success
 * @return -EBUSY if the clock is already locked by another consumer
 */
int clock_management_lock(const struct clock_management_data *data, clock_output_t clk);

/**
 * @brief Unlock a clock output to allow further reconfiguration
 *
 * Unlocks a clock output, allowing any reconfiguration to occur to the
 * clock. Should only be called by a consumer that has previously locked the
 * clock using @ref clock_management_lock.
 * @param data Clock management data structure for the device
 * @param clk clock output to unlock
 * @return 0 on success
 * @return -EPERM if the clock is not locked by the calling consumer
 */
int clock_management_unlock(const struct clock_management_data *data, clock_output_t clk);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_H_ */
