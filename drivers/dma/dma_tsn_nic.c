/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tsnlab_tsn_nic_dma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_tsn_nic, LOG_LEVEL_ERR);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>

#define DMA_ID_H2C 0x1fc0
#define DMA_ID_C2H 0x1fc1

#define DMA_CHANNEL_ID_MASK 0x00000f00
#define DMA_CHANNEL_ID_LSB  8
#define DMA_ENGINE_ID_MASK  0xffff0000
#define DMA_ENGINE_ID_LSB   16

#define DMA_ALIGN_BYTES_MASK       0x00ff0000
#define DMA_ALIGN_BYTES_LSB        16
#define DMA_GRANULARITY_BYTES_MASK 0x0000ff00
#define DMA_GRANULARITY_BYTES_LSB  8
#define DMA_ADDRESS_BITS_MASK      0x000000ff
#define DMA_ADDRESS_BITS_LSB       0

#define DMA_CTRL_RUN_STOP               (1UL << 0)
#define DMA_CTRL_IE_DESC_STOPPED        (1UL << 1)
#define DMA_CTRL_IE_DESC_COMPLETED      (1UL << 2)
#define DMA_CTRL_IE_DESC_ALIGN_MISMATCH (1UL << 3)
#define DMA_CTRL_IE_MAGIC_STOPPED       (1UL << 4)
#define DMA_CTRL_IE_IDLE_STOPPED        (1UL << 6)
#define DMA_CTRL_IE_READ_ERROR          (1UL << 9)
#define DMA_CTRL_IE_DESC_ERROR          (1UL << 19)
#define DMA_CTRL_NON_INCR_ADDR          (1UL << 25)
#define DMA_CTRL_POLL_MODE_WB           (1UL << 26)
#define DMA_CTRL_STM_MODE_WB            (1UL << 27)

#define DMA_CTRL_NON_INCR_ADDR (1UL << 25)

#define DMA_H2C 0
#define DMA_C2H 1

#define DMA_C2H_OFFSET 0x1000

#define DMA_CONFIG_BAR_IDX  1
/* Size of BAR1, it needs to be hard-coded as there is no PCIe API for this */
#define DMA_CONFIG_BAR_SIZE 0x10000

#define DMA_ENGINE_START 16268831
#define DMA_ENGINE_STOP  16268830

struct dma_tsn_nic_config_regs {
	uint32_t identifier;
	uint32_t reserved_1[4];
	uint32_t msi_enable;
};

struct dma_tsn_nic_engine_regs {
	uint32_t identifier;
	uint32_t control;
	uint32_t control_w1s;
	uint32_t control_w1c;
	uint32_t reserved_1[12]; /* padding */

	uint32_t status;
	uint32_t status_rc;
	uint32_t completed_desc_count;
	uint32_t alignments;
	uint32_t reserved_2[14]; /* padding */

	uint32_t poll_mode_wb_lo;
	uint32_t poll_mode_wb_hi;
	uint32_t interrupt_enable_mask;
	uint32_t interrupt_enable_mask_w1s;
	uint32_t interrupt_enable_mask_w1c;
	uint32_t reserved_3[9]; /* padding */

	uint32_t perf_ctrl;
	uint32_t perf_cyc_lo;
	uint32_t perf_cyc_hi;
	uint32_t perf_dat_lo;
	uint32_t perf_dat_hi;
	uint32_t perf_pnd_lo;
	uint32_t perf_pnd_hi;
} __packed; /* TODO: Move these to header file */

struct engine_sgdma_regs {
	uint32_t identifier;
	uint32_t reserved_1[31]; /* padding */

	/* bus address to first descriptor in Root Complex Memory */
	uint32_t first_desc_lo;
	uint32_t first_desc_hi;
	/* number of adjacent descriptors at first_desc */
	uint32_t first_desc_adjacent;
	uint32_t credits;
} __packed;

struct dma_tsn_nic_config {
	const struct device *pci_dev;
};

