/*
 * Copyright 2024 NXP
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * APIs for managing clock objects
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_H_


#include <zephyr/devicetree/clock_management.h>
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

/** @brief Clock frequency type. Defined for future portability */
typedef int32_t clock_freq_t;

/** @brief Flag value used to identify the end of a clock list. */
#define CLOCK_LIST_END INT16_MIN
/** @brief Flag value used to identify a NULL clock handle */
#define CLOCK_HANDLE_NULL 0

/** @brief Identifier for a "standard clock" with one parent */
#define CLK_TYPE_STANDARD 1
/** @brief Identifier for a "mux clock" with multiple parents */
#define CLK_TYPE_MUX 2
/** @brief Identifier for a "root clock" with no parents */
#define CLK_TYPE_ROOT 3
/** @brief Identifier for a "leaf clock" with no children */
#define CLK_TYPE_LEAF 4

/**
 * @brief Data used by the clock management subsystem
 *
 * Clock state data used by the clock management subsystem in runtime mode
 */
struct clk_subsys_data {
	clock_freq_t rate; /**< Current clock rate in Hz */
	uint8_t usage_cnt; /**< Number of users of this clock */
};

/**
 * @brief Shared clock data
 *
 * This structure describes shared clock data. It must always be the first
 * member of a clock structure.
 */
struct clk {
	/** Pointer to private clock hardware data. May be in ROM or RAM. */
	void *hw_data;
	/** API pointer */
	const void *api;
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME) || defined(__DOXYGEN__)
	/** Children nodes of the clock */
	const clock_handle_t *children;
	struct clk_subsys_data *subsys_data;
	/** Clock ranking for this clock */
	uint8_t rank;
	/** Factor to scale frequency by for this clock ranking */
	uint8_t rank_factor;
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_CLK_NAME) || defined(__DOXYGEN__)
	/** Name of this clock */
	const char *clk_name;
#endif
};

/**
 * @brief Definition for standard clock subsystem data fields
 *
 * Clocks defined with @ref CLOCK_DT_DEFINE must declare this as the first
 * member of their private data structure, like so:
 * struct myclk_hw_data {
 *     STANDARD_CLK_SUBSYS_DATA_DEFINE
 *     // private clock data fields...
 * }
 */
#define STANDARD_CLK_SUBSYS_DATA_DEFINE                                        \
	const struct clk *parent;

/**
 * @brief Definition for multiplexer clock subsystem data fields
 *
 * Clocks defined with @ref MUX_CLOCK_DT_DEFINE must declare this as the first
 * member of their private data structure, like so:
 * struct myclk_hw_data {
 *     MUX_CLK_SUBSYS_DATA_DEFINE
 *     // private clock data fields...
 * }
 */
#define MUX_CLK_SUBSYS_DATA_DEFINE                                             \
	const struct clk *const *parents;                                      \
	uint8_t parent_cnt;

/**
 * @brief Macro to initialize standard clock data fields
 *
 * This macro initializes standard clock data fields used by the clock subsystem.
 * It should be placed within clock data structure initializations like so:
 * struct myclk_hw_data hw_data = {
 *     STANDARD_CLK_SUBSYS_DATA_INIT(parent_clk)
 * }
 * @param parent_clk Pointer to the parent clock @ref clk structure for this clock
 */
#define STANDARD_CLK_SUBSYS_DATA_INIT(parent_clk)                              \
	.parent = parent_clk,

/**
 * @brief Macro to initialize multiplexer clock data fields
 *
 * This macro initializes multiplexer clock data fields used by the clock subsystem.
 * It should be placed within clock data structure initializations like so:
 * struct myclk_hw_data hw_data = {
 *     MUX_CLK_SUBSYS_DATA_INIT(parent_clks, ARRAY_SIZE(parent_clks))
 * }
 * @param parent_clks pointer to array of parent clocks for this clock
 * @param parent_count Number of parent clocks in the array
 */
