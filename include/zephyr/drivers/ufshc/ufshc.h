/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHY_INCLUDE_DRIVERS_UFS_UFSHC_H_
#define ZEPHY_INCLUDE_DRIVERS_UFS_UFSHC_H_

#include <errno.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UFSHC Driver APIs
 * @defgroup ufshc_interface UFSHC Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief UFS Host Controller (UFSHC) Driver API interface
 *
 * This structure defines the set of functions that must be implemented by the
 * UFS Host Controller driver for PHY initialization and link startup notifications.
 */
__subsystem struct ufshc_driver_api {
	/**
	 * @brief PHY initialization for the UFS Host Controller.
	 *
	 * This function is called to initialize the PHY layer of the UFS Host Controller.
	 *
	 * @param dev The device pointer to the UFS Host Controller device.
	 *
	 * @return 0 on success, negative errno value on failure.
	 *         -ETIMEDOUT: command timed out during execution.
	 *         -ENOTSUP: host controller does not support this operation.
	 *         -EIO: I/O error.
	 */
	int32_t (*phy_initialization)(const struct device *dev);

	/**
	 * @brief Link startup notification for UFS Host Controller.
	 *
	 * This function notifies the host controller about the link startup status.
	 *
	 * @param dev The device pointer to the UFS Host Controller device.
	 * @param status The status of the link startup, either PRE or POST.
	 *
	 * @return 0 on success, negative errno value on failure.
	 *         -ETIMEDOUT: command timed out during execution.
	 *         -ENOTSUP: host controller does not support this operation.
	 *         -EIO: I/O error.
	 */
	int32_t (*link_startup_notify)(const struct device *dev, uint8_t status);
};

/**
 * @brief PHY initialization function for UFS Host Controller.
 *
 * This system call is used to initialize the PHY layer of the UFS Host Controller.
 * It interacts with the driver through the `ufshc_driver_api` interface.
 *
 * @param dev The device pointer to the UFS Host Controller device.
 *
 * @return 0 on success, negative errno value on failure.
 *         -ETIMEDOUT: command timed out during execution.
 *         -ENOTSUP: host controller does not support this operation.
 *         -EIO: I/O error.
 */
__syscall int32_t ufshc_variant_phy_initialization(const struct device *dev);

static inline int32_t z_impl_ufshc_variant_phy_initialization(const struct device *dev)
{
	const struct ufshc_driver_api *api = (const struct ufshc_driver_api *)dev->api;

	if (api->phy_initialization == NULL) {
		return -ENOSYS;
	}

	return api->phy_initialization(dev);
}

/**
 * @brief Link startup notification function for UFS Host Controller.
 *
 * This system call is used to notify the UFS Host Controller about the status of
 * the link startup (PRE or POST).
 *
 * @param dev The device pointer to the UFS Host Controller device.
 * @param status The status of the link startup, either PRE or POST.
 *
 * @return 0 on success, negative errno value on failure.
 *         -ETIMEDOUT: command timed out during execution.
 *         -ENOTSUP: host controller does not support this operation.
 *         -EIO: I/O error.
 */
__syscall int32_t ufshc_variant_link_startup_notify(const struct device *dev,
					uint8_t status);

static inline int32_t z_impl_ufshc_variant_link_startup_notify(const struct device *dev,
	uint8_t status)
{
	const struct ufshc_driver_api *api = (const struct ufshc_driver_api *)dev->api;

	if (api->link_startup_notify == NULL) {
		return -ENOSYS;
	}

	return api->link_startup_notify(dev, status);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/ufshc.h>

#endif /* ZEPHY_INCLUDE_DRIVERS_UFS_UFSHC_H_ */
