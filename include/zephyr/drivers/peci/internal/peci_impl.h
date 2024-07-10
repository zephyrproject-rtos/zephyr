/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for PECI driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PECI_H_
#error "Should only be included by zephyr/drivers/peci.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_PECI_INTERNAL_PECI_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PECI_INTERNAL_PECI_IMPL_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_peci_config(const struct device *dev,
				     uint32_t bitrate)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->config(dev, bitrate);
}

static inline int z_impl_peci_enable(const struct device *dev)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->enable(dev);
}

static inline int z_impl_peci_disable(const struct device *dev)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->disable(dev);
}

static inline int z_impl_peci_transfer(const struct device *dev,
				       struct peci_msg *msg)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->transfer(dev, msg);
}


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PECI_INTERNAL_PECI_IMPL_H_ */
