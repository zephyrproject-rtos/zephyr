/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_

/**
 * @brief Interfaces for Inter-VM Shared Memory (ivshmem).
 * @defgroup ivshmem Inter-VM Shared Memory
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

/**
 * @def_driverbackendgroup{Inter-VM Shared Memory,ivshmem}
 * @{
 */

/**
 * @brief Callback API to get the inter-VM shared memory.
 * See ivshmem_get_mem() for argument description.
 */
typedef size_t (*ivshmem_get_mem_f)(const struct device *dev,
				    uintptr_t *memmap);

/**
 * @brief Callback API to get our VM ID.
 * See ivshmem_get_id() for argument description.
 */
typedef uint32_t (*ivshmem_get_id_f)(const struct device *dev);

/**
 * @brief Callback API to get the number of interrupt vectors we can use.
 * See ivshmem_get_vectors() for argument description.
 */
typedef uint16_t (*ivshmem_get_vectors_f)(const struct device *dev);

/**
 * @brief Callback API to interrupt another VM.
 * See ivshmem_int_peer() for argument description.
 */
typedef int (*ivshmem_int_peer_f)(const struct device *dev,
				  uint32_t peer_id, uint16_t vector);

/**
 * @brief Callback API to register a vector notification (interrupt) handler.
 * See ivshmem_register_handler() for argument description.
 */
typedef int (*ivshmem_register_handler_f)(const struct device *dev,
					  struct k_poll_signal *signal,
					  uint16_t vector);

#if defined(CONFIG_IVSHMEM_V2) || defined(__DOXYGEN__)

/**
 * @brief Callback API to get the ivshmem read/write section.
 * See ivshmem_get_rw_mem_section() for argument description.
 */
typedef size_t (*ivshmem_get_rw_mem_section_f)(const struct device *dev,
					       uintptr_t *memmap);

/**
 * @brief Callback API to get the ivshmem output section for a peer.
 * See ivshmem_get_output_mem_section() for argument description.
 */
typedef size_t (*ivshmem_get_output_mem_section_f)(const struct device *dev,
						   uint32_t peer_id,
						   uintptr_t *memmap);

/**
 * @brief Callback API to get the state value of a peer.
 * See ivshmem_get_state() for argument description.
 */
typedef uint32_t (*ivshmem_get_state_f)(const struct device *dev,
					uint32_t peer_id);

/**
 * @brief Callback API to set our state.
 * See ivshmem_set_state() for argument description.
 */
typedef int (*ivshmem_set_state_f)(const struct device *dev,
				   uint32_t state);

/**
 * @brief Callback API to get the maximum number of peers supported.
 * See ivshmem_get_max_peers() for argument description.
 */
typedef uint32_t (*ivshmem_get_max_peers_f)(const struct device *dev);

/**
 * @brief Callback API to get the protocol used by this ivshmem instance.
 * See ivshmem_get_protocol() for argument description.
 */
typedef uint16_t (*ivshmem_get_protocol_f)(const struct device *dev);

/**
 * @brief Callback API to set the interrupt enablement for our VM.
 * See ivshmem_enable_interrupts() for argument description.
 */
typedef int (*ivshmem_enable_interrupts_f)(const struct device *dev,
					   bool enable);

#endif /* CONFIG_IVSHMEM_V2 */

/**
 * @driver_ops{Inter-VM Shared Memory}
 */
__subsystem struct ivshmem_driver_api {
	/** @driver_ops_mandatory @copybrief ivshmem_get_mem */
	ivshmem_get_mem_f get_mem;
	/** @driver_ops_mandatory @copybrief ivshmem_get_id */
	ivshmem_get_id_f get_id;
	/** @driver_ops_mandatory @copybrief ivshmem_get_vectors */
	ivshmem_get_vectors_f get_vectors;
	/** @driver_ops_mandatory @copybrief ivshmem_int_peer */
	ivshmem_int_peer_f int_peer;
	/** @driver_ops_mandatory @copybrief ivshmem_register_handler */
	ivshmem_register_handler_f register_handler;
#if defined(CONFIG_IVSHMEM_V2) || defined(__DOXYGEN__)
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_get_rw_mem_section
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_get_rw_mem_section_f get_rw_mem_section;
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_get_output_mem_section
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_get_output_mem_section_f get_output_mem_section;
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_get_state
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_get_state_f get_state;
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_set_state
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_set_state_f set_state;
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_get_max_peers
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_get_max_peers_f get_max_peers;
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_get_protocol
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_get_protocol_f get_protocol;
	/**
	 * @driver_ops_mandatory @copybrief ivshmem_enable_interrupts
	 * @kconfig_dep{CONFIG_IVSHMEM_V2}
	 */
	ivshmem_enable_interrupts_f enable_interrupts;
#endif
};

/** @} */

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
	return DEVICE_API_GET(ivshmem, dev)->get_mem(dev, memmap);
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
	return DEVICE_API_GET(ivshmem, dev)->get_id(dev);
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
	return DEVICE_API_GET(ivshmem, dev)->get_vectors(dev);
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
	return DEVICE_API_GET(ivshmem, dev)->int_peer(dev, peer_id, vector);
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
	return DEVICE_API_GET(ivshmem, dev)->register_handler(dev, signal, vector);
}

#if defined(CONFIG_IVSHMEM_V2) || defined(__DOXYGEN__)

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
	return DEVICE_API_GET(ivshmem, dev)->get_rw_mem_section(dev, memmap);
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
	return DEVICE_API_GET(ivshmem, dev)->get_output_mem_section(dev, peer_id, memmap);
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
	return DEVICE_API_GET(ivshmem, dev)->get_state(dev, peer_id);
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
	return DEVICE_API_GET(ivshmem, dev)->set_state(dev, state);
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
	return DEVICE_API_GET(ivshmem, dev)->get_max_peers(dev);
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
	return DEVICE_API_GET(ivshmem, dev)->get_protocol(dev);
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
	return DEVICE_API_GET(ivshmem, dev)->enable_interrupts(dev, enable);
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
