/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pcie/endpoint/pcie_ep.h>

#define LOG_LEVEL CONFIG_PCIE_EP_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(iproc_pcie);

#include <soc.h>

#include "pcie_ep_bcm_iproc.h"
#include "pcie_ep_bcm_iproc_regs.h"

#define DT_DRV_COMPAT brcm_iproc_pcie_ep

/* Helper macro to read 64-bit data using two 32-bit data read */
#define sys_read64(addr)    (((uint64_t)(sys_read32(addr + 4)) << 32) | \
			     sys_read32(addr))

static int iproc_pcie_conf_read(struct device *dev, uint32_t offset,
				uint32_t *data)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;

	/* Write offset to Configuration Indirect Address register */
	pcie_write32(offset, &cfg->base->paxb_config_ind_addr);

	/* Read data from Configuration Indirect Data register */
	*data = pcie_read32(&cfg->base->paxb_config_ind_data);

	return 0;
}

static void iproc_pcie_conf_write(struct device *dev, uint32_t offset,
				  uint32_t data)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;

	/* Write offset to Configuration Indirect Address register */
	pcie_write32(offset, &cfg->base->paxb_config_ind_addr);

	/* Write data to Configuration Indirect Data register */
	pcie_write32(data, &cfg->base->paxb_config_ind_data);
}

static int iproc_pcie_map_addr(struct device *dev, uint64_t pcie_addr,
			       uint64_t *mapped_addr, uint32_t size,
			       enum pcie_ob_mem_type ob_mem_type)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	uint64_t pcie_ob_base, pcie_ob_size, pcie_addr_start, offset;
	uint32_t mapped_size;
	enum pcie_outbound_map idx;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&ctx->ob_map_lock);

	/* We support 2 outbound windows,
	 * one in highmem region and another in lowmem region
	 */
	if ((ob_mem_type == PCIE_OB_HIGHMEM ||
	     ob_mem_type == PCIE_OB_ANYMEM) && !ctx->highmem_in_use) {
		idx = PCIE_MAP_HIGHMEM_IDX;
		pcie_ob_base = cfg->map_high_base;
		pcie_ob_size = cfg->map_high_size;
	} else if ((ob_mem_type == PCIE_OB_LOWMEM ||
		    ob_mem_type == PCIE_OB_ANYMEM) && !ctx->lowmem_in_use) {
		idx = PCIE_MAP_LOWMEM_IDX;
		pcie_ob_base = cfg->map_low_base;
		pcie_ob_size = cfg->map_low_size;
	} else {
		ret = -EBUSY;
		goto out;
	}

	/* check if the selected OB window supports size we want to map */
	if (size > pcie_ob_size) {
		ret = -ENOTSUP;
		goto out;
	}

	/* Host PCIe address should be aligned to outbound window size */
	pcie_addr_start = pcie_addr & ~(pcie_ob_size - 1);

	/* Program OARR with PCIe outbound address */
	pcie_write32(((pcie_ob_base & ~(pcie_ob_size - 1)) | PAXB_OARR_VALID),
		     &cfg->base->paxb_oarr[idx].lower);
	pcie_write32(pcie_ob_base >> 32, &cfg->base->paxb_oarr[idx].upper);

	/* Program OMAP with Host PCIe address */
	pcie_write32((uint32_t)pcie_addr_start,
		     &cfg->base->paxb_omap[idx].lower);
	pcie_write32((uint32_t)(pcie_addr_start >> 32),
		     &cfg->base->paxb_omap[idx].upper);

	/* Mark usage of outbound window */
	if (idx == PCIE_MAP_HIGHMEM_IDX) {
		ctx->highmem_in_use = true;
	} else {
		ctx->lowmem_in_use = true;
	}

	/* offset holds extra size mapped due to alignment requirement */
	offset = pcie_addr - pcie_addr_start;
	*mapped_addr = pcie_ob_base + offset;
	mapped_size = pcie_ob_size - offset;
	ret = ((mapped_size >= size) ? size : mapped_size);
out:
	k_spin_unlock(&ctx->ob_map_lock, key);

	return ret;
}

