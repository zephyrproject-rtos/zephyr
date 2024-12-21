/*
 * Copyright 2023 NXP
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * based on dmic_nrfx_pdm.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>

#include <fsl_dmic.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dmic_mcux, CONFIG_AUDIO_DMIC_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_dmic

struct mcux_dmic_pdm_chan {
	dmic_channel_config_t dmic_channel_cfg;
	const struct device *dma;
	uint8_t dma_chan;
};

struct mcux_dmic_drv_data {
	struct k_mem_slab *mem_slab;
	void *dma_bufs[CONFIG_DMIC_MCUX_DMA_BUFFERS];
	uint8_t active_buf_idx;
	uint32_t block_size;
	DMIC_Type *base_address;
	struct mcux_dmic_pdm_chan **pdm_channels;
	uint8_t act_num_chan;
	struct k_msgq *rx_queue;
	uint32_t chan_map_lo;
	uint32_t chan_map_hi;
	enum dmic_state dmic_state;
};

struct mcux_dmic_cfg {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_name;
	bool use2fs;
};

static int dmic_mcux_get_osr(uint32_t pcm_rate, uint32_t bit_clk, bool use_2fs)
{
	uint32_t use2fs_div = use_2fs ? 1 : 2;

	/* Note that the below calculation assumes the following:
	 * - DMIC DIVHFCLK is set to 0x0 (divide by 1)
	 * - DMIC PHY_HALF is set to 0x0 (standard sample rate)
	 */
	return (uint32_t)(bit_clk / (2 * pcm_rate * use2fs_div));
}

/* Gets hardware channel index from logical channel */
static uint8_t dmic_mcux_hw_chan(struct mcux_dmic_drv_data *drv_data,
				 uint8_t log_chan)
{
	enum pdm_lr lr;
	uint8_t hw_chan;

	/* This function assigns hardware channel "n" to the left channel,
	 * and hardware channel "n+1" to the right channel. This choice is
	 * arbitrary, but must be followed throughout the driver.
	 */
	dmic_parse_channel_map(drv_data->chan_map_lo,
			       drv_data->chan_map_hi,
			       log_chan, &hw_chan, &lr);
	if (lr == PDM_CHAN_LEFT) {
		return hw_chan * 2;
	} else {
		return (hw_chan * 2) + 1;
	}
}

static void dmic_mcux_activate_channels(struct mcux_dmic_drv_data *drv_data,
					bool enable)
{

	/* PDM channel 0 must always be enabled, as the RM states:
	 * "In order to output 8 channels of PDM Data, PDM_CLK01 must be used"
	 * therefore, even if we don't intend to capture PDM data from the
	 * channel 0 FIFO, we still enable the channel so the clock is active.
	 */
	uint32_t mask = 0x1;

	for (uint8_t chan = 0; chan < drv_data->act_num_chan; chan++) {
		/* Set bitmask of hw channel to enable */
		mask |= BIT(dmic_mcux_hw_chan(drv_data, chan));
	}

	if (enable) {
		DMIC_EnableChannnel(drv_data->base_address, mask);
	} else {
		/* No function to disable channels, we must bypass HAL here */
		drv_data->base_address->CHANEN &= ~mask;
	}
}

static int dmic_mcux_enable_dma(struct mcux_dmic_drv_data *drv_data, bool enable)
{
	struct mcux_dmic_pdm_chan *pdm_channel;
	uint8_t num_chan = drv_data->act_num_chan;
	uint8_t hw_chan;
	int ret = 0;

	for (uint8_t chan = 0; chan < num_chan; chan++) {
		/* Parse the channel map data */
		hw_chan = dmic_mcux_hw_chan(drv_data, chan);
		pdm_channel = drv_data->pdm_channels[hw_chan];
		if (enable) {
			ret = dma_start(pdm_channel->dma, pdm_channel->dma_chan);
			if (ret < 0) {
				LOG_ERR("Could not start DMA for HW channel %d",
					hw_chan);
				return ret;
			}
		} else {
			if (dma_stop(pdm_channel->dma, pdm_channel->dma_chan)) {
				ret = -EIO;
			}
		}
		DMIC_EnableChannelDma(drv_data->base_address,
				      (dmic_channel_t)hw_chan, enable);
	}

	return ret;
}

