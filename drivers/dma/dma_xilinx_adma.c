/*
 * Copyright (c) 2025 Advanced Micro Devices.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/barrier.h>

#include "dma_xilinx_adma.h"

#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_dma_1_0)
#define DT_DRV_COMPAT xlnx_zynqmp_dma_1_0
#elif DT_HAS_COMPAT_STATUS_OKAY(amd_versal2_dma_1_0)
#define DT_DRV_COMPAT amd_versal2_dma_1_0
#endif

LOG_MODULE_REGISTER(dma_xilinx_adma, CONFIG_DMA_LOG_LEVEL);

/* global configuration per DMA device */
struct dma_xilinx_adma_config {
	DEVICE_MMIO_ROM;
	bool cachecoherent;
	const struct device *main_clock;
	const struct device *apb_clock;
	uint8_t channel_id;
	void (*irq_configure)();
	uint8_t bus_width;
};

struct dma_xilinx_adma_chan {
	dma_callback_t dma_callback;
	void *callback_user_data;
	uint64_t src_addr;
	uint64_t dst_addr;
	uint32_t block;
	struct dma_xilinx_adma_desc_sw *desc;
	uint32_t desc_count;
	uint32_t src_burst_len;
	uint32_t dst_burst_len;
};

struct dma_xilinx_adma_data {
	DEVICE_MMIO_RAM;
	struct dma_context ctx;
	struct dma_xilinx_adma_chan chan;
	struct sys_mem_blocks *dma_desc_pool;
	struct k_spinlock lock;
	bool device_has_been_reset;
};

struct dma_xilinx_adma_desc_ll {
	uint64_t addr;
	uint32_t size;
	uint32_t ctrl;
	uint64_t nxtdscraddr;
	uint64_t rsvd;
};

struct dma_xilinx_adma_desc_sw {
	struct dma_xilinx_adma_desc_ll src_desc;
	struct dma_xilinx_adma_desc_ll dst_desc;
};

static inline void adma_write_reg(uint32_t val, volatile uint32_t *reg)
{
	barrier_dmem_fence_full();
	sys_write32(val, (mem_addr_t)reg);
}

static inline uint32_t adma_read_reg(volatile uint32_t *reg)
{
	uint32_t val = sys_read32((mem_addr_t)reg);

	barrier_dmem_fence_full();
	return val;
}

static void adma_invalidate_sg_buffers(struct dma_xilinx_adma_data *data)
{
	if (!data->chan.desc || data->chan.desc_count == 0) {
		LOG_ERR("SG mode but no descriptors found! desc=%p count=%d", data->chan.desc,
			data->chan.desc_count);
		return;
	}

	for (int i = 0; i < data->chan.desc_count; i++) {
		struct dma_xilinx_adma_desc_ll *dst_desc = &data->chan.desc[i].dst_desc;

		if (!dst_desc->addr || dst_desc->size == 0) {
			LOG_ERR("Invalid desc %d: addr=0x%llx size=%d", i, dst_desc->addr,
				dst_desc->size);
			return;
		}

		sys_cache_data_invd_range((void *)(uintptr_t)dst_desc->addr, dst_desc->size);
	}

	barrier_dmem_fence_full();
}

static void dma_xilinx_adma_done(const struct device *dev,
				 struct dma_xilinx_adma_data *data)
{
	volatile struct dma_xilinx_adma_registers *reg =
		(struct dma_xilinx_adma_registers *)DEVICE_MMIO_GET(dev);
	uint32_t ctrl0 = adma_read_reg(&reg->chan_cntrl0);

	if (ctrl0 & XILINX_ADMA_POINT_TYPE_SG) {
		adma_invalidate_sg_buffers(data);
	} else {
		sys_cache_data_invd_range((void *)(uintptr_t)data->chan.dst_addr, data->chan.block);
	}
}

