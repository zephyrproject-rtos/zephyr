/** @file
 * @brief Offloaded iface API
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_OFFLOADED_IF_H_
#define ZEPHYR_INCLUDE_OFFLOADED_IF_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Offloaded ifaces
 * @defgroup offloaded_if Offloaded ifaces
 * @ingroup networking
 * @{
 */

/**
 * @brief Extended net_if_api for offloaded ifaces, allowing direct handling of
 *	  admin up/down state changes via start/stop
 */
struct offloaded_if_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Enable or disable the device (in response to admin state change) */
	int (*enable)(const struct device *dev, bool state);
};

/* Ensure offloaded_if_api is compatible with net_if_api */
BUILD_ASSERT(offsetof(struct offloaded_if_api, iface_api) == 0);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/ethernet.h>

#endif /* ZEPHYR_INCLUDE_OFFLOADED_IF_H_ */