#define MUX_CLK_SUBSYS_DATA_INIT(parent_clks, parent_count)                    \
	.parents = parent_clks,                                                \
	.parent_cnt = parent_count,


/** @cond INTERNAL_HIDDEN */

/**
 * @brief Structure to access generic clock subsystem data for standard clocks
 */
struct clk_standard_subsys_data {
	STANDARD_CLK_SUBSYS_DATA_DEFINE;
};

/**
 * @brief Structure to access generic clock subsystem data for mux clocks
 */
struct clk_mux_subsys_data {
	MUX_CLK_SUBSYS_DATA_DEFINE;
};

/**
 * @brief Get clock identifier
 */
#define Z_CLOCK_DT_CLK_ID(node_id) _CONCAT(clk_dts_ord_, DT_DEP_ORD(node_id))

/**
 * @brief Name for subsys data
 */
#define Z_CLOCK_SUBSYS_NAME(node_id) \
	_CONCAT(__clock_subsys_data_, Z_CLOCK_DT_CLK_ID(node_id))

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
 * @brief Section name for root clock object
 *
 * Section name for root clock object. Each clock object uses a named section so
 * the linker can optimize unused clocks out of the build. Each type of clock
 * is given a section name prefix, so that the type can be identified at
 * runtime.
 * @param node_id The devicetree node identifier.
 */
#define Z_ROOT_CLOCK_SECTION_NAME(node_id)                                     \
	_CONCAT(.clk_node_root_, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Section name for standard clock object
 *
 * Section name for standard clock object. Each clock object uses a named section
 * so the linker can optimize unused clocks out of the build. Each type of clock
 * is given a section name prefix, so that the type can be identified at
 * runtime.
 * @param node_id The devicetree node identifier.
 */
#define Z_STANDARD_CLOCK_SECTION_NAME(node_id)                                 \
	_CONCAT(.clk_node_standard_, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Section name for mux clock object
 *
 * Section name for mux clock object. Each clock object uses a named section so
 * the linker can optimize unused clocks out of the build. Each type of clock
 * is given a section name prefix, so that the type can be identified at
 * runtime.
 * @param node_id The devicetree node identifier.
 */
#define Z_MUX_CLOCK_SECTION_NAME(node_id)                                      \
	_CONCAT(.clk_node_mux_, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Section name for leaf clock object
 *
 * Section name for leaf clock object. Each clock object uses a named section so
 * the linker can optimize unused clocks out of the build. Each type of clock
 * is given a section name prefix, so that the type can be identified at
 * runtime.
 * @param node_id The devicetree node identifier.
 */
#define Z_LEAF_CLOCK_SECTION_NAME(node_id)                                     \
	_CONCAT(.clk_node_leaf_, Z_CLOCK_DT_CLK_ID(node_id))

/**
 * @brief Initialize a clock object
 *
 * @param children_ Children of this clock
 * @param hw_data Pointer to the clock's private data
 * @param api_ Pointer to the clock's API structure.
 * @param subsys_data_ Subsystem data for this clock
 * @param name_ clock name
 * @param subsys_data_ clock subsystem data
 * @param rank_ clock rank
 */
#define Z_CLOCK_INIT(children_, hw_data_, api_, name_, subsys_data_, rank_,            \
		     rank_factor_)                                                     \
	{                                                                              \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME, (.children = children_,))  \
		.hw_data = (void *)hw_data_,                                           \
		.api = api_,                                                           \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_CLK_NAME, (.clk_name = name_,))     \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME, (.subsys_data = subsys_data_,)) \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_SET_RATE, (.rank = rank_,))         \
		IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_SET_RATE, (.rank_factor = rank_factor_,)) \
	}

/**
 * @brief Helper to define clock structure
 *
 * @param secname Section to place clock into
 * @param clk_id clock identifier
 * Defines clock structure and variable name for a clock
 */
