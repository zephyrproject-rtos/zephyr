/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pcie/endpoint/pcie_ep.h>
#include <logging/log.h>

#include "pcie_ep_iproc.h"

LOG_MODULE_DECLARE(iproc_pcie, CONFIG_PCIE_EP_LOG_LEVEL);

/* Helper macro to read 64-bit data using two 32-bit data read */
#define sys_read64(addr)    (((uint64_t)(sys_read32(addr + 4)) << 32) | \
			     sys_read32(addr))

#ifdef PCIE_EP_IPROC_INIT_CFG
void iproc_pcie_msix_config(const struct device *dev)
{
	/*
	 * Configure capability of generating 16 messages,
	 * MSI-X Table offset 0x10000 on BAR2,
	 * MSI-X PBA offset 0x10800 on BAR2.
	 */
	pcie_ep_conf_write(dev, MSIX_CONTROL, (MSIX_TABLE_SIZE - 1));
	pcie_ep_conf_write(dev, MSIX_TBL_OFF_BIR, MSIX_TBL_B2_10000);
	pcie_ep_conf_write(dev, MSIX_PBA_OFF_BIR, MSIX_PBA_B2_10800);
}

void iproc_pcie_msi_config(const struct device *dev)
{
	uint32_t data;

	/* Configure capability of generating 16 messages */
	pcie_ep_conf_read(dev, ID_VAL4_OFFSET, &data);
	data = (data & ~(MSI_COUNT_MASK)) | (MSI_COUNT_VAL << MSI_COUNT_SHIFT);
	pcie_ep_conf_write(dev, ID_VAL4_OFFSET, data);
}
#endif

int iproc_pcie_generate_msi(const struct device *dev, const uint32_t msi_num)
{
	int ret = 0;
#ifdef CONFIG_PCIE_EP_IPROC_V2
	uint64_t addr;
	uint32_t data;

	pcie_ep_conf_read(dev, MSI_ADDR_H, &data);
	addr = ((uint64_t)data) << 32;
	pcie_ep_conf_read(dev, MSI_ADDR_L, &data);
	addr = addr | data;

	if (data == 0) {
		/*
		 * This is mostly the case where the test is being run
		 * from device before host driver sets up MSI.
		 * Returning zero instead of error because of this.
		 */
		LOG_WRN("MSI is not setup, skipping MSI");
		return 0;
	}

	pcie_ep_conf_read(dev, MSI_DATA, &data);
	data |= msi_num;

	ret = pcie_ep_xfer_data_memcpy(dev, addr,
				       (uintptr_t *)&data, sizeof(data),
				       PCIE_OB_LOWMEM, DEVICE_TO_HOST);

#else
	const struct iproc_pcie_ep_config *cfg = dev->config;

	pcie_write32(msi_num, &cfg->base->paxb_pcie_sys_msi_req);
#endif
	return ret;
}

int iproc_pcie_generate_msix(const struct device *dev, const uint32_t msix_num)
{
	uint64_t addr;
	uint32_t data, msix_offset;
	int ret;

	msix_offset = MSIX_TABLE_BASE + (msix_num * MSIX_TABLE_ENTRY_SIZE);

	addr = sys_read64(msix_offset);

	if (addr == 0) {
		/*
		 * This is mostly the case where the test is being run
		 * from device before host driver has setup MSIX table.
		 * Returning zero instead of error because of this.
		 */
		LOG_WRN("MSIX table is not setup, skipping MSIX\n");
		return 0;
	}

	data = sys_read32(msix_offset + MSIX_TBL_DATA_OFF);

	ret = pcie_ep_xfer_data_memcpy(dev, addr,
				       (uintptr_t *)&data, sizeof(data),
				       PCIE_OB_LOWMEM, DEVICE_TO_HOST);

	return ret;
}
