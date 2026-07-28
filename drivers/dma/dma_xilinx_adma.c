/*
 * Copyright (c) 2026 Advanced Micro Devices.
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

/* Interrupt registers bit field definitions */
#define XILINX_ADMA_DONE             BIT(10)
#define XILINX_ADMA_AXI_WR_DATA      BIT(9)
#define XILINX_ADMA_AXI_RD_DATA      BIT(8)
#define XILINX_ADMA_AXI_RD_DST_DSCR  BIT(7)
#define XILINX_ADMA_AXI_RD_SRC_DSCR  BIT(6)
#define XILINX_ADMA_IRQ_DST_ACCT_ERR BIT(5)
#define XILINX_ADMA_IRQ_SRC_ACCT_ERR BIT(4)
#define XILINX_ADMA_BYTE_CNT_OVRFL   BIT(3)
#define XILINX_ADMA_DST_DSCR_DONE    BIT(2)
#define XILINX_ADMA_INV_APB          BIT(0)

/* Control 0 register bit field definitions */
#define XILINX_ADMA_OVR_FETCH     BIT(7)
#define XILINX_ADMA_POINT_TYPE_SG BIT(6)
#define XILINX_ADMA_RATE_CTRL_EN  BIT(3)

/* Control 1 register bit field definitions */
#define XILINX_ADMA_SRC_ISSUE GENMASK(4, 0)

/* Data Attribute register bit field definitions */
#define XILINX_ADMA_ARBURST      GENMASK(27, 26)
#define XILINX_ADMA_ARCACHE      GENMASK(25, 22)
#define XILINX_ADMA_ARCACHE_OFST 22
#define XILINX_ADMA_ARQOS        GENMASK(21, 18)
#define XILINX_ADMA_ARQOS_OFST   18
#define XILINX_ADMA_ARLEN        GENMASK(17, 14)
#define XILINX_ADMA_ARLEN_OFST   14
#define XILINX_ADMA_AWBURST      GENMASK(13, 12)
#define XILINX_ADMA_AWCACHE      GENMASK(11, 8)
#define XILINX_ADMA_AWCACHE_OFST 8
#define XILINX_ADMA_AWQOS        GENMASK(7, 4)
#define XILINX_ADMA_AWQOS_OFST   4
#define XILINX_ADMA_AWLEN        GENMASK(3, 0)
#define XILINX_ADMA_AWLEN_OFST   0

/* Descriptor Attribute register bit field definitions */
#define XILINX_ADMA_AXCOHRNT     BIT(8)
#define XILINX_ADMA_AXCACHE      GENMASK(7, 4)
#define XILINX_ADMA_AXCACHE_OFST 4
#define XILINX_ADMA_AXQOS        GENMASK(3, 0)
#define XILINX_ADMA_AXQOS_OFST   0

/* Control register 2 bit field definitions */
#define XILINX_ADMA_ENABLE BIT(0)

/* Buffer Descriptor definitions */
#define XILINX_ADMA_DESC_CTRL_STOP     0x10
#define XILINX_ADMA_DESC_CTRL_COMP_INT 0x4
#define XILINX_ADMA_DESC_CTRL_SIZE_256 0x2
#define XILINX_ADMA_DESC_CTRL_COHRNT   0x1

#define XILINX_ADMA_START 0x1

/* Interrupt Mask specific definitions */
#define XILINX_ADMA_INT_ERR                                                                        \
	(XILINX_ADMA_AXI_RD_DATA | XILINX_ADMA_AXI_WR_DATA | XILINX_ADMA_AXI_RD_DST_DSCR |         \
	 XILINX_ADMA_AXI_RD_SRC_DSCR | XILINX_ADMA_INV_APB)
#define XILINX_ADMA_INT_OVRFL                                                                      \
	(XILINX_ADMA_BYTE_CNT_OVRFL | XILINX_ADMA_IRQ_SRC_ACCT_ERR | XILINX_ADMA_IRQ_DST_ACCT_ERR)
#define XILINX_ADMA_INT_DONE (XILINX_ADMA_DONE | XILINX_ADMA_DST_DSCR_DONE)
#define XILINX_ADMA_INT_EN_DEFAULT_MASK                                                            \
	(XILINX_ADMA_INT_DONE | XILINX_ADMA_INT_ERR | XILINX_ADMA_INT_OVRFL |                      \
	 XILINX_ADMA_DST_DSCR_DONE)

/* Max number of descriptors per channel */
#define XILINX_ADMA_NUM_DESCS 32

/* Max transfer size per descriptor */
#define XILINX_ADMA_MAX_TRANS_LEN 0x40000000

/* Default burst length (ARLEN/AWLEN are 4-bit fields, max 16 beats) */
#define XILINX_ADMA_DEFAULT_BURST_LEN 16U
#define XILINX_ADMA_MAX_BURST_LEN     16U

/* Reset values for data attributes */
#define XILINX_ADMA_AXCACHE_VAL 0xF

