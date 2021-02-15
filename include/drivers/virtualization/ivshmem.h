/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_

/**
 * @brief ivshmem reference API
 * @defgroup ivshmem ivshmem reference API
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*ivshmem_get_mem_f)(const struct device *dev,
				    uintptr_t *memmap);

typedef uint32_t (*ivshmem_get_id_f)(const struct device *dev);

typedef uint16_t (*ivshmem_get_vectors_f)(const struct device *dev);

typedef int (*ivshmem_int_peer_f)(const struct device *dev,
				  uint32_t peer_id, uint16_t vector);

typedef int (*ivshmem_register_handler_f)(const struct device *dev,
					  struct k_poll_signal *signal,
					  uint16_t vector);

__subsystem struct ivshmem_driver_api {
	ivshmem_get_mem_f get_mem;
	ivshmem_get_id_f get_id;
	ivshmem_get_vectors_f get_vectors;
	ivshmem_int_peer_f int_peer;
	ivshmem_register_handler_f register_handler;
};

/**
 * @brief Get the inter-VM shared memory
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param memmap A pointer to fill in with the  memory address
 *
 * @return the size of the memory mapped, or 0
 */
__syscall size_t ivshmem_get_mem(const struct device *dev,
				 uintptr_t *memmap);

static inline size_t z_impl_ivshmem_get_mem(const struct device *dev,
					    uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_mem(dev, memmap);
}

/**
 * @brief Get our VM ID
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return our VM ID or 0 if we are not running on doorbell version
 */
__syscall uint32_t ivshmem_get_id(const struct device *dev);

static inline uint32_t z_impl_ivshmem_get_id(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_id(dev);
}

/**
 * @brief Get the number of interrupt vectors we can use
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return the number of available interrupt vectors
 */
__syscall uint16_t ivshmem_get_vectors(const struct device *dev);

static inline uint16_t z_impl_ivshmem_get_vectors(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_vectors(dev);
}

/**
 * @brief Interrupt another VM
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param peer_id The VM ID to interrupt
 * @param vector The interrupt vector to use
 *
 * @return 0 on success, a negative errno otherwise
 */
__syscall int ivshmem_int_peer(const struct device *dev,
			       uint32_t peer_id, uint16_t vector);

static inline int z_impl_ivshmem_int_peer(const struct device *dev,
					  uint32_t peer_id, uint16_t vector)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->int_peer(dev, peer_id, vector);
}

/**
 * @brief Register a vector notification (interrupt) handler
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param signal A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. Or NULL to unregister any handler
 *        registered for the given vector.
 * @param vector The interrupt vector to get notification from
 *
 * Note: The returned status, if positive, to a raised signal is the vector
 *       that generated the signal. This lets the possibility to the user
 *       to have one signal for all vectors, or one per-vector.
 *
 * @return 0 on success, a negative errno otherwise
 */
__syscall int ivshmem_register_handler(const struct device *dev,
				       struct k_poll_signal *signal,
				       uint16_t vector);

static inline int z_impl_ivshmem_register_handler(const struct device *dev,
						  struct k_poll_signal *signal,
						  uint16_t vector)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->register_handler(dev, signal, vector);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/ivshmem.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_ */
