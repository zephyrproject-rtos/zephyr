/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(dmic_nxp_micfil, CONFIG_AUDIO_DMIC_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_micfil

struct nxp_micfil_cfg {
	PDM_Type *base;
	uint8_t quality_mode;
	uint8_t fifo_watermark;
	uint8_t cic_decimation_rate;
	uint8_t chan_dc_cutoff[4];	/* dc remover cutoff frequency. */
	uint8_t chan_gain[4];		/* decimation filter gain. */
	uint8_t ch_enabled_mask;
	uint32_t sample_rate;		/* MICFIL sample rate */
	const struct device *clock_dev;
	clock_control_subsys_t clock_name;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
};

struct nxp_micfil_drv_data {
	const struct device *dev;
	PDM_Type *base;
	struct k_msgq *rx_msg_queue;
	volatile enum dmic_state state;
	uint8_t hw_chan[4];	/* Requested hardware channel order, maximun 4 channels. */
	uint8_t channels;	/* Current active channels count */
	uint8_t sample_bytes;	/* bytes per sample (4 for 32bits). */
	uint16_t block_size;
	struct k_mem_slab *mem_slab;
	void *active_buf;	/* Current active buffer */
	uint16_t write_off;	/* Bytes written into active_buf */
	uint8_t fifo_wm;	/* FIFO watermark */
};

static int nxp_micfil_configure(const struct device *dev, struct dmic_cfg *cfg_in)
{
	struct nxp_micfil_drv_data *data = dev->data;
	const struct nxp_micfil_cfg *cfg = dev->config;
	struct pdm_chan_cfg *chan = &cfg_in->channel;
	struct pcm_stream_cfg *stream = &cfg_in->streams[0];
	uint8_t act = 0U;
	uint8_t micfil_idx;
	enum pdm_lr lr;

	if (data->state == DMIC_STATE_ACTIVE) {
		return -EBUSY;
	}

	if (stream->pcm_rate == 0U || stream->pcm_width == 0U ||
		stream->mem_slab == NULL || stream->block_size == 0U) {
		return -EINVAL;
	}

	/* NXP MICFIL FIFO data width is 32-bit, only the 24 more significant bits
	 * have information, and the other bits are always 0. We output 32-bit PCM
	 * to keep alignment and simplify processing.
	 */
	if (stream->pcm_width != 32U) {
		LOG_ERR("Unsupported pcm width %u", stream->pcm_width);
		return -EINVAL;
	}

	if (chan->req_num_streams != 1U) {
		LOG_ERR("Only 1 stream supported");
		return -EINVAL;
	}

	/* Basic channel count sanity and support limit */
	if ((chan->req_num_chan == 0U) || (chan->req_num_chan > ARRAY_SIZE(data->hw_chan))) {
		LOG_ERR("Unsupported number of channels: %u", chan->req_num_chan);
		return -ENOTSUP;
	}

	/* Parse the requested logical channels and build HW channel list. */
	for (uint8_t index = 0U; index < chan->req_num_chan; index++) {
		dmic_parse_channel_map(chan->req_chan_map_lo, chan->req_chan_map_hi,
					index, &micfil_idx, &lr);

		/* New mapping model:
		 * - The micfil number in the map is used directly as the DMIC channel number,
		 *   which corresponds to the hardware DATACH index.
		 * - The lr value selects which side (Left/Right) that DMIC channel represents
		 *   within its stereo pair; adjacency/consecutiveness is validated later.
		 */
		uint8_t hw_chan = micfil_idx;

		if (hw_chan >= ARRAY_SIZE(data->hw_chan)) {
			LOG_ERR("Requested hw channel index %u exceeds supported %u",
					hw_chan, (uint32_t)ARRAY_SIZE(data->hw_chan));
			return -EINVAL;
		}

		if ((cfg->ch_enabled_mask & BIT(hw_chan)) == 0U) {
			LOG_ERR("Requested hw channel %u not enabled in DT", hw_chan);
			return -EINVAL;
		}

		/* Avoid duplicates */
		for (uint8_t i = 0U; i < act; i++) {
			if (data->hw_chan[i] == hw_chan) {
				LOG_ERR("Duplicate channel request for hw channel %u", hw_chan);
				return -EINVAL;
			}
		}

		data->hw_chan[act++] = hw_chan;
	}

	/* Ensure no extra mappings beyond req_num_chan are set */
	for (uint8_t index = chan->req_num_chan; index < 16U; index++) {
		uint32_t chan_map;

		if (index < 8U) {
			chan_map = (chan->req_chan_map_lo >> (index * 4U)) & 0xFU;
		} else {
			chan_map = (chan->req_chan_map_hi >> ((index - 8U) * 4U)) & 0xFU;
		}
		if (chan_map != 0U) {
			LOG_ERR("Extra mapping present for logical channel %u", index);
			return -EINVAL;
		}
	}

	/* Validate adjacency for each stereo pair (L/R in any order)
	 * New model requires paired dmics to use consecutive DMIC channel numbers
	 * (e.g., 0/1, 2/3, ...), not the same micfil number. This preserves the API
	 * constraint that L and R are adjacent while allowing explicit control
	 * over which channel number is Left/Right.
	 */
	for (uint8_t index = 0U; index + 1U < chan->req_num_chan; index += 2U) {
		uint8_t micfil0, micfil1;
		enum pdm_lr lr0, lr1;

		dmic_parse_channel_map(chan->req_chan_map_lo, chan->req_chan_map_hi,
					index, &micfil0, &lr0);
		dmic_parse_channel_map(chan->req_chan_map_lo, chan->req_chan_map_hi,
					index + 1U, &micfil1, &lr1);

		if (lr0 == lr1) {
			LOG_ERR("Pair %u/%u has same L/R selection", index, index + 1U);
			return -EINVAL;
		}
		/* Require consecutive DMIC channel numbers within a pair (e.g., 0/1, 2/3).
		 * Enforce that the smaller of the two is even to avoid crossing pairs (e.g., 1/2).
		 */
		uint8_t minp = MIN(micfil0, micfil1);
		uint8_t maxp = MAX(micfil0, micfil1);

		if (!((maxp == (uint8_t)(minp + 1U)) && ((minp & 0x1U) == 0U))) {
			LOG_ERR("Pair %u/%u must map to consecutive DMIC channels.",
				index, index + 1U);
			return -EINVAL;
		}
	}

	if (act == 0U) {
		LOG_ERR("No channels requested");
		return -EINVAL;
	}

	data->channels = act;
	data->sample_bytes = stream->pcm_width / 8U;
	data->block_size = stream->block_size;
	data->mem_slab = stream->mem_slab;

	/* Validate block_size alignment to complete frames */
	uint32_t frame_bytes = (uint32_t)data->channels * (uint32_t)data->sample_bytes;

	if ((data->block_size % frame_bytes) != 0U) {
		LOG_ERR("block_size %u not aligned to frame size %u (channels=%u)",
			data->block_size, (uint32_t)frame_bytes, data->channels);
		return -EINVAL;
	}

	/* Populate act_* fields according to accepted configuration */
	chan->act_num_streams = 1U;
	chan->act_num_chan = chan->req_num_chan;
	chan->act_chan_map_lo = chan->req_chan_map_lo;
	chan->act_chan_map_hi = chan->req_chan_map_hi;

	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

/**
 * @brief Start MICFIL capture:
 *   1. Allocate first buffer
 *   2. Clear pending status
 *   3. Configure FIFO interrupt
 *   4. Enable requested channels
 *   5. Enable MICFIL interface.
 */
static int nxp_micfil_start_capture(struct nxp_micfil_drv_data *data)
{
	void *buf = NULL;

	if (k_mem_slab_alloc(data->mem_slab, &buf, K_NO_WAIT) != 0) {
		return -ENOMEM;
	}
	data->active_buf = buf;
	data->write_off = 0;

	/* Clear any pending status before enabling data interrupts */
	uint32_t st = data->base->FIFO_STAT;

	if (st) {
		data->base->FIFO_STAT = st;
	}
	st = data->base->STAT;
	if (st) {
		data->base->STAT = st;
	}

	/* Enable data FIFO watermark interrupts only (DISEL=2). */
	data->base->CTRL_1 = ((data->base->CTRL_1 & ~PDM_CTRL_1_DISEL_MASK) |
			PDM_CTRL_1_DISEL(2U));

	/* Enable the requested channels */
	for (uint8_t index = 0; index < data->channels; index++) {
		data->base->CTRL_1 |= BIT(data->hw_chan[index]);
	}

	/* Enable MICFIL. */
	data->base->CTRL_1 |= PDM_CTRL_1_PDMIEN_MASK;

	data->state = DMIC_STATE_ACTIVE;

	return 0;
}

/**
 * @brief Stop/Pause/Reset MICFIL capture and clean up buffers/queues.
 */
static void nxp_micfil_stop_or_reset(struct nxp_micfil_drv_data *data, enum dmic_trigger cmd)
{
	/* Check if we are in a state that can be stopped/paused/reset */
	if (data->state == DMIC_STATE_ACTIVE || data->state == DMIC_STATE_PAUSED ||
		data->state == DMIC_STATE_ERROR) {
		/* Disable MICFIL */
		data->base->CTRL_1 &= ~PDM_CTRL_1_PDMIEN_MASK;

		/* Disable the requested channels */
		for (uint8_t index = 0; index < data->channels; index++) {
			data->base->CTRL_1 &= ~(BIT(data->hw_chan[index]));
		}

		/* Disable fifo interrupts. */
		data->base->CTRL_1 &= ~PDM_CTRL_1_DISEL_MASK;

		/* Set state early so any in-flight ISR bails out */
		data->state = (cmd == DMIC_TRIGGER_RESET) ? DMIC_STATE_UNINIT :
				DMIC_STATE_CONFIGURED;

		/* Clear any pending status flags */
		uint32_t st = data->base->FIFO_STAT;

		if (st) {
			data->base->FIFO_STAT = st;
		}

		st = data->base->STAT;

		if (st) {
			data->base->STAT = st;
		}
	}

	/* Free active buffer if any */
	if (data->active_buf) {
		void *tmp = data->active_buf;

		data->active_buf = NULL;
		k_mem_slab_free(data->mem_slab, tmp);
	}

	/* Drain and free any queued buffers that were filled
	 * but not yet read to avoid leaks.
	 */
	if (data->rx_msg_queue) {
		void *queued;

		while (k_msgq_get(data->rx_msg_queue, &queued, K_NO_WAIT) == 0) {
			k_mem_slab_free(data->mem_slab, queued);
		}
	}
}

static int nxp_micfil_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct nxp_micfil_drv_data *data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_START:
	case DMIC_TRIGGER_RELEASE: {
		/* Check if we are in a state that can be started/released */
		if (data->state != DMIC_STATE_CONFIGURED &&
			data->state != DMIC_STATE_PAUSED) {
			return -EIO;
		}

		int ret = nxp_micfil_start_capture(data);

		if (ret) {
			LOG_ERR("Failed to start capture: %d", ret);
			return ret;
		}

		break;
	}

	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
	case DMIC_TRIGGER_RESET: {
		nxp_micfil_stop_or_reset(data, cmd);

		break;
	}

	default:
		return -EINVAL;
	}

	return 0;
}

