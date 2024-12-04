/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>

#define LOG_LEVEL CONFIG_PCIE_EP_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(iproc_pcie);

#include "pcie_ep_iproc.h"

#define DT_DRV_COMPAT brcm_iproc_pcie_ep

static int iproc_pcie_conf_read(const struct device *dev, uint32_t offset,
				uint32_t *data)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;

	/* Write offset to Configuration Indirect Address register */
	pcie_write32(offset, &cfg->base->paxb_config_ind_addr);

	/* Read data from Configuration Indirect Data register */
	*data = pcie_read32(&cfg->base->paxb_config_ind_data);

	return 0;
}

static void iproc_pcie_conf_write(const struct device *dev, uint32_t offset,
				  uint32_t data)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;

	/* Write offset to Configuration Indirect Address register */
	pcie_write32(offset, &cfg->base->paxb_config_ind_addr);

	/* Write data to Configuration Indirect Data register */
	pcie_write32(data, &cfg->base->paxb_config_ind_data);
}

static int iproc_pcie_map_addr(const struct device *dev, uint64_t pcie_addr,
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

static void iproc_pcie_unmap_addr(const struct device *dev,
				  uint64_t mapped_addr)
{
	struct iproc_pcie_ep_ctx *ctx = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&ctx->ob_map_lock);

	if (mapped_addr >> 32) {
		ctx->highmem_in_use = false;
	} else {
		ctx->lowmem_in_use = false;
	}

	k_spin_unlock(&ctx->ob_map_lock, key);
}

static int iproc_pcie_raise_irq(const struct device *dev,
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

static int iproc_pcie_register_reset_cb(const struct device *dev,
					enum pcie_reset reset,
					pcie_ep_reset_callback_t cb, void *arg)
{
	struct iproc_pcie_ep_ctx *ctx = dev->data;

	if (reset < PCIE_PERST || reset >= PCIE_RESET_MAX) {
		return -EINVAL;
	}

	LOG_DBG("Registering the callback for reset %d", reset);
	ctx->reset_cb[reset] = cb;
	ctx->reset_data[reset] = arg;

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(dmas)
static int iproc_pcie_pl330_dma_xfer(const struct device *dev,
				     uint64_t mapped_addr,
				     uintptr_t local_addr, uint32_t size,
				     const enum xfer_direction dir)
{
	const struct iproc_pcie_ep_config *cfg = dev->config;
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_block_cfg = { 0 };
	uint32_t chan_id;
	int ret = -EINVAL;

	if (!device_is_ready(cfg->pl330_dev)) {
		LOG_ERR("DMA controller is not ready\n");
		ret = -ENODEV;
		goto out;
	}

	/* configure DMA */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	dma_block_cfg.block_size = size;
	if (dir == DEVICE_TO_HOST) {
		dma_block_cfg.source_address = local_addr;
		dma_block_cfg.dest_address = mapped_addr;
		chan_id = cfg->pl330_tx_chan_id;
	} else {
		dma_block_cfg.source_address = mapped_addr;
		dma_block_cfg.dest_address = local_addr;
		chan_id = cfg->pl330_rx_chan_id;
	}

	ret = dma_config(cfg->pl330_dev, chan_id,  &dma_cfg);
	if (ret) {
		LOG_ERR("DMA config failed\n");
		goto out;
	}

	/* start DMA */
	ret = dma_start(cfg->pl330_dev, chan_id);
	if (ret) {
		LOG_ERR("DMA transfer failed\n");
	}
out:
	return ret;
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, perst)
static void iproc_pcie_perst(const struct device *dev)
{
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
static void iproc_pcie_hot_reset(const struct device *dev)
{
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
static void iproc_pcie_flr(const struct device *dev)
{
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

static void iproc_pcie_reset_config(const struct device *dev)
{
	__unused uint32_t data;
	__unused const struct iproc_pcie_ep_config *cfg = dev->config;

#if DT_INST_IRQ_HAS_NAME(0, perst)
	/* Clear any possible prior pending PERST interrupt */
	sys_write32(PCIE0_PERST_INTR, CRMU_MCU_EXTRA_EVENT_CLEAR);

	/* Enable PERST interrupt */
	data = sys_read32(PCIE_PERSTB_INTR_CTL_STS);
	data |= PCIE0_PERST_FE_INTR;
	sys_write32(data, PCIE_PERSTB_INTR_CTL_STS);

	data = sys_read32(CRMU_MCU_EXTRA_EVENT_MASK);
	data &= ~PCIE0_PERST_INTR;
	sys_write32(data, CRMU_MCU_EXTRA_EVENT_MASK);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, perst, irq),
		    DT_INST_IRQ_BY_NAME(0, perst, priority),
		    iproc_pcie_perst, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, perst, irq));
#endif

#if DT_INST_IRQ_HAS_NAME(0, perst_inband)
	/* Clear any possible prior pending Inband PERST interrupt */
	sys_write32(PCIE0_PERST_INB_INTR, CRMU_MCU_EXTRA_EVENT_CLEAR);

	/* Enable Inband PERST interrupt */
	data = sys_read32(PCIE_PERSTB_INTR_CTL_STS);
	data |= PCIE0_PERST_INB_FE_INTR;
	sys_write32(data, PCIE_PERSTB_INTR_CTL_STS);

	data = sys_read32(CRMU_MCU_EXTRA_EVENT_MASK);
	data &= ~PCIE0_PERST_INB_INTR;
	sys_write32(data, CRMU_MCU_EXTRA_EVENT_MASK);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, perst_inband, irq),
		    DT_INST_IRQ_BY_NAME(0, perst_inband, priority),
		    iproc_pcie_hot_reset, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, perst_inband, irq));