struct dma_tsn_nic_data {
	mm_reg_t bar[DMA_CONFIG_BAR_IDX + 1];
	struct dma_tsn_nic_engine_regs *regs[2];
};

static int dma_tsn_nic_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	return -ENOTSUP;
}

static int dma_tsn_nic_reload(const struct device *dev, uint32_t channel, uint32_t src,
			      uint32_t dst, size_t size)
{
	return -ENOTSUP;
}

static int dma_tsn_nic_start(const struct device *dev, uint32_t channel)
{
	const struct dma_tsn_nic_data *data = dev->data;

	sys_write32(DMA_ENGINE_START, (mem_addr_t)&data->regs[channel]->control);

	return 0;
}

static int dma_tsn_nic_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_tsn_nic_data *data = dev->data;

	/* There is only one channel for each direction for now */
	sys_write32(DMA_ENGINE_STOP, (mem_addr_t)&data->regs[channel]->control);

	return 0;
}

static int dma_tsn_nic_suspend(const struct device *dev, uint32_t channel)
{
	return -ENOTSUP;
}

static int dma_tsn_nic_resume(const struct device *dev, uint32_t channel)
{
	return -ENOTSUP;
}

static int dma_tsn_nic_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *status)
{
	return -ENOTSUP;
}

static int dma_tsn_nic_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	return -ENOTSUP;
}

static bool dma_tsn_nic_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	return -ENOTSUP;
}

static const struct dma_driver_api dma_tsn_nic_api = {
	.config = dma_tsn_nic_config,
	.reload = dma_tsn_nic_reload,
	.start = dma_tsn_nic_start,
	.stop = dma_tsn_nic_stop,
	.suspend = dma_tsn_nic_suspend,
	.resume = dma_tsn_nic_resume,
	.get_status = dma_tsn_nic_get_status,
	.get_attribute = dma_tsn_nic_get_attribute,
	.chan_filter = dma_tsn_nic_chan_filter,
};

static int get_engine_channel_id(struct dma_tsn_nic_engine_regs *regs)
{
	int value = sys_read32((mem_addr_t)&regs->identifier);

	return (value & DMA_CHANNEL_ID_MASK) >> DMA_CHANNEL_ID_LSB;
}

static int get_engine_id(struct dma_tsn_nic_engine_regs *regs)
{
	int value = sys_read32((mem_addr_t)&regs->identifier);

	return (value & DMA_ENGINE_ID_MASK) >> DMA_ENGINE_ID_LSB;
}

static int engine_init_regs(struct dma_tsn_nic_engine_regs *regs)
{
	uint32_t align_bytes, granularity_bytes, address_bits;
	uint32_t tmp, flags;

	sys_write32(DMA_CTRL_NON_INCR_ADDR, (mem_addr_t)&regs->control_w1c);
	tmp = sys_read32((mem_addr_t)&regs->alignments);
	/* These values will be used in other operations */
	if (tmp != 0) {
		align_bytes = (tmp & DMA_ALIGN_BYTES_MASK) >> DMA_ALIGN_BYTES_LSB;
		granularity_bytes = (tmp & DMA_GRANULARITY_BYTES_MASK) >> DMA_GRANULARITY_BYTES_LSB;
		address_bits = (tmp & DMA_ADDRESS_BITS_MASK) >> DMA_ADDRESS_BITS_LSB;
	} else {
		align_bytes = 1;
		granularity_bytes = 1;
		address_bits = 64;
	}

	flags = (DMA_CTRL_IE_DESC_ALIGN_MISMATCH | DMA_CTRL_IE_MAGIC_STOPPED |
		 DMA_CTRL_IE_IDLE_STOPPED | DMA_CTRL_IE_READ_ERROR | DMA_CTRL_IE_DESC_ERROR |
		 DMA_CTRL_IE_DESC_STOPPED | DMA_CTRL_IE_DESC_COMPLETED);

	sys_write32(flags, (mem_addr_t)&regs->interrupt_enable_mask);

	flags = (DMA_CTRL_RUN_STOP | DMA_CTRL_IE_READ_ERROR | DMA_CTRL_IE_DESC_ERROR |
		 DMA_CTRL_IE_DESC_ALIGN_MISMATCH | DMA_CTRL_IE_MAGIC_STOPPED |
		 DMA_CTRL_POLL_MODE_WB);

	sys_write32(flags, (mem_addr_t)&regs->control);

	return 0;
}

