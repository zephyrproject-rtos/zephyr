/*
 * Copyright (c) 2022 Andrei-Edward Popa <andrei.popa105@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup reset_controller_interface
 * @brief Main header file for reset controller driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RESET_H_
#define ZEPHYR_INCLUDE_DRIVERS_RESET_H_

/**
 * @brief Interfaces for reset controllers.
 * @defgroup reset_controller_interface Reset Controller
 * @since 3.1
 * @version 0.2.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reset controller device configuration. */
struct reset_dt_spec {
	/** Reset controller device. */
	const struct device *dev;
	/** Reset line. */
	uint32_t id;
};

/**
 * @brief Static initializer for a @p reset_dt_spec
 *
 * This returns a static initializer for a @p reset_dt_spec structure given a
 * devicetree node identifier, a property specifying a Reset Controller and an index.
 *
 * Example devicetree fragment:
 *
 *	n: node {
 *		resets = <&reset 10>;
 *	}
 *
 * Example usage:
 *
 *	const struct reset_dt_spec spec = RESET_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), 0);
 *	// Initializes 'spec' to:
 *	// {
 *	//         .dev = DEVICE_DT_GET(DT_NODELABEL(reset)),
 *	//         .id = 10
 *	// }
 *
 * The 'reset' field must still be checked for readiness, e.g. using
 * device_is_ready(). It is an error to use this macro unless the node
 * exists, has the given property, and that property specifies a reset
 * controller reset line id as shown above.
 *
 * @param node_id devicetree node identifier
 * @param idx logical index into "resets"
 * @return static initializer for a struct reset_dt_spec for the property
 */
#define RESET_DT_SPEC_GET_BY_IDX(node_id, idx)					\
	{									\
		.dev = DEVICE_DT_GET(DT_RESET_CTLR_BY_IDX(node_id, idx)),	\
		.id = DT_RESET_ID_BY_IDX(node_id, idx)				\
	}

/**
 * @brief Like RESET_DT_SPEC_GET_BY_IDX(), with a fallback to a default value
 *
 * If the devicetree node identifier 'node_id' refers to a node with a
 * 'resets' property, this expands to
 * <tt>RESET_DT_SPEC_GET_BY_IDX(node_id, idx)</tt>. The @p
 * default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id devicetree node identifier
 * @param idx logical index into the 'resets' property
 * @param default_value fallback value to expand to
 * @return static initializer for a struct reset_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define RESET_DT_SPEC_GET_BY_IDX_OR(node_id, idx, default_value)	\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, resets),			\
		    (RESET_DT_SPEC_GET_BY_IDX(node_id, idx)),		\
		    (default_value))

/**
 * @brief Equivalent to RESET_DT_SPEC_GET_BY_IDX(node_id, 0).
 *
 * @param node_id devicetree node identifier
 * @return static initializer for a struct reset_dt_spec for the property
 * @see RESET_DT_SPEC_GET_BY_IDX()
 */
#define RESET_DT_SPEC_GET(node_id) \
	RESET_DT_SPEC_GET_BY_IDX(node_id, 0)

/**
 * @brief Equivalent to
 *	  RESET_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value).
 *
 * @param node_id devicetree node identifier
 * @param default_value fallback value to expand to
 * @return static initializer for a struct reset_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define RESET_DT_SPEC_GET_OR(node_id, default_value)			\
	RESET_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value)

/**
 * @brief Static initializer for a @p reset_dt_spec from a DT_DRV_COMPAT
 * instance's Reset Controller property at an index.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into "resets"
 * @return static initializer for a struct reset_dt_spec for the property
 * @see RESET_DT_SPEC_GET_BY_IDX()
 */
#define RESET_DT_SPEC_INST_GET_BY_IDX(inst, idx) \
	RESET_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Static initializer for a @p reset_dt_spec from a DT_DRV_COMPAT
 *	  instance's 'resets' property at an index, with fallback
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into the 'resets' property
 * @param default_value fallback value to expand to
 * @return static initializer for a struct reset_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define RESET_DT_SPEC_INST_GET_BY_IDX_OR(inst, idx, default_value)	\
	COND_CODE_1(DT_PROP_HAS_IDX(DT_DRV_INST(inst), resets, idx),	\
		    (RESET_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)),	\
		    (default_value))

/**
 * @brief Equivalent to RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return static initializer for a struct reset_dt_spec for the property
 * @see RESET_DT_SPEC_INST_GET_BY_IDX()
 */
#define RESET_DT_SPEC_INST_GET(inst) \
	RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0)

/**
 * @brief Equivalent to
 *	  RESET_DT_SPEC_INST_GET_BY_IDX_OR(node_id, 0, default_value).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param default_value fallback value to expand to
 * @return static initializer for a struct reset_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define RESET_DT_SPEC_INST_GET_OR(inst, default_value)			\
	RESET_DT_SPEC_INST_GET_BY_IDX_OR(inst, 0, default_value)

/** @cond INTERNAL_HIDDEN */