/* Helper to reload DMA engine for all active channels with new buffer */
static void dmic_mcux_reload_dma(struct mcux_dmic_drv_data *drv_data,
				 void *buffer)
{
	int ret;
	uint8_t hw_chan;
	struct mcux_dmic_pdm_chan *pdm_channel;
	uint8_t num_chan = drv_data->act_num_chan;
	uint32_t dma_buf_size = drv_data->block_size / num_chan;
	uint32_t src, dst;

	/* This function reloads the DMA engine for all active DMA channels
	 * with the provided buffer. Each DMA channel will start
	 * at a different initial address to interleave channel data.
	 */
	for (uint8_t chan = 0; chan < num_chan; chan++) {
		/* Parse the channel map data */
		hw_chan = dmic_mcux_hw_chan(drv_data, chan);
		pdm_channel = drv_data->pdm_channels[hw_chan];
		src = DMIC_FifoGetAddress(drv_data->base_address, hw_chan);
		dst = (uint32_t)(((uint16_t *)buffer) + chan);
		ret = dma_reload(pdm_channel->dma, pdm_channel->dma_chan,
				 src, dst, dma_buf_size);
		if (ret < 0) {
			LOG_ERR("Could not reload DMIC HW channel %d", hw_chan);
			return;
		}
	}
}

/* Helper to get next buffer index for DMA */
static uint8_t dmic_mcux_next_buf_idx(uint8_t current_idx)
{
	if ((current_idx + 1) == CONFIG_DMIC_MCUX_DMA_BUFFERS) {
		return 0;
	}
	return current_idx + 1;
}

static int dmic_mcux_stop(struct mcux_dmic_drv_data *drv_data)
{
	/* Disable active channels */
	dmic_mcux_activate_channels(drv_data, false);
	/* Disable DMA */
	dmic_mcux_enable_dma(drv_data, false);

	/* Free all memory slabs */
	for (uint32_t i = 0; i < CONFIG_DMIC_MCUX_DMA_BUFFERS; i++) {
		k_mem_slab_free(drv_data->mem_slab, drv_data->dma_bufs[i]);
	}

	/* Purge the RX queue as well. */
	k_msgq_purge(drv_data->rx_queue);

	drv_data->dmic_state = DMIC_STATE_CONFIGURED;

	return 0;
}

