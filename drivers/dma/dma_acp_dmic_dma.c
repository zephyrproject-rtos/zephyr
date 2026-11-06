/*
 * Copyright (c) 2024 AMD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#if defined(CONFIG_SOC_ACP_7_X)
#include <acp7x_fw_scratch_mem.h>
#include <acp7x_chip_offsets.h>
#include <acp7x_chip_reg.h>
#endif

/* used for driver binding */
#define DT_DRV_COMPAT amd_acp_dmic_dma

LOG_MODULE_REGISTER(acp_dmic_dma_common, CONFIG_DMA_LOG_LEVEL);

static uint32_t dmic_rngbuff_size;
struct acp_dmic_silence acp_initsilence;
#define DMIC_INSTANCES 2
struct dmic_context *dmic_ctx[DMIC_INSTANCES] = {0};

int acp_dma_dmic_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	LOG_DBG("config: ch=%u dir=%u", channel, cfg->channel_direction);
	struct acp7x_dma_dev_data *dev_data = dev->data;
	uint32_t ring_buff_addr;
	uint32_t timeperiod_ms;
	acp_wov_rx_ringbufsize_t dmic_ringbuff_size;
	acp_wov_rx_intr_watermark_size_t watermark;
	acp_pdm2_dma_enable_t pdm2_dma_enable;
	struct dmic_context *dmic_data_ctx;

	dev_data->chan_data[channel].direction = cfg->channel_direction;
	dmic_data_ctx = (struct dmic_context *)dev_data->dai_index_ptr;
	LOG_DBG("config: dmic data_ptr=%p", dmic_data_ctx);
	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if ((dmic_ctx[i]->dma_channel == 0xFFFF) && (i == dmic_data_ctx->index)) {
			dmic_ctx[i]->dma_channel = channel;
			dmic_ctx[i]->pin_dir = cfg->channel_direction;
			dmic_ctx[i]->index = dmic_data_ctx->index;
			break;
		}
	}

	uint32_t index = 0xFFFF;

	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if (dmic_ctx[i]->dma_channel == channel &&
		    dmic_ctx[i]->pin_dir == cfg->channel_direction) {
			index = dmic_ctx[i]->index;
			dmic_ctx[i]->prev_pos = 0;
			break;
		}
	}
	LOG_DBG("configure DMIC%d", index);
	acp_initsilence.bytes_per_sample = 4;
	switch (cfg->channel_direction) {
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		acp_initsilence.dmic_rngbuff_addr1 = (char *)cfg->head_block[0].dest_address;
		cfg->head_block[0].dest_address =
			(cfg->head_block[0].dest_address & ACP_DRAM_ADDRESS_MASK);
		ring_buff_addr = (cfg->head_block[0].dest_address | ACP_SRAM);
		if (index == 0) { /* PDM0 */
			LOG_DBG("config: PDM0 ring_addr=0x%x", ring_buff_addr);
			/* Load Ring buffer address */
			io_reg_write((PU_REGISTER_BASE + ACP_PDM_RX_RINGBUFADDR), ring_buff_addr);
			/* Load Ring buffer Size */
			dmic_rngbuff_size = (cfg->head_block[0].block_size * cfg->block_count);
			dmic_ringbuff_size.bits.rx_ringbufsize = dmic_rngbuff_size;
			io_reg_write((PU_REGISTER_BASE + ACP_PDM_RX_RINGBUFSIZE),
				     dmic_ringbuff_size.u32all);
			LOG_DBG("config: PDM0 ringbuf_sz=%u wmark=%u", dmic_rngbuff_size,
				dmic_rngbuff_size / 2);
			/* Write the ring buffer size to register */
			watermark.bits.rx_intr_watermark_size = (dmic_rngbuff_size / 2);
			io_reg_write((PU_REGISTER_BASE + ACP_PDM_RX_INTR_WATERMARK_SIZE),
				     watermark.u32all);
		} else if (index == 1) { /* PDM1 */
			LOG_DBG("config: PDM1 ring_addr=0x%x", ring_buff_addr);
			pdm2_dma_enable = (acp_pdm2_dma_enable_t)io_reg_read(PU_REGISTER_BASE +
									     ACP_PDM_2_DMA_Enable);
			if (pdm2_dma_enable.bits.pdm_dma_en_96k == 1) {
				/* Load Ring buffer address */
				io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_RX_RINGBUFADDR_96K,
					     ring_buff_addr);
				/* Load Ring buffer Size */
				dmic_rngbuff_size =
					(cfg->head_block[0].block_size * cfg->block_count);
				dmic_ringbuff_size.bits.rx_ringbufsize = dmic_rngbuff_size;
				io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_RX_RINGBUFSIZE_96K,
					     dmic_ringbuff_size.u32all);
				LOG_DBG("config: PDM1_96K ringbuf_sz=%u wmark=%u",
					dmic_rngbuff_size, dmic_rngbuff_size >> 1);
				/* Write the ring buffer size to register */
				watermark.bits.rx_intr_watermark_size = (dmic_rngbuff_size >> 1);
				io_reg_write(PU_REGISTER_BASE +
						     ACP_PDM_2_RX_INTR_WATERMARK_SIZE_96K,
					     watermark.u32all);
			} else if (pdm2_dma_enable.bits.pdm_dma_en_48k == 1) {
				/* Load Ring buffer address */
				io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_RX_RINGBUFADDR_48K,
					     ring_buff_addr);
				/* Load Ring buffer Size */
				dmic_rngbuff_size =
					(cfg->head_block[0].block_size * cfg->block_count);
				dmic_ringbuff_size.bits.rx_ringbufsize = dmic_rngbuff_size;
				io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_RX_RINGBUFSIZE_48K,
					     dmic_ringbuff_size.u32all);
				LOG_DBG("config: PDM1_48K ringbuf_sz=%u wmark=%u",
					dmic_rngbuff_size, dmic_rngbuff_size >> 1);
				/* Write the ring buffer size to register */
				watermark.bits.rx_intr_watermark_size = (dmic_rngbuff_size >> 1);
				io_reg_write(PU_REGISTER_BASE +
						     ACP_PDM_2_RX_INTR_WATERMARK_SIZE_48K,
					     watermark.u32all);
			} else if (pdm2_dma_enable.bits.pdm_dma_en_16k == 1) {
				LOG_INF("config: PDM1 16k ring=0x%x dma_en=0x%x",
					ring_buff_addr, pdm2_dma_enable.u32all);
				/* Load Ring buffer address */
				io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_RX_RINGBUFADDR_16K,
					     ring_buff_addr);
				/* Load Ring buffer Size */
				dmic_rngbuff_size =
					(cfg->head_block[0].block_size * cfg->block_count);
				dmic_ringbuff_size.bits.rx_ringbufsize = dmic_rngbuff_size;
				io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_RX_RINGBUFSIZE_16K,
					     dmic_ringbuff_size.u32all);
				LOG_DBG("config: PDM1_16K ringbuf_sz=%u wmark=%u",
					dmic_rngbuff_size, dmic_rngbuff_size >> 1);
				/* Write the ring buffer size to register */
				watermark.bits.rx_intr_watermark_size = (dmic_rngbuff_size >> 1);
				io_reg_write(PU_REGISTER_BASE +
						     ACP_PDM_2_RX_INTR_WATERMARK_SIZE_16K,
					     watermark.u32all);
			} else {
				return -EINVAL;
			}
		}
		timeperiod_ms = dmic_rngbuff_size /
				(acp_initsilence.num_chs * acp_initsilence.samplerate_khz *
				 acp_initsilence.bytes_per_sample * 2);
		acp_initsilence.silence_cnt = DMIC_SETTLING_TIME_MS / timeperiod_ms;
		acp_initsilence.numfilterbuffers = DMIC_SMOOTH_TIME_MS / timeperiod_ms;
		LOG_DBG("config: silence_cnt=%u numfilterbuf=%u", acp_initsilence.silence_cnt,
			acp_initsilence.numfilterbuffers);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int acp_dmic_dma_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			size_t bytes)
{
	LOG_DBG("reload: ch=%u bytes=%zu silence_incr=%u", channel, bytes,
		acp_initsilence.silence_incr);
	uint32_t i;
	uint32_t j;
	int numsamples;
	char *dmic_rngbuff_addr2 = acp_initsilence.dmic_rngbuff_addr1;

	if (acp_initsilence.silence_incr < acp_initsilence.silence_cnt) {
		if (acp_initsilence.silence_incr & 1) {
			dmic_rngbuff_addr2 =
				acp_initsilence.dmic_rngbuff_addr1 + (dmic_rngbuff_size / 2);
		}
		LOG_DBG("reload: applying silence fill");
		for (i = 0; i < (dmic_rngbuff_size / 2); i++) {
			dmic_rngbuff_addr2[i] = 0;
		}
		acp_initsilence.silence_incr++;
	} else if (acp_initsilence.silence_incr <
		   acp_initsilence.silence_cnt + acp_initsilence.numfilterbuffers) {
		LOG_DBG("reload: smooth ramp coeff=%u", acp_initsilence.coeff);
		numsamples = (dmic_rngbuff_size / 2) /
			     (acp_initsilence.num_chs * acp_initsilence.bytes_per_sample);
		if (acp_initsilence.silence_incr & 1) {
			dmic_rngbuff_addr2 =
				acp_initsilence.dmic_rngbuff_addr1 + (dmic_rngbuff_size / 2);
		}
		acp_initsilence.dmic_rngbuff_iaddr = (int *)dmic_rngbuff_addr2;
		for (i = 0; i < numsamples * acp_initsilence.num_chs;
		     i += acp_initsilence.num_chs, acp_initsilence.coeff++) {
			for (j = 0; j < acp_initsilence.num_chs; j++) {
				acp_initsilence.dmic_rngbuff_iaddr[i + j] =
					(acp_initsilence.dmic_rngbuff_iaddr[i + j] /
					 (numsamples * acp_initsilence.numfilterbuffers)) *
					acp_initsilence.coeff;
			}
		}
		acp_initsilence.silence_incr++;
	}
	return 0;
}

