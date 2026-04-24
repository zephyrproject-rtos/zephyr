/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for NXP power-rail controllers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_POWER_RAIL_NXP_POWER_RAIL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_POWER_RAIL_NXP_POWER_RAIL_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NXP power-rail controller driver API.
 *
 * Implemented by concrete controllers (PMC, SLEEPCON, ...). Consumers
 * call the helpers below rather than these function pointers directly.
 */
__subsystem struct nxp_power_rail_driver_api {
	/** Request a power rail. */
	int (*request)(const struct device *dev, uint16_t id);
	/** Release a previously requested power rail. */
	int (*release)(const struct device *dev, uint16_t id);
};

/**
 * @brief NXP power-rail specification.
 *
 * A rail is identified by a controller device plus a 16-bit identifier
 * in that controller's namespace. The interpretation of @ref id is
 * controller-specific:
 *  - PMC: pdruncfg_index * 32 + bit_within_register
 *  - SLEEPCON: bit position within the RUNCFG register
 */
struct nxp_power_rail_spec {
	/** Controller device. */
	const struct device *dev;

	/** Controller-local rail identifier. */
	uint16_t id;
};

/**
 * @brief Build an @ref nxp_power_rail_spec initializer from a devicetree
 *        'nxp,power-rails' property.
 *
 * @param node_id Consumer devicetree node identifier.
 * @param idx     Index into 'nxp,power-rails' (0-based).
 */
#define NXP_POWER_RAIL_DT_SPEC_GET_BY_IDX(node_id, idx)						\
	{											\
		.dev = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, nxp_power_rails, idx)),		\
		.id = DT_PHA_BY_IDX(node_id, nxp_power_rails, idx, id),				\
	}

/** @brief Equivalent to 'NXP_POWER_RAIL_DT_SPEC_GET_BY_IDX(node_id, 0)'. */
#define NXP_POWER_RAIL_DT_SPEC_GET(node_id) NXP_POWER_RAIL_DT_SPEC_GET_BY_IDX(node_id, 0)

/** @brief 'DT_DRV_COMPAT' variant of 'NXP_POWER_RAIL_DT_SPEC_GET_BY_IDX()'. */
#define NXP_POWER_RAIL_DT_INST_SPEC_GET_BY_IDX(inst, idx)					\
	NXP_POWER_RAIL_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/** @brief 'DT_DRV_COMPAT' variant of 'NXP_POWER_RAIL_DT_SPEC_GET()'. */
#define NXP_POWER_RAIL_DT_INST_SPEC_GET(inst) NXP_POWER_RAIL_DT_INST_SPEC_GET_BY_IDX(inst, 0)

/**
 * @brief Request a power rail.
 *
 * Calls are reference-counted by the controller driver: the physical
 * rail is released only when the first consumer requests it.
 *
 * @param dev Controller device.
 * @param id Controller-local rail identifier.
 *
 * @retval 0        On success.
 * @retval -ENODEV  @p dev is not ready.
 * @retval -EINVAL  @p id is out of range.
 */
static inline int nxp_power_rail_request(const struct device *dev, uint16_t id)
{
	const struct nxp_power_rail_driver_api *api;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	api = (const struct nxp_power_rail_driver_api *)dev->api;

	return api->request(dev, id);
}

/**
 * @brief Release a previous @ref nxp_power_rail_request request.
 *
 * The physical rail is asserted (powered down) only when the last
 * outstanding request is released.
 *
 * @param dev Controller device.
 * @param id Controller-local rail identifier.
 *
 * @retval 0        On success.
 * @retval -ENODEV  @p dev is not ready.
 * @retval -EINVAL  @p id is out of range.
 */
static inline int nxp_power_rail_release(const struct device *dev, uint16_t id)
{
	const struct nxp_power_rail_driver_api *api;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	api = (const struct nxp_power_rail_driver_api *)dev->api;

	return api->release(dev, id);
}

/** @brief Wrapper around @ref nxp_power_rail_request using a rail spec. */
static inline int nxp_power_rail_request_spec(const struct nxp_power_rail_spec *spec)
{
	return nxp_power_rail_request(spec->dev, spec->id);
}

/** @brief Wrapper around @ref nxp_power_rail_release using a rail spec. */
static inline int nxp_power_rail_release_spec(const struct nxp_power_rail_spec *spec)
{
	return nxp_power_rail_release(spec->dev, spec->id);
}

/**
 * @brief Request every rail in an array of specs.
 *
 * Bails out on the first failing request; previously-requested rails
 * are left in the requested state.
 *
 * @param specs Array of rail specs.
 * @param count Number of entries in @p specs.
 *
 * @retval 0 On success (including @p count == 0).
 * @retval <0 The error code returned by the first failing request.
 */
static inline int nxp_power_rail_request_all(const struct nxp_power_rail_spec *specs,
					     uint8_t count)
{
	for (uint8_t i = 0; i < count; i++) {
		int ret = nxp_power_rail_request_spec(&specs[i]);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

/** @cond INTERNAL_HIDDEN */
#define Z_NXP_POWER_RAIL_DT_SPEC_ENTRY(node_id, prop, idx)	\
	NXP_POWER_RAIL_DT_SPEC_GET_BY_IDX(node_id, idx),
/** @endcond */

/**
 * @brief Define a static array of rail specs for a 'DT_DRV_COMPAT' instance.
 *
 * Expands to
 *
 * @code{.c}
 *     static const struct nxp_power_rail_spec nxp_power_rail_specs_<inst>[]
 * @endcode
 *
 * definition populated from the instance's 'nxp,power-rails' property.
 * Expands to nothing if the property is absent on the instance.
 *
 * Intended to be paired with @ref NXP_POWER_RAIL_DT_INST_SPECS_INIT inside
 * the device-instantiation macro of a consumer driver.
 *
 * @param inst 'DT_DRV_COMPAT' instance number.
 */
#define NXP_POWER_RAIL_DT_INST_SPECS_DEFINE(inst)					\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, nxp_power_rails),			\
		(static const struct nxp_power_rail_spec				\
			nxp_power_rail_specs_##inst[] = {				\
			DT_INST_FOREACH_PROP_ELEM(inst, nxp_power_rails,		\
						  Z_NXP_POWER_RAIL_DT_SPEC_ENTRY)	\
		};))

/**
 * @brief Initializer for the standard power-rail fields of a consumer
 *        driver's config struct.
 *
 * Drivers are expected to declare the fields as
 *
 * @code{.c}
 *     const struct nxp_power_rail_spec *power_rails;
 *     uint8_t power_rail_count;
 * @endcode
 *
 * and include this macro in their 'DEVICE_DT_INST_DEFINE' expansion
 * after calling @ref NXP_POWER_RAIL_DT_INST_SPECS_DEFINE.
 *
 * @param inst ``DT_DRV_COMPAT`` instance number.
 */
#define NXP_POWER_RAIL_DT_INST_SPECS_INIT(inst)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, nxp_power_rails),		\
		(.power_rails = nxp_power_rail_specs_##inst,			\
		 .power_rail_count = ARRAY_SIZE(nxp_power_rail_specs_##inst),),	\
		(.power_rails = NULL, .power_rail_count = 0,))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_POWER_RAIL_NXP_POWER_RAIL_H_ */