static void dmic_mcux_dma_cb(const struct device *dev, void *user_data,
			     uint32_t channel, int status)
{

	struct mcux_dmic_drv_data *drv_data = (struct mcux_dmic_drv_data *)user_data;
	int ret;
	void *done_buffer = drv_data->dma_bufs[drv_data->active_buf_idx];
	void *new_buffer;

	LOG_DBG("CB: channel is %u", channel);

	if (status < 0) {
		/* DMA has failed, free allocated blocks */
		LOG_ERR("DMA reports error");
		dmic_mcux_enable_dma(drv_data, false);
		dmic_mcux_activate_channels(drv_data, false);
		/* Free all allocated DMA buffers */
		dmic_mcux_stop(drv_data);
		drv_data->dmic_state = DMIC_STATE_ERROR;
		return;
	}

	/* Before we queue the current buffer, make sure we can allocate
	 * another one to replace it.
	 */
	ret = k_mem_slab_alloc(drv_data->mem_slab, &new_buffer, K_NO_WAIT);
	if (ret < 0) {
		/* We can't allocate a new buffer to replace the current
		 * one, so we cannot release the current buffer to the
		 * rx queue (or the DMA would stave). Therefore, we just
		 * leave the current buffer in place to be overwritten
		 * by the DMA.
		 */
		LOG_ERR("Could not allocate RX buffer. Dropping RX data");
		drv_data->dmic_state = DMIC_STATE_ERROR;
		/* Reload DMA */
		dmic_mcux_reload_dma(drv_data, done_buffer);
		/* Advance active buffer index */
		drv_data->active_buf_idx =
			dmic_mcux_next_buf_idx(drv_data->active_buf_idx);
		return;
	}

	/* DMA issues an interrupt at the completion of every block.
	 * we should put the active buffer into the rx queue for the
	 * application to read. The application is responsible for
	 * freeing this buffer once it processes it.
	 */
	ret = k_msgq_put(drv_data->rx_queue, &done_buffer, K_NO_WAIT);
	if (ret < 0) {
		/* Free the newly allocated buffer, we won't need it. */
		k_mem_slab_free(drv_data->mem_slab, new_buffer);
		/* We cannot enqueue the current buffer, so we will drop
		 * the current buffer data and leave the current buffer
		 * in place to be overwritten by the DMA
		 */
		LOG_ERR("RX queue overflow, dropping RX buffer data");
		drv_data->dmic_state = DMIC_STATE_ERROR;
		/* Reload DMA */
		dmic_mcux_reload_dma(drv_data, done_buffer);
		/* Advance active buffer index */
		drv_data->active_buf_idx =
			dmic_mcux_next_buf_idx(drv_data->active_buf_idx);
		return;
	}

	/* Previous buffer was enqueued, and new buffer is allocated.
	 * Replace pointer to previous buffer in our dma slots array,
	 * and reload DMA with next buffer.
	 */
	drv_data->dma_bufs[drv_data->active_buf_idx] = new_buffer;
	dmic_mcux_reload_dma(drv_data, new_buffer);
	/* Advance active buffer index */
	drv_data->active_buf_idx = dmic_mcux_next_buf_idx(drv_data->active_buf_idx);
}

static int dmic_mcux_setup_dma(const struct device *dev)
{
	struct mcux_dmic_drv_data *drv_data = dev->data;
	struct mcux_dmic_pdm_chan *pdm_channel;
	struct dma_block_config blk_cfg[CONFIG_DMIC_MCUX_DMA_BUFFERS] = {0};
	struct dma_config dma_cfg = {0};
	uint8_t num_chan = drv_data->act_num_chan;
	uint32_t dma_buf_size = drv_data->block_size / num_chan;
	uint8_t dma_buf_idx = 0;
	void *dma_buf = drv_data->dma_bufs[dma_buf_idx];
	uint8_t hw_chan;
	int ret = 0;


	/* Setup DMA configuration common between all channels */
	dma_cfg.user_data = drv_data;
	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = sizeof(uint16_t); /* Each sample is 16 bits */
	dma_cfg.dest_data_size = sizeof(uint16_t);
	dma_cfg.block_count = CONFIG_DMIC_MCUX_DMA_BUFFERS;
	dma_cfg.head_block = &blk_cfg[0];
	dma_cfg.complete_callback_en = 1; /* Callback at each block */
	dma_cfg.dma_callback = dmic_mcux_dma_cb;

	/* When multiple channels are enabled simultaneously, the DMA
	 * completion interrupt from one channel will signal that DMA data
	 * from multiple channels may be collected, provided the same
	 * amount of data was transferred. Therefore, we only enable the
	 * DMA completion callback for the first channel we setup
	 */
	for (uint8_t chan = 0; chan < num_chan; chan++) {
		/* Parse the channel map data */
		hw_chan = dmic_mcux_hw_chan(drv_data, chan);
		/* Configure blocks for hw_chan */
		for (uint32_t blk = 0; blk < CONFIG_DMIC_MCUX_DMA_BUFFERS; blk++) {
			blk_cfg[blk].source_address =
				DMIC_FifoGetAddress(drv_data->base_address, hw_chan);
			/* We interleave samples within the output buffer
			 * based on channel map. So for a channel map like so:
			 * [pdm0_l, pdm0_r, pdm1_r, pdm1_l]
			 * the resulting DMA buffer would look like:
			 * [pdm0_l_s0, pdm0_r_s0, pdm1_r_s0, pdm1_l_s0,
			 *  pdm0_l_s1, pdm0_r_s1, pdm1_r_s1, pdm1_l_s1, ...]
			 * Each sample is 16 bits wide.
			 */
			blk_cfg[blk].dest_address =
				(uint32_t)(((uint16_t *)dma_buf) + chan);
			blk_cfg[blk].dest_scatter_interval =
				num_chan * sizeof(uint16_t);
			blk_cfg[blk].dest_scatter_en = 1;
			blk_cfg[blk].source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			blk_cfg[blk].dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
			blk_cfg[blk].block_size = dma_buf_size;
			/* Enable circular mode- when the final DMA block
			 * is exhausted, we want the DMA controller
			 * to restart with the first one.
			 */
			blk_cfg[blk].source_reload_en = 1;
			blk_cfg[blk].dest_reload_en = 1;
			if (blk < (CONFIG_DMIC_MCUX_DMA_BUFFERS - 1)) {
				blk_cfg[blk].next_block = &blk_cfg[blk + 1];
			} else {
				/* Last block, enable circular reload */
				blk_cfg[blk].next_block = NULL;
			}
			/* Select next dma buffer in array */
			dma_buf_idx = dmic_mcux_next_buf_idx(dma_buf_idx);
			dma_buf = drv_data->dma_bufs[dma_buf_idx];
		}
		pdm_channel = drv_data->pdm_channels[hw_chan];
		/* Set configuration for hw_chan_0 */
		ret = dma_config(pdm_channel->dma, pdm_channel->dma_chan, &dma_cfg);
		if (ret < 0) {
			LOG_ERR("Could not configure DMIC channel %d", hw_chan);
			return ret;
		}
		/* First channel is configured. Do not install callbacks for
		 * other channels.
		 */
		dma_cfg.dma_callback = NULL;
	}

	return 0;
}

