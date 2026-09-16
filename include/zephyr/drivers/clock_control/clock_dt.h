/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup clock_control_interface
 * @brief Devicetree clock objects for the clock control API.
 *
 * A clock controller driver defines one public object per element of the
 * @c clocks property of every enabled devicetree node that references it,
 * using CLOCK_DT_DEFINE_CONSUMERS(). Consumers obtain a pointer to their
 * objects with CLOCK_DT_GET() and friends, without knowing how the controller
 * encodes its clock specifier cells.
 *
 * The objects are ordinary read-only globals. They are not kept alive by the
 * linker script, so the linker discards every object no compiled driver
 * references.
 *
 * This header is included by <zephyr/drivers/clock_control.h>; do not include
 * it directly.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_DT_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_DT_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup clock_control_interface
 * @{
 */

/**
 * @brief Clock described by one element of a devicetree @c clocks property.
 *
 * Instances are defined by the clock controller driver; consumers only hold
 * pointers to them. The payload behind @ref subsys is private to the
 * controller.
 */
struct clock_dt_spec {
	/** Clock controller device. */
	const struct device *dev;
	/** Controller-private clock identifier. */
	clock_control_subsys_t subsys;
};

/**
 * @cond INTERNAL_HIDDEN
 */

/* Name of the public clock object: __clk_dts_ord_<ordinal>_idx_<idx> */
#define Z_CLOCK_DT_NAME(node_id, idx)                                                              \
	_CONCAT(_CONCAT(_CONCAT(__clk_dts_ord_, DT_DEP_ORD(node_id)), _idx_), idx)

/* Name of the controller-private payload of a clock object */
#define Z_CLOCK_DT_DATA_NAME(node_id, idx) _CONCAT(Z_CLOCK_DT_NAME(node_id, idx), _data)

#define Z_CLOCK_DT_GET_ELEM(node_id, prop, idx) CLOCK_DT_GET_BY_IDX(node_id, idx)

#define Z_CLOCK_DT_DECLARE_ELEM(node_id, prop, idx)                                                \
	extern const struct clock_dt_spec Z_CLOCK_DT_NAME(node_id, idx);

#define Z_CLOCK_DT_MAYBE_DECLARE(node_id)                                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, clocks),                                              \
		   (DT_FOREACH_PROP_ELEM(node_id, clocks, Z_CLOCK_DT_DECLARE_ELEM)))

/* Declare every clock object of every enabled node up front, like device.h
 * does for devices, so consumers can take their address without a
 * declaration of their own.
 */
DT_FOREACH_STATUS_OKAY_NODE(Z_CLOCK_DT_MAYBE_DECLARE)

/* Callback for DT_FOREACH_PROP_ELEM_VARGS(): defines the object for one
 * element of a clocks property if that element references ctlr.
 */
#define Z_CLOCK_DT_DEFINE_ELEM(node_id, prop, idx, ctlr, subsys_fn)                               \
	COND_CODE_1(DT_SAME_NODE(ctlr, DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),                       \
		    (const struct clock_dt_spec Z_CLOCK_DT_NAME(node_id, idx) = {                  \
			     .dev = DEVICE_DT_GET(ctlr),                                           \
			     .subsys = subsys_fn(node_id, idx),                                    \
		     };),                                                                          \
		    ())

#define Z_CLOCK_DT_DEFINE_NODE(node_id, ctlr, subsys_fn)                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, clocks),                                              \
		   (DT_FOREACH_PROP_ELEM_VARGS(node_id, clocks, Z_CLOCK_DT_DEFINE_ELEM, ctlr,      \
					       subsys_fn)))

/* Same, with a controller-private payload object the subsys points to. */
#define Z_CLOCK_DT_DEFINE_ELEM_DATA(node_id, prop, idx, ctlr, data_type, init_fn)                 \
	COND_CODE_1(DT_SAME_NODE(ctlr, DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),                       \
		    (static const data_type Z_CLOCK_DT_DATA_NAME(node_id, idx) =                   \
			     init_fn(node_id, idx);                                                \
		     const struct clock_dt_spec Z_CLOCK_DT_NAME(node_id, idx) = {                  \
			     .dev = DEVICE_DT_GET(ctlr),                                           \
			     .subsys = (clock_control_subsys_t)&Z_CLOCK_DT_DATA_NAME(node_id,      \
										      idx),        \
		     };),                                                                          \
		    ())

#define Z_CLOCK_DT_DEFINE_NODE_DATA(node_id, ctlr, data_type, init_fn)                            \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, clocks),                                              \
		   (DT_FOREACH_PROP_ELEM_VARGS(node_id, clocks, Z_CLOCK_DT_DEFINE_ELEM_DATA,       \
					       ctlr, data_type, init_fn)))

#define Z_CLOCK_DT_SUBSYS_NULL(node_id, idx) NULL

/**
 * @endcond
 */