#define Z_CLOCK_DEF(secname, clk_id)                                           \
	const Z_DECL_ALIGN(struct clk) Z_GENERIC_SECTION(secname) CLOCK_NAME_GET(clk_id)

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
	IF_ENABLED(CONFIG_CLOCK_MANAGEMENT_RUNTIME, (                          \
	struct clk_subsys_data Z_CLOCK_SUBSYS_NAME(node_id);                   \
	))                                                                     \
	Z_CLOCK_DEF(secname, clk_id) =                                         \
		Z_CLOCK_INIT(Z_CLOCK_GET_CHILDREN(node_id),                    \
			     hw_data, api, DT_NODE_FULL_NAME(node_id),         \
			     COND_CODE_1(CONFIG_CLOCK_MANAGEMENT_RUNTIME,      \
			     (&Z_CLOCK_SUBSYS_NAME(node_id)), NULL),           \
			     DT_PROP(node_id, clock_ranking),                  \
			     DT_PROP(node_id, clock_rank_factor));

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
 * @brief Create a "standard clock" object from a devicetree node identifier
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

#define CLOCK_DT_DEFINE(node_id, hw_data, api)                                 \
	Z_CLOCK_BASE_DEFINE(node_id, Z_CLOCK_DT_CLK_ID(node_id), hw_data,      \
			    api, Z_STANDARD_CLOCK_SECTION_NAME(node_id));

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
 * @brief Create a "multiplexer clock" object from a devicetree node identifier
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

#define MUX_CLOCK_DT_DEFINE(node_id, hw_data, api)                             \
	Z_CLOCK_BASE_DEFINE(node_id, Z_CLOCK_DT_CLK_ID(node_id), hw_data,      \
			    api, Z_MUX_CLOCK_SECTION_NAME(node_id));

/**
 * @brief Like CLOCK_MUX_DT_DEFINE(), but uses an instance of `DT_DRV_COMPAT`
 * compatible instead of a node identifier
 * @param inst Instance number. The `node_id` argument to CLOCK_MUX_DT_DEFINE is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by CLOCK_MUX_DT_DEFINE().
 */
#define MUX_CLOCK_DT_INST_DEFINE(inst, ...)                                    \
	MUX_CLOCK_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Create a "root clock" object from a devicetree node identifier
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

#define ROOT_CLOCK_DT_DEFINE(node_id, hw_data, api)                            \
	Z_CLOCK_BASE_DEFINE(node_id, Z_CLOCK_DT_CLK_ID(node_id), hw_data,      \
			    api, Z_ROOT_CLOCK_SECTION_NAME(node_id));

/**
 * @brief Like ROOT_CLOCK_DT_DEFINE(), but uses an instance of `DT_DRV_COMPAT`
 * compatible instead of a node identifier
 * @param inst Instance number. The `node_id` argument to ROOT_CLOCK_DT_DEFINE is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by ROOT_CLOCK_DT_DEFINE().
 */
#define ROOT_CLOCK_DT_INST_DEFINE(inst, ...)                                   \
	ROOT_CLOCK_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Create a "leaf clock" object from a devicetree node identifier
 *
 * This API should only be used by the clock management subsystem- no clock
 * drivers should be defined as leaf nodes.
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
 */

#define LEAF_CLOCK_DT_DEFINE(node_id, hw_data)                                 \
	Z_CLOCK_BASE_DEFINE(node_id, Z_CLOCK_DT_CLK_ID(node_id), hw_data,      \
			    NULL, Z_LEAF_CLOCK_SECTION_NAME(node_id));

/**
 * @brief Like LEAF_CLOCK_DT_DEFINE(), but uses an instance of `DT_DRV_COMPAT`
 * compatible instead of a node identifier
 * @param inst Instance number. The `node_id` argument to LEAF_CLOCK_DT_DEFINE is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by LEAF_CLOCK_DT_DEFINE().
 */
#define LEAF_CLOCK_DT_INST_DEFINE(inst, ...)                                   \
	LEAF_CLOCK_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_H_ */
