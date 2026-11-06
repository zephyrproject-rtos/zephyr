/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SFP/SFP+ pluggable optical module drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SFP_H_
#define ZEPHYR_INCLUDE_DRIVERS_SFP_H_

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/sff/sff8472.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SFP pluggable module interface
 * @defgroup sfp_interface SFP pluggable module interface
 * @ingroup io_interfaces
 * @{
 */

/** @cond INTERNAL_HIDDEN */
__subsystem struct sfp_driver_api {
	int (*read_id)(const struct device *dev, struct sff8472_id *id);
	int (*read_diagnostics)(const struct device *dev, struct sff8472_diagnostics *diag);
};
/** @endcond */

/**
 * @brief Read and decode the module serial ID (A0h).
 *
 * @param dev SFP module device.
 * @param id Destination for the decoded identification data.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the driver does not implement this call.
 * @retval -errno Negative errno from the underlying bus transfer.
 */
static inline int sfp_read_id(const struct device *dev, struct sff8472_id *id)
{
	const struct sfp_driver_api *api = (const struct sfp_driver_api *)dev->api;

	if (api->read_id == NULL) {
		return -ENOSYS;
	}

	return api->read_id(dev, id);
}

/**
 * @brief Read and decode the module real-time diagnostics (A2h).
 *
 * @param dev SFP module device.
 * @param diag Destination for the decoded diagnostics.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the driver does not implement this call.
 * @retval -errno Negative errno from the underlying bus transfer.
 */
static inline int sfp_read_diagnostics(const struct device *dev, struct sff8472_diagnostics *diag)
{
	const struct sfp_driver_api *api = (const struct sfp_driver_api *)dev->api;

	if (api->read_diagnostics == NULL) {
		return -ENOSYS;
	}

	return api->read_diagnostics(dev, diag);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SFP_H_ */