#endif

#if DT_INST_IRQ_HAS_NAME(0, flr)
	/* Clear any possible prior pending FLR */
	pcie_write32(PCIE0_FLR_INTR, &cfg->base->paxb_paxb_intr_clear);

	/* Set auto clear FLR and auto clear CRS post FLR */
	iproc_pcie_conf_read(dev, PCIE_TL_CTRL0_OFFSET, &data);
	data |= (AUTO_CLR_CRS_POST_FLR | AUTO_CLR_FLR_AFTER_DELAY);
	iproc_pcie_conf_write(dev, PCIE_TL_CTRL0_OFFSET, data);

	/* Enable Function Level Reset */
	data = pcie_read32(&cfg->base->paxb_paxb_intr_en);
	data |= PCIE0_FLR_INTR;
	pcie_write32(data, &cfg->base->paxb_paxb_intr_en);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, flr, irq),
		    DT_INST_IRQ_BY_NAME(0, flr, priority),
		    iproc_pcie_flr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, flr, irq));
#endif
}

#ifdef CONFIG_PCIE_EP_IPROC_V2
static void iproc_pcie_msix_pvm_config(const struct device *dev)
{
	__unused const struct iproc_pcie_ep_config *cfg = dev->config;
	__unused struct iproc_pcie_reg *base = cfg->base;
	__unused uint32_t data;

	/* configure snoop irq 1 for monitoring MSIX_CAP register */
#if DT_INST_IRQ_HAS_NAME(0, snoop_irq1)
	data = pcie_read32(&cfg->base->paxb_snoop_addr_cfg[1]);
	data &= ~SNOOP_ADDR1_MASK;
	data |= (SNOOP_ADDR1 | SNOOP_ADDR1_EN);
	pcie_write32(data, &cfg->base->paxb_snoop_addr_cfg[1]);

	data = pcie_read32(&base->paxb_pcie_cfg_intr_mask);
	data &= ~SNOOP_VALID_INTR;
	pcie_write32(data, &base->paxb_pcie_cfg_intr_mask);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, snoop_irq1, irq),
		    DT_INST_IRQ_BY_NAME(0, snoop_irq1, priority),
		    iproc_pcie_func_mask_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, snoop_irq1, irq));

	LOG_DBG("snoop interrupt configured\n");
