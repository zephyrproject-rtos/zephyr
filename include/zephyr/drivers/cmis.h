/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for CMIS pluggable optical module drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CMIS_H_
#define ZEPHYR_INCLUDE_DRIVERS_CMIS_H_

#include <errno.h>

#include <zephyr/cmis/cmis.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CMIS pluggable module interface
 * @defgroup cmis_interface CMIS pluggable module interface
 * @ingroup io_interfaces
 * @{
 */

/** @cond INTERNAL_HIDDEN */
__subsystem struct cmis_driver_api {
	int (*read_id)(const struct device *dev, struct cmis_id *id);
	int (*read_monitors)(const struct device *dev, struct cmis_monitors *mon);
};
/** @endcond */

/**
 * @brief Read and decode the module identification (Page 00h).
 *
 * @param dev CMIS module device.
 * @param id Destination for the decoded identification data.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the driver does not implement this call.
 * @retval -errno Negative errno from the underlying bus transfer.
 */
static inline int cmis_read_id(const struct device *dev, struct cmis_id *id)
{
	const struct cmis_driver_api *api = (const struct cmis_driver_api *)dev->api;

	if (api->read_id == NULL) {
		return -ENOSYS;
	}

	return api->read_id(dev, id);
}

/**
 * @brief Read and decode the module-level monitors (Lower Memory).
 *
 * @param dev CMIS module device.
 * @param mon Destination for the decoded monitors.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the driver does not implement this call.
 * @retval -errno Negative errno from the underlying bus transfer.
 */
static inline int cmis_read_monitors(const struct device *dev, struct cmis_monitors *mon)
{
	const struct cmis_driver_api *api = (const struct cmis_driver_api *)dev->api;

	if (api->read_monitors == NULL) {
		return -ENOSYS;
	}

	return api->read_monitors(dev, mon);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CMIS_H_ */
