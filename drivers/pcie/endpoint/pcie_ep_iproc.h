/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_EP_IPROC_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_EP_IPROC_H_

#include <soc.h>
#include <sys/util.h>

#include "pcie_ep_iproc_regs.h"

#define PCIE_LINK_STATUS_CONTROL	0xbc
#define PCIE_LINKSPEED_SHIFT		16
#define PCIE_LINKWIDTH_SHIFT		20
#define PCIE_LINKSPEED_MASK		0xf
#define PCIE_LINKWIDTH_MASK		0x3f
#define PCIE_RC_MODE_MASK		0x1

#define MSI_ADDR_L			0x5c
#define MSI_ADDR_H			0x60
#define MSI_DATA			0x64

#define MSI_COUNT_SHIFT			12
#define MSI_COUNT_MASK			0x7000
#define MSI_COUNT_VAL			4

#define MSI_CSR_MASK			0xffffffff
#define MSI_EN_MASK			0xf

#define MSIX_CAP			0xa0
#define MSIX_FUNC_MASK			BIT(30)

#define ID_VAL4_OFFSET			0x440
#define MSIX_CONTROL			0x4c0
#define MSIX_TBL_OFF_BIR		0x4c4
#define MSIX_PBA_OFF_BIR		0x4c8

#define MSIX_TBL_B2_10000		0x10002
#define MSIX_PBA_B2_10800		0x10802

#define MSIX_TABLE_BASE			0x20010000
#define MSIX_TABLE_SIZE			16	/* we support 16 MSI-X */
#define MSIX_TBL_ENTRY_SIZE		16
#define MSIX_TBL_ADDR_OFF		0
#define MSIX_TBL_DATA_OFF		8
#define MSIX_TBL_VECTOR_CTRL_OFF	12
#define MSIX_VECTOR_MASK		BIT(0)
#define MSIX_VECTOR_OFF(x)		(MSIX_TABLE_BASE + \
					 (MSIX_TBL_ENTRY_SIZE * (x)))

#define PBA_TABLE_BASE			0x20010800
#define PBA_TABLE_SIZE			0x800
#define PBA_OFFSET(x)			(PBA_TABLE_BASE + (4 * ((x) / 32)))
#define PENDING_BIT(x)			((x) % 32)

#define PAXB_OARR_VALID			BIT(0)

#ifdef CONFIG_PCIE_EP_IPROC_V2

#define SNOOP_VALID_INTR		BIT(3)
#define SNOOP_ADDR1_EN			BIT(31)
#define SNOOP_ADDR1_MASK		0x1fff
#define SNOOP_ADDR1			MSIX_CAP

#define PMON_LITE_PCIE_INTERRUPT_ENABLE		(PMON_LITE_PCIE_BASE + 0xc)
#define PMON_LITE_PCIE_INTERRUPT_STATUS		(PMON_LITE_PCIE_BASE + 0x10)
#define PMON_LITE_PCIE_INTERRUPT_CLEAR		(PMON_LITE_PCIE_BASE + 0x14)
#define WR_ADDR_CHK_INTR_EN			2

#define PMON_LITE_PCIE_AXI_FILTER_0_CONTROL	(PMON_LITE_PCIE_BASE + 0xd4)
#define AXI_FILTER_0_ENABLE			(BIT(30) | BIT(2) | \
						 BIT(1) | BIT(0))

#define AXI_FILTER_0_ADDR_START_LOW		(PMON_LITE_PCIE_BASE + 0xd8)
#define AXI_FILTER_0_ADDR_START_HIGH		(PMON_LITE_PCIE_BASE + 0xdc)
#define AXI_FILTER_0_ADDR_END_LOW		(PMON_LITE_PCIE_BASE + 0xe0)
#define AXI_FILTER_0_ADDR_END_HIGH		(PMON_LITE_PCIE_BASE + 0xe4)

#endif

#define PCIE_DEV_CTRL_OFFSET		0x4d8
#define FLR_IN_PROGRESS			BIT(27)

#define PCIE_TL_CTRL0_OFFSET		0x800
#define AUTO_CLR_FLR_AFTER_DELAY	BIT(13) /* Clears FLR after 55ms */
#define AUTO_CLR_CRS_POST_FLR		BIT(14)

#define PCIE0_FLR_INTR			BIT(20)
#define PCIE0_FLR_PERST_INTR		BIT(21)

enum pcie_outbound_map {
	PCIE_MAP_LOWMEM_IDX,
	PCIE_MAP_HIGHMEM_IDX,
};

struct iproc_pcie_ep_config {
	struct iproc_pcie_reg *base;	/* Base address of PAXB registers */
	uint32_t reg_size;
	uint32_t map_low_base;	/* Base addr of outbound mapping at lowmem */
	uint32_t map_low_size;
	uint64_t map_high_base;	/* Base addr of outbound mapping at highmem */
	uint32_t map_high_size;
	unsigned int id;
	const char *pl330_dev_name; /* pl330 device's "label" property */
	uint32_t pl330_tx_chan_id;  /* channel used for Device to Host write */
	uint32_t pl330_rx_chan_id;  /* channel used for Host to Device read  */
};

struct iproc_pcie_ep_ctx {
	struct k_spinlock ob_map_lock;
	struct k_spinlock raise_irq_lock;
	struct k_spinlock pba_lock;
	bool highmem_in_use;
	bool lowmem_in_use;
	/* Callback function for reset interrupt */
	pcie_ep_reset_callback_t reset_cb[PCIE_RESET_MAX];
	/* Callback data for reset interrupt */
	void *reset_data[PCIE_RESET_MAX];
};

void iproc_pcie_msix_config(const struct device *dev);
void iproc_pcie_msi_config(const struct device *dev);

int iproc_pcie_generate_msi(const struct device *dev, const uint32_t msi_num);
int iproc_pcie_generate_msix(const struct device *dev, const uint32_t msix_num);

void iproc_pcie_func_mask_isr(void *arg);
void iproc_pcie_vector_mask_isr(void *arg);

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_EP_IPROC_H_ */
