/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_fw_scratch_mem.h>
#include <acp70_chip_offsets.h>
#include <acp70_chip_reg.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include "../interrupt_controller/intc_acp.h"
#include "dma_amd_acp_host.h"

/* Macro to align size down to the nearest multiple of alignment */
#define ALIGN_DOWN(size, alignment) ((size) & ~((alignment) - 1))

/* used for driver binding */
#define DT_DRV_COMPAT amd_acp_host_dma
static uint32_t *host_read_ptr;
static uint32_t *host_wr_ptr;

/* Static storage for DMA status of each channel */
static struct dma_status channel_status[8];

LOG_MODULE_REGISTER(host_dma_acp, CONFIG_DMA_LOG_LEVEL);

static void dma_config_descriptor(uint32_t dscr_start_idx, uint32_t dscr_count,
				  acp_cfg_dma_descriptor_t *psrc_dscr,
				  acp_cfg_dma_descriptor_t *pdest_dscr)
{
	uint16_t dscr, count = 0;

	if (dscr_count && psrc_dscr && pdest_dscr && dscr_start_idx < MAX_NUM_DMA_DESC_DSCR) {
		for (dscr = 0; dscr < dscr_count; dscr++) {
			pdest_dscr[dscr_start_idx + dscr].src_addr = psrc_dscr[dscr].src_addr;
			pdest_dscr[dscr_start_idx + dscr].dest_addr = psrc_dscr[dscr].dest_addr;
			pdest_dscr[dscr_start_idx + dscr].trns_cnt.u32all =
				psrc_dscr[dscr].trns_cnt.u32all;
			count = pdest_dscr[dscr_start_idx + dscr].trns_cnt.u32all;
		}
	}
}

