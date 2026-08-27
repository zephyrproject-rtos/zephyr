/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_
#define ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_

#include <zephyr/drivers/pcie/pcie.h>

#ifdef CONFIG_IVSHMEM_DOORBELL
#include <zephyr/drivers/pcie/msi.h>
#endif

#define PCIE_CONF_CMDSTAT_INTX_DISABLE	0x0400
#define PCIE_CONF_INTR_PIN(x)		(((x) >> 8) & 0xFFu)

#define IVSHMEM_CFG_ID			0x00
#define IVSHMEM_CFG_NEXT_CAP		0x01
#define IVSHMEM_CFG_LENGTH		0x02
#define IVSHMEM_CFG_PRIV_CNTL		0x03
#define IVSHMEM_PRIV_CNTL_ONESHOT_INT	BIT(0)
#define IVSHMEM_CFG_STATE_TAB_SZ	0x04
#define IVSHMEM_CFG_RW_SECTION_SZ	0x08
#define IVSHMEM_CFG_OUTPUT_SECTION_SZ	0x10
#define IVSHMEM_CFG_ADDRESS		0x18

#define IVSHMEM_INT_ENABLE		BIT(0)

#define IVSHMEM_PCIE_REG_BAR_IDX	0
#define IVSHMEM_PCIE_MSI_X_BAR_IDX	1
#define IVSHMEM_PCIE_SHMEM_BAR_IDX	2

#define PCIE_INTX_PIN_MIN		1
#define PCIE_INTX_PIN_MAX		4

#define INTX_IRQ_UNUSED			UINT32_MAX

struct ivshmem_param {
	const struct device *dev;
	struct k_poll_signal *signal;
	uint8_t vector;
};

struct ivshmem {
	DEVICE_MMIO_RAM;
	struct pcie_dev *pcie;
	uintptr_t shmem;
	size_t size;
#ifdef CONFIG_IVSHMEM_DOORBELL
	msi_vector_t vectors[CONFIG_IVSHMEM_MSI_X_VECTORS];
	struct ivshmem_param params[CONFIG_IVSHMEM_MSI_X_VECTORS];
	uint16_t n_vectors;
#endif
#ifdef CONFIG_IVSHMEM_V2
	bool ivshmem_v2;
	uint32_t max_peers;
	size_t rw_section_size;
	size_t output_section_size;
	uintptr_t state_table_shmem;
	uintptr_t rw_section_shmem;
	uintptr_t output_section_shmem[CONFIG_IVSHMEM_V2_MAX_PEERS];
#endif
};

struct ivshmem_reg {
	uint32_t int_mask;
	uint32_t int_status;
	uint32_t iv_position;
	uint32_t doorbell;
};

#ifdef CONFIG_IVSHMEM_V2

struct ivshmem_v2_reg {
	uint32_t id;
	uint32_t max_peers;
	uint32_t int_control;
	uint32_t doorbell;
	uint32_t state;
};

struct ivshmem_cfg {
	struct intx_info {
		uint32_t irq;
		uint32_t priority;
		uint32_t flags;
	} intx_info[PCIE_INTX_PIN_MAX];
};

#endif /* CONFIG_IVSHMEM_V2 */

#define IVSHMEM_GEN_DOORBELL(i, v) ((i << 16) | (v & 0xFFFF))

#endif /* ZEPHYR_DRIVERS_VIRTUALIZATION_VIRT_IVSHMEM_H_ */