static int nxp_micfil_read(const struct device *dev, uint8_t stream,
			void **buffer, size_t *size, int32_t timeout)
{
	struct nxp_micfil_drv_data *data = dev->data;

	ARG_UNUSED(stream);

	/* Check if we are in a state that can read */
	if (data->state != DMIC_STATE_ACTIVE && data->state != DMIC_STATE_PAUSED) {
		return -EIO;
	}

	/* Get the filled buffer from the queue */
	int ret = k_msgq_get(data->rx_msg_queue, buffer, SYS_TIMEOUT_MS(timeout));

	if (ret == 0) {
		*size = data->block_size;
		return 0;
	}

	/* Fallback: if active but no IRQ-produced data arrived within timeout,
	 * return a zero-filled block so API semantics (non-timeout) are satisfied.
	 */
	if (data->state == DMIC_STATE_ACTIVE) {
		void *buf = NULL;
		static uint32_t last_warn_ms;

		if (k_mem_slab_alloc(data->mem_slab, &buf, K_NO_WAIT) != 0) {
			return ret; /* original error */
		}
		/* Provide silence */
		(void)memset(buf, 0, data->block_size);

		uint32_t now = k_uptime_get_32();

		if ((now - last_warn_ms) > 1000U) {
			LOG_ERR("DMIC fallback: no IRQ data yet, returning silence\n");
			last_warn_ms = now;
		}

		*buffer = buf;
		*size = data->block_size;

		return 0;
	}

	return ret;
}

