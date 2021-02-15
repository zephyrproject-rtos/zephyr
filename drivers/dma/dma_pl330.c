/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/dma.h>
#include <errno.h>
#include <init.h>
#include <string.h>
#include <soc.h>
#include <sys/__assert.h>
#include "dma_pl330.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_pl330);

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((const struct dma_pl330_config *const)(dev)->config)

#define DEV_DATA(dev) \
	((struct dma_pl330_dev_data *const)(dev)->data)

#define BYTE_WIDTH(burst_size) (1 << (burst_size))

static int dma_pl330_submit(const struct device *dev, uint64_t dst,
			    uint64_t src, uint32_t channel, uint32_t size);

static void dma_pl330_get_counter(struct dma_pl330_ch_internal *ch_handle,
				  uint32_t *psrc_byte_width,
				  uint32_t *pdst_byte_width,
				  uint32_t *ploop_counter,
				  uint32_t *presidue)
{
	uint32_t srcbytewidth, dstbytewidth;
	uint32_t loop_counter, residue;

	srcbytewidth = BYTE_WIDTH(ch_handle->src_burst_sz);
	dstbytewidth = BYTE_WIDTH(ch_handle->dst_burst_sz);

	loop_counter = ch_handle->trans_size /
		       (srcbytewidth * (ch_handle->src_burst_len + 1));

	residue = ch_handle->trans_size - loop_counter *
		  (srcbytewidth * (ch_handle->src_burst_len + 1));

	*psrc_byte_width = srcbytewidth;
	*pdst_byte_width = dstbytewidth;
	*ploop_counter = loop_counter;
	*presidue = residue;
}

static uint32_t dma_pl330_ch_ccr(struct dma_pl330_ch_internal *ch_handle)
{
	uint32_t ccr;
	int secure = ch_handle->nonsec_mode ? SRC_PRI_NONSEC_VALUE :
		     SRC_PRI_SEC_VALUE;

	ccr = ((ch_handle->dst_cache_ctrl & CC_SRCCCTRL_MASK) <<
		CC_DSTCCTRL_SHIFT) +
	       ((ch_handle->nonsec_mode) << CC_DSTNS_SHIFT) +
	       (ch_handle->dst_burst_len << CC_DSTBRSTLEN_SHIFT) +
	       (ch_handle->dst_burst_sz << CC_DSTBRSTSIZE_SHIFT) +
	       (ch_handle->dst_inc << CC_DSTINC_SHIFT) +
	       ((ch_handle->src_cache_ctrl & CC_SRCCCTRL_MASK) <<
		CC_SRCCCTRL_SHIFT) +
	       (secure << CC_SRCPRI_SHIFT) +
	       (ch_handle->src_burst_len << CC_SRCBRSTLEN_SHIFT)  +
	       (ch_handle->src_burst_sz << CC_SRCBRSTSIZE_SHIFT)  +
	       (ch_handle->src_inc << CC_SRCINC_SHIFT);

	return ccr;
}

static void dma_pl330_calc_burstsz_len(struct dma_pl330_ch_internal *ch_handle,
				       uint64_t dst, uint64_t src,
				       uint32_t size)
{
	uint32_t byte_width, burst_sz, burst_len;

	burst_sz = MAX_BURST_SIZE_LOG2;
	/* src, dst and size should be aligned to burst size in bytes */
	while ((src | dst | size) & ((BYTE_WIDTH(burst_sz)) - 1)) {
		burst_sz--;
	}

	byte_width = BYTE_WIDTH(burst_sz);

	burst_len = MAX_BURST_LEN;
	while (burst_len) {
		/* Choose burst length so that size is aligned */
		if (!(size % ((burst_len + 1) << byte_width))) {
			break;
		}

		burst_len--;
	}

	ch_handle->src_burst_len = burst_len;
	ch_handle->src_burst_sz = burst_sz;
	ch_handle->dst_burst_len = burst_len;
	ch_handle->dst_burst_sz = burst_sz;
}

#ifdef CONFIG_DMA_64BIT
static void dma_pl330_cfg_dmac_add_control(uint32_t control_reg_base,
					   uint64_t dst, uint64_t src, int ch)
{
	uint32_t src_h = src >> 32;
	uint32_t dst_h = dst >> 32;
	uint32_t dmac_higher_addr;

	dmac_higher_addr = ((dst_h & HIGHER_32_ADDR_MASK) << DST_ADDR_SHIFT) |
			   (src_h & HIGHER_32_ADDR_MASK);

	sys_write32(dmac_higher_addr,
		    control_reg_base +
		    (ch * CONTROL_OFFSET)
		   );
}
#endif

