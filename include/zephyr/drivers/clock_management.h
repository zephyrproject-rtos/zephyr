/*
 * Copyright 2024 NXP
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
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @typedef clock_management_state_t
 * @brief Define the clock management state identifier
 */
typedef uint8_t clock_management_state_t;

/**
 * @brief Clock management callback data
 *
 * Describes clock management callback data. Drivers should not directly access
 * or modify these fields.
 */
struct clock_management_callback {
	clock_management_callback_handler_t clock_callback;
	const void *user_data;
};

/**
 * @brief Clock rate request structure
 *
 * Clock rate request structure, used for passing a request for a new
 * frequency to a clock producer.
 */
struct clock_management_rate_req {
	/** Minimum acceptable frequency */
	int min_freq;
	/** Maximum acceptable frequency */
	int max_freq;
};

/**
 * @brief Clock output structure
 *
 * This structure describes a clock output node. The user should
 * not initialize a clock output directly, but instead define it using
 * @ref CLOCK_MANAGEMENT_DEFINE_OUTPUT or @ref CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT,
 * then get a reference to the output using @ref CLOCK_MANAGEMENT_GET_OUTPUT
 * or @ref CLOCK_MANAGEMENT_DT_GET_OUTPUT.
 */
struct clock_output {
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Clock management callback */
	struct clock_management_callback *cb;
	/** Parameters of the frequency request this output has on its clock */
	struct clock_management_rate_req *req;
#endif
	/** Internal clock structure for output clock */
	const struct clk *clk_core;
};


/** @cond INTERNAL_HIDDEN */

/**
 * @brief Clock management callback name
 * @param symname Base symbol name for variables related to this clock output
 */
#define Z_CLOCK_MANAGEMENT_CALLBACK_NAME(symname)                                    \
	_CONCAT(symname, _clock_callback)

/**
 * @brief Clock management request structure name
 * @param symname Base symbol name for variables related to this clock output
 */
#define Z_CLOCK_MANAGEMENT_REQ_NAME(symname)                                         \
	_CONCAT(symname, _clock_req)

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
	struct clock_management_rate_req Z_CLOCK_MANAGEMENT_REQ_NAME(symname) = {          \
		.min_freq = 0U,                                                \
		.max_freq = INT32_MAX,                                         \
	};                                                                     \
	/* Define output clock structure */                                    \
	static const struct clock_output                                       \
	Z_GENERIC_SECTION(secname) Z_CLOCK_MANAGEMENT_OUTPUT_NAME(symname) = {       \
		.clk_core = CLOCK_DT_GET(node_id),                             \
		.cb = &Z_CLOCK_MANAGEMENT_CALLBACK_NAME(symname),                    \
		.req = &Z_CLOCK_MANAGEMENT_REQ_NAME(symname),                        \
	};))

/** @endcond */

/**
 * @brief Defines clock output for a clock node within the system clock tree
 *
 * Defines a clock output for a clock node directly. The clock node provided
 * should have the compatible "clock-output". This macro should be used when
 * defining a clock output for access outside of device drivers, devices
 * described in devicetree should use @ref CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT
 * @param node_id Node identifier for the clock node to define an output for
 * @param name Software defined name for this clock output
 */
#define CLOCK_MANAGEMENT_DEFINE_OUTPUT(node_id, name)                                \
	Z_CLOCK_MANAGEMENT_DEFINE_OUTPUT(node_id,                                    \
		Z_CLOCK_OUTPUT_SECTION_NAME(node_id),                          \
		Z_CLOCK_OUTPUT_SYMBOL_NAME(node_id, name))


/**
 * @brief Defines clock output for system clock node at with name @p name in
 * "clock-outputs" property on device with node ID @p dev_node
 *
 * Defines a clock output for the system clock node with name @p name in the
 * device's "clock-outputs" property. This phandle must refer to a system clock
 * node with the dt compatible "clock-output".
 * @param dev_node Device node with a clock-outputs property.
 * @param name Name of the clock output
 */
#define CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(dev_node, name)                    \
	CLOCK_MANAGEMENT_DEFINE_OUTPUT(                                              \
		DT_PHANDLE_BY_IDX(dev_node, clock_outputs,                     \
		DT_CLOCK_OUTPUT_NAME_IDX(dev_node, name)),                     \
		DT_DEP_ORD(dev_node))

/**
 * @brief Defines clock output for system clock node at with name @p name in
 * "clock-outputs" property on instance @p inst of DT_DRV_COMPAT
 *
 * Defines a clock output for the system clock node with name @p name in the
 * device's "clock-outputs" property. This phandle must refer to a system clock
 * node with the dt compatible "clock-output".
 * @param inst DT_DRV_COMPAT instance number
 * @param name Name of the clock output
 */
#define CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT_BY_NAME(inst, name)                   \
	CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Defines clock output for system clock node at index @p idx in
 * "clock-outputs" property on device with node ID @p dev_node
 *
 * Defines a clock output for the system clock node at index @p idx the device's
 * "clock-outputs" property. This phandle must refer to a system clock node with
 * the dt compatible "clock-output".
 * @param dev_node Device node with a clock-outputs property.
 * @param idx Index within the "clock-outputs" property
 */