int acp_dma_dmic_start(const struct device *dev, uint32_t channel)
{
	LOG_DBG("start: ch=%u", channel);
	struct acp7x_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	acp_wov_pdm_dma_enable_t pdm_dma_enable;
	acp_pdm2_dma_enable_t pdm2_dma_enable;
	uint32_t index = 0xFFFF;

	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if (dmic_ctx[i]->dma_channel == channel &&
		    dmic_ctx[i]->pin_dir == chan->direction) {
			index = dmic_ctx[i]->index;
			dmic_ctx[i]->prev_pos = 0;
			break;
		}
	}
	uint64_t deadline = k_cycle_get_64() + k_us_to_cyc_ceil64(500);

	acp_initsilence.silence_incr = 0;
	acp_initsilence.coeff = 0;

/* ACP 7.X firmware sets up the audio clocks once at boot in platform_init. */
	if (chan->direction == PERIPHERAL_TO_MEMORY) {
		chan->state = ACP_DMA_ACTIVE;
		if (index == 0) {
			LOG_DBG("start: PDM0 starting");
			/* PDM Enable */
			io_reg_write((PU_REGISTER_BASE + ACP_PDM_ENABLE), 1);
			/* PDM DMA Enable */
			io_reg_write((PU_REGISTER_BASE + ACP_PDM_DMA_ENABLE), 1);
			/* Check the PDM DMA Enable status bit */
			pdm_dma_enable = (acp_wov_pdm_dma_enable_t)io_reg_read(
				PU_REGISTER_BASE + ACP_PDM_DMA_ENABLE);
			while (!(uint32_t)pdm_dma_enable.bits.pdm_dma_en_status) {
				if (deadline < k_cycle_get_64()) {
					if ((uint32_t)pdm_dma_enable.bits.pdm_dma_en_status) {
						return 0;
					}
					return -ETIME;
				}
				pdm_dma_enable = (acp_wov_pdm_dma_enable_t)io_reg_read(
					PU_REGISTER_BASE + ACP_PDM_DMA_ENABLE);
			}
			LOG_INF("start: PDM0 started");
		} else if (index == 1) { /* pdm 2 */
			LOG_INF("start: PDM1 dma_en=0x%x",
				io_reg_read(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable));
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_ENABLE, 1);
			/* PDM DMA Enable */
			/* Check the PDM DMA Enable status bit */
			pdm2_dma_enable = (acp_pdm2_dma_enable_t)io_reg_read(PU_REGISTER_BASE +
									     ACP_PDM_2_DMA_Enable);
			while (!(((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_96k) ||
				 ((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_48k) ||
				 ((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_16k))) {
				if (deadline < k_cycle_get_64()) {
					/* safe check in case we've got preempted after read */
					if ((((uint32_t)
						      pdm2_dma_enable.bits.pdm_dma_en_status_96k) ||
					     ((uint32_t)
						      pdm2_dma_enable.bits.pdm_dma_en_status_48k) ||
					     ((uint32_t)pdm2_dma_enable.bits
						      .pdm_dma_en_status_16k))) {
						return 0;
					}
					return -ETIME;
				}
				pdm2_dma_enable = (acp_pdm2_dma_enable_t)io_reg_read(
					PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable);
			}
		}
	}
	return 0;
}

/* Stop the requested channel */
static int acp_dma_dmic_stop(const struct device *dev, uint32_t channel)
{
	LOG_DBG("stop: ch=%u", channel);
	acp_wov_pdm_dma_enable_t pdm_dma_enable;
	acp_pdm2_dma_enable_t pdm2_dma_enable;
	uint64_t deadline = k_cycle_get_64() + k_us_to_cyc_ceil64(500);
	struct acp7x_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint32_t index = 0xFFFF;

	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if (dmic_ctx[i]->dma_channel == channel &&
		    dmic_ctx[i]->pin_dir == chan->direction) {
			index = dmic_ctx[i]->index;
			break;
		}
	}
	LOG_DBG("stop: idx=%u state=%u", index, chan->state);
	switch (chan->state) {
	case ACP_DMA_READY:
	case ACP_DMA_PREPARED:
		return 0; /* do not try to stop multiple times */
	case ACP_DMA_SUSPENDED:
	case ACP_DMA_ACTIVE:
		break;
	default:
		return -EINVAL;
	}
	chan->state = ACP_DMA_READY;
	LOG_DBG("start: idx=%d", index);
	if (index == 0) { /* pdm 1 */
		LOG_DBG("stop: PDM0 stopping");
		/* Disable PDM DMA */
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_DMA_ENABLE, 0);
		/* Disable PDM */
		pdm_dma_enable = (acp_wov_pdm_dma_enable_t)io_reg_read(PU_REGISTER_BASE +
								       ACP_PDM_DMA_ENABLE);
		/* check PDM DMA status*/
		while ((uint32_t)pdm_dma_enable.bits.pdm_dma_en_status) {

			if (deadline < k_cycle_get_64()) {
				/* safe check in case we've got preempted after read */
				if ((uint32_t)pdm_dma_enable.bits.pdm_dma_en_status) {
					return 0;
				}
				return -ETIME;
			}
			pdm_dma_enable = (acp_wov_pdm_dma_enable_t)io_reg_read(
				PU_REGISTER_BASE + ACP_PDM_DMA_ENABLE);
		}
		/* Disable  PDM */
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_ENABLE, 0);
		/* Clear PDM FIFO */
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_FIFO_FLUSH, 1);
		LOG_INF("stop: PDM0 stopped");
	} else if (index == 1) { /* PDM1 */
		LOG_DBG("stop: PDM1 stopping");
		pdm2_dma_enable =
			(acp_pdm2_dma_enable_t)io_reg_read(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable);
		uint32_t flush_var = 0;

		if (pdm2_dma_enable.bits.pdm_dma_en_96k == 1) {
			pdm2_dma_enable.bits.pdm_dma_en_96k = 0;
			flush_var = 1;
		}
		if (pdm2_dma_enable.bits.pdm_dma_en_48k == 1) {
			pdm2_dma_enable.bits.pdm_dma_en_48k = 0;
			flush_var = 2;
		}
		if (pdm2_dma_enable.bits.pdm_dma_en_16k == 1) {
			pdm2_dma_enable.bits.pdm_dma_en_16k = 0;
			if (flush_var == 0) {
				flush_var = 3;
			}
		}
		/* Disable PDM DMA */
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable, pdm2_dma_enable.u32all);
		pdm2_dma_enable =
			(acp_pdm2_dma_enable_t)io_reg_read(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable);
		/* Check PDM DMA Status */
		while (((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_96k) ||
		       ((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_48k) ||
		       ((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_16k)) {
			if (deadline < k_cycle_get_64()) {
				/* safe check in case we've got preempted after read */
				if (((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_96k) ||
				    ((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_48k) ||
				    ((uint32_t)pdm2_dma_enable.bits.pdm_dma_en_status_16k)) {
					return 0;
				}
				return -ETIME;
			}
			pdm2_dma_enable = (acp_pdm2_dma_enable_t)io_reg_read(PU_REGISTER_BASE +
									     ACP_PDM_2_DMA_Enable);
		}
		/* Disable  PDM */
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_ENABLE, 0);
		/* Clear PDM FIFO */
		switch (flush_var) {
		case 1:
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_FIFO_FLUSH_96K, 1);
			break;
		case 2:
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_FIFO_FLUSH_48K, 1);
			break;
		case 3:
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_FIFO_FLUSH_16K, 1);
			break;
		default:
			return -EINVAL;
		}
		LOG_INF("stop: PDM1 stopped");
	}
	/* ACP 7.X keeps the boot clock setup across the last stop as well. */
	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if (dmic_ctx[i]->index == index) {
			dmic_ctx[i]->index = 0xFFFF;
			dmic_ctx[i]->pin_dir = 0xFFFF;
			dmic_ctx[i]->dma_channel = 0xFFFF;
			dmic_ctx[i]->prev_pos = 0;
			break;
		}
	}
	return 0;
}