static int map_bar(const struct device *dev, int idx, size_t size)
{
	const struct dma_tsn_nic_config *config = dev->config;
	struct dma_tsn_nic_data *data = dev->data;
	uintptr_t bar_addr, bus_addr;
	bool ret;

	ret = pcie_ctrl_region_allocate(config->pci_dev, PCIE_BDF(idx, 0, 0), true, false,
					DMA_CONFIG_BAR_SIZE, &bus_addr);
	if (!ret) {
		return -EINVAL;
	}

	ret = pcie_ctrl_region_translate(config->pci_dev, PCIE_BDF(idx, 0, 0), true, false,
					 bus_addr, &bar_addr);
	if (!ret) {
		return -EINVAL;
	}

	device_map(&data->bar[idx], bar_addr, size, K_MEM_CACHE_NONE);

	return 0;
}

static int dma_tsn_nic_init(const struct device *dev)
{
	/* const struct dma_tsn_nic_config *config = dev->config; */
	struct dma_tsn_nic_data *data = dev->data;
	struct dma_tsn_nic_engine_regs *regs;
	int engine_id, channel_id;

	if (map_bar(dev, 0, 0x1000) != 0) {
		return -EINVAL;
	}

	if (map_bar(dev, DMA_CONFIG_BAR_IDX, DMA_CONFIG_BAR_SIZE) != 0) {
		return -EINVAL;
	}

	regs = (struct dma_tsn_nic_engine_regs *)(data->bar[DMA_CONFIG_BAR_IDX]);
	engine_id = get_engine_id(regs);
	channel_id = get_engine_channel_id(regs);
	printk("H2C\n");
	printk("engine_id 0x%x\n", engine_id);
	printk("channel_id 0x%x\n", channel_id);
	if ((engine_id != DMA_ID_H2C) || (channel_id != 0)) {
		return -EINVAL;
	}

	engine_init_regs(regs);
	data->regs[DMA_H2C] = regs;

	regs = (struct dma_tsn_nic_engine_regs *)(data->bar[DMA_CONFIG_BAR_IDX] + DMA_C2H_OFFSET);
	engine_id = get_engine_id(regs);
	channel_id = get_engine_channel_id(regs);
	printk("C2H\n");
	printk("engine_id 0x%x\n", engine_id);
	printk("channel_id 0x%x\n", channel_id);
	if ((engine_id != DMA_ID_C2H) || (channel_id != 0)) {
		return -EINVAL;
	}

	engine_init_regs(regs);
	data->regs[DMA_C2H] = regs;

	/* TSN registers */
	sys_write32(0x1, data->bar[0] + 0x0008);
	sys_write32(0x800f0000, data->bar[0] + 0x0610);
	sys_write32(0x10, data->bar[0] + 0x0620);

	return 0;
}

/* TODO: POST_KERNEL is set to use printk, revert this after the development is done */
#define DMA_TSN_NIC_INIT(n)                                                                        \
	static struct dma_tsn_nic_data dma_tsn_nic_data_##n = {};                                  \
                                                                                                   \
	static const struct dma_tsn_nic_config dma_tsn_nic_cfg_##n = {                             \
		.pci_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, dma_tsn_nic_init, NULL, &dma_tsn_nic_data_##n,                    \
			      &dma_tsn_nic_cfg_##n, POST_KERNEL, 98, &dma_tsn_nic_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_TSN_NIC_INIT)
