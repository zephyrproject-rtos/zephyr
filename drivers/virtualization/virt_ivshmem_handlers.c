/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/virtualization/ivshmem.h>
#include <syscall_handler.h>
#include <string.h>

static inline size_t z_vrfy_ivshmem_get_mem(const struct device *dev,
					    uintptr_t *memmap)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IVSHMEM(dev, get_mem));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(memmap, sizeof(uintptr_t)));

	return z_impl_ivshmem_get_mem(dev, memmap);
}
#include <syscalls/ivshmem_get_mem_mrsh.c>

static inline uint32_t z_vrfy_ivshmem_get_id(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IVSHMEM(dev, get_id));

	return z_impl_ivshmem_get_id(dev);
}
#include <syscalls/ivshmem_get_id_mrsh.c>

static inline uint16_t z_vrfy_ivshmem_get_vectors(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IVSHMEM(dev, get_vectors));

	return z_impl_ivshmem_get_vectors(dev);
}
#include <syscalls/ivshmem_get_vectors_mrsh.c>

static inline int z_vrfy_ivshmem_int_peer(const struct device *dev,
					  uint32_t peer_id, uint16_t vector)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IVSHMEM(dev, int_peer));

	return z_impl_ivshmem_int_peer(dev, peer_id, vector);
}
#include <syscalls/ivshmem_int_peer_mrsh.c>

static inline int z_vrfy_ivshmem_register_handler(const struct device *dev,
						  struct k_poll_signal *signal,
						  uint16_t vector)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IVSHMEM(dev, register_handler));
	Z_OOPS(Z_SYSCALL_OBJ(signal, K_OBJ_POLL_SIGNAL));

	return z_impl_ivshmem_register_handler(dev, signal, vector);
}

#include <syscalls/ivshmem_register_handler_mrsh.c>