static void dma_xilinx_adma_isr(const struct device *dev)
{
	const struct dma_xilinx_adma_config *cfg = dev->config;
	struct dma_xilinx_adma_data *data = dev->data;
	volatile struct dma_xilinx_adma_registers *reg =
		(struct dma_xilinx_adma_registers *)DEVICE_MMIO_GET(dev);
	dma_callback_t callback = NULL;
	void *callback_user_data = NULL;
	int callback_status = DMA_STATUS_COMPLETE;
	k_spinlock_key_t key;
	uint32_t status;

	key = k_spin_lock(&data->lock);

	status = adma_read_reg(&reg->chan_isr);

	if (!data->device_has_been_reset) {
		LOG_ERR("DMA not ready, ignoring the interrupt");
		k_spin_unlock(&data->lock, key);
		return;
	}

	adma_write_reg(status, &reg->chan_isr);

	if (status & XILINX_ADMA_INT_ERR) {
		LOG_ERR("DMA AXI error occurred:0x%x", status);
		callback_status = -EIO;
	} else if (status & XILINX_ADMA_INT_OVRFL) {
		LOG_ERR("DMA overflow error occurred: 0x%x", status);
		callback_status = -EOVERFLOW;
	}

	if (status & (XILINX_ADMA_INT_ERR | XILINX_ADMA_INT_OVRFL | XILINX_ADMA_INT_DONE)) {
		dma_xilinx_adma_done(dev, data);
		callback = data->chan.dma_callback;
		callback_user_data = data->chan.callback_user_data;
	}

	adma_write_reg(XILINX_ADMA_IDS_DEFAULT_MASK, &reg->chan_ids);

	k_spin_unlock(&data->lock, key);

	if (callback) {
		callback(dev, callback_user_data, cfg->channel_id, callback_status);
	}
}

static int dma_xilinx_adma_setup_sg_descriptors(const struct device *dev,
						struct dma_config *dma_cfg)
{
	const struct dma_xilinx_adma_config *cfg = dev->config;
	struct dma_block_config *block = dma_cfg->head_block;
	struct dma_xilinx_adma_data *data = dev->data;
	struct dma_block_config *temp_block = block;
	struct dma_xilinx_adma_desc_sw *desc;
	int ret, desc_count = 0;

	while (temp_block) {
		desc_count++;
		if (desc_count > XILINX_ADMA_NUM_DESCS) {
			LOG_ERR("Too many descriptors: %d (max: %d)", desc_count,
				XILINX_ADMA_NUM_DESCS);
			return -EINVAL;
		}
		temp_block = temp_block->next_block;
	}

	ret = sys_mem_blocks_alloc_contiguous(data->dma_desc_pool, desc_count, (void **)&desc);
	if (ret < 0) {
		LOG_ERR("Failed to allocate SG descriptors");
		return ret;
	}

	for (int i = 0; i < desc_count; i++) {
		struct dma_xilinx_adma_desc_ll *src_desc = &desc[i].src_desc;
		struct dma_xilinx_adma_desc_ll *dst_desc = &desc[i].dst_desc;

		src_desc->addr = block->source_address;
		src_desc->size = block->block_size & XILINX_ADMA_WORD2_SIZE_MASK;
		src_desc->ctrl = XILINX_ADMA_DESC_CTRL_SIZE_256;
		if (cfg->cachecoherent) {
			src_desc->ctrl |= XILINX_ADMA_DESC_CTRL_COHRNT;
		}

		dst_desc->addr = block->dest_address;
		dst_desc->size = block->block_size & XILINX_ADMA_WORD2_SIZE_MASK;
		dst_desc->ctrl = XILINX_ADMA_DESC_CTRL_SIZE_256;

		if (cfg->cachecoherent) {
			dst_desc->ctrl |= XILINX_ADMA_DESC_CTRL_COHRNT;
		}

		if (i == desc_count - 1) {
			src_desc->ctrl |= XILINX_ADMA_DESC_CTRL_STOP;
			dst_desc->ctrl |=
				XILINX_ADMA_DESC_CTRL_COMP_INT | XILINX_ADMA_DESC_CTRL_STOP;
			src_desc->nxtdscraddr = 0;
			dst_desc->nxtdscraddr = 0;
		} else {
			uint64_t next_src_addr = (uintptr_t)&desc[i + 1].src_desc;
			uint64_t next_dst_addr = (uintptr_t)&desc[i + 1].dst_desc;

			src_desc->nxtdscraddr = next_src_addr;
			dst_desc->nxtdscraddr = next_dst_addr;
		}

		/*
		 * Flush and invalidate destination before DMA writes to it so
		 * that no dirty cache line can be written back over DMA data
		 * and the post-transfer invalidate sees fresh DMA content.
		 */
		sys_cache_data_flush_and_invd_range(
			(void *)(uintptr_t)block->dest_address, block->block_size);
		sys_cache_data_flush_range((void *)(uintptr_t)block->source_address,
					   block->block_size);
		block = block->next_block;
	}

	sys_cache_data_flush_range(desc, desc_count * sizeof(struct dma_xilinx_adma_desc_sw));
	data->chan.desc = desc;
	data->chan.desc_count = desc_count;
	return desc_count;
}

