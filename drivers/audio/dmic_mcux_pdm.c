/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include <fsl_pdm.h>

LOG_MODULE_REGISTER(dmic_mcux_pdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_dmic_pdm

/* MICFIL FIFO data path is 32-bit wide. */
#define PDM_SAMPLE_BYTES 4U

/*
 * Byte distance between adjacent DATACH FIFO registers (source stride per
 * beat) and the width of one FIFO sample. These come from the HAL feature
 * header; fall back to the 32-bit / 4-byte layout used by every current part.
 */
#if defined(FSL_FEATURE_PDM_FIFO_OFFSET)
#define PDM_FIFO_OFFSET FSL_FEATURE_PDM_FIFO_OFFSET
#else
#define PDM_FIFO_OFFSET 4U
#endif

#if defined(FSL_FEATURE_PDM_FIFO_WIDTH)
#define PDM_FIFO_WIDTH FSL_FEATURE_PDM_FIFO_WIDTH
#else
#define PDM_FIFO_WIDTH 4U
#endif

#if defined(FSL_FEATURE_PDM_CHANNEL_NUM)
#define PDM_MAX_CHANNELS FSL_FEATURE_PDM_CHANNEL_NUM
#else
#define PDM_MAX_CHANNELS 8U
#endif

/* Number of DMA buffers kept in the cyclic ring (double buffering minimum). */
#define PDM_DMA_BUFFERS 2U

/*
 * eDMA CITER/BITER is a 15-bit field on a single (non-minor-linked) channel,
 * so the per-block major-loop count (frames) must not exceed this.
 */
#define PDM_MAX_MAJOR_COUNT 32767U


struct mcux_pdm_cfg {
	PDM_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_name;
	const struct pinctrl_dev_config *pcfg;
	const struct device *dma_dev;
	uint8_t dma_channel;
	uint8_t dma_source;
	uint8_t quality_mode;
	uint8_t cic_decimation_rate;
	uint8_t fifo_watermark;
	uint32_t sample_rate;
	uint8_t chan_dc_cutoff[PDM_MAX_CHANNELS];
	uint8_t chan_gain[PDM_MAX_CHANNELS];
	uint32_t ch_enabled_mask;
};

struct mcux_pdm_data {
	const struct device *dev;
	PDM_Type *base;
	enum dmic_state state;

	uint32_t chan_map_lo;
	uint32_t chan_map_hi;
	uint8_t hw_chan[PDM_MAX_CHANNELS];	/* logical index -> DATACH index */
	uint8_t base_hw_chan;			/* lowest DATACH index in the map */
	uint8_t channels;			/* number of active channels */

	uint32_t block_size;
	struct k_mem_slab *mem_slab;
	struct k_msgq *rx_queue;

	/* Cyclic ring bookkeeping. */
	void *dma_bufs[PDM_DMA_BUFFERS];
	uint8_t cur_ring;			/* ring slot currently being filled */

	/* Scratch block-config list used only while building the cyclic ring in
	 * mcux_pdm_setup_dma().
	 */
	struct dma_block_config blk[PDM_DMA_BUFFERS];
};


/* DMA completion callback. */
static void mcux_pdm_dma_cb(const struct device *dma_dev, void *arg,
			    uint32_t channel, int status);

/* Map a logical channel to its hardware DATACH index using the channel map. */
static uint8_t mcux_pdm_hw_chan(struct mcux_pdm_data *data, uint8_t log_chan)
{
	enum pdm_lr lr;

	uint8_t hw_chan;

	dmic_parse_channel_map(data->chan_map_lo, data->chan_map_hi,
			       log_chan, &hw_chan, &lr);

	/* The DATACH index encodes the stereo pair (hw_chan) and the L/R side:
	 * left  -> even index, right -> odd index.
	 */
	return (uint8_t)((hw_chan << 1) | (lr == PDM_CHAN_RIGHT ? 1U : 0U));
}

/*
 * Configure a single cyclic eDMA channel that drains N contiguous DATACH FIFOs
 * per MICFIL request into an interleaved PCM ring, then arm it. Called once at
 * capture start.
 */
static int mcux_pdm_setup_dma(const struct device *dev)
{
	const struct mcux_pdm_cfg *cfg = dev->config;
	struct mcux_pdm_data *data = dev->data;
	uint8_t num_chan = data->channels;
	struct dma_block_config *blk = data->blk;
	struct dma_config dma_cfg = {0};
	uint32_t src = PDM_GetDataRegisterAddress(data->base, data->base_hw_chan);
	int ret;

	memset(data->blk, 0, sizeof(data->blk));

	for (uint8_t b = 0; b < PDM_DMA_BUFFERS; b++) {
		blk[b].source_address = src;
		blk[b].source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		blk[b].dest_address = (uint32_t)data->dma_bufs[b];
		blk[b].dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		blk[b].block_size = data->block_size;

		if (num_chan > 1U) {
			/* Walk one sample from each of the N adjacent FIFOs per
			 * minor loop (SOFF = FIFO_OFFSET), then rewind the source
			 * back to the base FIFO (source minor-loop offset).
			 */
			blk[b].source_gather_en = 1;
			blk[b].source_gather_interval = PDM_FIFO_OFFSET;
			blk[b].source_minor_loop_offset_en = 1;
			blk[b].source_minor_loop_offset =
				-(int32_t)((uint32_t)num_chan * PDM_FIFO_OFFSET);
		}

		blk[b].next_block =
			(b + 1U < PDM_DMA_BUFFERS) ? &blk[b + 1U] : NULL;
	}

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = PDM_FIFO_WIDTH;
	dma_cfg.dest_data_size = PDM_SAMPLE_BYTES;
	/* One minor loop = one interleaved frame (one sample from each channel).
	 * With a single channel this is a single sample.
	 */
	dma_cfg.source_burst_length = (uint32_t)num_chan * PDM_FIFO_WIDTH;
	dma_cfg.dest_burst_length = (uint32_t)num_chan * PDM_SAMPLE_BYTES;
	dma_cfg.block_count = PDM_DMA_BUFFERS;
	dma_cfg.head_block = &blk[0];
	dma_cfg.complete_callback_en = 1;
	dma_cfg.cyclic = 1;
	dma_cfg.dma_callback = mcux_pdm_dma_cb;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot = cfg->dma_source;

	ret = dma_config(cfg->dma_dev, cfg->dma_channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("dma_config failed on channel %u: %d", cfg->dma_channel, ret);
		return ret;
	}

	ret = dma_start(cfg->dma_dev, cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("dma_start failed on channel %u: %d", cfg->dma_channel, ret);
		return ret;
	}

	return 0;
}

static void mcux_pdm_stop_dma(const struct device *dev)
{
	const struct mcux_pdm_cfg *cfg = dev->config;

	(void)dma_stop(cfg->dma_dev, cfg->dma_channel);
}

static void mcux_pdm_enable(struct mcux_pdm_data *data, bool enable)
{
	if (enable) {
		for (uint8_t k = 0; k < data->channels; k++) {
			PDM_EnableChannel(data->base, data->hw_chan[k], true);
		}
		PDM_EnableDMA(data->base, true);
		PDM_Enable(data->base, true);
	} else {
		PDM_Enable(data->base, false);
		PDM_EnableDMA(data->base, false);
		for (uint8_t k = 0; k < data->channels; k++) {
			PDM_EnableChannel(data->base, data->hw_chan[k], false);
		}
	}
}

static void mcux_pdm_cleanup(const struct device *dev)
{
	struct mcux_pdm_data *data = dev->data;
	void *queued;

	mcux_pdm_enable(data, false);
	mcux_pdm_stop_dma(dev);

	for (uint8_t b = 0; b < PDM_DMA_BUFFERS; b++) {
		if (data->dma_bufs[b] != NULL) {
			k_mem_slab_free(data->mem_slab, data->dma_bufs[b]);
			data->dma_bufs[b] = NULL;
		}
	}

	while (k_msgq_get(data->rx_queue, &queued, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, queued);
	}
}

/*
 * DMA completion callback. Each per-block completion publishes the finished
 * block to the application and re-arms the freed ring slot so the cyclic
 * capture keeps running gap-free. The completion just freed one TCD, so the
 * dma_reload() below can never run out of slots.
 *
 * Two delivery strategies are compiled in behind PDM_CAPTURE_ZERO_COPY:
 *
 *  DMA writes straight into an application slab
 *  block. The finished block is handed to the application as-is (no copy) and
 *  the ring slot is re-armed with a freshly allocated slab block. If no free
 *  block is available the just-finished block is recycled as the DMA target
 *  (dropped, not published) so the ring always has a valid destination.
 */
static void mcux_pdm_dma_cb(const struct device *dma_dev, void *arg,
			    uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct mcux_pdm_cfg *cfg = dev->config;
	struct mcux_pdm_data *data = dev->data;
	uint8_t completed_ring;
	uint32_t src;
	void *rearm_buf;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		LOG_ERR("DMA reports error %d on channel %u", status, channel);
		mcux_pdm_cleanup(dev);
		data->state = DMIC_STATE_ERROR;
		return;
	}

	if (data->state != DMIC_STATE_ACTIVE) {
		return;
	}

	completed_ring = data->cur_ring;

	/*
	 * The completed ring slot IS the application block: the DMA filled a
	 * slab block in place. Try to allocate a fresh slab block to re-arm the
	 * slot; only if that succeeds can the finished block be published (the
	 * ring must always retain a valid DMA target).
	 */
	void *fresh = NULL;

	if (k_mem_slab_alloc(data->mem_slab, &fresh, K_NO_WAIT) != 0) {
		/* 
		 * No free block to take over the ring slot. Recycle the
		 * just-finished block as the DMA target and drop it
		 * rather than starving the ring / overflowing the FIFOs.
		 */
		LOG_WRN("No RX buffer available, dropping block");
		rearm_buf = data->dma_bufs[completed_ring];
	} else if (k_msgq_put(data->rx_queue,
					&data->dma_bufs[completed_ring],
					K_NO_WAIT) != 0) {
		/*
		 * Consumer is behind: drop the finished block, hand the
		 * freshly allocated one back to the slab, and recycle the
		 * finished block as the DMA target.
		 */
		LOG_WRN("RX queue full, dropping block");
		k_mem_slab_free(data->mem_slab, fresh);
		rearm_buf = data->dma_bufs[completed_ring];
	} else {
		/* 
		 * Published live with no copy. The fresh block takes over
		 * the ring slot as the next DMA target.
		 */
		data->dma_bufs[completed_ring] = fresh;
		rearm_buf = fresh;
	}

	/* Re-arm this ring slot so the cyclic capture keeps running. */
	src = PDM_GetDataRegisterAddress(data->base, data->base_hw_chan);
	if (dma_reload(cfg->dma_dev, cfg->dma_channel, src,
		       (uint32_t)rearm_buf, data->block_size) < 0) {
		LOG_ERR("dma_reload failed on channel %u", cfg->dma_channel);
		mcux_pdm_cleanup(dev);
		data->state = DMIC_STATE_ERROR;
		return;
	}

	data->cur_ring = (completed_ring + 1U) % PDM_DMA_BUFFERS;
}