#define CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_IDX(dev_node, idx)                      \
	CLOCK_MANAGEMENT_DEFINE_OUTPUT(                                              \
		DT_PHANDLE_BY_IDX(dev_node, clock_outputs, idx),               \
		DT_DEP_ORD(dev_node))

/**
 * @brief Defines clock output for system clock node at index @p idx in
 * "clock-outputs" property on instance @p inst of DT_DRV_COMPAT
 *
 * Defines a clock output for the system clock node at index @p idx the device's
 * "clock-outputs" property. This phandle must refer to a system clock node with
 * the dt compatible "clock-output".
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Index within the "clock-outputs" property
 */
#define CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT_BY_IDX(inst, idx)                     \
	CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Defines clock output for a device described in devicetree by @p
 * dev_node
 *
 * Defines a clock output for device described in devicetree. The output will be
 * defined from the first phandle in the node's "clock-outputs" property. The
 * phandle must refer to a system clock node with the dt compatible
 * "clock-output". Note this is equivalent to
 * CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_IDX(dev_node, 0)
 * @param dev_node Device node with a clock-outputs property.
 */
#define CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT(dev_node)                                  \
	CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_IDX(dev_node, 0)

/**
 * @brief Defines clock output for instance @p inst of a DT_DRV_COMPAT
 *
 * Defines a clock output for device described in devicetree. The output will be
 * defined from the first phandle in the node's "clock-outputs" property. The
 * phandle must refer to a system clock node with the dt compatible
 * "clock-output". Note this is equivalent to
 * CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_IDX(dev_node, 0)
 * @param inst DT_DRV_COMPAT instance number
 */
#define CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT(inst)                                 \
	CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT(DT_DRV_INST(inst))

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
#define CLOCK_MANAGEMENT_GET_OUTPUT(node_id, name)                                   \
	/* We only actually define output objects if runtime clocking is on */ \
	COND_CODE_1(CONFIG_CLOCK_MANAGEMENT_RUNTIME, (                               \
	&Z_CLOCK_MANAGEMENT_OUTPUT_NAME(Z_CLOCK_OUTPUT_SYMBOL_NAME(node_id, name))), \
	((const struct clock_output *)CLOCK_DT_GET(node_id)))

/**
 * @brief Gets a clock output for system clock node at with name @p name in
 * "clock-outputs" property on device with node ID @p dev_node
 *
 * Gets a clock output for the system clock node with name @p name in the
 * device's "clock-outputs" property. Before using this macro, @ref
 * CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME should be used for defining the clock
 * output
 * @param dev_node Device node with a clock-outputs property.
 * @param name Name of the clock output
 */
#define CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(dev_node, name)                       \
	CLOCK_MANAGEMENT_GET_OUTPUT(                                                 \
		DT_PHANDLE_BY_IDX(dev_node, clock_outputs,                     \
		DT_CLOCK_OUTPUT_NAME_IDX(dev_node, name)),                     \
		DT_DEP_ORD(dev_node))

/**
 * @brief Gets a clock output for system clock node at with name @p name in
 * "clock-outputs" property on instance @p inst of DT_DRV_COMPAT
 *
 * Gets a clock output for the system clock node with name @p name in the
 * device's "clock-outputs" property. Before using this macro, @ref
 * CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT_BY_NAME should be used for defining the
 * clock output
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
 * device's "clock-outputs" property. Before using this macro, @ref
 * CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_IDX should be used for defining the clock
 * output
 * @param dev_node Device node with a clock-outputs property.
 * @param idx Index within the "clock-outputs" property
 */
#define CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_IDX(dev_node, idx)                         \
	CLOCK_MANAGEMENT_GET_OUTPUT(                                                 \
		DT_PHANDLE_BY_IDX(dev_node, clock_outputs, idx),               \
		DT_DEP_ORD(dev_node))

/**
 * @brief Gets a clock output for system clock node at index @p idx in
 * "clock-outputs" property on instance @p inst of DT_DRV_COMPAT
 *
 * Gets a clock output for the system clock node with index @p idx in the
 * device's "clock-outputs" property. Before using this macro, @ref
 * CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT_BY_IDX should be used for defining the clock
 * output
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
 * Before using this macro, @ref CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT should be used for
 * defining the clock output. Note this is equivalent to
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
 * Before using this macro, @ref CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT should be used
 * for defining the clock output. Note this is equivalent to
 * CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 */
#define CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT(inst)                                    \
	CLOCK_MANAGEMENT_DT_GET_OUTPUT(DT_DRV_INST(inst))