static void __maybe_unused dma_xilinx_adma_free_sg_descriptors(const struct device *dev)
{
	struct dma_xilinx_adma_data *data = dev->data;

	if (data->chan.desc) {
		sys_mem_blocks_free_contiguous(data->dma_desc_pool, data->chan.desc,
					       data->chan.desc_count);
		data->chan.desc = NULL;
		data->chan.desc_count = 0;
	}
}

static int dma_xilinx_adma_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_xilinx_adma_config *cfg = dev->config;
	struct dma_xilinx_adma_data *data = dev->data;
	volatile struct dma_xilinx_adma_registers *reg =
		(struct dma_xilinx_adma_registers *)DEVICE_MMIO_GET(dev);
	k_spinlock_key_t key;

	if (channel != cfg->channel_id) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* Disable the channel to stop ongoing transfers */
	adma_write_reg(XILINX_ADMA_RESET_VAL2, &reg->chan_cntrl2);

	/* Disable all interrupts */
	adma_write_reg(XILINX_ADMA_IDS_DEFAULT_MASK, &reg->chan_ids);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dma_xilinx_adma_get_status(const struct device *dev, uint32_t channel,
				      struct dma_status *stat)
{
	struct dma_xilinx_adma_data *data = dev->data;
	volatile struct dma_xilinx_adma_registers *reg =
		(struct dma_xilinx_adma_registers *)DEVICE_MMIO_GET(dev);
	k_spinlock_key_t key;
	uint32_t status;

	if (!stat) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	status = adma_read_reg(&reg->chan_sts);
	stat->busy = !!(status & XILINX_ADMA_START);
	stat->dir = MEMORY_TO_MEMORY;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dma_xilinx_adma_configure(const struct device *dev, uint32_t channel,
				     struct dma_config *dma_cfg)
{
	const struct dma_xilinx_adma_config *cfg = dev->config;
	struct dma_xilinx_adma_data *data = dev->data;
	volatile struct dma_xilinx_adma_registers *reg =
		(struct dma_xilinx_adma_registers *)DEVICE_MMIO_GET(dev);
	struct dma_block_config *current_block = dma_cfg->head_block;
	k_spinlock_key_t key;
	uint64_t addr;
	uint32_t val;
	int ret = 0;

	if (channel != cfg->channel_id) {
		return -EINVAL;
	}

	if (!dma_cfg->head_block) {
		return -EINVAL;
	}

	if (dma_cfg->dest_data_size != dma_cfg->source_data_size) {
		LOG_ERR("Source and dest data size differ.");
		return -EINVAL;
	}

	if (cfg->bus_width != XILINX_ADMA_BUS_WIDTH_64 &&
	    cfg->bus_width != XILINX_ADMA_BUS_WIDTH_128) {
		LOG_ERR("Invalid bus-width value: %u", cfg->bus_width);
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->device_has_been_reset) {
		LOG_INF("Soft-resetting the DMA core!");

		adma_write_reg(XILINX_ADMA_IDS_DEFAULT_MASK, &reg->chan_ids);
		val = adma_read_reg(&reg->chan_isr);
		adma_write_reg(val, &reg->chan_isr);

		/* Configurations reset */
		adma_write_reg(XILINX_ADMA_RESET_VAL, &reg->chan_cntrl0);
		adma_write_reg(XILINX_ADMA_RESET_VAL1, &reg->chan_cntrl1);
		adma_write_reg(XILINX_ADMA_RESET_VAL2, &reg->chan_cntrl2);
		adma_write_reg(XILINX_ADMA_DATA_ATTR_RST_VAL, &reg->chan_data_attr);

		if (cfg->cachecoherent) {
			val = XILINX_ADMA_AXCOHRNT;
			val = (val & ~XILINX_ADMA_AXCACHE) |
			      (XILINX_ADMA_AXCACHE_VAL << XILINX_ADMA_AXCACHE_OFST);
			adma_write_reg(val, &reg->chan_dscr_attr);
		}

		val = adma_read_reg(&reg->chan_data_attr);
		if (cfg->cachecoherent) {
			val = (val & ~XILINX_ADMA_ARCACHE) |
			      (XILINX_ADMA_AXCACHE_VAL << XILINX_ADMA_ARCACHE_OFST);
			val = (val & ~XILINX_ADMA_AWCACHE) |
			      (XILINX_ADMA_AXCACHE_VAL << XILINX_ADMA_AWCACHE_OFST);
		}
		adma_write_reg(val, &reg->chan_data_attr);

		val = adma_read_reg(&reg->chan_irq_src_acct);
		val = adma_read_reg(&reg->chan_irq_dst_acct);

		data->device_has_been_reset = true;
	}

