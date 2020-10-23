/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_
#define ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_

#define IVSHMEM_VENDOR_ID		0x1AF4
#define IVSHMEM_DEVICE_ID		0x1110

#define IVSHMEM_PCIE_REG_BAR_IDX	0
#define IVSHMEM_PCIE_SHMEM_BAR_IDX	1

#define MAX_BUS				(0xFFFFFFFF & PCIE_BDF_BUS_MASK)
#define MAX_DEV				(0xFFFFFFFF & PCIE_BDF_DEV_MASK)
#define MAX_FUNC			(0xFFFFFFFF & PCIE_BDF_FUNC_MASK)

struct ivshmem {
	DEVICE_MMIO_RAM;
	pcie_bdf_t bdf;
	uintptr_t shmem;
	size_t size;
};

#endif /* ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_ */