static void dma_pl330_config_channel(struct dma_pl330_ch_config *ch_cfg,
				     uint64_t dst, uint64_t src, uint32_t size)
{
	struct dma_pl330_ch_internal *ch_handle = &ch_cfg->internal;

	memset(ch_handle, 0, sizeof(*ch_handle));
	ch_handle->src_addr = src;
	ch_handle->dst_addr = dst;
	ch_handle->trans_size = size;

	if (ch_cfg->src_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		ch_handle->src_inc = 1;
	}

	if (ch_cfg->dst_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		ch_handle->dst_inc = 1;
	}
}

static inline uint32_t dma_pl330_gen_mov(uint8_t *buf,
					 enum dmamov_type type,
					 uint32_t val)
{
	sys_write8(OP_DMA_MOV, (uint32_t)buf + 0);
	sys_write8(type, (uint32_t)buf + 1);
	sys_write8(val, (uint32_t)buf + 2);
	sys_write8(val >> 8, (uint32_t)buf + 3);
	sys_write8(val >> 16, (uint32_t)buf + 4);
	sys_write8(val >> 24, (uint32_t)buf + 5);

	return SZ_CMD_DMAMOV;
}

static inline void dma_pl330_gen_op(uint8_t opcode, uint32_t addr, uint32_t val)
{
	sys_write8(opcode, addr);
	sys_write8(val, addr + 1);
}

static int dma_pl330_setup_ch(const struct device *dev,
			      struct dma_pl330_ch_internal *ch_dat,
			      int ch)
{
	uint32_t dma_exe_addr, offset = 0, ccr;
	uint32_t lp0_start, lp1_start;
	uint32_t loop_counter0 = 0, loop_counter1 = 0;
	uint32_t srcbytewidth, dstbytewidth;
	uint32_t loop_counter, residue;
	struct dma_pl330_dev_data *const dev_data = DEV_DATA(dev);
	struct dma_pl330_ch_config *channel_cfg;
	uint8_t *dma_exec_addr8;
	int secure = ch_dat->nonsec_mode ? SRC_PRI_NONSEC_VALUE :
				SRC_PRI_SEC_VALUE;

	channel_cfg = &dev_data->channels[ch];
	dma_exe_addr = channel_cfg->dma_exe_addr;
	dma_exec_addr8 = (uint8_t *)dma_exe_addr;

	offset  += dma_pl330_gen_mov(dma_exec_addr8,
				     SAR, ch_dat->src_addr);

	offset  += dma_pl330_gen_mov(dma_exec_addr8 + offset,
				     DAR, ch_dat->dst_addr);

	ccr = dma_pl330_ch_ccr(ch_dat);

	offset  += dma_pl330_gen_mov(dma_exec_addr8 + offset,
				     CCR, ccr);

	dma_pl330_get_counter(ch_dat, &srcbytewidth, &dstbytewidth,
			      &loop_counter, &residue);

	if (loop_counter >= PL330_LOOP_COUNTER0_MAX) {
		loop_counter0 = PL330_LOOP_COUNTER0_MAX - 1;
		loop_counter1 = loop_counter / PL330_LOOP_COUNTER0_MAX - 1;
		dma_pl330_gen_op(OP_DMA_LOOP_COUNT1, dma_exe_addr + offset,
				 (loop_counter1 & 0xff));
		offset = offset + 2;
		dma_pl330_gen_op(OP_DMA_LOOP, dma_exe_addr + offset,
				 (loop_counter0 & 0xff));
		offset = offset + 2;
		lp1_start = offset;
		lp0_start = offset;
		sys_write8(OP_DMA_LD, (dma_exe_addr + offset));
		sys_write8(OP_DMA_ST, (dma_exe_addr + offset + 1));
		offset = offset + 2;
		dma_pl330_gen_op(OP_DMA_LP_BK_JMP1, (dma_exe_addr + offset),
				 ((offset - lp0_start) & 0xff));
		offset = offset + 2;
		dma_pl330_gen_op(OP_DMA_LOOP, (dma_exe_addr + offset),
				 (loop_counter0 & 0xff));
		offset = offset + 2;
		loop_counter1--;
		dma_pl330_gen_op(OP_DMA_LP_BK_JMP2, (dma_exe_addr + offset),
				 ((offset - lp1_start) & 0xff));
		offset = offset + 2;
	}

	if ((loop_counter % PL330_LOOP_COUNTER0_MAX) != 0) {
		loop_counter0 = (loop_counter % PL330_LOOP_COUNTER0_MAX) - 1;
		dma_pl330_gen_op(OP_DMA_LOOP, (dma_exe_addr + offset),
				 (loop_counter0 & 0xff));
		offset = offset + 2;
		loop_counter1--;
		lp0_start = offset;
		sys_write8(OP_DMA_LD, (dma_exe_addr + offset));
		sys_write8(OP_DMA_ST, (dma_exe_addr + offset + 1));
		offset = offset + 2;
		dma_pl330_gen_op(OP_DMA_LP_BK_JMP1, (dma_exe_addr + offset),
				 ((offset - lp0_start) & 0xff));
		offset = offset + 2;
	}

	if (residue != 0) {
		ccr = ((ch_dat->nonsec_mode) << CC_DSTNS_SHIFT) +
		       (0x0 << CC_DSTBRSTLEN_SHIFT) +
		       (0x0 << CC_DSTBRSTSIZE_SHIFT) +
		       (ch_dat->dst_inc << CC_DSTINC_SHIFT) +
		       (secure << CC_SRCPRI_SHIFT) +
		       (0x0 << CC_SRCBRSTLEN_SHIFT) +
		       (0x0 << CC_SRCBRSTSIZE_SHIFT) +
		       ch_dat->src_inc;
		offset += dma_pl330_gen_mov(dma_exec_addr8 + offset,
					    CCR, ccr);
		dma_pl330_gen_op(OP_DMA_LOOP, (dma_exe_addr + offset),
				 ((residue - 1) & 0xff));
		offset = offset + 2;
		lp0_start = offset;
		sys_write8(OP_DMA_LD, (dma_exe_addr + offset));
		sys_write8(OP_DMA_ST, (dma_exe_addr + offset + 1));
		offset = offset + 2;
		dma_pl330_gen_op(OP_DMA_LP_BK_JMP1, (dma_exe_addr + offset),
				 ((offset - lp0_start) & 0xff));
		offset = offset + 2;
	}

	sys_write8(OP_DMA_END, (dma_exe_addr + offset));
	sys_write8(OP_DMA_END, (dma_exe_addr + offset + 1));
	sys_write8(OP_DMA_END, (dma_exe_addr + offset + 2));
	sys_write8(OP_DMA_END, (dma_exe_addr + offset + 3));

	return 0;
}