/**
 * API template to get the reset status of the device.
 *
 * @see reset_status
 */
typedef int (*reset_api_status)(const struct device *dev, uint32_t id, uint8_t *status);

/**
 * API template to put the device in reset state.
 *
 * @see reset_line_assert
 */
typedef int (*reset_api_line_assert)(const struct device *dev, uint32_t id);

/**
 * API template to take out the device from reset state.
 *
 * @see reset_line_deassert
 */
typedef int (*reset_api_line_deassert)(const struct device *dev, uint32_t id);

/**
 * API template to reset the device.
 *
 * @see reset_line_toggle
 */
typedef int (*reset_api_line_toggle)(const struct device *dev, uint32_t id);

/**
 * @brief Reset Controller driver API
 */
__subsystem struct reset_driver_api {
	reset_api_status status;
	reset_api_line_assert line_assert;
	reset_api_line_deassert line_deassert;
	reset_api_line_toggle line_toggle;
};

/** @endcond */

/**
 * @brief Get the reset status
 *
 * This function returns the reset status of the device.
 *
 * @param dev Reset controller device.
 * @param id Reset line.
 * @param status Where to write the reset status.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the functionality is not implemented by the driver.
 * @retval -errno Other negative errno in case of failure.
 */
__syscall int reset_status(const struct device *dev, uint32_t id, uint8_t *status);

static inline int z_impl_reset_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->status == NULL) {
		return -ENOSYS;
	}

	return api->status(dev, id, status);
}

/**
 * @brief Get the reset status from a @p reset_dt_spec.
 *
 * This is equivalent to:
 *
 *    reset_status(spec->dev, spec->id, status);
 *
 * @param spec Reset controller specification from devicetree
 * @param status Where to write the reset status.
 *
 * @return a value from reset_status()
 */
static inline int reset_status_dt(const struct reset_dt_spec *spec, uint8_t *status)
{
	return reset_status(spec->dev, spec->id, status);
}

/**
 * @brief Put the device in reset state
 *
 * This function sets/clears the reset bits of the device,
 * depending on the logic level (active-high/active-low).
 *
 * @param dev Reset controller device.
 * @param id Reset line.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the functionality is not implemented by the driver.
 * @retval -errno Other negative errno in case of failure.
 */
__syscall int reset_line_assert(const struct device *dev, uint32_t id);

static inline int z_impl_reset_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->line_assert == NULL) {
		return -ENOSYS;
	}

	return api->line_assert(dev, id);
}

/**
 * @brief Assert the reset state from a @p reset_dt_spec.
 *
 * This is equivalent to:
 *
 *     reset_line_assert(spec->dev, spec->id);
 *
 * @param spec Reset controller specification from devicetree
 *
 * @return a value from reset_line_assert()
 */
static inline int reset_line_assert_dt(const struct reset_dt_spec *spec)
{
	return reset_line_assert(spec->dev, spec->id);
}

/**
 * @brief Take out the device from reset state.
 *
 * This function sets/clears the reset bits of the device,
 * depending on the logic level (active-low/active-high).
 *
 * @param dev Reset controller device.
 * @param id Reset line.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the functionality is not implemented by the driver.
 * @retval -errno Other negative errno in case of failure.
 */
__syscall int reset_line_deassert(const struct device *dev, uint32_t id);

static inline int z_impl_reset_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->line_deassert == NULL) {
		return -ENOSYS;
	}

	return api->line_deassert(dev, id);
}

/**
 * @brief Deassert the reset state from a @p reset_dt_spec.
 *
 * This is equivalent to:
 *
 *     reset_line_deassert(spec->dev, spec->id)
 *
 * @param spec Reset controller specification from devicetree
 *
 * @return a value from reset_line_deassert()
 */
static inline int reset_line_deassert_dt(const struct reset_dt_spec *spec)
{
	return reset_line_deassert(spec->dev, spec->id);
}

/**
 * @brief Reset the device.
 *
 * This function performs reset for a device (assert + deassert).
 *
 * @param dev Reset controller device.
 * @param id Reset line.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the functionality is not implemented by the driver.
 * @retval -errno Other negative errno in case of failure.
 */
__syscall int reset_line_toggle(const struct device *dev, uint32_t id);

static inline int z_impl_reset_line_toggle(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->line_toggle == NULL) {
		return -ENOSYS;
	}

	return api->line_toggle(dev, id);
}

/**
 * @brief Reset the device from a @p reset_dt_spec.
 *
 * This is equivalent to:
 *
 *     reset_line_toggle(spec->dev, spec->id)
 *
 * @param spec Reset controller specification from devicetree
 *
 * @return a value from reset_line_toggle()
 */
static inline int reset_line_toggle_dt(const struct reset_dt_spec *spec)
{
	return reset_line_toggle(spec->dev, spec->id);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/reset.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RESET_H_ */