static void iproc_pcie_unmap_addr(struct device *dev, uint64_t mapped_addr)
{
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&ctx->ob_map_lock);

	/*
	 * When doing Host writes using PCIe outbound window, it is seen
	 * that before the writes gets completed using the existing outbound
	 * window mapping, next mapping is overwriting it, causing few bytes
	 * write failure with former mapping.
	 *
	 * To safeguard outbound window mapping, perform PCIe read in unmap,
	 * which ensures that all PCIe writes before the read
	 * are completed with this window.
	 */
	sys_read8(mapped_addr);

	if (mapped_addr >> 32) {
		ctx->highmem_in_use = false;
	} else {
		ctx->lowmem_in_use = false;
	}

	k_spin_unlock(&ctx->ob_map_lock, key);
}

static int iproc_pcie_generate_msi(struct device *dev, const uint32_t msi_num)
{
	int ret = 0;
#ifdef CONFIG_PCIE_EP_BCM_IPROC_V2
	uint64_t addr;
	uint32_t data;

	iproc_pcie_conf_read(dev, MSI_ADDR_H, &data);
	addr = ((uint64_t)data) << 32;
	iproc_pcie_conf_read(dev, MSI_ADDR_L, &data);
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

	iproc_pcie_conf_read(dev, MSI_DATA, &data);
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

static int iproc_pcie_generate_msix(struct device *dev, const uint32_t msix_num)
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

static int iproc_pcie_raise_irq(struct device *dev,
				enum pci_ep_irq_type irq_type,
				uint32_t irq_num)
{
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&ctx->raise_irq_lock);

	switch (irq_type) {
	case PCIE_EP_IRQ_MSI:
		ret = iproc_pcie_generate_msi(dev, irq_num);
		break;
	case PCIE_EP_IRQ_MSIX:
		ret = iproc_pcie_generate_msix(dev, irq_num);
		break;
	case PCIE_EP_IRQ_LEGACY:
		ret = -ENOTSUP;
		break;
	default:
		LOG_ERR("Unknown IRQ type\n");
		ret = -EINVAL;
	}

	k_spin_unlock(&ctx->raise_irq_lock, key);
	return ret;
}

static int iproc_pcie_register_reset_cb(struct device *dev,
					enum pcie_reset reset,
					pcie_ep_reset_callback_t cb, void *arg)
{
	struct iproc_pcie_ep_ctx *ctx = dev->data;

	if (reset < PCIE_PERST || reset >= PCIE_RESET_MAX)
		return -EINVAL;

	LOG_DBG("Registering the callback for reset %d", reset);
	ctx->reset_cb[reset] = cb;
	ctx->reset_data[reset] = arg;

	return 0;
}

#if DT_INST_IRQ_HAS_NAME(0, perst)
static void iproc_pcie_perst(void *arg)
{
	struct device *dev = arg;
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	void *reset_data;
	uint32_t data;

	data = sys_read32(CRMU_MCU_EXTRA_EVENT_STATUS);

	if (data & PCIE0_PERST_INTR) {
		LOG_DBG("PERST interrupt [0x%x]", data);
		sys_write32(PCIE0_PERST_INTR, CRMU_MCU_EXTRA_EVENT_CLEAR);

		if (ctx->reset_cb[PCIE_PERST] != NULL) {
			reset_data = ctx->reset_data[PCIE_PERST];
			ctx->reset_cb[PCIE_PERST](reset_data);
		}
	}
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, perst_inband)
static void iproc_pcie_hot_reset(void *arg)
{
	struct device *dev = arg;
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	void *reset_data;
	uint32_t data;

	data = sys_read32(CRMU_MCU_EXTRA_EVENT_STATUS);

	if (data & PCIE0_PERST_INB_INTR) {
		LOG_DBG("INBAND PERST interrupt [0x%x]", data);
		sys_write32(PCIE0_PERST_INB_INTR, CRMU_MCU_EXTRA_EVENT_CLEAR);

		if (ctx->reset_cb[PCIE_PERST_INB] != NULL) {
			reset_data = ctx->reset_data[PCIE_PERST_INB];
			ctx->reset_cb[PCIE_PERST_INB](reset_data);
		}
	}
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, flr)
static void iproc_pcie_flr(void *arg)
{
	struct device *dev = arg;
	const struct iproc_pcie_ep_config *cfg = dev->config;
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	void *reset_data;
	uint32_t data;

	data = pcie_read32(&cfg->base->paxb_paxb_intr_status);

	if (data & PCIE0_FLR_INTR) {
		LOG_DBG("FLR interrupt[0x%x]", data);
		pcie_write32(PCIE0_FLR_INTR, &cfg->base->paxb_paxb_intr_clear);

		if (ctx->reset_cb[PCIE_FLR] != NULL) {
			reset_data = ctx->reset_data[PCIE_FLR];
			ctx->reset_cb[PCIE_FLR](reset_data);
		}
	} else {
		/*
		 * Other interrupts like PAXB ECC Error interrupt
		 * could show up at the beginning which are harmless.
		 * So simply clearing those interrupts here
		 */
		LOG_DBG("PAXB interrupt[0x%x]", data);
		pcie_write32(data, &cfg->base->paxb_paxb_intr_clear);
	}

	/* Clear FLR in Progress bit */
	iproc_pcie_conf_read(dev, PCIE_DEV_CTRL_OFFSET, &data);
	data |= FLR_IN_PROGRESS;
	iproc_pcie_conf_write(dev, PCIE_DEV_CTRL_OFFSET, data);
}
#endif