static int mcux_pdm_configure(const struct device *dev, struct dmic_cfg *config)
{
	const struct mcux_pdm_cfg *cfg = dev->config;
	struct mcux_pdm_data *data = dev->data;
	struct pdm_chan_cfg *chan = &config->channel;
	struct pcm_stream_cfg *stream = &config->streams[0];
	uint32_t clk_rate = 0U;
	uint32_t frames;
	uint8_t act = 0U;
	uint8_t min_hw = PDM_MAX_CHANNELS;
	uint8_t max_hw = 0U;
	int ret;

	if (data->state == DMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot configure while active");
		return -EBUSY;
	}

	if (chan->req_num_streams != 1U) {
		return -EINVAL;
	}

	/* Zero rate/width tears the interface down. */
	if (stream->pcm_rate == 0U || stream->pcm_width == 0U) {
		if (data->state == DMIC_STATE_CONFIGURED) {
			PDM_Deinit(data->base);
			data->state = DMIC_STATE_UNINIT;
		}
		return 0;
	}

	/* MICFIL FIFO samples are 32-bit. */
	if (stream->pcm_width != 32U) {
		LOG_ERR("Only 32-bit samples are supported");
		return -ENOTSUP;
	}

	if (chan->req_num_chan == 0U || chan->req_num_chan > PDM_MAX_CHANNELS) {
		LOG_ERR("Unsupported channel count %u", chan->req_num_chan);
		return -ENOTSUP;
	}

	if (stream->mem_slab == NULL || stream->block_size == 0U) {
		return -EINVAL;
	}

	/* block_size must hold a whole number of interleaved frames. */
	if ((stream->block_size % (chan->req_num_chan * PDM_SAMPLE_BYTES)) != 0U) {
		LOG_ERR("block_size %u not aligned to frame size %u",
			stream->block_size, chan->req_num_chan * PDM_SAMPLE_BYTES);
		return -EINVAL;
	}

	data->chan_map_lo = chan->req_chan_map_lo;
	data->chan_map_hi = chan->req_chan_map_hi;

	/* Resolve the logical-to-hardware channel map and find its range. */
	for (uint8_t index = 0; index < chan->req_num_chan; index++) {
		uint8_t hw_chan = mcux_pdm_hw_chan(data, index);

		if (hw_chan >= PDM_MAX_CHANNELS) {
			LOG_ERR("hw channel %u out of range", hw_chan);
			return -EINVAL;
		}
		if ((cfg->ch_enabled_mask & BIT(hw_chan)) == 0U) {
			LOG_ERR("hw channel %u not enabled in DT", hw_chan);
			return -EINVAL;
		}
		data->hw_chan[act] = hw_chan;
		min_hw = MIN(min_hw, hw_chan);
		max_hw = MAX(max_hw, hw_chan);
		act++;
	}

	/*
	 * The single-channel minor-loop scheme reads a CONTIGUOUS, ASCENDING run
	 * of DATACH FIFOs starting at the lowest hardware channel and writes them
	 * in that order. Require logical channel k -> hardware channel min_hw+k so
	 * the interleaved output honours the requested channel order. Reject any
	 * non-contiguous or reordered map.
	 */
	if ((uint8_t)(max_hw - min_hw + 1U) != act) {
		LOG_ERR("channel map is not contiguous (hw %u..%u for %u channels)",
			min_hw, max_hw, act);
		return -ENOTSUP;
	}
	for (uint8_t k = 0; k < act; k++) {
		if (data->hw_chan[k] != (uint8_t)(min_hw + k)) {
			LOG_ERR("channel map must be ascending (logical %u -> hw %u)",
				k, data->hw_chan[k]);
			return -ENOTSUP;
		}
	}
	data->base_hw_chan = min_hw;

	/* Per-block major-loop count (interleaved frames) must fit the 15-bit
	 * eDMA CITER/BITER field on a single channel.
	 */
	frames = stream->block_size / (act * PDM_SAMPLE_BYTES);
	if (frames == 0U) {
		LOG_ERR("block_size %u too small for %u channels",
			stream->block_size, act);
		return -EINVAL;
	}
	if (frames > PDM_MAX_MAJOR_COUNT) {
		LOG_ERR("block_size %u too large: %u frames exceeds eDMA major limit %u",
			stream->block_size, frames, PDM_MAX_MAJOR_COUNT);
		return -EINVAL;
	}

	if (PDM_DMA_BUFFERS > CONFIG_DMA_TCD_QUEUE_SIZE) {
		LOG_ERR("need CONFIG_DMA_TCD_QUEUE_SIZE >= %u (have %u)",
			PDM_DMA_BUFFERS, CONFIG_DMA_TCD_QUEUE_SIZE);
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_name, &clk_rate);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Re-run PDM_Init here rather than a bare PDM_Reset(). PDM_Reset() only
	 * pulses the CTRL_1 SRES software-reset, which CLEARS CTRL_2 (the CICOSR
	 * oversample-rate and QSEL quality-mode fields). PDM_SetSampleRateConfig()
	 * below READS CICOSR/QSEL from CTRL_2 to derive the clock divider, so
	 * leaving them at zero mis-configures the decimator. PDM_Init() performs
	 * the SRES and then re-programs CICOSR/QSEL/watermark.
	 */
	pdm_config_t pdm_config = {0};

	pdm_config.qualityMode = (pdm_df_quality_mode_t)cfg->quality_mode;
	pdm_config.fifoWatermark = cfg->fifo_watermark;
	pdm_config.cicOverSampleRate = cfg->cic_decimation_rate;
	PDM_Init(data->base, &pdm_config);

	for (uint8_t k = 0; k < act; k++) {
		pdm_channel_config_t chan_cfg = {0};
		uint8_t hw_chan = data->hw_chan[k];

#if (defined(FSL_FEATURE_PDM_HAS_DC_OUT_CTRL) && FSL_FEATURE_PDM_HAS_DC_OUT_CTRL)
		chan_cfg.outputCutOffFreq =
			(pdm_dc_remover_t)cfg->chan_dc_cutoff[hw_chan];
#endif
#if !(defined(FSL_FEATURE_PDM_DC_CTRL_VALUE_FIXED) && FSL_FEATURE_PDM_DC_CTRL_VALUE_FIXED)
		chan_cfg.cutOffFreq = (pdm_dc_remover_t)cfg->chan_dc_cutoff[hw_chan];
#endif
		chan_cfg.gain = (pdm_df_output_gain_t)cfg->chan_gain[hw_chan];

		PDM_SetChannelConfig(data->base, hw_chan, &chan_cfg);
	}

	ret = PDM_SetSampleRateConfig(data->base, clk_rate, stream->pcm_rate);
	if (ret != kStatus_Success) {
		LOG_ERR("Unable to set sample rate %u", stream->pcm_rate);
		return -EINVAL;
	}

	data->channels = act;
	data->block_size = stream->block_size;
	data->mem_slab = stream->mem_slab;

	chan->act_num_streams = 1U;
	chan->act_num_chan = act;
	chan->act_chan_map_lo = chan->req_chan_map_lo;
	chan->act_chan_map_hi = chan->req_chan_map_hi;

	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

static int mcux_pdm_start(const struct device *dev)
{
	struct mcux_pdm_data *data = dev->data;
	int ret;

	data->cur_ring = 0U;

	for (uint8_t b = 0; b < PDM_DMA_BUFFERS; b++) {
		if (k_mem_slab_alloc(data->mem_slab, &data->dma_bufs[b], K_NO_WAIT) != 0) {
			LOG_ERR("Failed to allocate RX buffer %u", b);
			for (uint8_t f = 0; f < b; f++) {
				k_mem_slab_free(data->mem_slab, data->dma_bufs[f]);
				data->dma_bufs[f] = NULL;
			}
			return -ENOMEM;
		}
	}

	ret = mcux_pdm_setup_dma(dev);
	if (ret < 0) {
		mcux_pdm_stop_dma(dev);
		for (uint8_t b = 0; b < PDM_DMA_BUFFERS; b++) {
			if (data->dma_bufs[b] != NULL) {
				k_mem_slab_free(data->mem_slab, data->dma_bufs[b]);
				data->dma_bufs[b] = NULL;
			}
		}
		return ret;
	}

	mcux_pdm_enable(data, true);

	return 0;
}

static int mcux_pdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct mcux_pdm_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	key = irq_lock();

	switch (cmd) {
	case DMIC_TRIGGER_START:
	case DMIC_TRIGGER_RELEASE:
		if (data->state == DMIC_STATE_ACTIVE) {
			break;
		}
		if (data->state != DMIC_STATE_CONFIGURED &&
		    data->state != DMIC_STATE_PAUSED) {
			LOG_ERR("Invalid state %d for start", data->state);
			ret = -EIO;
			break;
		}
		ret = mcux_pdm_start(dev);
		if (ret == 0) {
			data->state = DMIC_STATE_ACTIVE;
		}
		break;

	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
		if (data->state == DMIC_STATE_ACTIVE) {
			mcux_pdm_cleanup(dev);
		}
		data->state = (cmd == DMIC_TRIGGER_PAUSE) ?
			DMIC_STATE_PAUSED : DMIC_STATE_CONFIGURED;
		break;

	case DMIC_TRIGGER_RESET:
		mcux_pdm_cleanup(dev);
		PDM_Deinit(data->base);
		data->state = DMIC_STATE_UNINIT;
		break;

	default:
		LOG_ERR("Invalid trigger %d", cmd);
		ret = -EINVAL;
		break;
	}

	irq_unlock(key);
	return ret;
}