/**
 * @brief Get a clock state identifier from a "clock-state-n" property
 *
 * Gets a clock state identifier from a "clock-state-n" property, given
 * the name of the state as well as the name of the clock output.
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
 *         clock-state-0 = <&hsclk_state0> <&lpclk_state0>;
 *         clock-state-1 = <&hsclk_state1> <&lpclk_state1>;
 *         clock-state-names = "active", "sleep";
 * };
 * @endcode
 * The clock state identifiers could be accessed like so:
 * @code{.c}
 *     // Get identifier to apply "lpclk_state1" (low-power clock, sleep state)
 *     CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(my_dev), low_power, sleep)
 *     // Get identifier to apply "hsclk_state0" (highspeed clock, active state)
 *     CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(my_dev), highspeed, active)
 *     #define Z_CLOCK_MANAGEMENT_VND_MUX_DATA_DEFINE(node_id, prop, idx)
 *     #define Z_CLOCK_MANAGEMENT_VND_DIV_DATA_DEFINE(node_id, prop, idx)
 * @endcode
 * @param dev_id Node identifier for device with "clock-outputs" property
 * @param output_name Name of clock output to read state for
 * @param state_name Name of clock state to get for this clock output
 */
#define CLOCK_MANAGEMENT_DT_GET_STATE(dev_id, output_name, state_name)               \
	DT_NODE_CHILD_IDX(DT_PHANDLE_BY_IDX(dev_id, CONCAT(clock_state_,       \
		DT_CLOCK_STATE_NAME_IDX(dev_id, state_name)),                  \
		DT_CLOCK_OUTPUT_NAME_IDX(dev_id, output_name)))

/**
 * @brief Get a clock state identifier from a "clock-state-n" property
 *
 * Gets a clock state identifier from a "clock-state-n" property, given the name
 * of the state as well as the name of the clock output. Note this is equivalent
 * to CLOCK_MANAGEMENT_DT_GET_STATE(DT_DRV_INST(inst), output_name, state_name)
 * @param inst DT_DRV_COMPAT instance number
 * @param output_name Name of clock output to read state for
 * @param state_name Name of clock state to get for this clock output
 */
#define CLOCK_MANAGEMENT_DT_INST_GET_STATE(inst, output_name, state_name)            \
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_DRV_INST(inst), output_name, state_name)

/**
 * @brief Get clock rate for given output
 *
 * Gets output clock rate in Hz for provided clock output.
 * @param clk Clock output to read rate of
 * @return -EINVAL if parameters are invalid
 * @return -ENOSYS if clock does not implement get_rate API
 * @return -EIO if clock could not be read
 * @return frequency of clock output in HZ
 */
int clock_management_get_rate(const struct clock_output *clk);

/**
 * @brief Request a frequency for the clock output
 *
 * Requests a new rate for a clock output. The clock will select the best state
 * given the constraints provided in @p req. If enabled via
 * `CONFIG_CLOCK_MANAGEMENT_RUNTIME`, existing constraints on the clock will be
 * accounted for when servicing this request. Additionally, if enabled via
 * `CONFIG_CLOCK_MANAGEMENT_SET_RATE`, the clock will dynamically request a new rate
 * from its parent if none of the statically defined states satisfy the request.
 * An error will be returned if the request cannot be satisfied.
 * @param clk Clock output to request rate for
 * @param req Rate request for clock output
 * @return -EINVAL if parameters are invalid
 * @return -ENOENT if request could not be satisfied
 * @return -EPERM if clock is not configurable
 * @return -EIO if configuration of a clock failed
 * @return frequency of clock output in HZ on success
 */
int clock_management_req_rate(const struct clock_output *clk,
			const struct clock_management_rate_req *req);

/**
 * @brief Apply a clock state based on a devicetree clock state identifier
 *
 * Apply a clock state based on a clock state identifier. State identifiers are
 * defined devices that include a "clock-states" devicetree property, and may be
 * retrieved using the @ref CLOCK_MANAGEMENT_DT_GET_STATE macro
 * @param clk Clock output to apply state for
 * @param state Clock management state ID to apply
 * @return -EIO if configuration of a clock failed
 * @return -EINVAL if parameters are invalid
 * @return -EPERM if clock is not configurable
 * @return frequency of clock output in HZ on success
 */
int clock_management_apply_state(const struct clock_output *clk,
			   clock_management_state_t state);

/**
 * @brief Set callback for clock output reconfiguration
 *
 * Set callback, which will fire when a clock output (or any of its parents) are
 * reconfigured. A negative return value from this callback will prevent the
 * clock from being reconfigured.
 * @param clk Clock output to add callback for
 * @param callback Callback function to install
 * @param user_data User data to issue with callback (can be NULL)
 * @return -EINVAL if parameters are invalid
 * @return -ENOTSUP if callbacks are not supported
 * @return 0 on success
 */
static inline int clock_management_set_callback(const struct clock_output *clk,
					  clock_management_callback_handler_t callback,
					  const void *user_data)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	if ((!clk) || (!callback)) {
		return -EINVAL;
	}

	clk->cb->clock_callback = callback;
	clk->cb->user_data = user_data;
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
static inline void clock_management_disable_unused(void)
{
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	const struct clock_management_event event = {
		.type = CLOCK_MANAGEMENT_QUERY_RATE_CHANGE,
		.old_rate = 0,
		.new_rate = 0,
	};
	STRUCT_SECTION_FOREACH_ALTERNATE(clk_root, clk, clk) {
		/* Call clock_notify on each root clock. Clocks can use this
		 * notification event to determine if they are able
		 * to gate themselves
		 */
		clock_notify(clk, NULL, &event);
	}
#endif
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_H_ */