	data->chan.src_burst_len = dma_cfg->source_burst_length;
	data->chan.dst_burst_len = dma_cfg->dest_burst_length;

	if (data->chan.src_burst_len == 0) {
		data->chan.src_burst_len = XILINX_ADMA_DEFAULT_BURST_LEN;
	}
	if (data->chan.dst_burst_len == 0) {
		data->chan.dst_burst_len = XILINX_ADMA_DEFAULT_BURST_LEN;
	}

	if (data->chan.src_burst_len > XILINX_ADMA_MAX_BURST_LEN ||
	    data->chan.dst_burst_len > XILINX_ADMA_MAX_BURST_LEN) {
		LOG_ERR("Burst length exceeds max (%u): src=%u dst=%u",
			XILINX_ADMA_MAX_BURST_LEN,
			data->chan.src_burst_len, data->chan.dst_burst_len);
		ret = -EINVAL;
		goto unlock;
	}

	val = adma_read_reg(&reg->chan_data_attr);
	val = (val & ~XILINX_ADMA_ARLEN) |
	      ((data->chan.src_burst_len - 1) << XILINX_ADMA_ARLEN_OFST);
	val = (val & ~XILINX_ADMA_AWLEN) |
	      ((data->chan.dst_burst_len - 1) << XILINX_ADMA_AWLEN_OFST);
	adma_write_reg(val, &reg->chan_data_attr);

	if (dma_cfg->head_block->next_block) {
		/* Configure for scatter gather mode */
		int desc_count = dma_xilinx_adma_setup_sg_descriptors(dev, dma_cfg);

		if (desc_count < 0) {
			ret = desc_count;
			goto unlock;
		}

		val = adma_read_reg(&reg->chan_cntrl0);
		val |= XILINX_ADMA_POINT_TYPE_SG;
		adma_write_reg(val, &reg->chan_cntrl0);

		LOG_INF("Configured SG mode with %d descriptors", desc_count);
	} else {
		/* Simple mode - single block transfer */
		data->chan.src_addr = current_block->source_address;
		data->chan.dst_addr = current_block->dest_address;
		data->chan.block = current_block->block_size;

		val = adma_read_reg(&reg->chan_cntrl0);
		val &= ~XILINX_ADMA_POINT_TYPE_SG;
		adma_write_reg(val, &reg->chan_cntrl0);
	}

	val = adma_read_reg(&reg->chan_cntrl0);
	val |= XILINX_ADMA_OVR_FETCH;
	adma_write_reg(val, &reg->chan_cntrl0);

	data->chan.dma_callback = dma_cfg->dma_callback;
	data->chan.callback_user_data = dma_cfg->user_data;