static void nxp_micfil_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct nxp_micfil_drv_data *data = dev->data;
	uint32_t state = data->base->FIFO_STAT;

	/* Clear status flags */
	if (state) {
		data->base->FIFO_STAT = state;
	}

	state = data->base->STAT;

	if (state) {
		data->base->STAT = state;
	}

	/* Check if we are in a state that can read */
	if (!data->active_buf || data->state != DMIC_STATE_ACTIVE) {
		return;
	}

	/* Read data from all enabled channels */
	uint8_t *dst = (uint8_t *)data->active_buf;
	/* Each frame is channels * sample_bytes. Hardware returns 32-bit samples in DATACH[].
	 * We output little-endian 32-bit PCM: LSB first, 4 bytes per sample.
	 */
	uint16_t frame_bytes = (uint16_t)(data->channels * data->sample_bytes);
	/* Calculate how many complete frames are left in the current block that can be
	 * written. This limits the maximum number of frames that this ISR can take from
	 * the hardware FIFO to avoid write overflow and maintain frame alignment.
	 */
	uint16_t frames_remaining = (uint16_t)((data->block_size - data->write_off) / frame_bytes);
	/* Read up to fifo_wm frames. */
	uint16_t frames_to_read = (uint16_t)data->fifo_wm;

	/* Adjust if more frames than remaining in buffer */
	if (frames_to_read > frames_remaining) {
		frames_to_read = frames_remaining;
	}

	/* Read frames from all active channels' FIFO. */
	for (uint16_t frame = 0; frame < frames_to_read; frame++) {
		for (uint8_t chan = 0; chan < data->channels; chan++) {
			/* Read one 32-bit sample from the selected hardware channel FIFO. */
			uint8_t hw = data->hw_chan[chan];
			volatile uint32_t raw_data = data->base->DATACH[hw];

			dst[data->write_off + 0] = (uint8_t)(raw_data & 0xFFU);
			dst[data->write_off + 1] = (uint8_t)((raw_data >> 8U) & 0xFFU);
			dst[data->write_off + 2] = (uint8_t)((raw_data >> 16U) & 0xFFU);
			dst[data->write_off + 3] = (uint8_t)((raw_data >> 24U) & 0xFFU);

			data->write_off += 4;
		}
	}

	/* Check if active buffer is full. Hand off to queue and rotate buffers safely. */
	if (data->write_off >= data->block_size) {
		void *completed = data->active_buf;
		void *new_buf = NULL;

		/* Allocate next buffer first to avoid using a freed buffer */
		if (k_mem_slab_alloc(data->mem_slab, &new_buf, K_NO_WAIT) != 0) {
			/* No memory available: enter error state and stop capturing */
			data->active_buf = NULL;
			data->state = DMIC_STATE_ERROR;

			/* Disable MICFIL */
			data->base->CTRL_1 &= ~PDM_CTRL_1_PDMIEN_MASK;

			/* Disable the requested channels */
			for (uint8_t index = 0; index < data->channels; index++) {
				data->base->CTRL_1 &= ~(BIT(data->hw_chan[index]));
			}

			/* Disable fifo interrupts. */
			data->base->CTRL_1 &= ~PDM_CTRL_1_DISEL_MASK;
			return;
		}

		/* Try to enqueue the completed buffer. If queue is full, free it. */
		if (k_msgq_put(data->rx_msg_queue, &completed, K_NO_WAIT) != 0) {
			k_mem_slab_free(data->mem_slab, completed);
		}

		/* Switch to the new active buffer */
		data->active_buf = new_buf;
		data->write_off = 0;
	}
}

