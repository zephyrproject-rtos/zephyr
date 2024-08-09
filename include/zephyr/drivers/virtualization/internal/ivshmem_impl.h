/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_IVSHMEM_H_
#error "Should only be included by zephyr/drivers/virtualization/ivshmem.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_INTERNAL_IVSHMEM_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_INTERNAL_IVSHMEM_IMPL_H_

static inline size_t z_impl_ivshmem_get_mem(const struct device *dev,
					    uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_mem(dev, memmap);
}

static inline uint32_t z_impl_ivshmem_get_id(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_id(dev);
}

static inline uint16_t z_impl_ivshmem_get_vectors(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_vectors(dev);
}

static inline int z_impl_ivshmem_int_peer(const struct device *dev,
					  uint32_t peer_id, uint16_t vector)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->int_peer(dev, peer_id, vector);
}

static inline int z_impl_ivshmem_register_handler(const struct device *dev,
						  struct k_poll_signal *ksignal,
						  uint16_t vector)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->register_handler(dev, ksignal, vector);
}

#ifdef CONFIG_IVSHMEM_V2

static inline size_t z_impl_ivshmem_get_rw_mem_section(const struct device *dev,
						       uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_rw_mem_section(dev, memmap);
}

static inline size_t z_impl_ivshmem_get_output_mem_section(const struct device *dev,
							   uint32_t peer_id,
							   uintptr_t *memmap)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_output_mem_section(dev, peer_id, memmap);
}

static inline uint32_t z_impl_ivshmem_get_state(const struct device *dev,
						uint32_t peer_id)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_state(dev, peer_id);
}

static inline int z_impl_ivshmem_set_state(const struct device *dev,
					   uint32_t state)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->set_state(dev, state);
}

static inline uint32_t z_impl_ivshmem_get_max_peers(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_max_peers(dev);
}

static inline uint16_t z_impl_ivshmem_get_protocol(const struct device *dev)
{
	const struct ivshmem_driver_api *api =
		(const struct ivshmem_driver_api *)dev->api;

	return api->get_protocol(dev);
}

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

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIRTUALIZATION_INTERNAL_IVSHMEM_IMPL_H_ */
