/*
 * Copyright (c) 2025 Jocelyn Masserot <jocemass@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup digitpot_interface
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
 * @typedef digipot_set_wiper_t
 * @brief Digipot API function for setting the wiper position.
 *
 * See digipot_set_wiper() for argument description
 */
typedef int (*digipot_set_wiper_t)(const struct device *dev, uint8_t channel, uint16_t position);


/**
 * @typedef digipot_get_wiper_t
 * @brief Digipot API function for getting the wiper position.
 *
 * See digipot_get_wiper() for argument description
 */
typedef int (*digipot_get_wiper_t)(const struct device *dev, uint8_t channel, uint16_t *position);

/**
 * @typedef digipot_reset_wiper_t
 * @brief Digipot API function for reseting the wiper to midscale position.
 *
 * See digipot_reset_wiper() for argument description
 */
typedef int (*digipot_reset_wiper_t)(const struct device *dev, uint8_t channel);

/**
 * @typedef digipot_shutdown_t
 * @brief Digipot API function for shutting down the device's channel.
 *
 * See digipot_shutdown() for argument description
 */
typedef int (*digipot_shutdown_t)(const struct device *dev, uint8_t channel);

/**
 * @brief Digipot device API
 *
 *
 */
__subsystem struct digipot_driver_api {
    digipot_set_wiper_t set_position;
    digipot_get_wiper_t get_position;
    digipot_reset_wiper_t reset_position;
    digipot_shutdown_t shutdown;
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
static inline int digipot_set_wiper(const struct device *dev, uint8_t channel, uint16_t pos)
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
static inline int digipot_get_wiper(const struct device *dev, uint8_t channel, uint16_t *pos)
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
static inline int digipot_reset_wiper(const struct device *dev, uint8_t channel)
{
    const struct digipot_driver_api *api = (const struct digipot_driver_api *)dev->api;
    return api->reset_position(dev, channel);
}

/**
 * @brief Shutdown the digipot wiper channel
 *
 * @param dev Pointer to the digipot device
 * @param channel digipot RDAC channel
 *
 * @retval 0 if successful
 * @retval < 0 if setting wiper failed
 */
static inline int digipot_shutdown(const struct device *dev, uint8_t channel)
{
    const struct digipot_driver_api *api = (const struct digipot_driver_api *)dev->api;
    return api->shutdown(dev, channel);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DIGIPOT_H_ */