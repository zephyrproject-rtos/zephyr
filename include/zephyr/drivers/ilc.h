/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for Interrupt Latency Counter drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ILC_H_
#define ZEPHYR_INCLUDE_DRIVERS_ILC_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ilc read params structure */
struct ilc_params {
	/* Number of port connected */
	uint8_t port_count;
	/* ILC Input frequency */
	uint32_t frequency;
};

typedef int (*ilc_api_enable_t)(const struct device *ilc_dev);
typedef int (*ilc_api_disable_t)(const struct device *ilc_dev);
typedef int (*ilc_api_read_params_t)(const struct device *ilc_dev, struct ilc_params *params);
typedef int (*ilc_api_read_counter_t)(const struct device *ilc_dev, unsigned int *counter,
				      unsigned char port);

__subsystem struct ilc_driver_api {
	ilc_api_enable_t enable;
	ilc_api_disable_t disable;
	ilc_api_read_params_t read_params;
	ilc_api_read_counter_t read_counter;
};

/**
 *  @brief  Enable ILC (Interrupt Latency Counter) Soft IP.
 *
 *  @param  dev             : ILC device instance.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int ilc_enable(const struct device *dev);

static inline int z_impl_ilc_enable(const struct device *dev)
{
	const struct ilc_driver_api *api = (const struct ilc_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 *  @brief  disable ILC (Interrupt Latency Counter) Soft IP.
 *
 *  @param  dev             : ILC device instance.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int ilc_disable(const struct device *dev);

static inline int z_impl_ilc_disable(const struct device *dev)
{
	const struct ilc_driver_api *api = (const struct ilc_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 *  @brief  Read ILC (Interrupt Latency Counter) parameters.
 *
 *  @param  dev             : ILC device instance.
 *  @param  ilc_params      : Pointer to read ILC parameters.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int ilc_read_params(const struct device *dev, struct ilc_params *params);

static inline int z_impl_ilc_read_params(const struct device *dev, struct ilc_params *params)
{
	const struct ilc_driver_api *api = (const struct ilc_driver_api *)dev->api;

	return api->read_params(dev, params);
}

/**
 *  @brief  Read counter for specified port from ILC (Interrupt Latency Counter)
 *
 *  @param dev              : ilc dev
 *  @param counter          : The counter from which to read within the ILC.
 *  @param port             : Port number to be read from the ILC.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int ilc_read_counter(const struct device *dev, uint32_t *counter, uint8_t port);

static inline int z_impl_ilc_read_counter(const struct device *dev, uint32_t *counter, uint8_t port)
{
	const struct ilc_driver_api *api = (const struct ilc_driver_api *)dev->api;
	int ret;

	ret = api->read_counter(dev, counter, port);
	return ret;
}

#ifdef __cplusplus
}
#endif

#include <syscalls/ilc.h>

#endif
