/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_

/**
 * @brief Opamp Interface
 * @defgroup opamp_interface Opamp Interface
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumerations for opamp functional mode.
 */
enum opamp_functional_mode {
	/** PGA mode */
	OPAMP_PGA_MODE = 0,
	/** Inverting mode */
	OPAMP_INVERTING_MODE,
	/** Non-inverting mode */
	OPAMP_NON_INVERTING_MODE,
	/** Follower mode */
	OPAMP_FOLLOWER_MODE,
};

/**
 * @brief Structure for specifying the configuration of a OPAMP gain.
 */
struct opamp_gain_cfg {
	/** Inverting gain identifier of the OPAMP that should be configured. */
	uint8_t inverting_gain;
	/** Non-inverting gain identifier of the OPAMP that should be configured. */
	uint8_t non_inverting_gain;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

typedef int (*opamp_api_enable_t)(const struct device *dev);
typedef int (*opamp_api_disable_t)(const struct device *dev);
typedef int (*opamp_api_set_gain_t)(const struct device *dev, const struct opamp_gain_cfg *cfg);

__subsystem struct opamp_driver_api {
	opamp_api_enable_t enable;
	opamp_api_disable_t disable;
	opamp_api_set_gain_t set_gain;
};

/** @endcond */

/**
 * @brief Enable opamp.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 *
 * @retval 0		If opamp has been successfully enabled.
 * @retval -errno	Negative errno in case of failure.
 */
__syscall int opamp_enable(const struct device *dev);

static inline int z_impl_opamp_enable(const struct device *dev)
{
	return DEVICE_API_GET(opamp, dev)->enable(dev);
}

/**
 * @brief Disable opamp.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 *
 * @retval 0		If opamp has been successfully disabled.
 * @retval -errno	Negative errno in case of failure.
 */
__syscall int opamp_disable(const struct device *dev);

static inline int z_impl_opamp_disable(const struct device *dev)
{
	return DEVICE_API_GET(opamp, dev)->disable(dev);
}

/**
 * @brief Set opamp inverting and non-inverting gain.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param cfg		Opamp gain configuration.
 *
 * @retval 0		If opamp inverting and non-inverting gain has been successfully set.
 * @retval -errno	Negative errno in case of failure.
 */
__syscall int opamp_set_gain(const struct device *dev, const struct opamp_gain_cfg *cfg);

static inline int z_impl_opamp_set_gain(const struct device *dev, const struct opamp_gain_cfg *cfg)
{
	return DEVICE_API_GET(opamp, dev)->set_gain(dev, cfg);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/opamp.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_ */