DEVICE_DECLARE(iproc_pcie_ep_0);

static void iproc_pcie_reset_config(struct device *dev)
{
	uint32_t data;
	const struct iproc_pcie_ep_config *cfg = dev->config;

	/* Clear any possible prior pending interrupts */
	sys_write32(PCIE0_PERST_INTR | PCIE0_PERST_INB_INTR,
		    CRMU_MCU_EXTRA_EVENT_CLEAR);
	pcie_write32(PCIE0_FLR_INTR, &cfg->base->paxb_paxb_intr_clear);

	/* Enable PERST and Inband PERST interrupts */
	data = sys_read32(PCIE_PERSTB_INTR_CTL_STS);
	data |= (PCIE0_PERST_FE_INTR | PCIE0_PERST_INB_FE_INTR);
	sys_write32(data, PCIE_PERSTB_INTR_CTL_STS);

	data = sys_read32(CRMU_MCU_EXTRA_EVENT_MASK);
	data &= ~(PCIE0_PERST_INTR | PCIE0_PERST_INB_INTR);
	sys_write32(data, CRMU_MCU_EXTRA_EVENT_MASK);

	/* Set auto clear FLR and auto clear CRS post FLR */
	iproc_pcie_conf_read(dev, PCIE_TL_CTRL0_OFFSET, &data);
	data |= (AUTO_CLR_CRS_POST_FLR | AUTO_CLR_FLR_AFTER_DELAY);
	iproc_pcie_conf_write(dev, PCIE_TL_CTRL0_OFFSET, data);

	/* Enable Function Level Reset */
	data = pcie_read32(&cfg->base->paxb_paxb_intr_en);
	data |= PCIE0_FLR_INTR;
	pcie_write32(data, &cfg->base->paxb_paxb_intr_en);

#if DT_INST_IRQ_HAS_NAME(0, perst)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, perst, irq),
		    DT_INST_IRQ_BY_NAME(0, perst, priority),
		    iproc_pcie_perst, DEVICE_GET(iproc_pcie_ep_0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, perst, irq));
#endif

#if DT_INST_IRQ_HAS_NAME(0, perst_inband)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, perst_inband, irq),
		    DT_INST_IRQ_BY_NAME(0, perst_inband, priority),
		    iproc_pcie_hot_reset, DEVICE_GET(iproc_pcie_ep_0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, perst_inband, irq));
#endif

#if DT_INST_IRQ_HAS_NAME(0, flr)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, flr, irq),
		    DT_INST_IRQ_BY_NAME(0, flr, priority),
		    iproc_pcie_flr, DEVICE_GET(iproc_pcie_ep_0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, flr, irq));
#endif
}

