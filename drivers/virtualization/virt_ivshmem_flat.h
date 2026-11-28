/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_FLAT_H_
#define ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_FLAT_H_

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define IVSHMEM_FLAT_IVPOSITION 0x08U
#define IVSHMEM_FLAT_DOORBELL   0x0cU
#define IVSHMEM_FLAT_MMR_SIZE   0x10U

#define DEV_CFG(dev)  ((const struct ivshmem_flat_config *)(dev)->config)
#define DEV_DATA(dev) ((struct ivshmem_flat_data *)(dev)->data)

#define IVSHMEM_FLAT_IRQ_ENABLED(n)                                                                \
	COND_CODE_1(CONFIG_IVSHMEM_DOORBELL, (DT_INST_IRQ_HAS_IDX(n, 0)), (0))

struct ivshmem_flat_config {
	DEVICE_MMIO_NAMED_ROM(mmr);
	DEVICE_MMIO_NAMED_ROM(shmem);
	size_t shmem_size;
	void (*irq_config)(const struct device *dev);
};

struct ivshmem_flat_data {
	DEVICE_MMIO_NAMED_RAM(mmr);
	DEVICE_MMIO_NAMED_RAM(shmem);
	struct k_spinlock lock;
	struct k_poll_signal *signal;
};

#endif /*ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_FLAT_H_ */