#define XILINX_ADMA_SRC_ISSUE_RST_VAL 0x1F

#define XILINX_ADMA_IDS_DEFAULT_MASK 0xFFF

#define XILINX_ADMA_DATA_ATTR_RST_VAL 0x0483D20F

/* Reset value for control reg attributes */
#define XILINX_ADMA_RESET_VAL  0x80
#define XILINX_ADMA_RESET_VAL1 0x3FF
#define XILINX_ADMA_RESET_VAL2 0x0

#define XILINX_ADMA_WORD0_LSB_MASK (0xFFFFFFFFU)

/* @name Channel Source/Destination Word1 register bit mask */
#define XILINX_ADMA_WORD1_MSB_MASK  (0x0001FFFFU) /**< MSB Address mask */
#define XILINX_ADMA_WORD1_MSB_SHIFT (32U)         /**< MSB Address shift */
#define XILINX_ADMA_WORD2_SIZE_MASK (0x3FFFFFFFU) /**< Size mask */

struct __attribute__((__packed__)) dma_xilinx_adma_registers {
	uint32_t err_cr;         /* Error control register */
	uint32_t reserved_0[63]; /* Reserved space */
#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_dma_1_0)
	uint32_t chan_isr; /* Interrupt Status Register */
	uint32_t chan_imr; /* Interrupt Mask Register */
	uint32_t chan_ien; /* Interrupt Enable Register */
	uint32_t chan_ids; /* Interrupt Disable Register */
#elif DT_HAS_COMPAT_STATUS_OKAY(amd_versal2_dma_1_0)
	uint32_t chan_err_isr; /* Error Status Register */
	uint32_t chan_err_imr; /* Error Mask Register */
	uint32_t chan_err_ien; /* Error Enable Register */
	uint32_t chan_err_ids; /* Error Disable Register */
#endif
	uint32_t chan_cntrl0;       /* Channel Control Register 0 */
	uint32_t chan_cntrl1;       /* Channel Flow Control Register */
	uint32_t chan_fci;          /* Channel Control Register 1 */
	uint32_t chan_sts;          /* Channel Status Register */
	uint32_t chan_data_attr;    /* Channel Data Register */
	uint32_t chan_dscr_attr;    /* Channel DSCR Register */
	uint32_t chan_srcdscr_wrd0; /* SRC DSCR Word 0 */
	uint32_t chan_srcdscr_wrd1; /* SRC DSCR Word 1 */
	uint32_t chan_srcdscr_wrd2; /* SRC DSCR Word 2 */
	uint32_t chan_srcdscr_wrd3; /* SRC DSCR Word 3 */
	uint32_t chan_dstdscr_wrd0; /* DST DSCR Word 0 */
	uint32_t chan_dstdscr_wrd1; /* DST DSCR Word 1 */
	uint32_t chan_dstdscr_wrd2; /* DST DSCR Word 2 */
	uint32_t chan_dstdscr_wrd3; /* DST DSCR Word 3 */
	uint32_t chan_wronly_wrd0;  /* Write only Data Word 0 */
	uint32_t chan_wronly_wrd1;  /* Write only Data Word 1 */
	uint32_t chan_wronly_wrd2;  /* Write only Data Word 2 */
	uint32_t chan_wronly_wrd3;  /* Write only Data Word 3 */
	uint32_t chan_srcdesc;      /* SRC DSCR Start Address LSB Register */
	uint32_t chan_srcdesc_msb;  /* SRC DSCR Start Address MSB Register */
	uint32_t chan_dstdesc;      /* DST DSCR Start Address LSB Register */
	uint32_t chan_dstdesc_msb;  /* DST DSCR Start Address MSB Register */
	uint32_t reserved_1[9];     /* Reserved space */
	uint32_t chan_rate_cntrl;   /* Rate Control Count Register */
	uint32_t chan_irq_src_acct; /* SRC Interrupt Account Count Register */
	uint32_t chan_irq_dst_acct; /* DST Interrupt Account Count Register */
	uint32_t reserved_2[26];    /* Register Space */
	uint32_t chan_cntrl2;       /* Control Register */
	uint32_t reserved_3[129];   /* Register Space */
#if DT_HAS_COMPAT_STATUS_OKAY(amd_versal2_dma_1_0)
	uint32_t chan_isr; /* Interrupt Status Register */
	uint32_t chan_imr; /* Interrupt Mask Register */
	uint32_t chan_ien; /* Interrupt Enable Register */
	uint32_t chan_ids; /* Interrupt Disable Register */
	uint32_t chan_itr;
#endif
} __aligned(64);

#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_dma_1_0)
#define DT_DRV_COMPAT xlnx_zynqmp_dma_1_0
#elif DT_HAS_COMPAT_STATUS_OKAY(amd_versal2_dma_1_0)
#define DT_DRV_COMPAT amd_versal2_dma_1_0
#endif