static int acp_host_dma_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	struct acp_dma_dev_data *dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	struct acp_dma_ptr_data *ptr_data;

	uint32_t dscr_cnt, dscr = 0;
	uint32_t tc;
	uint32_t *phy_off;
	uint32_t *syst_buff_size;
	uint32_t src;
	uint32_t dest;
	uint32_t buff_size = 0;
	uint16_t dscr_strt_idx = 0;
	acp_dma_cntl_0_t dma_cntl;

	dev_data->dma_config = cfg;
	volatile acp_scratch_mem_config_t *pscratch_mem_cfg =
		(volatile acp_scratch_mem_config_t *)(PU_SCRATCH_REG_BASE + SCRATCH_REG_OFFSET);

	dscr_cnt = 2;
	if (channel) {
		dscr_strt_idx = 0;
	}
	/* ACP DMA Descriptor in scratch memory */
	acp_cfg_dma_descriptor_t *dma_config_dscr;

	dma_config_dscr = (acp_cfg_dma_descriptor_t *)(pscratch_mem_cfg->acp_cfg_dma_descriptor);
	/* physical offset of system memory */
	phy_off = (uint32_t *)(pscratch_mem_cfg->phy_offset);
	/* size of system memory buffer */
	syst_buff_size = (uint32_t *)(pscratch_mem_cfg->syst_buff_size);
	chan->direction = cfg->channel_direction;
	for (dscr = 0; dscr < dscr_cnt; dscr++) {
		acp_cfg_dma_descriptor_t *desc = &dma_config_dscr[dscr_strt_idx + dscr];

		if (cfg->channel_direction == HOST_TO_MEMORY) {
			if (channel == ACP_PROBE_CHANNEL) {
				desc->src_addr = cfg->head_block->source_address +
						 ACP_SYST_MEM_WINDOW + buff_size;
			} else {
				desc->src_addr =
					(phy_off[channel] + ACP_SYST_MEM_WINDOW + buff_size);
			}
			dest = cfg->head_block->dest_address + buff_size;
			dest = (dest & ACP_DRAM_ADDRESS_MASK);
			desc->dest_addr = (dest | ACP_SRAM);
			desc->trns_cnt.u32all = 0;
			desc->trns_cnt.bits.trns_cnt = cfg->head_block->block_size / dscr_cnt;
		} else {
			if (channel != ACP_PROBE_CHANNEL) {
				desc->dest_addr =
					(phy_off[channel] + ACP_SYST_MEM_WINDOW + buff_size);
			} else {
				desc->dest_addr =
					(cfg->head_block->dest_address + ACP_SYST_MEM_WINDOW);
			}
			src = cfg->head_block->source_address;
			src = (src & ACP_DRAM_ADDRESS_MASK);
			desc->src_addr = (src | ACP_SRAM);
			desc->trns_cnt.u32all = 0;
			desc->trns_cnt.bits.trns_cnt = cfg->head_block->block_size / dscr_cnt;
		}
		desc->trns_cnt.u32all = 0;
		desc->trns_cnt.bits.trns_cnt = cfg->head_block->block_size / dscr_cnt;
		buff_size = cfg->head_block->block_size / dscr_cnt;
	}
	dma_config_dscr[dscr_strt_idx + (dscr - 1)].trns_cnt.bits.ioc = 0;
	ptr_data = &chan->ptr_data;

	/* bytes of data to be transferred for the dma */
	tc = dma_config_dscr[dscr_strt_idx].trns_cnt.bits.trns_cnt;

	/* DMA configuration for stream */
	if (!ptr_data->size) {
		acp_cfg_dma_descriptor_t *desc = &dma_config_dscr[dscr_strt_idx];

		ptr_data->phy_off = phy_off[channel];
		ptr_data->size = tc * dscr_cnt;
		ptr_data->sys_buff_size = syst_buff_size[channel];
		if (cfg->channel_direction == HOST_TO_MEMORY) {
			/* Playback */
			desc->dest_addr = (desc->dest_addr & ACP_DRAM_ADDRESS_MASK);
			ptr_data->base = desc->dest_addr | ACP_SRAM;
			ptr_data->wr_size = 0;
			ptr_data->wr_ptr = ptr_data->base;
			ptr_data->rd_size = ptr_data->size;
			/* Initialize DMA status for playback */
			channel_status[channel].pending_length = ptr_data->size;
			channel_status[channel].free = 0;
			channel_status[channel].dir = HOST_TO_MEMORY;
			channel_status[channel].busy = 0;
			channel_status[channel].read_position = ptr_data->rd_size;
			channel_status[channel].write_position = ptr_data->wr_size;
		} else {
			/* Capture */
			desc->src_addr = (desc->src_addr & ACP_DRAM_ADDRESS_MASK);
			ptr_data->base = desc->src_addr | ACP_SRAM;
			ptr_data->rd_size = 0;
			ptr_data->rd_ptr = ptr_data->base;
			if (channel == ptr_data->probe_channel) {
				ptr_data->wr_size = 0;
			} else {
				ptr_data->wr_size = ptr_data->size;
			}
			/* Initialize DMA status for capture */
			channel_status[channel].pending_length = 0;
			channel_status[channel].free = ptr_data->sys_buff_size;
			channel_status[channel].dir = MEMORY_TO_HOST;
			channel_status[channel].busy = 0;
			channel_status[channel].read_position = ptr_data->rd_size;
			channel_status[channel].write_position = ptr_data->wr_size;
		}
	}
	/* clear the dma channel control bits */
	dma_cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_0);

	dma_cntl.bits.dmachrun = 0;
	dma_cntl.bits.dmachiocen = 0;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);
	/* Program DMAChDscrStrIdx to the index
	 * number of the first descriptor to be processed.
	 */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_STRT_IDX_0, dscr_strt_idx);
	/* program DMAChDscrdscrcnt to the
	 * number of descriptors to be processed in the transfer
	 */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_0, dscr_cnt);
	/* set DMAChPrioLvl according to the priority */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_PRIO_0, 1);
	chan->state = ACP_DMA_PREPARED;

	return 0;
}

