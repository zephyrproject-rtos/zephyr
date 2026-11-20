/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SFP/SFP+ pluggable module drivers
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
 * @brief Interfaces for SFF-8472 managed pluggable transceivers.
 * @defgroup sfp_interface SFP pluggable module
 * @since 4.6
 * @version 0.1.0
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
 * @brief Read and decode the serial ID (A0h) of the module in the cage.
 *
 * @param dev SFP cage device.
 * @param id Destination for the decoded identification data.
 *
 * @retval 0 Success.
 * @retval -ENODEV The cage reports that no module is inserted.
 * @retval -ENOSYS The driver does not implement this call.
 * @retval -errno Negative errno from the presence input or the bus transfer.
 */
__syscall int sfp_read_id(const struct device *dev, struct sff8472_id *id);

static inline int z_impl_sfp_read_id(const struct device *dev, struct sff8472_id *id)
{
	const struct sfp_driver_api *api = (const struct sfp_driver_api *)dev->api;

	if (api->read_id == NULL) {
		return -ENOSYS;
	}

	return api->read_id(dev, id);
}

/**
 * @brief Read and decode the real-time diagnostics (A2h) of the module.
 *
 * Only internally calibrated diagnostics are decoded. Modules that do not
 * implement diagnostics, that need external calibration constants or that
 * require an address change sequence before A2h can be read are rejected
 * with -ENOTSUP rather than returning values that look valid but are not.
 *
 * @param dev SFP cage device.
 * @param diag Destination for the decoded diagnostics.
 *
 * @retval 0 Success.
 * @retval -ENODEV The cage reports that no module is inserted.
 * @retval -ENOTSUP The module's diagnostics cannot be decoded (see above).
 * @retval -ENOSYS The driver does not implement this call.
 * @retval -errno Negative errno from the presence input or the bus transfer.
 */
__syscall int sfp_read_diagnostics(const struct device *dev, struct sff8472_diagnostics *diag);

static inline int z_impl_sfp_read_diagnostics(const struct device *dev,
					      struct sff8472_diagnostics *diag)
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

#include <zephyr/syscalls/sfp.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SFP_H_ */
