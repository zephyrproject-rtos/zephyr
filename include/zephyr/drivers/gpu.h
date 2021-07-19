/*
 * Copyright 2022, Basalte bv
 * All right reserved.
 *
 * SPDX-License-Identifier: apache-2.0
 */

/**
 * @file
 *
 * @brief Dedicated APIs for GPU drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPU_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPU_H_

#include <zephyr/types.h>
#include <device.h>

/**
 * @brief GPU Interface
 * @defgroup gpu_interface GPU Interface
 * @{
 */
typedef int (*gpu_api_start)(const struct device *dev);
typedef int (*gpu_api_wait_complete)(const struct device *dev);
typedef int (*gpu_api_stop)(const struct device *dev);

__subsystem struct gpu_driver_api {
	gpu_api_start start;
	gpu_api_wait_complete wait_complete;
	gpu_api_stop stop;
};

/**
 * @brief Start GPU processing.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval negative on error.
 */

__syscall int gpu_start(const struct device *dev);
static inline int z_impl_gpu_start(const struct device *dev)
{
	const struct gpu_driver_api *api =
		(const struct gpu_driver_api *)dev->api;

	if (api->start == NULL) {
		return -ENOTSUP;
	}

	return api->start(dev);
}

/**
 * @brief Wait until the GPU processing is done.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval negative on error.
 */

__syscall int gpu_wait_complete(const struct device *dev);
static inline int z_impl_gpu_wait_complete(const struct device *dev)
{
	const struct gpu_driver_api *api =
		(const struct gpu_driver_api *)dev->api;

	if (api->wait_complete == NULL) {
		return -ENOTSUP;
	}

	return api->wait_complete(dev);
}

/**
 * @brief Stop the GPU.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval negative on error.
 */

__syscall int gpu_stop(const struct device *dev);
static inline int z_impl_gpu_stop(const struct device *dev)
{
	const struct gpu_driver_api *api =
		(const struct gpu_driver_api *)dev->api;

	if (api->stop == NULL) {
		return -ENOTSUP;
	}

	return api->stop(dev);
}

/**
 * @}
 */

#include <syscalls/gpu.h>
#endif/* ZEPHYR_INCLUDE_DRIVERS_GPU_H_ */