LOG_MODULE_REGISTER(dma_xilinx_adma, CONFIG_DMA_LOG_LEVEL);

/* global configuration per DMA device */
struct dma_xilinx_adma_config {
	DEVICE_MMIO_ROM;
	bool dma_coherent;
	const struct device *main_clock;
	const struct device *apb_clock;
	uint8_t channel_id;
	uint32_t irq_num;
	void (*irq_configure)(void);
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

	/* Drain IRQ accounting registers to prevent overflow */
	(void)adma_read_reg(&reg->chan_irq_src_acct);
	(void)adma_read_reg(&reg->chan_irq_dst_acct);

	if (status & XILINX_ADMA_INT_ERR) {
		LOG_ERR("DMA AXI error occurred:0x%x", status);
		callback_status = -EIO;
	} else if ((status & XILINX_ADMA_INT_OVRFL) && !(status & XILINX_ADMA_INT_DONE)) {
		LOG_ERR("DMA overflow error occurred: 0x%x", status);
		callback_status = -EOVERFLOW;
	}

	if (status & (XILINX_ADMA_INT_ERR | XILINX_ADMA_INT_OVRFL | XILINX_ADMA_INT_DONE)) {
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
		if (cfg->dma_coherent) {
			src_desc->ctrl |= XILINX_ADMA_DESC_CTRL_COHRNT;
		}

		dst_desc->addr = block->dest_address;
		dst_desc->size = block->block_size & XILINX_ADMA_WORD2_SIZE_MASK;
		dst_desc->ctrl = XILINX_ADMA_DESC_CTRL_SIZE_256;

		if (cfg->dma_coherent) {
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

		block = block->next_block;
	}

	sys_cache_data_flush_range(desc, desc_count * sizeof(struct dma_xilinx_adma_desc_sw));
	data->chan.desc = desc;
	data->chan.desc_count = desc_count;
	return desc_count;
}

static void dma_xilinx_adma_free_sg_descriptors(const struct device *dev)
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

	dma_xilinx_adma_free_sg_descriptors(dev);

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

		if (cfg->dma_coherent) {
			val = XILINX_ADMA_AXCOHRNT;
			val = (val & ~XILINX_ADMA_AXCACHE) |
			      (XILINX_ADMA_AXCACHE_VAL << XILINX_ADMA_AXCACHE_OFST);
			adma_write_reg(val, &reg->chan_dscr_attr);
		}

		val = adma_read_reg(&reg->chan_data_attr);
		if (cfg->dma_coherent) {
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
		LOG_ERR("Burst length exceeds max (%u): src=%u dst=%u", XILINX_ADMA_MAX_BURST_LEN,
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

	dma_xilinx_adma_free_sg_descriptors(dev);

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

		LOG_DBG("Configured SG mode with %d descriptors", desc_count);
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

		adma_write_reg((src_desc_addr & XILINX_ADMA_WORD0_LSB_MASK), &reg->chan_srcdesc);
		adma_write_reg((src_desc_addr >> XILINX_ADMA_WORD1_MSB_SHIFT),
			       &reg->chan_srcdesc_msb);

		uint64_t dst_desc_addr = (uintptr_t)&data->chan.desc[0].dst_desc;

		adma_write_reg((dst_desc_addr & XILINX_ADMA_WORD0_LSB_MASK), &reg->chan_dstdesc);
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
		if (cfg->dma_coherent) {
			val |= XILINX_ADMA_DESC_CTRL_COHRNT;
		}

		adma_write_reg(val, &reg->chan_srcdscr_wrd3);
		adma_write_reg(val, &reg->chan_dstdscr_wrd3);
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

	if (data->chan.desc && data->chan.desc_count > 0) {
		sys_cache_data_flush_range(data->chan.desc,
					   data->chan.desc_count *
						   sizeof(struct dma_xilinx_adma_desc_sw));
		barrier_dmem_fence_full();
	}

	/*
	 * Re-route the completion interrupt to the CPU starting the transfer.
	 * On GICv3 SMP, irq_enable() updates IROUTER with the caller's MPIDR.
	 */
	irq_enable(cfg->irq_num);

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
		.dma_coherent = DT_INST_PROP(n, dma_coherent),                                     \
		.main_clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(n, 0)),                     \
		.apb_clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(n, 1)),                      \
		.channel_id = 0,                                                                  \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.irq_configure = dma_xilinx_adma##n##_irq_configure,                               \
	};                                                                                         \
	static struct dma_xilinx_adma_data dma_xilinx_adma##n##_data = {                           \
		.ctx = {.magic = DMA_MAGIC, .atomic = NULL},                                       \
		.dma_desc_pool = &desc_pool_##n,                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &dma_xilinx_adma_init, NULL, &dma_xilinx_adma##n##_data,          \
			      &dma_xilinx_adma##n##_config, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, \
			      &dma_xilinx_adma_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XILINX_ADMA_INIT)