static int mcux_pdm_read(const struct device *dev, uint8_t stream,
			 void **buffer, size_t *size, int32_t timeout)
{
	struct mcux_pdm_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (data->state == DMIC_STATE_ERROR) {
		LOG_ERR("Device in error state, reset and reconfigure");
		return -EIO;
	}

	if (data->state != DMIC_STATE_ACTIVE &&
	    data->state != DMIC_STATE_PAUSED &&
	    data->state != DMIC_STATE_CONFIGURED) {
		return -EIO;
	}

	ret = k_msgq_get(data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}

	*size = data->block_size;

	return 0;
}


static int mcux_pdm_init(const struct device *dev)
{
	const struct mcux_pdm_cfg *cfg = dev->config;
	struct mcux_pdm_data *data = dev->data;
	pdm_config_t pdm_config = {0};
	int ret;

	data->dev = dev;
	data->base = cfg->base;

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	if (cfg->clock_dev != NULL) {
		if (!device_is_ready(cfg->clock_dev)) {
			LOG_ERR("clock controller not ready");
			return -ENODEV;
		}
		ret = clock_control_on(cfg->clock_dev, cfg->clock_name);
		if (ret < 0) {
			LOG_ERR("Failed to enable clock (%d)", ret);
			return ret;
		}
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl (%d)", ret);
		return ret;
	}

	pdm_config.qualityMode = (pdm_df_quality_mode_t)cfg->quality_mode;
	pdm_config.fifoWatermark = cfg->fifo_watermark;
	pdm_config.cicOverSampleRate = cfg->cic_decimation_rate;

	PDM_Init(data->base, &pdm_config);

	data->state = DMIC_STATE_INITIALIZED;

	return 0;
}

