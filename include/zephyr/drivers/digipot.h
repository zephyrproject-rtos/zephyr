/*
 * Copyright (c) 2025 Jocelyn Masserot <jocemass@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header file for digital potentiometer driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_
#define ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef digipot_set_t
 * @brief Digipot API function for setting the wiper position.
 *
 * See digipot_set() for argument description
 */
typedef int (*digipot_set_t)(const struct device *dev, uint8_t channel, uint16_t position);


/**
 * @typedef digipot_get_t
 * @brief Digipot API function for getting the wiper position.
 *
 * See digipot_get() for argument description
 */
typedef int (*digipot_get_t)(const struct device *dev, uint8_t channel, uint16_t *position);

/**
 * @typedef digipot_reset_t
 * @brief Digipot API function for reseting the wiper to midscale position.
 *
 * See digipot_reset() for argument description
 */
typedef int (*digipot_reset_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Digipot device API
 *
 *
 */
__subsystem struct digipot_driver_api {
	digipot_set_t set_position;
	digipot_get_t get_position;
	digipot_reset_t reset_position;
};

/**
 * @brief Set the digipot wiper position
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 * @param pos digipot wiper position
 *
 * @retval 0 if successful
 * @retval < 0 if setting wiper failed
 */
static inline int digipot_set(const struct device *dev, uint8_t channel, uint16_t pos)
{
    const struct digipot_driver_api *api = (const struct digipot_driver_api *)dev->api;
    return api->set_position(dev, channel, pos);
}

/**
 * @brief Get the digipot wiper position
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 * @param pos digipot wiper position
 *
 * @retval 0 if successful
 * @retval < 0 if setting wiper failed
 */
static inline int digipot_get(const struct device *dev, uint8_t channel, uint16_t *pos)
{
    const struct digipot_driver_api *api = (const struct digipot_driver_api *)dev->api;
    return api->get_position(dev, channel, pos);
}

/**
 * @brief Reset the digipot wiper to midscale position
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 *
 * @retval 0 if successful
 * @retval < 0 if setting wiper failed
 */
static inline int digipot_reset(const struct device *dev, uint8_t channel)
{
    const struct digipot_driver_api *api = (const struct digipot_driver_api *)dev->api;
    return api->reset_position(dev, channel);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_ */