static int dma_pl330_start_dma_ch(const struct device *dev,
				  uint32_t reg_base, int ch, int secure)
{
	struct dma_pl330_dev_data *const dev_data = DEV_DATA(dev);
	struct dma_pl330_ch_config *channel_cfg;
	uint32_t count = 0U;
	uint32_t data;

	channel_cfg = &dev_data->channels[ch];
	do {
		data = sys_read32(reg_base + DMAC_PL330_DBGSTATUS);
		if (++count > DMA_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	} while ((data & DATA_MASK) != 0);

	sys_write32(((ch << DMA_INTSR1_SHIFT) +
		    (DMA_INTSR0 << DMA_INTSR0_SHIFT) +
		    (secure << DMA_SECURE_SHIFT) + (ch << DMA_CH_SHIFT)),
		    reg_base + DMAC_PL330_DBGINST0);

	sys_write32(channel_cfg->dma_exe_addr,
		    reg_base + DMAC_PL330_DBGINST1);

	sys_write32(0x0, reg_base + DMAC_PL330_DBGCMD);

	count = 0U;
	do {
		data = sys_read32(reg_base + DMAC_PL330_DBGCMD);
		if (++count > DMA_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	} while ((data & DATA_MASK) != 0);

	return 0;
}

static int dma_pl330_wait(uint32_t reg_base, int ch)
{
	int count = 0U;
	uint32_t cs0_reg = reg_base + DMAC_PL330_CS0;

	do {
		if (++count > DMA_TIMEOUT_US) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	} while (((sys_read32(cs0_reg + ch * 8)) & CH_STATUS_MASK) != 0);

	return 0;
}

static int dma_pl330_xfer(const struct device *dev, uint64_t dst,
			  uint64_t src, uint32_t size, uint32_t channel,
			  uint32_t *xfer_size)
{
	struct dma_pl330_dev_data *const dev_data = DEV_DATA(dev);
	const struct dma_pl330_config *const dev_cfg = DEV_CFG(dev);
	struct dma_pl330_ch_config *channel_cfg;
	struct dma_pl330_ch_internal *ch_handle;
	int ret;
	uint32_t max_size;

	channel_cfg = &dev_data->channels[channel];
	ch_handle = &channel_cfg->internal;

	dma_pl330_calc_burstsz_len(ch_handle, dst, src, size);

	max_size = GET_MAX_DMA_SIZE((1 << ch_handle->src_burst_sz),
				    ch_handle->src_burst_len);

	if (size > max_size) {
		size = max_size;
	}

	dma_pl330_config_channel(channel_cfg, dst, src, size);
#ifdef CONFIG_DMA_64BIT
	/*
	 * Pl330 supports only 4GB boundary, but boundary region can be
	 * configured.
	 * Support added for 36bit address, lower 32bit address are configured
	 * in pl330 registers and higher 4bit address are configured in
	 * LS_ICFG_DMAC_AXI_ADD_CONTROL registers.
	 * Each channel has 1 control register to configure higher 4bit address.
	 */

	dma_pl330_cfg_dmac_add_control(dev_cfg->control_reg_base,
				       dst, src, channel);
#endif
	ret = dma_pl330_setup_ch(dev, ch_handle, channel);
	if (ret) {
		LOG_ERR("Failed to setup channel for DMA PL330");
		goto err;
	}

	ret = dma_pl330_start_dma_ch(dev, dev_cfg->reg_base, channel,
				     ch_handle->nonsec_mode);
	if (ret) {
		LOG_ERR("Failed to start DMA PL330");
		goto err;
	}

	ret = dma_pl330_wait(dev_cfg->reg_base, channel);
	if (ret) {
		LOG_ERR("Failed waiting to finish DMA PL330");
		goto err;
	}

	*xfer_size = size;
err:
	return ret;
}

#if CONFIG_DMA_64BIT
static int dma_pl330_handle_boundary(const struct device *dev, uint64_t dst,
				     uint64_t src, uint32_t channel,
				     uint32_t size)
{
	uint32_t dst_low = (uint32_t)dst;
	uint32_t src_low = (uint32_t)src;
	uint32_t transfer_size;
	int ret;

	/*
	 * Pl330 has only 32bit registers and supports 4GB memory.
	 * 4GB memory window can be configured using DMAC_AXI_ADD_CONTROL
	 * registers.
	 * Divide the DMA operation in 2 parts, 1st DMA from given address
	 * to boundary (0xffffffff) and 2nd DMA on remaining size.
	 */

	if (size > (PL330_MAX_OFFSET - dst_low)) {
		transfer_size = PL330_MAX_OFFSET - dst_low;
		ret = dma_pl330_submit(dev, dst, src, channel,
				       transfer_size);
		if (ret < 0) {
			return ret;
		}

		dst += transfer_size;
		src += transfer_size;
		size -= transfer_size;
		return dma_pl330_submit(dev, dst, src, channel, size);
	}

	if (size > (PL330_MAX_OFFSET - src_low)) {
		transfer_size = PL330_MAX_OFFSET - src_low;
		ret = dma_pl330_submit(dev, dst, src, channel, transfer_size);
		if (ret < 0) {
			return ret;
		}

		src += transfer_size;
		dst += transfer_size;
		size -= transfer_size;
		return dma_pl330_submit(dev, dst, src, channel, size);
	}

	return 0;
}
#endif

static int dma_pl330_submit(const struct device *dev, uint64_t dst,
			    uint64_t src,
			    uint32_t channel, uint32_t size)
{
	int ret;
	uint32_t xfer_size;

#if CONFIG_DMA_64BIT
	/*
	 * Pl330 has only 32bit registers and supports 4GB memory.
	 * 4GB memory window can be configured using DMAC_AXI_ADD_CONTROL
	 * registers. 32bit boundary (0xffffffff) should be check.
	 * DMA on boundary condition is taken care in below function.
	 */

	if ((size > (PL330_MAX_OFFSET - (uint32_t)dst)) ||
	    (size > (PL330_MAX_OFFSET - (uint32_t)src))) {
		return dma_pl330_handle_boundary(dev, dst, src,
						 channel, size);
	}
#endif
	while (size) {
		xfer_size = 0;
		ret = dma_pl330_xfer(dev, dst, src, size,
				     channel, &xfer_size);
		if (ret) {
			return ret;
		}
		if (xfer_size > size) {
			return -EFAULT;
		}
		size -= xfer_size;
		dst += xfer_size;
		src += xfer_size;
	}

	return 0;
}

static int dma_pl330_configure(const struct device *dev, uint32_t channel,
			       struct dma_config *cfg)
{
	struct dma_pl330_dev_data *const dev_data = DEV_DATA(dev);
	struct dma_pl330_ch_config *channel_cfg;

	if (channel >= MAX_DMA_CHANNELS) {
		return -EINVAL;
	}

	channel_cfg = &dev_data->channels[channel];
	k_mutex_lock(&channel_cfg->ch_mutex, K_FOREVER);
	if (channel_cfg->channel_active) {
		k_mutex_unlock(&channel_cfg->ch_mutex);
		return -EBUSY;
	}
	channel_cfg->channel_active = 1;
	k_mutex_unlock(&channel_cfg->ch_mutex);

	if (cfg->channel_direction != MEMORY_TO_MEMORY) {
		return -ENOTSUP;
	}

	channel_cfg->direction = cfg->channel_direction;
	channel_cfg->dst_addr_adj = cfg->head_block->dest_addr_adj;

	channel_cfg->src_addr = cfg->head_block->source_address;
	channel_cfg->dst_addr = cfg->head_block->dest_address;
	channel_cfg->trans_size = cfg->head_block->block_size;

	channel_cfg->dma_callback = cfg->dma_callback;
	channel_cfg->user_data = cfg->user_data;

	if (cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT ||
	    cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		channel_cfg->src_addr_adj = cfg->head_block->source_addr_adj;
	} else {
		return -ENOTSUP;
	}

	if (cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT ||
	    cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		channel_cfg->dst_addr_adj = cfg->head_block->dest_addr_adj;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int dma_pl330_transfer_start(const struct device *dev,
				    uint32_t channel)
{
	struct dma_pl330_dev_data *const dev_data = DEV_DATA(dev);
	struct dma_pl330_ch_config *channel_cfg;
	int ret;

	if (channel >= MAX_DMA_CHANNELS) {
		return -EINVAL;
	}

	channel_cfg = &dev_data->channels[channel];
	ret = dma_pl330_submit(dev, channel_cfg->dst_addr,
			       channel_cfg->src_addr, channel,
			       channel_cfg->trans_size);

	/* Execute callback */
	if (channel_cfg->dma_callback) {
		channel_cfg->dma_callback(dev, channel_cfg->user_data,
					  channel, ret);
	}

	k_mutex_lock(&channel_cfg->ch_mutex, K_FOREVER);
	channel_cfg->channel_active = 0;
	k_mutex_unlock(&channel_cfg->ch_mutex);

	return 0;
}

static int dma_pl330_transfer_stop(const struct device *dev, uint32_t channel)
{
	if (channel >= MAX_DMA_CHANNELS) {
		return -EINVAL;
	}

	/* Nothing as of now */
	return 0;
}

static int dma_pl330_initialize(const struct device *dev)
{
	const struct dma_pl330_config *const dev_cfg = DEV_CFG(dev);
	struct dma_pl330_dev_data *const dev_data = DEV_DATA(dev);
	struct dma_pl330_ch_config *channel_cfg;

	for (int channel = 0; channel < MAX_DMA_CHANNELS; channel++) {
		channel_cfg = &dev_data->channels[channel];
		channel_cfg->dma_exe_addr = dev_cfg->mcode_base +
					(channel * MICROCODE_SIZE_MAX);
		k_mutex_init(&channel_cfg->ch_mutex);
	}

	LOG_INF("Device %s initialized", DEV_NAME(dev));
	return 0;
}

static const struct dma_driver_api pl330_driver_api = {
	.config = dma_pl330_configure,
	.start = dma_pl330_transfer_start,
	.stop = dma_pl330_transfer_stop,
};

static const struct dma_pl330_config pl330_config = {
	.reg_base = DT_INST_REG_ADDR(0),
#ifdef CONFIG_DMA_64BIT
	.control_reg_base = DT_INST_REG_ADDR_BY_NAME(0, control_regs),
#endif
	.mcode_base = DT_INST_PROP_BY_IDX(0, microcode, 0),
};

static struct dma_pl330_dev_data pl330_data;

DEVICE_DT_INST_DEFINE(0, &dma_pl330_initialize, device_pm_control_nop,
		    &pl330_data, &pl330_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pl330_driver_api);
