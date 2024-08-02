/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_

/**
 * @brief Inter-VM Shared Memory (ivshmem) reference API
 * @defgroup ivshmem Inter-VM Shared Memory (ivshmem) reference API
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IVSHMEM_V2_PROTO_UNDEFINED	0x0000
#define IVSHMEM_V2_PROTO_NET		0x0001

typedef size_t (*ivshmem_get_mem_f)(const struct device *dev,
				    uintptr_t *memmap);

typedef uint32_t (*ivshmem_get_id_f)(const struct device *dev);

typedef uint16_t (*ivshmem_get_vectors_f)(const struct device *dev);

typedef int (*ivshmem_int_peer_f)(const struct device *dev,
				  uint32_t peer_id, uint16_t vector);

typedef int (*ivshmem_register_handler_f)(const struct device *dev,
					  struct k_poll_signal *signal,
					  uint16_t vector);

#ifdef CONFIG_IVSHMEM_V2

typedef size_t (*ivshmem_get_rw_mem_section_f)(const struct device *dev,
					       uintptr_t *memmap);

typedef size_t (*ivshmem_get_output_mem_section_f)(const struct device *dev,
						   uint32_t peer_id,
						   uintptr_t *memmap);

typedef uint32_t (*ivshmem_get_state_f)(const struct device *dev,
					uint32_t peer_id);

typedef int (*ivshmem_set_state_f)(const struct device *dev,
				   uint32_t state);

typedef uint32_t (*ivshmem_get_max_peers_f)(const struct device *dev);

typedef uint16_t (*ivshmem_get_protocol_f)(const struct device *dev);

typedef int (*ivshmem_enable_interrupts_f)(const struct device *dev,
					   bool enable);

#endif /* CONFIG_IVSHMEM_V2 */

__subsystem struct ivshmem_driver_api {
	ivshmem_get_mem_f get_mem;
	ivshmem_get_id_f get_id;
	ivshmem_get_vectors_f get_vectors;
	ivshmem_int_peer_f int_peer;
	ivshmem_register_handler_f register_handler;
#ifdef CONFIG_IVSHMEM_V2
	ivshmem_get_rw_mem_section_f get_rw_mem_section;
	ivshmem_get_output_mem_section_f get_output_mem_section;
	ivshmem_get_state_f get_state;
	ivshmem_set_state_f set_state;
	ivshmem_get_max_peers_f get_max_peers;
	ivshmem_get_protocol_f get_protocol;
	ivshmem_enable_interrupts_f enable_interrupts;
#endif
};

/**
 * @brief Get the inter-VM shared memory
 *
 * Note: This API is not supported for ivshmem-v2, as
 * the R/W and R/O areas may not be mapped contiguously.
 * For ivshmem-v2, use the ivshmem_get_rw_mem_section,
 * ivshmem_get_output_mem_section and ivshmem_get_state
 * APIs to access the shared memory.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param memmap A pointer to fill in with the memory address
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

#ifdef CONFIG_IVSHMEM_V2

/**
 * @brief Get the ivshmem read/write section (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param memmap A pointer to fill in with the memory address
 *
 * @return the size of the memory mapped, or 0
 */
__syscall size_t ivshmem_get_rw_mem_section(const struct device *dev,
					    uintptr_t *memmap);

static inline size_t z_impl_ivshmem_get_rw_mem_section(const struct device *dev,
						       uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_rw_mem_section(dev, memmap);
}

/**
 * @brief Get the ivshmem output section for a peer (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param peer_id The VM ID whose output memory section to get
 * @param memmap A pointer to fill in with the memory address
 *
 * @return the size of the memory mapped, or 0
 */
__syscall size_t ivshmem_get_output_mem_section(const struct device *dev,
						uint32_t peer_id,
						uintptr_t *memmap);

static inline size_t z_impl_ivshmem_get_output_mem_section(const struct device *dev,
							   uint32_t peer_id,
							   uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_output_mem_section(dev, peer_id, memmap);
}

/**
 * @brief Get the state value of a peer (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param peer_id The VM ID whose state to get
 *
 * @return the state value of the peer
 */
__syscall uint32_t ivshmem_get_state(const struct device *dev,
				     uint32_t peer_id);

static inline uint32_t z_impl_ivshmem_get_state(const struct device *dev,
						uint32_t peer_id)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_state(dev, peer_id);
}

/**
 * @brief Set our state (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param state The state value to set
 *
 * @return 0 on success, a negative errno otherwise
 */
__syscall int ivshmem_set_state(const struct device *dev,
				uint32_t state);

static inline int z_impl_ivshmem_set_state(const struct device *dev,
					   uint32_t state)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->set_state(dev, state);
}

/**
 * @brief Get the maximum number of peers supported (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return the maximum number of peers supported, or 0
 */
__syscall uint32_t ivshmem_get_max_peers(const struct device *dev);

static inline uint32_t z_impl_ivshmem_get_max_peers(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_max_peers(dev);
}

/**
 * @brief Get the protocol used by this ivshmem instance (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return the protocol
 */
__syscall uint16_t ivshmem_get_protocol(const struct device *dev);

static inline uint16_t z_impl_ivshmem_get_protocol(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_protocol(dev);
}

/**
 * @brief Set the interrupt enablement for our VM (ivshmem-v2 only)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable True to enable interrupts, false to disable
 *
 * @return 0 on success, a negative errno otherwise
 */
__syscall int ivshmem_enable_interrupts(const struct device *dev,
					bool enable);

static inline int z_impl_ivshmem_enable_interrupts(const struct device *dev,
						   bool enable)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->enable_interrupts(dev, enable);
}

#endif /* CONFIG_IVSHMEM_V2 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/ivshmem.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_ */