static int acp_host_dma_start(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint64_t deadline = k_cycle_get_64() + k_us_to_cyc_ceil64(ACP_DMA_START_TIMEOUT_US);
	uint32_t chan_sts, ret = 0;
	acp_dma_cntl_0_t dma_cntl;
	acp_dma_ch_sts_t dma_sts;

	/* validate channel */
	if (channel >= 10) {
		ret = -EINVAL;
		goto out;
	}
	if (chan->state != ACP_DMA_PREPARED && chan->state != ACP_DMA_SUSPENDED) {
		ret = -EINVAL;
	}
	chan->state = ACP_DMA_ACTIVE;
	/* Clear DMAChRun before starting the DMA Ch */
	dma_cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_0);

	dma_cntl.bits.dmachrun = 0;
	dma_cntl.bits.dmachiocen = 0;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);

	dma_cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_0);

	dma_cntl.bits.dmachrun = 1;
	dma_cntl.bits.dmachiocen = 0;

	/* set dmachrun bit to start the transfer */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);

	/* poll for the status bit
	 * to finish the dma transfer
	 * then initiate call back function
	 */
	do {
		dma_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
		chan_sts = dma_sts.u32all & (1 << channel);
		if (!chan_sts) {
			return 0;
		}
	} while (k_cycle_get_64() <= deadline);
out:
	return ret;
}

static bool acp_host_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t req_chan;

	if (!filter_param) {
		return true;
	}

	req_chan = *(uint32_t *)filter_param;

	if (channel == req_chan) {
		struct acp_dma_dev_data *const dev_data = dev->data;
		struct acp_dma_chan_data chan = dev_data->chan_data[channel];
		struct acp_dma_ptr_data *ptr_data = &chan.ptr_data;

		ptr_data->rd_size = 0;
		ptr_data->wr_size = 0;
		ptr_data->size = 0;
		return true;
	}
	return false;
}

static void acp_host_dma_chan_release(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan_data = &dev_data->chan_data[channel];

	chan_data->state = ACP_DMA_READY;
	/* reset read and write pointer */
	chan_data->ptr_data.rd_size = 0;
	chan_data->ptr_data.wr_size = 0;
	chan_data->ptr_data.size = 0;
}

/* Stop the requested channel */
static int acp_host_dma_stop(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint32_t dmach_mask;
	uint32_t delay_cnt = 10000;
	acp_dma_cntl_0_t dma_cntl;
	acp_dma_ch_sts_t ch_sts;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		return 0;
	}
	switch (chan->state) {
	case ACP_DMA_READY:
		/* fallthrough */
	case ACP_DMA_PREPARED:
		return 0;
	case ACP_DMA_SUSPENDED:
		/* fallthrough */
	case ACP_DMA_ACTIVE: /*breaks and continues the stop operation*/
		break;
	default:
		return -EINVAL;
	}
	chan->state = ACP_DMA_READY;
	dmach_mask = (1 << channel);
	dma_cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_0);

	/* Do the HW stop of the DMA */
	/* set DMAChRst bit to stop the transfer */
	dma_cntl.bits.dmachrun = 0;
	dma_cntl.bits.dmachiocen = 0;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);
	ch_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
	if (ch_sts.bits.dmachrunsts & dmach_mask) {
		/* set reset bit to stop the dma transfer */
		dma_cntl.bits.dmachrst = 1;
		io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);
	}
	while (delay_cnt > 0) {
		ch_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
		if (!(ch_sts.bits.dmachrunsts & dmach_mask)) {
			/* clear reset flag after stopping dma
			 * and break from the loop
			 */
			dma_cntl.bits.dmachrst = 0;
			io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);
			break;
		}
		delay_cnt--;
	}
	return 0;
}

static int acp_host_dma_resume(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan_data = &dev_data->chan_data[channel];
	int ret = 0;

	/* Validate channel index */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		ret = -EINVAL;
		goto out;
	}
	/* Validate channel state */
	if (chan_data->state != ACP_DMA_SUSPENDED) {
		ret = -EINVAL;
		goto out;
	}
	/* Channel is now active */
	chan_data->state = ACP_DMA_ACTIVE;
out:
	return ret;
}

static int acp_host_dma_suspend(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan_data = &dev_data->chan_data[channel];
	int ret = 0;

	/* Validate channel index */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		ret = -EINVAL;
		goto out;
	}
	/* Validate channel state */
	if (chan_data->state != ACP_DMA_ACTIVE) {
		ret = -EINVAL;
		goto out;
	}
	/* Channel is now active */
	chan_data->state = ACP_DMA_SUSPENDED;