/* Initializes a DMIC hardware channel */
static int dmic_mcux_init_channel(const struct device *dev, uint32_t osr,
				  uint8_t chan, enum pdm_lr lr)
{
	struct mcux_dmic_drv_data *drv_data = dev->data;

	if (!drv_data->pdm_channels[chan]) {
		/* Channel disabled at devicetree level */
		return -EINVAL;
	}

	drv_data->pdm_channels[chan]->dmic_channel_cfg.osr = osr;
	/* Configure channel settings */
	DMIC_ConfigChannel(drv_data->base_address, (dmic_channel_t)chan,
			   lr == PDM_CHAN_LEFT ? kDMIC_Left : kDMIC_Right,
			   &drv_data->pdm_channels[chan]->dmic_channel_cfg);
	/* Setup channel FIFO. We use maximum threshold to avoid triggering
	 * DMA too frequently
	 */
	DMIC_FifoChannel(drv_data->base_address, chan, 15, true, true);
	/* Disable interrupts. DMA will be enabled in dmic_mcux_trigger. */
	DMIC_EnableChannelInterrupt(drv_data->base_address, chan, false);
	return 0;
}

static int mcux_dmic_init(const struct device *dev)
{
	const struct mcux_dmic_cfg *config = dev->config;
	struct mcux_dmic_drv_data *drv_data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	DMIC_Init(drv_data->base_address);
	DMIC_Use2fs(drv_data->base_address, config->use2fs);
#if !(defined(FSL_FEATURE_DMIC_HAS_NO_IOCFG) && FSL_FEATURE_DMIC_HAS_NO_IOCFG)
	/* Set IO to dual mode */
	DMIC_SetIOCFG(drv_data->base_address, kDMIC_PdmDual);
#endif
	drv_data->dmic_state = DMIC_STATE_INITIALIZED;
	return 0;
}

static int dmic_mcux_configure(const struct device *dev,
			       struct dmic_cfg *config)
{

	const struct mcux_dmic_cfg *drv_config = dev->config;
	struct mcux_dmic_drv_data *drv_data = dev->data;
	struct pdm_chan_cfg *channel = &config->channel;
	struct pcm_stream_cfg *stream = &config->streams[0];
	enum pdm_lr lr_0 = 0, lr_1 = 0;
	uint8_t hw_chan_0 = 0, hw_chan_1 = 0;
	uint32_t bit_clk_rate, osr;
	int ret;