static bool acp_dmic_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t req_chan;

	if (!filter_param) {
		return true;
	}
	req_chan = *(uint32_t *)filter_param;
	LOG_DBG("filter: req=%d ch=%d", req_chan, channel);

	if (channel == req_chan) {
		return true;
	}
	return false;
}

static void acp_dmic_dma_chan_release(const struct device *dev, uint32_t channel)
{
}

static int acp_dmic_dma_resume(const struct device *dev, uint32_t channel)
{
	return 0;
}

static int acp_dmic_dma_suspend(const struct device *dev, uint32_t channel)
{
	return 0;
}

static int acp_dmic_dma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	struct acp7x_dma_dev_data *const dev_data = dev->data;
	acp_pdm2_dma_enable_t pdm2_dma_enable;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];

	if (channel >= dev_data->dma_ctx.dma_channels) {
		return -EINVAL;
	}
	uint32_t index = 0xFFFF;
	struct dmic_context *ctx = NULL;
	uint32_t rx_low = 0;
	uint32_t rx_high = 0;
	uint64_t dmic_curr_rx_pos = 0;

	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if (dmic_ctx[i]->dma_channel == channel &&
		    dmic_ctx[i]->pin_dir == chan->direction) {
			index = dmic_ctx[i]->index;
			ctx = dmic_ctx[i];
			break;
		}
	}
	if (!ctx) {
		return -EINVAL;
	}
	LOG_DBG("get_status: idx=%d", index);
	if (chan->direction == PERIPHERAL_TO_MEMORY) {
		if (index == 0) { /* pdm 2 */
			rx_low = (uint32_t)io_reg_read(PU_REGISTER_BASE +
						       ACP_PDM_RX_LINEARPOSITIONCNTR_LOW);
			rx_high = (uint32_t)io_reg_read(PU_REGISTER_BASE +
							ACP_PDM_RX_LINEARPOSITIONCNTR_HIGH);
			dmic_curr_rx_pos = (uint64_t)(((uint64_t)rx_high << 32) | rx_low);
			LOG_DBG("get_status: curr_pos=%llu", dmic_curr_rx_pos);
			LOG_DBG("get_status: rx_low=%u rx_high=%u", rx_low, rx_high);

		} else if (index == 1) {
			pdm2_dma_enable = (acp_pdm2_dma_enable_t)io_reg_read(PU_REGISTER_BASE +
									     ACP_PDM_2_DMA_Enable);
			if (pdm2_dma_enable.bits.pdm_dma_en_48k == 1) {
				rx_low = (uint32_t)io_reg_read(
					PU_REGISTER_BASE + ACP_PDM_2_RX_LINEARPOSITIONCNTR_LOW_48K);
				rx_high = (uint32_t)io_reg_read(
					PU_REGISTER_BASE +
					ACP_PDM_2_RX_LINEARPOSITIONCNTR_HIGH_48K);
				dmic_curr_rx_pos = (uint64_t)(((uint64_t)rx_high << 32) | rx_low);
				LOG_DBG("get_status: DMIC 48k");
			} else if (pdm2_dma_enable.bits.pdm_dma_en_16k == 1) {
				rx_low = (uint32_t)io_reg_read(
					PU_REGISTER_BASE + ACP_PDM_2_RX_LINEARPOSITIONCNTR_LOW_16K);
				rx_high = (uint32_t)io_reg_read(
					PU_REGISTER_BASE +
					ACP_PDM_2_RX_LINEARPOSITIONCNTR_HIGH_16K);
				dmic_curr_rx_pos = (uint64_t)(((uint64_t)rx_high << 32) | rx_low);
				LOG_DBG("get_status: DMIC 16k");
			} else if (pdm2_dma_enable.bits.pdm_dma_en_96k == 1) {
				rx_low = (uint32_t)io_reg_read(
					PU_REGISTER_BASE + ACP_PDM_2_RX_LINEARPOSITIONCNTR_LOW_96K);
				rx_high = (uint32_t)io_reg_read(
					PU_REGISTER_BASE +
					ACP_PDM_2_RX_LINEARPOSITIONCNTR_HIGH_96K);
				dmic_curr_rx_pos = (uint64_t)(((uint64_t)rx_high << 32) | rx_low);
				LOG_DBG("get_status: DMIC 96k");
			}
		}
		LOG_DBG("get_status: pending=%u", stat->pending_length);
		LOG_DBG("get_status: curr=%llu prev=%llu", dmic_curr_rx_pos, ctx->prev_pos);
	}
	stat->pending_length = (int32_t)(dmic_curr_rx_pos - ctx->prev_pos);
	LOG_DBG("get_status: pending=%u", stat->pending_length);
	LOG_DBG("get_status: curr=%llu prev=%llu", dmic_curr_rx_pos, ctx->prev_pos);
	if (stat->pending_length >= (dmic_rngbuff_size / 2)) {
		ctx->prev_pos += (dmic_rngbuff_size / 2);
	} else {
		stat->pending_length = 0;
	}
	stat->free = dmic_rngbuff_size - stat->pending_length;
	return 0;
}

