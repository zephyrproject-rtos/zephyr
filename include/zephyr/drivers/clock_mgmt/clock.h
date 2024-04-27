/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * APIs for managing clock objects
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_CLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_CLOCK_H_

#include <zephyr/devicetree/clock_mgmt.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clock Management Model
 * @defgroup clock_model Clock device model
 * @{
 */

/* Forward declaration of clock driver API struct */
struct clock_driver_api;

/**
 * @brief Type used to represent a "handle" for a device.
 *
 * Every @ref clk has an associated handle. You can get a pointer to a
 * @ref clk from its handle but the handle uses less space
 * than a pointer. The clock.h API uses handles to store lists of clocks
 * in a compact manner
 *
 * The extreme negative value has special significance (signalling the end
 * of a clock list). Zero signals a NULL clock handle.
 *
 * @see clk_from_handle()
 */
typedef int16_t clock_handle_t;

/** @brief Flag value used to identify the end of a clock list. */
#define CLOCK_LIST_END INT16_MIN
/** @brief Flag value used to identify a NULL clock handle */
#define CLOCK_HANDLE_NULL 0

/**
 * @brief Runtime clock structure (in ROM) for each clock node
 */
struct clk {
#if defined(CONFIG_CLOCK_MGMT_NOTIFY) || defined(__DOXYGEN__)
	/** Children nodes of the clock */
	const clock_handle_t *children;
#endif
	/** Pointer to private clock hardware data. May be in ROM or RAM. */
	void *hw_data;
	/** API pointer for clock node */
	const struct clock_driver_api *api;
#if defined(CONFIG_CLOCK_MGMT_SET_RATE) || defined(__DOXYGEN__)
	/** Handle of child clock that currently has a lock on this clock's
	 * configuration.
	 */
	clock_handle_t *owner;
#endif
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Get clock identifier
 */
#define Z_CLOCK_DT_CLK_ID(node_id) _CONCAT(clk_dts_ord_, DT_DEP_ORD(node_id))

/**
 * @brief Expands to the name of a global clock object.
 *
 * Return the full name of a clock object symbol created by CLOCK_DT_DEFINE(),
 * using the `clk_id` provided by Z_CLOCK_DT_CLK_ID(). This is the name of the
 * global variable storing the clock structure
 *
 * It is meant to be used for declaring extern symbols pointing to clock objects
 * before using the CLOCK_GET macro to get the device object.
 *
 * @param clk_id Clock identifier.
 *
 * @return The full name of the clock object defined by clock definition
 * macros.
 */
#define CLOCK_NAME_GET(clk_id) _CONCAT(__clock_, clk_id)

/**
 * @brief The name of the global clock object for @p node_id
 *
 * Returns the name of the global clock structure as a C identifier. The clock
 * must be allocated using CLOCK_DT_DEFINE() or CLOCK_DT_INST_DEFINE() for
 * this to work.
 *
 * @param node_id Devicetree node identifier
 *
 * @return The name of the clock object as a C identifier
 */
#define CLOCK_DT_NAME_GET(node_id) CLOCK_NAME_GET(Z_CLOCK_DT_CLK_ID(node_id))

/** @endcond */

/**
 * @brief Get a @ref clk reference from a clock devicetree node identifier.
 *
 * Returns a pointer to a clock object created from a devicetree node, if any
 * clock was allocated by a driver. If not such clock was allocated, this will
 * fail at linker time. If you get an error that looks like
 * `undefined reference to __device_dts_ord_<N>`, that is what happened.
 * Check to make sure your clock driver is being compiled,
 * usually by enabling the Kconfig options it requires.
 *
 * @param node_id A devicetree node identifier
 *
 * @return A pointer to the clock object created for that node
 */
#define CLOCK_DT_GET(node_id) (&CLOCK_DT_NAME_GET(node_id))

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Initializer for @ref clk.
 *
 * @param children_ Children of this clock
 * @param hw_data Pointer to the clock's private data
 * @param api_ Pointer to the clock's API structure.
 * @param owner_ Pointer to clock lock holder data
 */
#define Z_CLOCK_INIT(children_, hw_data_, api_, owner_)                        \
	{                                                                      \
		IF_ENABLED(CONFIG_CLOCK_MGMT_NOTIFY, (.children = children_,)) \
		.hw_data = (void *)hw_data_,                                   \
		.api = api_,                                                   \
		IF_ENABLED(CONFIG_CLOCK_MGMT_SET_RATE,                         \
		(.owner = owner_,))                                            \
	}

/**
 * @brief Section name for clock object
 *
 * Section name for clock object. Each clock object uses a named section so
 * the linker can optimize unused clocks out of the build.
 * @param node_id The devicetree node identifier.
 */
#define Z_CLOCK_SECTION_NAME(node_id)                                          \
	_CONCAT(.clk_node_, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Section name for root clock object
 *
 * Section name for root clock object. Each clock object uses a named section so
 * the linker can optimize unused clocks out of the build. Root clocks use
 * special section names so that the framework will only notify these clocks
 * when disabling unused clock sources.
 * @param node_id The devicetree node identifier.
 */
#define Z_ROOT_CLOCK_SECTION_NAME(node_id)                                     \
	_CONCAT(.clk_node_root, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Define a @ref clk object
 *
 * Defines and initializes configuration and data fields of a @ref clk
 * object
 * @param node_id The devicetree node identifier.
 * @param clk_id clock identifier (used to name the defined @ref clk).
 * @param hw_data Pointer to the clock's private data, which will be
 * stored in the @ref clk.hw_data field. This data may be in ROM or RAM.
 * @param config Pointer to the clock's private constant data, which will be
 * stored in the @ref clk.config field
 * @param api Pointer to the clock's API structure.
 * @param secname Section name to place clock object into
 */
#define Z_CLOCK_BASE_DEFINE(node_id, clk_id, hw_data, api, secname)            \
	Z_CLOCK_DEFINE_CHILDREN(node_id);                                      \
	Z_CLOCK_OWNER_DATA_DEFINE(clk_id);                                     \
	Z_CLOCK_STRUCT_DEF(secname) CLOCK_NAME_GET(clk_id) =                   \
		Z_CLOCK_INIT(Z_CLOCK_GET_CHILDREN(node_id),                    \
			     hw_data, api, Z_CLOCK_OWNER_DATA_GET(clk_id));

/**
 * @brief Declare a clock for each used clock node in devicetree
 *
 * @note Unused nodes should not result in clocks, so not predeclaring these
 * keeps drivers honest.
 *
 * This is only "maybe" a clock because some nodes have status "okay", but
 * don't have a corresponding @ref clk allocated. There's no way to figure
 * that out until after we've built the zephyr image, though.
 * @param node_id Devicetree node identifier
 */
#define Z_MAYBE_CLOCK_DECLARE_INTERNAL(node_id)                                \
	extern const struct clk CLOCK_DT_NAME_GET(node_id);

DT_FOREACH_STATUS_OKAY_NODE(Z_MAYBE_CLOCK_DECLARE_INTERNAL)

/**
 * @brief Helper to get a clock dependency ordinal if the clock is referenced
 *
 * The build system will convert these dependency ordinals into clock object
 * references after the first link phase is completed
 * @param node_id Clock identifier
 */
#define Z_GET_CLOCK_DEP_ORD(node_id)                                           \
	IF_ENABLED(DT_NODE_HAS_STATUS(node_id, okay),                          \
		(DT_DEP_ORD(node_id),))

/**
 * @brief Clock dependency array name
 * @param node_id Clock identifier
 */
#define Z_CLOCK_CHILDREN_NAME(node_id)                                         \
	_CONCAT(__clock_children_, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Define clock children array
 *
 * This macro defines a clock children array. A reference to
 * the clock dependency array can be retrieved with `Z_CLOCK_GET_CHILDREN`
 *
 * In the initial build, this array will expand to a list of clock ordinal
 * numbers that describe children of the clock, like so:
 * @code{.c}
 *     const clock_handle_t __weak __clock_children_clk_dts_ord_45[] = {
 *         66,
 *         30,
 *         55,
 *     }
 * @endcode
 *
 * In the second pass of the build, gen_clock_deps.py will create a strong
 * symbol to override the weak one, with each ordinal number resolved to
 * a clock handle (or omitted, if no clock structure was defined in the
 * build). The final array will look like so:
 * @code{.c}
 *     const clock_handle_t __clock_children_clk_dts_ord_45[] = {
 *         30, // Handle for clock with ordinal 66
 *         // Clock structure for ordinal 30 was not linked in build
 *         16, // Handle for clock with ordinal 55
 *         CLOCK_LIST_END, // Sentinel for end of list
 *     }
 * @endcode
 * This multi-phase build is necessary so that the linker will optimize out
 * any clock object that are not referenced elsewhere in the build. This way,
 * a clock object will be discarded in the first link phase unless another
 * structure references it (such as a clock referencing its parent object)
 * @param node_id Clock identifier
 */
#define Z_CLOCK_DEFINE_CHILDREN(node_id)                                       \
	const clock_handle_t __weak Z_CLOCK_CHILDREN_NAME(node_id)[] =         \
		{DT_SUPPORTS_CLK_ORDS(node_id)};

/**
 * @brief Get clock dependency array
 *
 * This macro gets the c identifier for the clock dependency array,
 * declared with `CLOCK_DEFINE_DEPS`, which will contain
 * an array of pointers to the clock objects dependent on this clock.
 * @param node_id Clock identifier
 */
#define Z_CLOCK_GET_CHILDREN(node_id) Z_CLOCK_CHILDREN_NAME(node_id)

/**
 * @brief Name of the symbol for clock owner data
 *
 * @param clk_id Clock identifier.
 *
 * @return The name of the symbol for clock owner data
 */
#define Z_CLOCK_OWNER_DATA_NAME(clk_id) _CONCAT(owner_data_, clk_id)

/**
 * @brief Helper to define structure name and section for a clock
 *
 * @param secname section name to place the clock in
 */
#define Z_CLOCK_STRUCT_DEF(secname) const struct clk Z_GENERIC_SECTION(secname)

/**
 * @brief Define clock owner data object
 * Defines the clock owner data object, used when CONFIG_CLOCK_MGMT_SET_RATE
 * is enabled
 *
 * @param clk_id Clock identifier.
 */
#define Z_CLOCK_OWNER_DATA_DEFINE(clk_id)                                      \
	IF_ENABLED(CONFIG_CLOCK_MGMT_SET_RATE,                                 \
		(clock_handle_t Z_CLOCK_OWNER_DATA_NAME(clk_id);))

/**
 * @brief Get reference to clock owner data object
 * Gets a reference to the clock owner data object, used when
 * CONFIG_CLOCK_MGMT_SET_RATE is enabled
 *
 * @param clk_id Clock identifier.
 */
#define Z_CLOCK_OWNER_DATA_GET(clk_id)                                         \
	COND_CODE_1(CONFIG_CLOCK_MGMT_SET_RATE,                                \
		(&Z_CLOCK_OWNER_DATA_NAME(clk_id)), (NULL))


/** @endcond */

/**
 * @brief Get the clock corresponding to a handle
 *
 * @param clock_handle the clock handle
 *
 * @return the clock that has that handle, or a null pointer if @p clock_handle
 * does not identify a clock.
 */
static inline const struct clk *clk_from_handle(clock_handle_t clock_handle)
{
	STRUCT_SECTION_START_EXTERN(clk);
	const struct clk *clk_hw = NULL;
	size_t numclk;

	STRUCT_SECTION_COUNT(clk, &numclk);

	if ((clock_handle > 0) && ((size_t)clock_handle <= numclk)) {
		clk_hw = &STRUCT_SECTION_START(clk)[clock_handle - 1];
	}

	return clk_hw;
}

/**
 * @brief Get the handle for a given clock
 *
 * @param clk_hw the clock for which a handle is desired.
 *
 * @return the handle for that clock, or a CLOCK_HANDLE_NULL pointer if the
 * device does not have an associated handles
 */
static inline clock_handle_t clk_handle_get(const struct clk *clk_hw)
{
	clock_handle_t ret = CLOCK_HANDLE_NULL;

	STRUCT_SECTION_START_EXTERN(clk);

	if (clk_hw != NULL) {
		ret = 1 + (clock_handle_t)(clk_hw - STRUCT_SECTION_START(clk));
	}

	return ret;
}

/**
 * @brief Create a clock object from a devicetree node identifier
 *
 * This macro defines a @ref clk. The global clock object's
 * name as a C identifier is derived from the node's dependency ordinal.
 *
 * Note that users should not directly reference clock objects, but instead
 * should use the clock management API. Clock objects are considered
 * internal to the clock subsystem.
 *
 * @param node_id The devicetree node identifier.
 * @param hw_data Pointer to the clock's private data, which will be
 * stored in the @ref clk.hw_data field. This data may be in ROM or RAM.
 * @param api Pointer to the clock's API structure.
 */

#define CLOCK_DT_DEFINE(node_id, hw_data, api, ...)                            \
	Z_CLOCK_BASE_DEFINE(node_id, Z_CLOCK_DT_CLK_ID(node_id), hw_data,      \
			    api, Z_CLOCK_SECTION_NAME(node_id))

/**
 * @brief Like CLOCK_DT_DEFINE(), but uses an instance of `DT_DRV_COMPAT`
 * compatible instead of a node identifier
 * @param inst Instance number. The `node_id` argument to CLOCK_DT_DEFINE is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by CLOCK_DT_DEFINE().
 */
#define CLOCK_DT_INST_DEFINE(inst, ...)                                        \
	CLOCK_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Create a root clock object from a devicetree node identifier
 *
 * This macro defines a root @ref clk. The global clock object's
 * name as a C identifier is derived from the node's dependency ordinal.
 * Root clocks will have their "notify" API implementation called by
 * the clock framework when the application requests unused clocks be
 * disabled. The "notify" implementation should forward clock notifications
 * to children so they can also evaluate if they need to gate.
 *
 * Note that users should not directly reference clock objects, but instead
 * should use the clock management API. Clock objects are considered
 * internal to the clock subsystem.
 *
 * @param node_id The devicetree node identifier.
 * @param hw_data Pointer to the clock's private data, which will be
 * stored in the @ref clk.hw_data field. This data may be in ROM or RAM.
 * @param api Pointer to the clock's API structure.
 */

#define ROOT_CLOCK_DT_DEFINE(node_id, hw_data, api, ...)                       \
	Z_CLOCK_BASE_DEFINE(node_id, Z_CLOCK_DT_CLK_ID(node_id), hw_data,      \
			    api, Z_ROOT_CLOCK_SECTION_NAME(node_id))

/**
 * @brief Like ROOT_CLOCK_DT_DEFINE(), but uses an instance of `DT_DRV_COMPAT`
 * compatible instead of a node identifier
 * @param inst Instance number. The `node_id` argument to ROOT_CLOCK_DT_DEFINE
 * is set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by CLOCK_DT_DEFINE().
 */
#define ROOT_CLOCK_DT_INST_DEFINE(inst, ...)                                   \
	ROOT_CLOCK_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MGMT_CLOCK_H_ */
