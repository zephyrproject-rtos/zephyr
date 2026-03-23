/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup wuc_interface
 * @brief Main header file for WUC (Wakeup Controller) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WUC_H_
#define ZEPHYR_INCLUDE_DRIVERS_WUC_H_

/**
 * @brief Wakeup Controller (WUC) Driver APIs
 * @defgroup wuc_interface WUC
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Wakeup controller device configuration. */
struct wuc_dt_spec {
	/** Wakeup controller device. */
	const struct device *dev;
	/** Wakeup source identifier. */
	uint32_t id;
};

/**
 * @brief Static initializer for a @p wuc_dt_spec
 *
 * This returns a static initializer for a @p wuc_dt_spec structure given a
 * devicetree node identifier, a property specifying a Wakeup Controller and an index.
 *
 * Example devicetree fragment:
 *
 *	n: node {
 *		wakeup-ctrls = <&wuc 10>;
 *	}
 *
 * Example usage:
 *
 *	const struct wuc_dt_spec spec = WUC_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), 0);
 *	Initializes 'spec' to:
 *	 {
 *	         .dev = DEVICE_DT_GET(DT_NODELABEL(wuc)),
 *	         .id = 10
 *	 }
 *
 * The 'wuc' field must still be checked for readiness, e.g. using
 * device_is_ready(). It is an error to use this macro unless the node
 * exists, has the given property, and that property specifies a wakeup
 * controller wakeup source id as shown above.
 *
 * @param node_id devicetree node identifier
 * @param idx logical index into "wakeup-ctrls"
 * @return static initializer for a struct wuc_dt_spec for the property
 */
#define WUC_DT_SPEC_GET_BY_IDX(node_id, idx)                                                       \
	{.dev = DEVICE_DT_GET(DT_WUC_BY_IDX(node_id, idx)), .id = DT_WUC_ID_BY_IDX(node_id, idx)}

/**
 * @brief Like WUC_DT_SPEC_GET_BY_IDX(), with a fallback to a default value
 *
 * If the devicetree node identifier 'node_id' refers to a node with a
 * 'wakeup-ctrls' property, this expands to
 * <tt>WUC_DT_SPEC_GET_BY_IDX(node_id, idx)</tt>. The @p
 * default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id devicetree node identifier
 * @param idx logical index into the 'wakeup-ctrls' property
 * @param default_value fallback value to expand to
 * @return static initializer for a struct wuc_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define WUC_DT_SPEC_GET_BY_IDX_OR(node_id, idx, default_value)                                     \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, wakeup_ctrls),			\
		    (WUC_DT_SPEC_GET_BY_IDX(node_id, idx)),		\
		    (default_value))

/**
 * @brief Equivalent to WUC_DT_SPEC_GET_BY_IDX(node_id, 0).
 *
 * @param node_id devicetree node identifier
 * @return static initializer for a struct wuc_dt_spec for the property
 * @see WUC_DT_SPEC_GET_BY_IDX()
 */
#define WUC_DT_SPEC_GET(node_id) WUC_DT_SPEC_GET_BY_IDX(node_id, 0)

/**
 * @brief Equivalent to
 *	  WUC_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value).
 *
 * @param node_id devicetree node identifier
 * @param default_value fallback value to expand to
 * @return static initializer for a struct wuc_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define WUC_DT_SPEC_GET_OR(node_id, default_value)                                                 \
	WUC_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value)

/**
 * @brief Static initializer for a @p wuc_dt_spec from a DT_DRV_COMPAT
 * instance's Wakeup Controller property at an index.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into "wakeup-ctrls"
 * @return static initializer for a struct wuc_dt_spec for the property
 * @see WUC_DT_SPEC_GET_BY_IDX()
 */
#define WUC_DT_SPEC_INST_GET_BY_IDX(inst, idx) WUC_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Static initializer for a @p wuc_dt_spec from a DT_DRV_COMPAT
 *	  instance's 'wakeup-ctrls' property at an index, with fallback
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into the 'wakeup-ctrls' property
 * @param default_value fallback value to expand to
 * @return static initializer for a struct wuc_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define WUC_DT_SPEC_INST_GET_BY_IDX_OR(inst, idx, default_value)                                   \
	COND_CODE_1(DT_PROP_HAS_IDX(DT_DRV_INST(inst), wakeup_ctrls, idx), \
		(WUC_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)),	\
		(default_value))

/**
 * @brief Equivalent to WUC_DT_SPEC_INST_GET_BY_IDX(inst, 0).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return static initializer for a struct wuc_dt_spec for the property
 * @see WUC_DT_SPEC_INST_GET_BY_IDX()
 */
#define WUC_DT_SPEC_INST_GET(inst) WUC_DT_SPEC_INST_GET_BY_IDX(inst, 0)

/**
 * @brief Equivalent to
 *	  WUC_DT_SPEC_INST_GET_BY_IDX_OR(node_id, 0, default_value).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param default_value fallback value to expand to
 * @return static initializer for a struct wuc_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define WUC_DT_SPEC_INST_GET_OR(inst, default_value)                                               \
	WUC_DT_SPEC_INST_GET_BY_IDX_OR(inst, 0, default_value)

/**
 * @def_driverbackendgroup{WUC,wuc_interface}
 * @{
 */