#ifdef CONFIG_PCIE_EP_BCM_IPROC_INIT_CFG
static void iproc_pcie_msix_config(struct device *dev)
{
	/*
	 * Configure capability of generating 16 messages,
	 * MSI-X Table offset 0x10000 on BAR2,
	 * MSI-X PBA offset 0x10800 on BAR2.
	 */
	iproc_pcie_conf_write(dev, MSIX_CONTROL, (MSIX_TABLE_SIZE - 1));
	iproc_pcie_conf_write(dev, MSIX_TBL_OFF_BIR, MSIX_TBL_B2_10000);
	iproc_pcie_conf_write(dev, MSIX_PBA_OFF_BIR, MSIX_PBA_B2_10800);
}

static void iproc_pcie_msi_config(struct device *dev)
{
	uint32_t data;

	/* Configure capability of generating 16 messages */
	iproc_pcie_conf_read(dev, ID_VAL4_OFFSET, &data);
	data = (data & ~(MSI_COUNT_MASK)) | (MSI_COUNT_VAL << MSI_COUNT_SHIFT);
	iproc_pcie_conf_write(dev, ID_VAL4_OFFSET, data);
}
#endif

static int iproc_pcie_mode_check(const struct iproc_pcie_ep_config *cfg)
{
	uint32_t data;

	data = pcie_read32(&cfg->base->paxb_strap_status);
	LOG_DBG("PAXB_STRAP_STATUS = 0x%08X\n", data);

	if (data & PCIE_RC_MODE_MASK) {
		return -ENOTSUP;
	}

	return 0;
}

static int iproc_pcie_ep_init(struct device *dev)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	int ret;
	uint32_t data;

	ret = iproc_pcie_mode_check(cfg);
	if (ret) {
		LOG_ERR("ERROR: Only PCIe EP mode is supported\n");
		goto err_out;
	}

	iproc_pcie_conf_read(dev, PCIE_LINK_STATUS_CONTROL, &data);
	LOG_INF("PCIe linkup speed 0x%x\n", ((data >>
				PCIE_LINKSPEED_SHIFT) & PCIE_LINKSPEED_MASK));
	LOG_INF("PCIe linkup width 0x%x\n", ((data >>
				PCIE_LINKWIDTH_SHIFT) & PCIE_LINKWIDTH_MASK));

#ifdef CONFIG_PCIE_EP_BCM_IPROC_INIT_CFG
	iproc_pcie_msi_config(dev);
	iproc_pcie_msix_config(dev);
#endif

	iproc_pcie_reset_config(dev);

	ctx->highmem_in_use = false;
	ctx->lowmem_in_use = false;
	LOG_INF("PCIe initialized successfully\n");

err_out:
	return ret;
}

static struct iproc_pcie_ep_ctx iproc_pcie_ep_ctx_0;

static struct iproc_pcie_ep_config iproc_pcie_ep_config_0 = {
	.id = 0,
	.base = (struct iproc_pcie_reg *)DT_INST_REG_ADDR(0),
	.reg_size = DT_INST_REG_SIZE(0),
	.map_low_base = DT_INST_REG_ADDR_BY_NAME(0, map_lowmem),
	.map_low_size = DT_INST_REG_SIZE_BY_NAME(0, map_lowmem),
	.map_high_base = DT_INST_REG_ADDR_BY_NAME(0, map_highmem),
	.map_high_size = DT_INST_REG_SIZE_BY_NAME(0, map_highmem),
};

static struct pcie_ep_driver_api iproc_pcie_ep_api = {
	.conf_read = iproc_pcie_conf_read,
	.conf_write = iproc_pcie_conf_write,
	.map_addr = iproc_pcie_map_addr,
	.unmap_addr = iproc_pcie_unmap_addr,
	.raise_irq = iproc_pcie_raise_irq,
	.register_reset_cb = iproc_pcie_register_reset_cb,
};

DEVICE_AND_API_INIT(iproc_pcie_ep_0, DT_INST_LABEL(0),
		    &iproc_pcie_ep_init, &iproc_pcie_ep_ctx_0,
		    &iproc_pcie_ep_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &iproc_pcie_ep_api);
