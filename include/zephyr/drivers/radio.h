/*
 * Copyright (c) 2026 Shontal Biton
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup radio_interface
 * @brief Main header file for Radio driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RADIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_RADIO_H_

/**
 * @brief Interfaces for Radio controllers.
 * @defgroup radio_interface Radio
 * @since 1.0
 * @version 1.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <stddef.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Driver API structure. */
__subsystem struct radio_driver_api {
	int (*start_cw)(const struct device *dev);
	int (*stop_cw)(const struct device *dev);
	int (*send)(const struct device *dev, uint16_t channel, const uint8_t* payload, int len, bool clear_fifo);
	int (*set_frequency)(const struct device *dev, uint32_t frequency);
	int (*set_tx_power)(const struct device *dev, int16_t dbm);
};

__syscall int radio_start_cw(const struct device* dev);
static inline int z_impl_radio_start_cw(const struct device* dev)
{
	return DEVICE_API_GET(radio, dev)->start_cw(dev);
}

__syscall int radio_stop_cw(const struct device* dev);
static inline int z_impl_radio_stop_cw(const struct device* dev)
{
	return DEVICE_API_GET(radio, dev)->stop_cw(dev);
}

__syscall int radio_send(const struct device* dev, uint16_t channel, const uint8_t* payload, int len, bool clear_fifo);
static inline int z_impl_radio_send(const struct device* dev, uint16_t channel, const uint8_t* payload, int len, bool clear_fifo)
{
	return DEVICE_API_GET(radio, dev)->send(dev, channel, payload, len, clear_fifo);
}

__syscall int radio_set_frequency(const struct device* dev, uint32_t frequency);
static inline int z_impl_radio_set_frequency(const struct device* dev, uint32_t frequency)
{
	return DEVICE_API_GET(radio, dev)->set_frequency(dev, frequency);
}

__syscall int radio_set_tx_power(const struct device* dev, int16_t dbm);
static inline int z_impl_radio_set_tx_power(const struct device* dev, int16_t dbm)
{
	return DEVICE_API_GET(radio, dev)->set_tx_power(dev, dbm);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/radio.h>

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_DRIVERS_RADIO_H_ */