static int nxp_micfil_init(const struct device *dev)
{
	const struct nxp_micfil_cfg *cfg = dev->config;
	struct nxp_micfil_drv_data *data = dev->data;
	uint32_t clk_rate = 0U;
	int ret;

	data->dev = dev;
	data->base = cfg->base;

	if (cfg->clock_dev != NULL) {
		ret = clock_control_on(cfg->clock_dev, cfg->clock_name);
		if (ret) {
			LOG_ERR("Device clock turn on failed");
			return ret;
		}

		ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_name, &clk_rate);
		if (ret < 0) {
			LOG_WRN("Device clock rate not available (%d)", ret);
		}
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure pins (%d)", ret);
		return ret;
	}

	if (cfg->irq_config_func) {
		cfg->irq_config_func(dev);
	}

	/* MICFIL initialization. */
	/* Ensure module is enabled and interface/interrupts/channels
	 * are disabled before config.
	 */
	data->base->CTRL_1 &= ~(PDM_CTRL_1_MDIS_MASK | PDM_CTRL_1_PDMIEN_MASK |
		PDM_CTRL_1_ERREN_MASK);
	/* TODO: Use DT property instead of hardcoding channel numbers. */
	for (uint8_t ch = 0U; ch < 4U; ch++) {
		data->base->CTRL_1 &= ~BIT(ch);
	}

	/* Wait until all filters stopped if supported. */
	while ((data->base->STAT & PDM_STAT_BSY_FIL_MASK) != 0U) {
	}

	/* Do a software reset pulse before configuration. */
	data->base->CTRL_1 |= PDM_CTRL_1_SRES_MASK;

	/* Configure quality mode, CIC decimation rate. */
	data->base->CTRL_2 &= ~(PDM_CTRL_2_QSEL_MASK | PDM_CTRL_2_CICOSR_MASK);
	data->base->CTRL_2 |= PDM_CTRL_2_QSEL(cfg->quality_mode) |
			PDM_CTRL_2_CICOSR(cfg->cic_decimation_rate);

	/* Configure FIFO watermark. */
	data->base->FIFO_CTRL = (data->base->FIFO_CTRL & ~PDM_FIFO_CTRL_FIFOWMK_MASK) |
		PDM_FIFO_CTRL_FIFOWMK(cfg->fifo_watermark);

	/* Cache FIFO watermark for ISR. */
	data->fifo_wm = cfg->fifo_watermark;

	/* MICFIL channels initialization. */
	/* Configure DC remover cutoff per hardware channel. */
	for (uint8_t ch = 0U; ch < ARRAY_SIZE(cfg->chan_dc_cutoff); ch++) {
		uint32_t mask = PDM_DC_CTRL_DCCONFIG0_MASK << (ch * 2U);
		uint32_t val = (cfg->chan_dc_cutoff[ch] & PDM_DC_CTRL_DCCONFIG0_MASK) << (ch * 2U);

		data->base->DC_OUT_CTRL = ((data->base->DC_OUT_CTRL & ~mask) | val);
	}

	/* Configure decimation-filter-gain per hardware channel. */
	for (uint8_t ch = 0U; ch < ARRAY_SIZE(cfg->chan_gain); ch++) {
		uint32_t mask = PDM_RANGE_CTRL_RANGEADJ0_MASK << (ch * 4U);
		uint32_t val = (cfg->chan_gain[ch] & PDM_RANGE_CTRL_RANGEADJ0_MASK) << (ch * 4U);

		data->base->RANGE_CTRL = ((data->base->RANGE_CTRL & ~mask) | val);
	}

	/* Configure clock divider if clock rate and sample rate are known. */
	if (clk_rate != 0U && cfg->sample_rate != 0U) {
		uint32_t osr_reg_max = (PDM_CTRL_2_CICOSR_MASK >> PDM_CTRL_2_CICOSR_SHIFT);

		if (cfg->cic_decimation_rate > osr_reg_max) {
			LOG_ERR("CIC decimation rate %u exceeds max %u",
				cfg->cic_decimation_rate, (uint32_t)osr_reg_max);
			return -EINVAL;
		}

		/* Real OSR per MCUX SDK: (max + 1 - programmed). */
		uint32_t real_osr = osr_reg_max + 1U - (uint32_t)cfg->cic_decimation_rate;
		uint32_t micfil_clock_rate = cfg->sample_rate * real_osr * 8U;

		if (clk_rate < micfil_clock_rate) {
			LOG_ERR("Clock rate %u too low for sample rate %u (OSR=%u)",
					clk_rate, cfg->sample_rate, real_osr);
			return -EINVAL;
		}

		uint32_t reg_div = clk_rate / micfil_clock_rate;

		if (reg_div == 0U) {
			reg_div = 1U;
		}

		uint32_t clkdiv_max = (PDM_CTRL_2_CLKDIV_MASK >> PDM_CTRL_2_CLKDIV_SHIFT);

		if (reg_div > clkdiv_max) {
			LOG_WRN("CLKDIV %u exceeds max %u, clamping", reg_div, clkdiv_max);
			reg_div = clkdiv_max;
		}

		data->base->CTRL_2 = (data->base->CTRL_2 & (~PDM_CTRL_2_CLKDIV_MASK)) |
				PDM_CTRL_2_CLKDIV(reg_div);

		LOG_INF("MICFIL clk=%uHz sample=%u OSR=%u div=%u wm=%u",
			clk_rate, cfg->sample_rate, real_osr, reg_div, cfg->fifo_watermark);
	} else {
		LOG_WRN("Clock rate or sample rate is zero, cannot set clock divider");
	}

	data->state = DMIC_STATE_INITIALIZED;

	return 0;
}