static DEVICE_API(dmic, mcux_pdm_ops) = {
	.configure = mcux_pdm_configure,
	.trigger = mcux_pdm_trigger,
	.read = mcux_pdm_read,
};

/* Build per-hardware-channel arrays / masks from the DT child nodes. */
#define PDM_DC_CUTOFF_ITEM(node_id) \
	[DT_REG_ADDR(node_id)] = DT_PROP_OR(node_id, dc_remover_cutoff_freq, 0),
#define PDM_GAIN_ITEM(node_id) \
	[DT_REG_ADDR(node_id)] = DT_PROP_OR(node_id, decimation_filter_gain, 0),
#define PDM_CH_BIT(node_id) | BIT(DT_REG_ADDR(node_id))

#define MCUX_PDM_DEVICE(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
	K_MSGQ_DEFINE_STATIC_TYPE(mcux_pdm_msgq##inst, void *,				\
				  CONFIG_AUDIO_DMIC_MCUX_PDM_QUEUE_SIZE);		\
											\
	static struct mcux_pdm_data mcux_pdm_data_##inst = {				\
		.rx_queue = &mcux_pdm_msgq##inst,					\
		.state = DMIC_STATE_UNINIT,						\
	};										\
											\
	static const struct mcux_pdm_cfg mcux_pdm_cfg_##inst = {			\
		.base = (PDM_Type *)DT_INST_REG_ADDR(inst),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),			\
		.clock_name = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(inst, 0)),		\
		.dma_channel = DT_INST_DMAS_CELL_BY_IDX(inst, 0, mux),			\
		.dma_source = DT_INST_DMAS_CELL_BY_IDX(inst, 0, source),		\
		.quality_mode = DT_INST_PROP(inst, quality_mode),			\
		.cic_decimation_rate = DT_INST_PROP(inst, cic_decimation_rate),		\
		.fifo_watermark = DT_INST_PROP(inst, fifo_watermark),			\
		.sample_rate = DT_INST_PROP_OR(inst, sample_rate, 0),			\
		.chan_dc_cutoff = { DT_INST_FOREACH_CHILD_STATUS_OKAY(inst,		\
					PDM_DC_CUTOFF_ITEM) },				\
		.chan_gain = { DT_INST_FOREACH_CHILD_STATUS_OKAY(inst,			\
					PDM_GAIN_ITEM) },				\
		.ch_enabled_mask = (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst,		\
					PDM_CH_BIT)),					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, mcux_pdm_init, NULL,				\
			      &mcux_pdm_data_##inst, &mcux_pdm_cfg_##inst,		\
			      POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY,		\
			      &mcux_pdm_ops);

DT_INST_FOREACH_STATUS_OKAY(MCUX_PDM_DEVICE)
