/*
 * Copyright 2022 The Chromium OS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB-C VBUS device APIs
 *
 * This file contains the USB-C VBUS device APIs.
 * All USB-C VBUS measurement and control device drivers should
 * implement the APIs described in this file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_VBUS_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_VBUS_H_

/**
 * @brief USB-C VBUS API
 * @defgroup usbc_vbus_api USB-C VBUS API
 * @since 3.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb_c/usbc_tc.h>

#ifdef __cplusplus
extern "C" {
#endif

__subsystem struct usbc_vbus_driver_api {
	bool (*check_level)(const struct device *dev, enum tc_vbus_level level);
	int (*measure)(const struct device *dev, int *vbus_meas);
	int (*discharge)(const struct device *dev, bool enable);
	int (*enable)(const struct device *dev, bool enable);
};

/**
 * @brief Checks if VBUS is at a particular level
 *
 * @param dev    Runtime device structure
 * @param level  The level voltage to check against
 *
 * @retval true if VBUS is at the level voltage
 * @retval false if VBUS is not at that level voltage
 */
static inline bool usbc_vbus_check_level(const struct device *dev, enum tc_vbus_level level)
{
	const struct usbc_vbus_driver_api *api = (const struct usbc_vbus_driver_api *)dev->api;

	return api->check_level(dev, level);
}

/**
 * @brief Reads and returns VBUS measured in mV
 *
 * @param dev        Runtime device structure
 * @param meas       pointer where the measured VBUS voltage is stored
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static inline int usbc_vbus_measure(const struct device *dev, int *meas)
{
	const struct usbc_vbus_driver_api *api = (const struct usbc_vbus_driver_api *)dev->api;

	return api->measure(dev, meas);
}

/**
 * @brief Controls a pin that discharges VBUS
 *
 * @param dev        Runtime device structure
 * @param enable     Discharge VBUS when true
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOENT if discharge pin isn't defined
 */
static inline int usbc_vbus_discharge(const struct device *dev, bool enable)
{
	const struct usbc_vbus_driver_api *api = (const struct usbc_vbus_driver_api *)dev->api;

	return api->discharge(dev, enable);
}

/**
 * @brief Controls a pin that enables VBUS measurements
 *
 * @param dev     Runtime device structure
 * @param enable  enable VBUS measurements when true
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOENT if enable pin isn't defined
 */
static inline int usbc_vbus_enable(const struct device *dev, bool enable)
{
	const struct usbc_vbus_driver_api *api = (const struct usbc_vbus_driver_api *)dev->api;

	return api->enable(dev, enable);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_VBUS_H_ */
