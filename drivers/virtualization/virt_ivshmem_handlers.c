/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>

static inline size_t z_vrfy_ivshmem_get_mem(const struct device *dev,
					    uintptr_t *memmap)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_mem));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(memmap, sizeof(uintptr_t)));

	return z_impl_ivshmem_get_mem(dev, memmap);
}
#include <zephyr/syscalls/ivshmem_get_mem_mrsh.c>

static inline uint32_t z_vrfy_ivshmem_get_id(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_id));

	return z_impl_ivshmem_get_id(dev);
}
#include <zephyr/syscalls/ivshmem_get_id_mrsh.c>

static inline uint16_t z_vrfy_ivshmem_get_vectors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_vectors));

	return z_impl_ivshmem_get_vectors(dev);
}
#include <zephyr/syscalls/ivshmem_get_vectors_mrsh.c>

static inline int z_vrfy_ivshmem_int_peer(const struct device *dev,
					  uint32_t peer_id, uint16_t vector)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, int_peer));

	return z_impl_ivshmem_int_peer(dev, peer_id, vector);
}
#include <zephyr/syscalls/ivshmem_int_peer_mrsh.c>

static inline int z_vrfy_ivshmem_register_handler(const struct device *dev,
						  struct k_poll_signal *signal,
						  uint16_t vector)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, register_handler));
	K_OOPS(K_SYSCALL_OBJ(signal, K_OBJ_POLL_SIGNAL));

	return z_impl_ivshmem_register_handler(dev, signal, vector);
}
#include <zephyr/syscalls/ivshmem_register_handler_mrsh.c>

#ifdef CONFIG_IVSHMEM_V2

static inline size_t z_vrfy_ivshmem_get_rw_mem_section(const struct device *dev,
						       uintptr_t *memmap)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_rw_mem_section));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(memmap, sizeof(uintptr_t)));

	return z_impl_ivshmem_get_rw_mem_section(dev, memmap);
}
#include <zephyr/syscalls/ivshmem_get_rw_mem_section_mrsh.c>

static inline size_t z_vrfy_ivshmem_get_output_mem_section(const struct device *dev,
							   uint32_t peer_id,
							   uintptr_t *memmap)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_output_mem_section));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(memmap, sizeof(uintptr_t)));

	return z_impl_ivshmem_get_output_mem_section(dev, peer_id, memmap);
}
#include <zephyr/syscalls/ivshmem_get_output_mem_section_mrsh.c>

static inline uint32_t z_vrfy_ivshmem_get_state(const struct device *dev,
						uint32_t peer_id)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_state));

	return z_impl_ivshmem_get_state(dev, peer_id);
}
#include <zephyr/syscalls/ivshmem_get_state_mrsh.c>

static inline int z_vrfy_ivshmem_set_state(const struct device *dev,
					   uint32_t state)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, set_state));

	return z_impl_ivshmem_set_state(dev, state);
}
#include <zephyr/syscalls/ivshmem_set_state_mrsh.c>

static inline uint32_t z_vrfy_ivshmem_get_max_peers(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_max_peers));

	return z_impl_ivshmem_get_max_peers(dev);
}
#include <zephyr/syscalls/ivshmem_get_max_peers_mrsh.c>

static inline uint16_t z_vrfy_ivshmem_get_protocol(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, get_protocol));

	return z_impl_ivshmem_get_protocol(dev);
}
#include <zephyr/syscalls/ivshmem_get_protocol_mrsh.c>

static inline int z_vrfy_ivshmem_enable_interrupts(const struct device *dev,
						   bool enable)
{
	K_OOPS(K_SYSCALL_DRIVER_IVSHMEM(dev, enable_interrupts));

	return z_impl_ivshmem_enable_interrupts(dev, enable);
}
#include <zephyr/syscalls/ivshmem_enable_interrupts_mrsh.c>

#endif /* CONFIG_IVSHMEM_V2 */