/**
 * @brief Define the clock objects of every consumer of a clock controller.
 *
 * Expands to one @ref clock_dt_spec definition per element of the @c clocks
 * property of every enabled node that references @p ctlr.
 *
 * @p subsys_fn is a macro invoked as @c subsys_fn(node_id, idx) and must
 * expand to the @c clock_control_subsys_t value of that element, typically
 * built from the clock specifier cells with DT_CLOCKS_CELL_BY_IDX().
 *
 * Use this exactly once per controller node.
 *
 * @param ctlr Node identifier of the clock controller.
 * @param subsys_fn Macro producing the subsys value.
 */
#define CLOCK_DT_DEFINE_CONSUMERS(ctlr, subsys_fn)                                                \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(Z_CLOCK_DT_DEFINE_NODE, ctlr, subsys_fn)

/**
 * @brief Define the clock objects of every consumer of a clock controller, with payloads.
 *
 * Like CLOCK_DT_DEFINE_CONSUMERS(), for controllers whose @c subsys is a
 * pointer to a structure. Each object comes with a controller-private
 * payload of type @p data_type which its @c subsys member points to.
 *
 * @p init_fn is a macro invoked as @c init_fn(node_id, idx) and must expand
 * to a braced initializer of @p data_type built from the clock specifier
 * cells.
 *
 * @param ctlr Node identifier of the clock controller.
 * @param data_type Type of the controller-private payload.
 * @param init_fn Macro producing the payload initializer.
 */
#define CLOCK_DT_DEFINE_CONSUMERS_DATA(ctlr, data_type, init_fn)                                  \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(Z_CLOCK_DT_DEFINE_NODE_DATA, ctlr, data_type, init_fn)

/**
 * @brief Define the clock objects of every consumer of a payload-less clock controller.
 *
 * Like CLOCK_DT_DEFINE_CONSUMERS(), for controllers which ignore the
 * @c subsys argument of the clock control API. The objects' @c subsys member
 * is @c NULL.
 *
 * @param ctlr Node identifier of the clock controller.
 */
#define CLOCK_DT_DEFINE_CONSUMERS_NODATA(ctlr)                                                    \
	CLOCK_DT_DEFINE_CONSUMERS(ctlr, Z_CLOCK_DT_SUBSYS_NULL)

/**
 * @brief Get a pointer to the clock object of a @c clocks element by index.
 *
 * The object is defined by the driver of the referenced clock controller.
 * Referencing a disabled node, or a controller whose driver is not built,
 * produces an undefined reference to @c __clk_dts_ord_<N>_idx_<idx> at link
 * time.
 *
 * @param node_id Node identifier of the clock consumer.
 * @param idx Index into the node's @c clocks property.
 * @return Pointer to the @ref clock_dt_spec.
 */
#define CLOCK_DT_GET_BY_IDX(node_id, idx) (&Z_CLOCK_DT_NAME(node_id, idx))

/**
 * @brief Get a pointer to the clock object of a @c clocks element by name.
 *
 * @param node_id Node identifier of the clock consumer.
 * @param name Lowercase-and-underscores name from the node's @c clock-names property.
 * @return Pointer to the @ref clock_dt_spec.
 * @see CLOCK_DT_GET_BY_IDX()
 */
#define CLOCK_DT_GET_BY_NAME(node_id, name)                                                       \
	CLOCK_DT_GET_BY_IDX(node_id, DT_PHA_ELEM_IDX_BY_NAME(node_id, clocks, name))

/**
 * @brief Get a pointer to the clock object of the first @c clocks element.
 *
 * @param node_id Node identifier of the clock consumer.
 * @return Pointer to the @ref clock_dt_spec.
 * @see CLOCK_DT_GET_BY_IDX()
 */
#define CLOCK_DT_GET(node_id) CLOCK_DT_GET_BY_IDX(node_id, 0)

/**
 * @brief Like CLOCK_DT_GET_BY_IDX(), but @c NULL when the element does not exist.
 *
 * @param node_id Node identifier of the clock consumer.
 * @param idx Index into the node's @c clocks property.
 * @return Pointer to the @ref clock_dt_spec, or @c NULL.
 */
#define CLOCK_DT_GET_BY_IDX_OR_NULL(node_id, idx)                                                 \
	COND_CODE_1(DT_CLOCKS_HAS_IDX(node_id, idx), (CLOCK_DT_GET_BY_IDX(node_id, idx)), (NULL))

/**
 * @brief Like CLOCK_DT_GET_BY_NAME(), but @c NULL when the name does not exist.
 *
 * @param node_id Node identifier of the clock consumer.
 * @param name Lowercase-and-underscores name from the node's @c clock-names property.
 * @return Pointer to the @ref clock_dt_spec, or @c NULL.
 */
#define CLOCK_DT_GET_BY_NAME_OR_NULL(node_id, name)                                               \
	COND_CODE_1(DT_CLOCKS_HAS_NAME(node_id, name), (CLOCK_DT_GET_BY_NAME(node_id, name)),     \
		    (NULL))

/**
 * @brief Like CLOCK_DT_GET(), but @c NULL when the node has no @c clocks property.
 *
 * @param node_id Node identifier of the clock consumer.
 * @return Pointer to the @ref clock_dt_spec, or @c NULL.
 */