/**
 * @brief Callback API to enable a wakeup source.
 * See wuc_enable_wakeup_source() for argument description
 */
typedef int (*wuc_api_enable_wakeup_source)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to disable a wakeup source.
 * See wuc_disable_wakeup_source() for argument description
 */
typedef int (*wuc_api_disable_wakeup_source)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to check if a wakeup source triggered.
 * See wuc_check_wakeup_source_triggered() for argument description
 */
typedef int (*wuc_api_check_wakeup_source_triggered)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to clear a wakeup source triggered status.
 * See wuc_clear_wakeup_source_triggered() for argument description
 */
typedef int (*wuc_api_clear_wakeup_source_triggered)(const struct device *dev, uint32_t id);

/**
 * @driver_ops{WUC}
 */
__subsystem struct wuc_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief wuc_enable_wakeup_source
	 */
	wuc_api_enable_wakeup_source enable;
	/**
	 * @driver_ops_mandatory @copybrief wuc_disable_wakeup_source
	 */
	wuc_api_disable_wakeup_source disable;
	/**
	 * @driver_ops_optional @copybrief wuc_check_wakeup_source_triggered
	 */
	wuc_api_check_wakeup_source_triggered triggered;
	/**
	 * @driver_ops_optional @copybrief wuc_clear_wakeup_source_triggered
	 */
	wuc_api_clear_wakeup_source_triggered clear;
};
/**
 * @}
 */

/**
 * @brief Enable a wakeup source
 *
 * @param dev Pointer to the WUC device structure
 * @param id Wakeup source identifier
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure
 */
static inline int wuc_enable_wakeup_source(const struct device *dev, uint32_t id)
{
	return DEVICE_API_GET(wuc, dev)->enable(dev, id);
}

/**
 * @brief Enable a wakeup source using a @ref wuc_dt_spec.
 *
 * @param spec Pointer to the WUC devicetree spec structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int wuc_enable_wakeup_source_dt(const struct wuc_dt_spec *spec)
{
	return wuc_enable_wakeup_source(spec->dev, spec->id);
}

/**
 * @brief Disable a wakeup source.
 *
 * @param dev Pointer to the WUC device structure.
 * @param id Wakeup source identifier.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int wuc_disable_wakeup_source(const struct device *dev, uint32_t id)
{
	return DEVICE_API_GET(wuc, dev)->disable(dev, id);
}

/**
 * @brief Disable a wakeup source using a @ref wuc_dt_spec.
 *
 * @param spec Pointer to the WUC devicetree spec structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int wuc_disable_wakeup_source_dt(const struct wuc_dt_spec *spec)
{
	return wuc_disable_wakeup_source(spec->dev, spec->id);
}

/**
 * @brief Check if a wakeup source triggered.
 *
 * @param dev Pointer to the WUC device structure.
 * @param id Wakeup source identifier.
 *
 * @retval 1 If wakeup was triggered by this source.
 * @retval 0 If wakeup was not triggered by this source.
 * @retval -errno Negative errno code on failure.
 * @retval -ENOSYS if the interface is not implemented.
 */
static inline int wuc_check_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	if (DEVICE_API_GET(wuc, dev)->triggered == NULL) {
		return -ENOSYS;
	}
	return DEVICE_API_GET(wuc, dev)->triggered(dev, id);
}

/**
 * @brief Check if a wakeup source triggered using a @ref wuc_dt_spec.
 *
 * @param spec Pointer to the WUC devicetree spec structure.
 *
 * @retval 1 If wakeup was triggered by this source.
 * @retval 0 If wakeup was not triggered by this source.
 * @retval -errno Negative errno code on failure.
 * @retval -ENOSYS if the interface is not implemented.
 */
static inline int wuc_check_wakeup_source_triggered_dt(const struct wuc_dt_spec *spec)
{
	return wuc_check_wakeup_source_triggered(spec->dev, spec->id);
}

/**
 * @brief Clear a wakeup source triggered status.
 *
 * @param dev Pointer to the WUC device structure.
 * @param id Wakeup source identifier.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 * @retval -ENOSYS if the interface is not implemented.
 */
static inline int wuc_clear_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	if (DEVICE_API_GET(wuc, dev)->clear == NULL) {
		return -ENOSYS;
	}
	return DEVICE_API_GET(wuc, dev)->clear(dev, id);
}

/**
 * @brief Clear a wakeup source triggered status using a @ref wuc_dt_spec.
 *
 * @param spec Pointer to the WUC devicetree spec structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 * @retval -ENOSYS if the interface is not implemented.
 */
static inline int wuc_clear_wakeup_source_triggered_dt(const struct wuc_dt_spec *spec)
{
	return wuc_clear_wakeup_source_triggered(spec->dev, spec->id);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_WUC_H_ */