int acp_dma_dmic_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_COPY_ALIGNMENT:
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = CONFIG_ACP_DMA_BUFFER_ALIGN;
		break;
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = CONFIG_ACP_DMA_BUFFER_ADDRESS_ALIGN;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 2;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int acp_dmic_dma_init(const struct device *dev)
{
	LOG_INF("init: DMIC DMA driver");
	struct acp7x_dma_dev_data *const dev_data = dev->data;
	/* Setup context and atomics for channels */
	dev_data->dma_ctx.magic = DMA_MAGIC;
	dev_data->dma_ctx.dma_channels = CONFIG_DMA_ACP_CHANNEL_COUNT;
	dev_data->dma_ctx.atomic = dev_data->atomic;
	for (int i = 0; i < DMIC_INSTANCES; i++) {
		if (!dmic_ctx[i]) {
			dmic_ctx[i] = k_malloc(sizeof(struct dmic_context));
		}
		if (dmic_ctx[i]) {
			dmic_ctx[i]->index = 0xFFFF;
			dmic_ctx[i]->pin_dir = 0xFFFF;
			dmic_ctx[i]->dma_channel = 0xFFFF;
			dmic_ctx[i]->prev_pos = 0;
		} else {
			return -ENOMEM;
		}
	}
	LOG_INF("init: DMIC DMA completed");
	return 0;
}

static DEVICE_API(dma, acp_dmic_dma_ops) = {
	.config = acp_dma_dmic_config,
	.start = acp_dma_dmic_start,
	.stop = acp_dma_dmic_stop,
	.chan_filter = acp_dmic_dma_chan_filter,
	.chan_release = acp_dmic_dma_chan_release,
	.reload = acp_dmic_dma_reload,
	.suspend = acp_dmic_dma_suspend,
	.resume = acp_dmic_dma_resume,
	.get_status = acp_dmic_dma_get_status,
	.get_attribute = acp_dma_dmic_get_attribute,
};

static struct acp7x_dma_dev_data acp_dma_dev_data;

DEVICE_DT_DEFINE(DT_NODELABEL(acp_dmic_dma), &acp_dmic_dma_init, NULL, &acp_dma_dev_data,
		 NULL /*&acp_dmic_dma_dev_cfg*/, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY,
		 &acp_dmic_dma_ops);
