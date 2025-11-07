/*
 * Copyright (c) 2025 Jocelyn Masserot <jocemass@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup digipot_interface
 * @brief Main header file for digital potentiometer driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_
#define ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_

/**
 * @brief Interfaces for Digital Potentiometers.
 * @defgroup digipot_interface Digipots
 * @since 4.6
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def_driverbackendgroup{Digipot, digipot_interface}
 * @{
 */

/**
 * @brief Digipot API function for setting the wiper position.
 *
 * See digipot_set() for argument description.
 */
typedef int (*digipot_set_t)(const struct device *dev, uint8_t channel, uint16_t position);

/**
 * @brief Digipot API function for getting the wiper position.
 *
 * See digipot_get() for argument description.
 */
typedef int (*digipot_get_t)(const struct device *dev, uint8_t channel, uint16_t *position);

/**
 * @brief Digipot API function for resetting the wiper to midscale position.
 *
 * See digipot_reset() for argument description.
 */
typedef int (*digipot_reset_t)(const struct device *dev, uint8_t channel);

/**
 * @driver_ops{Digipot}
 */
__subsystem struct digipot_driver_api {
	/** @driver_ops_mandatory @copybrief digipot_set */
	digipot_set_t set_position;
	/** @driver_ops_mandatory @copybrief digipot_get */
	digipot_get_t get_position;
	/** @driver_ops_mandatory @copybrief digipot_reset */
	digipot_reset_t reset_position;
};

/** @} */

/**
 * @brief Set the digipot wiper position
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 * @param pos digipot wiper position
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid channel or position.
 * @retval -EIO I2C communication error.
 */
static inline int digipot_set(const struct device *dev, uint8_t channel, uint16_t pos)
{
	return DEVICE_API_GET(digipot, dev)->set_position(dev, channel, pos);
}

/**
 * @brief Get the digipot wiper position
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 * @param pos digipot wiper position
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid channel.
 * @retval -EIO I2C communication error.
 */
static inline int digipot_get(const struct device *dev, uint8_t channel, uint16_t *pos)
{
	return DEVICE_API_GET(digipot, dev)->get_position(dev, channel, pos);
}

/**
 * @brief Reset the digipot wiper to midscale position
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid channel.
 * @retval -EIO I2C communication error.
 */
static inline int digipot_reset(const struct device *dev, uint8_t channel)
{
	return DEVICE_API_GET(digipot, dev)->reset_position(dev, channel);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_ */