	if (drv_data->dmic_state == DMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot configure device while it is active");
		return -EBUSY;
	}

	/* Only one active channel is supported */
	if (channel->req_num_streams != 1) {
		return -EINVAL;
	}

	/* DMIC supports up to 8 active channels. Verify user is not
	 * requesting more
	 */
	if (channel->req_num_chan > FSL_FEATURE_DMIC_CHANNEL_NUM) {
		LOG_ERR("DMIC only supports 8 channels or less");
		return -ENOTSUP;
	}

	if (stream->pcm_rate == 0 || stream->pcm_width == 0) {
		if (drv_data->dmic_state == DMIC_STATE_CONFIGURED) {
			DMIC_DeInit(drv_data->base_address);
			drv_data->dmic_state = DMIC_STATE_UNINIT;
		}
		return 0;
	}

	/* If DMIC was deinitialized, reinit here */
	if (drv_data->dmic_state == DMIC_STATE_UNINIT) {
		ret = mcux_dmic_init(dev);
		if (ret < 0) {
			LOG_ERR("Could not reinit DMIC");
			return ret;
		}
	}

	/* Currently, we only support 16 bit samples. This is because the DMIC
	 * API dictates that samples should be interleaved between channels,
	 * IE: {C0, C1, C2, C0, C1, C2}. To achieve this we must use the
	 * "destination address increment" function of the LPC DMA IP. Since
	 * the LPC DMA IP does not support 3 byte wide transfers, we cannot
	 * effectively use destination address increments to interleave 24
	 * bit samples.
	 */
	if (stream->pcm_width != 16) {
		LOG_ERR("Only 16 bit samples are supported");
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(drv_config->clock_dev,
				     drv_config->clock_name, &bit_clk_rate);
	if (ret < 0) {
		return ret;
	}

	/* Check bit clock rate versus what user requested */
	if ((config->io.min_pdm_clk_freq > bit_clk_rate) ||
	    (config->io.max_pdm_clk_freq < bit_clk_rate)) {
		return -EINVAL;
	}
	/* Calculate the required OSR divider based on the PCM bit clock
	 * rate to the DMIC.
	 */
	osr = dmic_mcux_get_osr(stream->pcm_rate, bit_clk_rate, drv_config->use2fs);
	/* Now, parse the channel map and set up each channel we should
	 * make active. We parse two channels at once, that way we can
	 * check to make sure that the L/R channels of each PDM controller
	 * are adjacent.
	 */
	channel->act_num_chan = 0;
	/* Save channel request data */
	drv_data->chan_map_lo = channel->req_chan_map_lo;
	drv_data->chan_map_hi = channel->req_chan_map_hi;
	for (uint8_t chan = 0; chan < channel->req_num_chan; chan += 2) {
		/* Get the channel map data for channel pair */
		dmic_parse_channel_map(channel->req_chan_map_lo,
				       channel->req_chan_map_hi,
				       chan, &hw_chan_0, &lr_0);
		if ((chan + 1) < channel->req_num_chan) {
			/* Paired channel is enabled */
			dmic_parse_channel_map(channel->req_chan_map_lo,
					       channel->req_chan_map_hi,
					       chan + 1, &hw_chan_1, &lr_1);
			/* Verify that paired channels use same hardware index */
			if ((lr_0 == lr_1) ||
			    (hw_chan_0 != hw_chan_1)) {
				return -EINVAL;
			}
		}
		/* Configure selected channels in DMIC */
		ret = dmic_mcux_init_channel(dev, osr,
					     dmic_mcux_hw_chan(drv_data, chan),
					     lr_0);
		if (ret < 0) {
			return ret;
		}
		channel->act_num_chan++;
		if ((chan + 1) < channel->req_num_chan) {
			/* Paired channel is enabled */
			ret = dmic_mcux_init_channel(dev, osr,
						     dmic_mcux_hw_chan(drv_data,
								       chan + 1),
						     lr_1);
			if (ret < 0) {
				return ret;
			}
			channel->act_num_chan++;
		}
	}

	channel->act_chan_map_lo = channel->req_chan_map_lo;
	channel->act_chan_map_hi = channel->req_chan_map_hi;

	drv_data->mem_slab   = stream->mem_slab;
	drv_data->block_size   = stream->block_size;
	drv_data->act_num_chan = channel->act_num_chan;
	drv_data->dmic_state = DMIC_STATE_CONFIGURED;

	return 0;
}

static int dmic_mcux_start(const struct device *dev)
{
	struct mcux_dmic_drv_data *drv_data = dev->data;
	int ret;

	/* Allocate the initial set of buffers reserved for use by the hardware.
	 * We queue buffers so that when the DMA is operating on buffer "n",
	 * buffer "n+1" is already queued in the DMA hardware. When buffer "n"
	 * completes, we allocate another buffer and add it to the tail of the
	 * DMA descriptor chain. This approach requires the driver to allocate
	 * a minimum of two buffers
	 */

	for (uint32_t i = 0; i < CONFIG_DMIC_MCUX_DMA_BUFFERS; i++) {
		/* Allocate buffers for DMA */
		ret = k_mem_slab_alloc(drv_data->mem_slab,
				       &drv_data->dma_bufs[i], K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("failed to allocate buffer");
			return -ENOBUFS;
		}
	}

	ret = dmic_mcux_setup_dma(dev);
	if (ret < 0) {
		return ret;
	}

	ret = dmic_mcux_enable_dma(drv_data, true);
	if (ret < 0) {
		return ret;
	}
	dmic_mcux_activate_channels(drv_data, true);

	return 0;
}

static int dmic_mcux_trigger(const struct device *dev,
				 enum dmic_trigger cmd)
{
	struct mcux_dmic_drv_data *drv_data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_PAUSE:
		/* Disable active channels */
		if (drv_data->dmic_state == DMIC_STATE_ACTIVE) {
			dmic_mcux_activate_channels(drv_data, false);
		}
		drv_data->dmic_state = DMIC_STATE_PAUSED;
		break;
	case DMIC_TRIGGER_STOP:
		if (drv_data->dmic_state == DMIC_STATE_ACTIVE) {
			dmic_mcux_stop(drv_data);
		}
		drv_data->dmic_state = DMIC_STATE_CONFIGURED;
		break;
	case DMIC_TRIGGER_RELEASE:
		/* Enable active channels */
		if (drv_data->dmic_state == DMIC_STATE_PAUSED) {
			dmic_mcux_activate_channels(drv_data, true);
		}
		drv_data->dmic_state = DMIC_STATE_ACTIVE;
		break;
	case DMIC_TRIGGER_START:
		if ((drv_data->dmic_state != DMIC_STATE_CONFIGURED) &&
		    (drv_data->dmic_state != DMIC_STATE_ACTIVE)) {
			LOG_ERR("Device is not configured");
			return -EIO;
		} else if (drv_data->dmic_state != DMIC_STATE_ACTIVE) {
			if (dmic_mcux_start(dev) < 0) {
				LOG_ERR("Could not start DMIC");
				return -EIO;
			}
			drv_data->dmic_state = DMIC_STATE_ACTIVE;
		}
		break;
	case DMIC_TRIGGER_RESET:
		/* Reset DMIC to uninitialized state */
		DMIC_DeInit(drv_data->base_address);
		drv_data->dmic_state = DMIC_STATE_UNINIT;
		break;
	default:
		LOG_ERR("Invalid command: %d", cmd);
		return -EINVAL;
	}
	return 0;
}

