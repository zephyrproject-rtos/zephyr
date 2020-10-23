/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*ivshmem_get_mem_f)(const struct device *dev,
				    uintptr_t *memmap);

struct ivshmem_driver_api {
	ivshmem_get_mem_f get_mem;
};

/**
 * @brief Get the inter-VM shared memory
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param memmap A pointer to fill in with the  memory address
 *
 * @return the size of the memory mapped, or 0
 */
static inline size_t ivshmem_get_mem(const struct device *dev,
				     uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_mem(dev, memmap);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_ */