static const struct _dmic_ops dmic_ops = {
	.configure = nxp_micfil_configure,
	.trigger = nxp_micfil_trigger,
	.read = nxp_micfil_read,
};

#define NXP_MICFIL_IRQ_CONFIG(inst)							\
	static void _CONCAT(irq_config, inst)(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),		\
			nxp_micfil_isr, DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));						\
	}

/* Build per-hardware-channel DC cutoff array from DT child nodes. */
#define DC_CUTOFF_ITEM(node_id) [DT_REG_ADDR(node_id)] =				\
			DT_PROP(node_id, dc_remover_cutoff_freq),
/* Build per-hardware-channel OUT gain array from DT child nodes. */
#define OUT_GAIN_ITEM(node_id) [DT_REG_ADDR(node_id)] =					\
			DT_PROP_OR(node_id, decimation_filter_gain, 0),
/* Build bitmask of enabled channels by OR-ing BIT(reg) per child. */
#define CH_BIT(node_id) | BIT(DT_REG_ADDR(node_id))

#define NXP_MICFIL_DEFINE(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
	K_MSGQ_DEFINE(nxp_micfil_msgq##inst, sizeof(void *),				\
			CONFIG_DMIC_NXP_MICFIL_QUEUE_SIZE, 4);				\
											\
	NXP_MICFIL_IRQ_CONFIG(inst)							\
											\
	static struct nxp_micfil_drv_data _CONCAT(data, inst) = {			\
		.rx_msg_queue = &nxp_micfil_msgq##inst,					\
		.state = DMIC_STATE_UNINIT,						\
	};										\
											\
	static const struct nxp_micfil_cfg _CONCAT(cfg, inst) = {			\
		.base = (PDM_Type *)DT_INST_REG_ADDR(inst),				\
		.quality_mode = DT_INST_PROP(inst, quality_mode),			\
		.fifo_watermark = DT_INST_PROP(inst, fifo_watermark),			\
		.cic_decimation_rate = DT_INST_PROP(inst, cic_decimation_rate),		\
		.chan_dc_cutoff = { DT_INST_FOREACH_CHILD_STATUS_OKAY(inst,		\
					DC_CUTOFF_ITEM) },				\
		.chan_gain = { DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, OUT_GAIN_ITEM) },\
		.ch_enabled_mask = (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, CH_BIT)),	\
		.sample_rate = DT_INST_PROP(inst, sample_rate),				\
		.clock_dev = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(inst)),		\
		.clock_name = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst,	name),	\
		.irq_config_func = _CONCAT(irq_config, inst),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, nxp_micfil_init, NULL,				\
			&_CONCAT(data, inst), &_CONCAT(cfg, inst),			\
			POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_ops);

DT_INST_FOREACH_STATUS_OKAY(NXP_MICFIL_DEFINE)