static int dmic_mcux_read(const struct device *dev,
			      uint8_t stream,
			      void **buffer, size_t *size, int32_t timeout)
{
	struct mcux_dmic_drv_data *drv_data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (drv_data->dmic_state == DMIC_STATE_ERROR) {
		LOG_ERR("Device reports an error, please reset and reconfigure it");
		return -EIO;
	}

	if ((drv_data->dmic_state != DMIC_STATE_CONFIGURED) &&
	    (drv_data->dmic_state != DMIC_STATE_ACTIVE) &&
	    (drv_data->dmic_state != DMIC_STATE_PAUSED)) {
		LOG_ERR("Device state is not valid for read");
		return -EIO;
	}

	ret = k_msgq_get(drv_data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}
	*size = drv_data->block_size;

	LOG_DBG("read buffer = %p", *buffer);
	return 0;
}

static const struct _dmic_ops dmic_ops = {
	.configure = dmic_mcux_configure,
	.trigger = dmic_mcux_trigger,
	.read = dmic_mcux_read,
};

/* Converts integer gainshift into 5 bit 2's complement value for GAINSHIFT reg */
#define PDM_DMIC_GAINSHIFT(val)							\
	(val >= 0) ? (val & 0xF) : (BIT(4) | (0x10 - (val & 0xF)))