	/* Check if scatter-gather mode is enabled */
	val = adma_read_reg(&reg->chan_cntrl0);
	if (val & XILINX_ADMA_POINT_TYPE_SG) {
		if (!data->chan.desc) {
			LOG_ERR("No SG descriptors configured");
			ret = -EINVAL;
			goto unlock;
		}

		uint64_t src_desc_addr = (uintptr_t)&data->chan.desc[0].src_desc;

		adma_write_reg((src_desc_addr & XILINX_ADMA_WORD0_LSB_MASK),
			       &reg->chan_srcdesc);
		adma_write_reg((src_desc_addr >> XILINX_ADMA_WORD1_MSB_SHIFT),
			       &reg->chan_srcdesc_msb);

		uint64_t dst_desc_addr = (uintptr_t)&data->chan.desc[0].dst_desc;

		adma_write_reg((dst_desc_addr & XILINX_ADMA_WORD0_LSB_MASK),
			       &reg->chan_dstdesc);
		adma_write_reg((dst_desc_addr >> XILINX_ADMA_WORD1_MSB_SHIFT),
			       &reg->chan_dstdesc_msb);
	} else {
		adma_write_reg(((data->chan.src_addr) & XILINX_ADMA_WORD0_LSB_MASK),
			       &reg->chan_srcdscr_wrd0);
		addr = (uint64_t)data->chan.src_addr;
		adma_write_reg(((addr >> XILINX_ADMA_WORD1_MSB_SHIFT) & XILINX_ADMA_WORD1_MSB_MASK),
			       &reg->chan_srcdscr_wrd1);

		adma_write_reg(((data->chan.block) & XILINX_ADMA_WORD2_SIZE_MASK),
			       &reg->chan_srcdscr_wrd2);

		adma_write_reg(((data->chan.dst_addr) & XILINX_ADMA_WORD0_LSB_MASK),
			       &reg->chan_dstdscr_wrd0);
		addr = (uint64_t)data->chan.dst_addr;
		adma_write_reg(((addr >> XILINX_ADMA_WORD1_MSB_SHIFT) & XILINX_ADMA_WORD1_MSB_MASK),
			       &reg->chan_dstdscr_wrd1);

		adma_write_reg(((data->chan.block) & XILINX_ADMA_WORD2_SIZE_MASK),
			       &reg->chan_dstdscr_wrd2);

		val = XILINX_ADMA_DESC_CTRL_SIZE_256;
		if (cfg->cachecoherent) {
			val |= XILINX_ADMA_DESC_CTRL_COHRNT;
		}

		adma_write_reg(val, &reg->chan_srcdscr_wrd3);
		adma_write_reg(val, &reg->chan_dstdscr_wrd3);

		/*
		 * Flush and invalidate destination before DMA writes to it so
		 * that no dirty cache line can be written back over DMA data
		 * and the post-transfer invalidate sees fresh DMA content.
		 */
		sys_cache_data_flush_and_invd_range(
			(void *)(uintptr_t)data->chan.dst_addr, data->chan.block);
		sys_cache_data_flush_range((void *)(uintptr_t)data->chan.src_addr,
					   data->chan.block);
	}

unlock:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int dma_xilinx_adma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_xilinx_adma_config *cfg = dev->config;
	struct dma_xilinx_adma_data *data = dev->data;
	volatile struct dma_xilinx_adma_registers *reg =
		(struct dma_xilinx_adma_registers *)DEVICE_MMIO_GET(dev);
	k_spinlock_key_t key;

	if (channel != cfg->channel_id) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	adma_write_reg(XILINX_ADMA_INT_EN_DEFAULT_MASK, &reg->chan_ien);
	adma_write_reg(XILINX_ADMA_START, &reg->chan_cntrl2);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static DEVICE_API(dma, dma_xilinx_adma_driver_api) = {
	.config = dma_xilinx_adma_configure,
	.start = dma_xilinx_adma_start,
	.stop = dma_xilinx_adma_stop,
	.get_status = dma_xilinx_adma_get_status,
};

static int dma_xilinx_adma_init(const struct device *dev)
{
	const struct dma_xilinx_adma_config *cfg = dev->config;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = clock_control_on(cfg->apb_clock, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to enable APB clock");
		return ret;
	}

	ret = clock_control_on(cfg->main_clock, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to enable main clock");
		return ret;
	}

	cfg->irq_configure();
	return 0;
}

#define XILINX_ADMA_INIT(n)                                                                        \
	SYS_MEM_BLOCKS_DEFINE_STATIC(desc_pool_##n, sizeof(struct dma_xilinx_adma_desc_sw),        \
				     CONFIG_DMA_XILINX_ADMA_SG_BUFFER_COUNT,                       \
				     CONFIG_DMA_XILINX_ADMA_DESC_POOL_ALIGNMENT);                  \
	static void dma_xilinx_adma##n##_irq_configure(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), dma_xilinx_adma_isr,        \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct dma_xilinx_adma_config dma_xilinx_adma##n##_config = {                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.cachecoherent = DT_INST_PROP_OR(n, cache_coherent, 0),                            \
		.main_clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(n, 0)),                     \
		.apb_clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(n, 1)),                      \
		.channel_id = n, /* Assign channel ID based on instance */                         \
		.irq_configure = dma_xilinx_adma##n##_irq_configure,                               \
		.bus_width = DT_INST_PROP_OR(n, xlnx_bus_width, XILINX_ADMA_BUS_WIDTH_64),         \
	};                                                                                         \
	static struct dma_xilinx_adma_data dma_xilinx_adma##n##_data = {                           \
		.ctx = {.magic = DMA_MAGIC, .atomic = NULL},                                       \
		.dma_desc_pool = &desc_pool_##n,                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &dma_xilinx_adma_init, NULL, &dma_xilinx_adma##n##_data,          \
			      &dma_xilinx_adma##n##_config, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, \
			      &dma_xilinx_adma_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XILINX_ADMA_INIT)
