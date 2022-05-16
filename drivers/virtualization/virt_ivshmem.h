/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_
#define ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/msi.h>

#define IVSHMEM_VENDOR_ID		0x1AF4
#define IVSHMEM_DEVICE_ID		0x1110

#define IVSHMEM_PCIE_REG_BAR_IDX	0
#define IVSHMEM_PCIE_SHMEM_BAR_IDX	2

struct ivshmem_param {
	const struct device *dev;
	struct k_poll_signal *signal;
	uint8_t vector;
};

struct ivshmem {
	DEVICE_MMIO_RAM;
	pcie_bdf_t bdf;
	uint32_t dev_ven_id;
	uintptr_t shmem;
	size_t size;
#ifdef CONFIG_IVSHMEM_DOORBELL
	msi_vector_t vectors[CONFIG_IVSHMEM_MSI_X_VECTORS];
	struct ivshmem_param params[CONFIG_IVSHMEM_MSI_X_VECTORS];
	uint16_t n_vectors;
#endif
};

struct ivshmem_reg {
	uint32_t int_mask;
	uint32_t int_status;
	uint32_t iv_position;
	uint32_t doorbell;
};

#define IVSHMEM_GEN_DOORBELL(i, v) ((i << 16) | (v & 0xFFFF))

#endif /* ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_ */