out:
	return ret;
}

static int acp_host_dma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	struct acp_dma_dev_data *const dev_data = dev->data;

	if (!stat || channel >= dev_data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	/* Return the status initialized in config function */
	*stat = channel_status[channel];
	stat->busy = (dev_data->chan_data[channel].state == ACP_DMA_ACTIVE);

	return 0;
}

static int acp_host_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_COPY_ALIGNMENT:
		/* fallthrough */
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = CONFIG_DMA_AMD_ACP_BUFFER_ALIGN;
		break;
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = CONFIG_DMA_AMD_ACP_BUFFER_ADDRESS_ALIGN;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 2;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int acp_host_dma_reload(const struct device *dev, uint32_t channel, uint32_t src,
			       uint32_t dst, size_t bytes)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	struct acp_dma_ptr_data *ptr_data = &chan->ptr_data;

	uint32_t strt_idx = 0;
	uint32_t tail = 0, head = 0, ret = 0;
	uint32_t src1 = 0;
	uint32_t dest1 = 0;
	uint32_t host_rdptr, host_wrptr, pb_avail, pb_free, host_buff_size;
	uint32_t cap_host_rdptr, cap_host_wrptr, cap_avail, cap_free, cap_size;
	acp_cfg_dma_descriptor_t psrc_dscr[ACP_MAX_STREAMS];
	acp_cfg_dma_descriptor_t *pdest_dscr;
	acp_dma_cntl_0_t dma_cntl;

	/* Validate channel index */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		ret = -EINVAL;
	}
	volatile acp_scratch_mem_config_t *pscratch_mem_cfg =
		(volatile acp_scratch_mem_config_t *)(PU_SCRATCH_REG_BASE + SCRATCH_REG_OFFSET);

	pdest_dscr = (acp_cfg_dma_descriptor_t *)(pscratch_mem_cfg->acp_cfg_dma_descriptor);

	if (chan->direction == HOST_TO_MEMORY) {
		host_buff_size = (uint32_t)((uint32_t)(ptr_data->base + ptr_data->size) &
					    ACP_SYST_MEM_ADDR_MASK);
		host_rdptr = (uint32_t)((uint32_t)host_read_ptr & ACP_SYST_MEM_ADDR_MASK);
		host_wrptr = (uint32_t)((uint32_t)ptr_data->wr_ptr & ACP_SYST_MEM_ADDR_MASK);
		if (host_rdptr != host_wrptr) {
			pb_free = (int32_t)(host_rdptr - host_wrptr) > 0
					  ? (host_rdptr - host_wrptr)
					  : (host_buff_size - host_wrptr) +
						    (host_rdptr -
						     (ptr_data->base & ACP_SYST_MEM_ADDR_MASK));
			pb_avail = ptr_data->size - pb_free;
		} else {
			pb_free = ptr_data->size;
		}
		bytes = (size_t)ALIGN_DOWN(pb_free, 64);
		bytes = MIN(bytes, ptr_data->size >> 1);
		head = bytes;
		/* Update the read and write pointers */
		ptr_data->rd_ptr = ACP_SYST_MEM_WINDOW + ptr_data->phy_off + ptr_data->rd_size;
		ptr_data->wr_ptr = ptr_data->base + ptr_data->wr_size;
		psrc_dscr[strt_idx].src_addr = ptr_data->rd_ptr;
		ptr_data->wr_ptr = (ptr_data->wr_ptr & ACP_DRAM_ADDRESS_MASK);
		/* Known Data hack */
		psrc_dscr[strt_idx].dest_addr = (ptr_data->wr_ptr | ACP_SRAM);
		psrc_dscr[strt_idx].trns_cnt.bits.trns_cnt = bytes;
		/* Configure a single descrption */
		dma_config_descriptor(strt_idx, 1, psrc_dscr, pdest_dscr);
		io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_0, 1);
		if (ptr_data->rd_size + bytes > ptr_data->sys_buff_size) {
			/* Configure the descriptor  for head and tail */
			/* values for the wrap around case */
			tail = ptr_data->sys_buff_size - ptr_data->rd_size;
			head = bytes - tail;
			psrc_dscr[strt_idx].trns_cnt.bits.trns_cnt = tail;
			psrc_dscr[strt_idx + 1].src_addr = ACP_SYST_MEM_WINDOW + ptr_data->phy_off;
			dest1 = ptr_data->wr_ptr + tail;
			dest1 = (dest1 & ACP_DRAM_ADDRESS_MASK);
			psrc_dscr[strt_idx + 1].dest_addr = (dest1 | ACP_SRAM);
			psrc_dscr[strt_idx + 1].trns_cnt.bits.trns_cnt = head;
			dma_config_descriptor(strt_idx, 2, psrc_dscr, pdest_dscr);
			io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_0, 2);
			ptr_data->rd_size = 0;
		}
		/* Check for wrap-around case for host buffer */
		if (ptr_data->wr_size + bytes > ptr_data->size) {
			tail = ptr_data->size - ptr_data->wr_size;
			head = bytes - tail;

			psrc_dscr[strt_idx].trns_cnt.bits.trns_cnt = tail;
			psrc_dscr[strt_idx + 1].trns_cnt.bits.trns_cnt = head;

			psrc_dscr[strt_idx + 1].dest_addr =
				((ptr_data->base & ACP_DRAM_ADDRESS_MASK) | ACP_SRAM);

			src1 = ptr_data->rd_ptr + tail;
			psrc_dscr[strt_idx + 1].src_addr = src1;
			dma_config_descriptor(strt_idx, 2, psrc_dscr, pdest_dscr);
			io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_0, 2);
			ptr_data->wr_size = 0;
		}
		ptr_data->rd_size += head;
		ptr_data->rd_size %= ptr_data->sys_buff_size;
		ptr_data->wr_size += bytes;
		ptr_data->wr_size %= ptr_data->size;
		ptr_data->wr_ptr = ptr_data->base + ptr_data->wr_size;
	} else if (chan->direction == MEMORY_TO_HOST) {
		cap_size = (uint32_t)((uint32_t)(ptr_data->base + ptr_data->size) &
				      ACP_SYST_MEM_ADDR_MASK);
		cap_host_rdptr = (uint32_t)((uint32_t)ptr_data->rd_ptr & ACP_SYST_MEM_ADDR_MASK);
		cap_host_wrptr = (uint32_t)((uint32_t)host_wr_ptr & ACP_SYST_MEM_ADDR_MASK);
		if (cap_host_rdptr != cap_host_wrptr) {
			cap_avail = (int32_t)(cap_host_wrptr - cap_host_rdptr) > 0
					    ? (cap_host_wrptr - cap_host_rdptr)
					    : (cap_size - cap_host_rdptr) +
						      (cap_host_wrptr -
						       (ptr_data->base & ACP_SYST_MEM_ADDR_MASK));
			cap_free = ptr_data->size - cap_avail;
		} else {
			cap_avail = ptr_data->size;
		}
		bytes = (size_t)ALIGN_DOWN(cap_avail, 64);
		bytes = MIN(bytes, ptr_data->size >> 1);
		head = bytes;
		ptr_data->wr_ptr = ACP_SYST_MEM_WINDOW + ptr_data->phy_off + ptr_data->wr_size;
		ptr_data->rd_ptr = ptr_data->base + ptr_data->rd_size;

		ptr_data->rd_ptr = (ptr_data->rd_ptr & ACP_DRAM_ADDRESS_MASK);
		psrc_dscr[strt_idx].src_addr = (ptr_data->rd_ptr | ACP_SRAM);
		psrc_dscr[strt_idx].dest_addr = ptr_data->wr_ptr;
		psrc_dscr[strt_idx].trns_cnt.bits.trns_cnt = bytes;
		/* Configure a single descrption */
		dma_config_descriptor(strt_idx, 1, psrc_dscr, pdest_dscr);
		io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_0, 1);
		/* Check for wrap-around case for system  buffer */
		if (ptr_data->wr_size + bytes > ptr_data->sys_buff_size) {
			/* Configure the descriptor  for head and
			 * tail values for the wrap around case
			 */
			tail = ptr_data->sys_buff_size - ptr_data->wr_size;
			head = bytes - tail;
			psrc_dscr[strt_idx].trns_cnt.bits.trns_cnt = tail;
			src1 = ptr_data->rd_ptr + tail;
			psrc_dscr[strt_idx + 1].dest_addr = ACP_SYST_MEM_WINDOW + ptr_data->phy_off;
			psrc_dscr[strt_idx + 1].trns_cnt.bits.trns_cnt = head;
			src1 = (src1 & ACP_DRAM_ADDRESS_MASK);
			psrc_dscr[strt_idx + 1].src_addr = (src1 | ACP_SRAM);
			dma_config_descriptor(strt_idx, 2, psrc_dscr, pdest_dscr);
			io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_0, 2);
			ptr_data->wr_size = 0;
		}
		ptr_data->wr_size += head;
		ptr_data->wr_size %= ptr_data->sys_buff_size;
		ptr_data->rd_size += bytes;
		ptr_data->rd_size %= ptr_data->size;
		ptr_data->rd_ptr = ptr_data->base + ptr_data->rd_size;
	}
	/* clear the dma channel control bits */
	dma_cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_0);

	dma_cntl.bits.dmachrun = 0;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_0, dma_cntl.u32all);
	/* Load start index of decriptor and priority */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_STRT_IDX_0, strt_idx);
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_PRIO_0, 1);
	chan->state = ACP_DMA_PREPARED;
	acp_dma_ch_sts_t ch_sts;
	uint32_t dmach_mask = (1 << channel);

	ch_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
	while (ch_sts.bits.dmachrunsts & dmach_mask) {
		ch_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
	}

	acp_host_dma_start(dev, channel);
	ch_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
	while (ch_sts.bits.dmachrunsts & dmach_mask) {
		ch_sts = (acp_dma_ch_sts_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS);
	}
	acp_host_dma_stop(dev, channel);

	return 0;
}

