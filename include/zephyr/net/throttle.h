/**
 * @file
 * @brief Network traffic throttle API
 *
 * API can be used to tell the device driver to throttle
 * the network traffic.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_THROTTLE_H_
#define ZEPHYR_INCLUDE_NET_THROTTLE_H_

/**
 * @brief Network traffic throttle API
 * @defgroup net_traffic_throttle Network traffic throttle API
 * @ingroup networking
 * @{
 */

#include <sys/types.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_throttle_api {
	/**
	 * This function is used to init the throttle functionality
	 * in the network device driver.
	 * If is_enabled is set to true, then the throttler is enabled
	 * already at initialization.
	 * If speed (bytes / sec) is > 0, then the speed limit if turned
	 * enabled at initialization.
	 * If period is set to 0, then there is no period at initialization.
	 */
	void (*init)(struct net_if *iface, bool is_enabled,
		     uint64_t speed, k_timepoint_t period);

	/**
	 * This function is used to enable/disable received traffic temporarily
	 * over a network interface. The device driver should stop receiving
	 * any traffic, the upper stack needs to re-enable i.e., call this
	 * function again to allow network traffic to be received.
	 * The function returns <0 if error and >=0 if no error.
	 */
	int (*enable)(struct net_if *iface, bool state);

	/**
	 * This function is used to set the upper limit after which
	 * the network traffic is throttled.
	 * If this is set, then the device driver will start to throttle
	 * the received network data if the amount of received network data
	 * goes over the specified limit over a specified time period.
	 * If the time period is set to 0, then the device driver should limit
	 * the received data rate to the specified amount.
	 * This could be used if we cannot handle all the network traffic in
	 * the system.
	 * The function returns <0 if error and >=0 if no error.
	 */
	int (*watermark)(struct net_if *iface, uint64_t bytes_per_sec,
			 k_timepoint_t period);
};

struct net_throttle_desc {
	/**
	 * Throttle API functions.
	 */
	const struct net_throttle_api *api;

	/**
	 * Name for the throttle. This can be used to identify who has
	 * registered this service.
	 */
	const char * const name;

	/**
	 * Device that can be throttled.
	 */
	const struct device *dev;
};

/** @cond INTERNAL_HIDDEN */

#define __z_net_throttle_get_name(_name) __z_net_throttle_##_name

#define __z_net_throttle_define(_name, _dev, _api)		\
	static const STRUCT_SECTION_ITERABLE(net_throttle_desc,	\
					     _name) = {		\
		.name = STRINGIFY(_name),			\
		.dev = &(DEVICE_NAME_GET(_dev)),		\
		.api = (_api),					\
	}

/** @endcond */

/**
 * @brief ...
 *
 * @param _name Name of the throttler.
 * @param _dev Device that can be throttled.
 * @param _api API functions to be called by upper stack.
 */
#define NET_TRAFFIC_THROTTLE_DEFINE(_name, _dev, _api)		\
	IF_ENABLED(CONFIG_NET_THROTTLE,				\
		   (__z_net_throttle_define(_name, _dev, _api)))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_THROTTLE_H_ */
