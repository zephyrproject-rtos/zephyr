
/**
 * @file
 * @brief Ethernet public API header file.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INCLUDE_ETH_H__
#define __INCLUDE_ETH_H__

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ethernet driver API
 *
 * This structure holds all API function pointers.
 */
struct eth_driver_api {
	int (*send)(struct device *dev, uint8_t *buffer, uint16_t len);
	void (*register_callback)(struct device *dev,
		void (*cb)(uint8_t *buffer, uint16_t len));
};

/**
 * @brief Sends a frame to ethernet hardware
 *
 * This routine writes a buffer to be sent through the ethernet hardware.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buffer Byte array to be sent through the device
 * @param len Length of the array
 *
 * @return Error code
 */
static inline int eth_send(struct device *dev, uint8_t *buffer, uint16_t len)
{
	const struct eth_driver_api *api = dev->driver_api;

	return api->send(dev, buffer, len);
}

/**
 * @brief Register callback for received frame
 *
 * This routine writes a buffer to be sent through the ethernet hardware.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cb  Callback function to be registered in the driver.
 *
 * @return Error code
 */
static inline void eth_register_callback(struct device *dev,
		void (*cb)(uint8_t *buffer, uint16_t len))
{
	const struct eth_driver_api *api = dev->driver_api;

	api->register_callback(dev, cb);
}

#ifdef __cplusplus
}
#endif

#endif /*__INCLUDE_ETH_H__*/