#define CLOCK_DT_GET_OR_NULL(node_id) CLOCK_DT_GET_BY_IDX_OR_NULL(node_id, 0)

/**
 * @brief Initializer for an array of pointers to all clock objects of a node.
 *
 * Example:
 *
 * @code{.c}
 * static const struct clock_dt_spec *const clks[] = CLOCK_DT_SPECS_INIT(DT_NODELABEL(mac));
 * @endcode
 *
 * @param node_id Node identifier of the clock consumer.
 */
#define CLOCK_DT_SPECS_INIT(node_id)                                                              \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM_SEP(node_id, clocks, Z_CLOCK_DT_GET_ELEM, (,))                \
	}

/**
 * @brief Like CLOCK_DT_GET_BY_IDX(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 * @param idx Index into the instance's @c clocks property.
 * @return Pointer to the @ref clock_dt_spec.
 */
#define CLOCK_DT_INST_GET_BY_IDX(inst, idx) CLOCK_DT_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Like CLOCK_DT_GET_BY_NAME(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 * @param name Lowercase-and-underscores name from the instance's @c clock-names property.
 * @return Pointer to the @ref clock_dt_spec.
 */
#define CLOCK_DT_INST_GET_BY_NAME(inst, name) CLOCK_DT_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Like CLOCK_DT_GET(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 * @return Pointer to the @ref clock_dt_spec.
 */
#define CLOCK_DT_INST_GET(inst) CLOCK_DT_GET(DT_DRV_INST(inst))

/**
 * @brief Like CLOCK_DT_GET_BY_IDX_OR_NULL(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 * @param idx Index into the instance's @c clocks property.
 * @return Pointer to the @ref clock_dt_spec, or @c NULL.
 */
#define CLOCK_DT_INST_GET_BY_IDX_OR_NULL(inst, idx)                                               \
	CLOCK_DT_GET_BY_IDX_OR_NULL(DT_DRV_INST(inst), idx)

/**
 * @brief Like CLOCK_DT_GET_BY_NAME_OR_NULL(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 * @param name Lowercase-and-underscores name from the instance's @c clock-names property.
 * @return Pointer to the @ref clock_dt_spec, or @c NULL.
 */
#define CLOCK_DT_INST_GET_BY_NAME_OR_NULL(inst, name)                                             \
	CLOCK_DT_GET_BY_NAME_OR_NULL(DT_DRV_INST(inst), name)

/**
 * @brief Like CLOCK_DT_GET_OR_NULL(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 * @return Pointer to the @ref clock_dt_spec, or @c NULL.
 */
#define CLOCK_DT_INST_GET_OR_NULL(inst) CLOCK_DT_GET_OR_NULL(DT_DRV_INST(inst))

/**
 * @brief Like CLOCK_DT_SPECS_INIT(), for a @c DT_DRV_COMPAT instance.
 *
 * @param inst Instance number.
 */
#define CLOCK_DT_INST_SPECS_INIT(inst) CLOCK_DT_SPECS_INIT(DT_DRV_INST(inst))

/**
 * @brief Enable a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @return See clock_control_on().
 */
static inline int clock_control_on_dt(const struct clock_dt_spec *spec)
{
	return clock_control_on(spec->dev, spec->subsys);
}

/**
 * @brief Disable a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @return See clock_control_off().
 */
static inline int clock_control_off_dt(const struct clock_dt_spec *spec)
{
	return clock_control_off(spec->dev, spec->subsys);
}

/**
 * @brief Request asynchronous enabling of a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @param cb Callback invoked once the clock is ready.
 * @param user_data User context passed to @p cb.
 * @return See clock_control_async_on().
 */
static inline int clock_control_async_on_dt(const struct clock_dt_spec *spec,
					    clock_control_cb_t cb, void *user_data)
{
	return clock_control_async_on(spec->dev, spec->subsys, cb, user_data);
}

/**
 * @brief Get the status of a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @return See clock_control_get_status().
 */
static inline enum clock_control_status
clock_control_get_status_dt(const struct clock_dt_spec *spec)
{
	return clock_control_get_status(spec->dev, spec->subsys);
}

/**
 * @brief Get the rate of a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @param[out] rate Clock rate in Hz.
 * @return See clock_control_get_rate().
 */
static inline int clock_control_get_rate_dt(const struct clock_dt_spec *spec, uint32_t *rate)
{
	return clock_control_get_rate(spec->dev, spec->subsys, rate);
}

/**
 * @brief Set the rate of a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @param rate Requested rate.
 * @return See clock_control_set_rate().
 */
static inline int clock_control_set_rate_dt(const struct clock_dt_spec *spec,
					    clock_control_subsys_rate_t rate)
{
	return clock_control_set_rate(spec->dev, spec->subsys, rate);
}

/**
 * @brief Configure a clock described by a @ref clock_dt_spec.
 *
 * @param spec Clock object.
 * @param data Controller-specific configuration data.
 * @return See clock_control_configure().
 */
static inline int clock_control_configure_dt(const struct clock_dt_spec *spec, void *data)
{
	return clock_control_configure(spec->dev, spec->subsys, data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_DT_H_ */