static int acp_host_dma_init(const struct device *dev)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	uint32_t descr_base;

	volatile acp_scratch_mem_config_t *pscratch_mem_cfg =
		(volatile acp_scratch_mem_config_t *)(PU_SCRATCH_REG_BASE + SCRATCH_REG_OFFSET);
	descr_base = (uint32_t)(&pscratch_mem_cfg->acp_cfg_dma_descriptor);
	descr_base = (descr_base - ACP_DSP_DRAM_BASE);
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DESC_BASE_ADDR, descr_base);
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DESC_MAX_NUM_DSCR, 0x2);
	/* Setup context and atomics for channels */
	dev_data->dma_ctx.magic = DMA_MAGIC;
	dev_data->dma_ctx.dma_channels = CONFIG_DMA_AMD_ACP_HOST_CHANNEL_COUNT;
	dev_data->dma_ctx.atomic = dev_data->atomic;

	return 0;
}

static DEVICE_API(dma, acp_host_dma_api) = {
	.config = acp_host_dma_config,
	.start = acp_host_dma_start,
	.stop = acp_host_dma_stop,
	.chan_filter = acp_host_dma_chan_filter,
	.chan_release = acp_host_dma_chan_release,
	.reload = acp_host_dma_reload,
	.suspend = acp_host_dma_suspend,
	.resume = acp_host_dma_resume,
	.get_status = acp_host_dma_get_status,
	.get_attribute = acp_host_dma_get_attribute,
};

static struct acp_dma_dev_data acp_dma_dev_data = {
	.dma_ctx.magic = DMA_MAGIC,
	.dma_ctx.dma_channels = CONFIG_DMA_AMD_ACP_HOST_CHANNEL_COUNT,
};

DEVICE_DT_DEFINE(DT_NODELABEL(acp_host_dma), &acp_host_dma_init, NULL, &acp_dma_dev_data, NULL,
		 POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, &acp_host_dma_api);