#endif

	/* configure pmon lite interrupt for monitoring MSIX table */
#if DT_INST_IRQ_HAS_NAME(0, pcie_pmon_lite)
	data = sys_read32(PMON_LITE_PCIE_AXI_FILTER_0_CONTROL);
	data |= AXI_FILTER_0_ENABLE;
	sys_write32(data, PMON_LITE_PCIE_AXI_FILTER_0_CONTROL);

	sys_write32(MSIX_TABLE_BASE, AXI_FILTER_0_ADDR_START_LOW);
	/* Start of PBA is end of MSI-X table in our case */
	sys_write32(PBA_TABLE_BASE, AXI_FILTER_0_ADDR_END_LOW);

	sys_set_bit(PMON_LITE_PCIE_INTERRUPT_ENABLE, WR_ADDR_CHK_INTR_EN);

	memset((void *)PBA_TABLE_BASE, 0, PBA_TABLE_SIZE);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, pcie_pmon_lite, irq),
		    DT_INST_IRQ_BY_NAME(0, pcie_pmon_lite, priority),
		    iproc_pcie_vector_mask_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, pcie_pmon_lite, irq));

	LOG_DBG("pcie pmon lite interrupt configured\n");
#endif
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

static int iproc_pcie_ep_init(const struct device *dev)
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

#ifdef PCIE_EP_IPROC_INIT_CFG
	iproc_pcie_msi_config(dev);
	iproc_pcie_msix_config(dev);
#endif

	/* configure interrupts for MSI-X Per-Vector Masking feature */
#ifdef CONFIG_PCIE_EP_IPROC_V2
	iproc_pcie_msix_pvm_config(dev);
#endif

	iproc_pcie_reset_config(dev);

	ctx->highmem_in_use = false;
	ctx->lowmem_in_use = false;
	LOG_INF("PCIe initialized successfully\n");

err_out:
	return ret;
}

static struct iproc_pcie_ep_ctx iproc_pcie_ep_ctx_0;

static const struct iproc_pcie_ep_config iproc_pcie_ep_config_0 = {
	.id = 0,
	.base = (struct iproc_pcie_reg *)DT_INST_REG_ADDR(0),
	.reg_size = DT_INST_REG_SIZE(0),
	.map_low_base = DT_INST_REG_ADDR_BY_NAME(0, map_lowmem),
	.map_low_size = DT_INST_REG_SIZE_BY_NAME(0, map_lowmem),
	.map_high_base = DT_INST_REG_ADDR_BY_NAME(0, map_highmem),
	.map_high_size = DT_INST_REG_SIZE_BY_NAME(0, map_highmem),
#if DT_INST_NODE_HAS_PROP(0, dmas)
	.pl330_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(0, 0)),
	.pl330_tx_chan_id = DT_INST_DMAS_CELL_BY_NAME(0, txdma, channel),
	.pl330_rx_chan_id = DT_INST_DMAS_CELL_BY_NAME(0, rxdma, channel),
#endif
};

static DEVICE_API(pcie_ep, iproc_pcie_ep_api) = {
	.conf_read = iproc_pcie_conf_read,
	.conf_write = iproc_pcie_conf_write,
	.map_addr = iproc_pcie_map_addr,
	.unmap_addr = iproc_pcie_unmap_addr,
	.raise_irq = iproc_pcie_raise_irq,
	.register_reset_cb = iproc_pcie_register_reset_cb,
#if DT_INST_NODE_HAS_PROP(0, dmas)
	.dma_xfer = iproc_pcie_pl330_dma_xfer,
#endif
};

DEVICE_DT_INST_DEFINE(0, &iproc_pcie_ep_init, NULL,
		    &iproc_pcie_ep_ctx_0,
		    &iproc_pcie_ep_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &iproc_pcie_ep_api);