/* Defines structure for a given PDM channel node */
#define PDM_DMIC_CHAN_DEFINE(pdm_node)						\
	static struct mcux_dmic_pdm_chan					\
		pdm_channel_##pdm_node = {					\
		.dma = DEVICE_DT_GET(DT_DMAS_CTLR(pdm_node)),			\
		.dma_chan = DT_DMAS_CELL_BY_IDX(pdm_node, 0, channel),		\
		.dmic_channel_cfg = {						\
			.gainshft = PDM_DMIC_GAINSHIFT(DT_PROP(pdm_node,	\
							       gainshift)),	\
			.preac2coef = DT_ENUM_IDX(pdm_node, compensation_2fs),	\
			.preac4coef = DT_ENUM_IDX(pdm_node, compensation_4fs),	\
			.dc_cut_level = DT_ENUM_IDX(pdm_node, dc_cutoff),	\
			.post_dc_gain_reduce = DT_PROP(pdm_node, dc_gain),	\
			.sample_rate = kDMIC_PhyFullSpeed,			\
			.saturate16bit = 1U,					\
		},								\
	};

/* Defines structures for all enabled PDM channels */
#define PDM_DMIC_CHANNELS_DEFINE(idx)						\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, PDM_DMIC_CHAN_DEFINE)

/* Gets pointer for a given PDM channel node */
#define PDM_DMIC_CHAN_GET(pdm_node)						\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(pdm_node),				\
		    (&pdm_channel_##pdm_node), (NULL)),

/* Gets array of pointers to PDM channels */
#define PDM_DMIC_CHANNELS_GET(idx)						\
	DT_INST_FOREACH_CHILD(idx, PDM_DMIC_CHAN_GET)

#define MCUX_DMIC_DEVICE(idx)							\
	PDM_DMIC_CHANNELS_DEFINE(idx);						\
	static struct mcux_dmic_pdm_chan					\
		*pdm_channels##idx[FSL_FEATURE_DMIC_CHANNEL_NUM] = {		\
			PDM_DMIC_CHANNELS_GET(idx)				\
	};									\
	K_MSGQ_DEFINE(dmic_msgq##idx, sizeof(void *),				\
		      CONFIG_DMIC_MCUX_QUEUE_SIZE, 1);				\
	static struct mcux_dmic_drv_data mcux_dmic_data##idx = {		\
		.pdm_channels = pdm_channels##idx,				\
		.base_address = (DMIC_Type *) DT_INST_REG_ADDR(idx),		\
		.dmic_state = DMIC_STATE_UNINIT,				\
		.rx_queue = &dmic_msgq##idx,					\
		.active_buf_idx = 0U,						\
	};									\
										\
	PINCTRL_DT_INST_DEFINE(idx);						\
	static struct mcux_dmic_cfg mcux_dmic_cfg##idx = {			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),		\
		.clock_name = (clock_control_subsys_t)				\
				DT_INST_CLOCKS_CELL(idx, name),			\
		.use2fs = DT_INST_PROP(idx, use2fs),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(idx, mcux_dmic_init, NULL,			\
			&mcux_dmic_data##idx, &mcux_dmic_cfg##idx,		\
			POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY,		\
			&dmic_ops);

/* Existing SoCs only have one PDM instance. */
DT_INST_FOREACH_STATUS_OKAY(MCUX_DMIC_DEVICE)
