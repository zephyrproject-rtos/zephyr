/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Ugo Marchand
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_dac_audio

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <soc.h>

LOG_MODULE_REGISTER(dac_audio_sam, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DAC_AUDIO_SAM_BYTES_PER_SAMPLE 2
#define DAC_AUDIO_SAM_RESOLUTION       12
#define DAC_AUDIO_SAM_PCM_SIGN_BIT     0x8000U
#define DAC_AUDIO_SAM_PCM_SHIFT        (16 - DAC_AUDIO_SAM_RESOLUTION)
#define DAC_AUDIO_SAM_SILENCE          (BIT(DAC_AUDIO_SAM_RESOLUTION) / 2U)
/* Reset value of the analog output current, which the datasheet ties to the conversion rate */
#define DAC_AUDIO_SAM_IBCTL            2

#define DAC_AUDIO_SAM_SAMPLES_PER_BLOCK                                                            \
	(CONFIG_AUDIO_CODEC_DAC_SAM_MAX_BLOCK_SIZE / DAC_AUDIO_SAM_BYTES_PER_SAMPLE)

BUILD_ASSERT((CONFIG_AUDIO_CODEC_DAC_SAM_MAX_BLOCK_SIZE % DAC_AUDIO_SAM_BYTES_PER_SAMPLE) == 0,
	     "CONFIG_AUDIO_CODEC_DAC_SAM_MAX_BLOCK_SIZE must be even: one sample is two bytes");

struct dac_audio_sam_config {
	Dacc *dacc;
	const struct device *dacc_dev;
	Tc *tc;
	const struct atmel_sam_pmc_config tc_clock_cfgs[TCCHANNEL_NUMBER];
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint32_t dma_slot;
	uint8_t dac_channel;
	uint8_t tc_channel;
};

struct dac_audio_sam_data {
	struct k_spinlock lock;
	audio_codec_tx_done_callback_t tx_cb;
	void *tx_cb_user_data;
	size_t block_size;
	uint8_t active_block;
	bool writable;
	bool ready;
	bool started;
	uint16_t dma_buf[2][DAC_AUDIO_SAM_SAMPLES_PER_BLOCK] __aligned(sizeof(uint32_t));
};

static void dac_audio_sam_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				       int status);

static uint32_t dac_audio_sam_cdr(const struct dac_audio_sam_config *dev_cfg)
{
	return (uint32_t)&dev_cfg->dacc->DACC_CDR[dev_cfg->dac_channel];
}

static void dac_audio_sam_fill_silence(uint16_t *samples, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		samples[i] = DAC_AUDIO_SAM_SILENCE;
	}
}

static int dac_audio_sam_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct dac_audio_sam_config *dev_cfg = dev->config;
	struct dac_audio_sam_data *dev_data = dev->data;
	Dacc *dacc = dev_cfg->dacc;
	TcChannel *tc_ch = &dev_cfg->tc->TcChannel[dev_cfg->tc_channel];

	if (cfg == NULL) {
		LOG_ERR("configure: no config");
		return -EINVAL;
	}

	if (dev_data->started) {
		LOG_ERR("configure: output running");
		return -EBUSY;
	}

	if (cfg->dai_cfg.pcm.dir != AUDIO_DAI_DIR_TX) {
		LOG_WRN("configure: dir 0x%x not supported, TX only", cfg->dai_cfg.pcm.dir);
		return -ENOTSUP;
	}

	if ((cfg->dai_type != AUDIO_DAI_TYPE_PCM) ||
	    (cfg->dai_cfg.pcm.pcm_width != AUDIO_PCM_WIDTH_16_BITS) ||
	    (cfg->dai_cfg.pcm.channels != 1)) {
		LOG_ERR("configure: need mono 16-bit PCM (dai %d, width %d, ch %u)", cfg->dai_type,
			cfg->dai_cfg.pcm.pcm_width, cfg->dai_cfg.pcm.channels);
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.pcm.block_size == 0) ||
	    (cfg->dai_cfg.pcm.block_size % DAC_AUDIO_SAM_BYTES_PER_SAMPLE != 0) ||
	    (cfg->dai_cfg.pcm.block_size > CONFIG_AUDIO_CODEC_DAC_SAM_MAX_BLOCK_SIZE)) {
		LOG_ERR("configure: bad block size %zu (max %d, must be even)",
			cfg->dai_cfg.pcm.block_size, CONFIG_AUDIO_CODEC_DAC_SAM_MAX_BLOCK_SIZE);
		return -EINVAL;
	}

	/*
	 * The TC is clocked by the undivided master clock, and its TIOA output
	 * goes high on the RC compare that also wraps the counter, so one
	 * period is RC + 1 ticks.
	 */
	uint32_t freq = SOC_ATMEL_SAM_MCK_FREQ_HZ;
	uint32_t rate = cfg->dai_cfg.pcm.samplerate;
	uint32_t ticks = (rate != 0U) ? (freq / rate) : 0U;

	/* The TC counter is 16 bit wide, and RC must stay above RA */
	if (!IN_RANGE(ticks, 3, BIT(16))) {
		LOG_ERR("configure: samplerate %u out of range for %u Hz timer", rate, freq);
		return -EINVAL;
	}

	if ((freq % rate) != 0U) {
		LOG_WRN("configure: samplerate %u not exact, running at %u Hz", rate, freq / ticks);
	}

	struct dac_channel_cfg dac_ch_cfg = {
		.channel_id = dev_cfg->dac_channel,
		.resolution = DAC_AUDIO_SAM_RESOLUTION,
	};
	int ret = dac_channel_setup(dev_cfg->dacc_dev, &dac_ch_cfg);

	if (ret < 0) {
		LOG_ERR("configure: DACC channel %u setup failed (%d)", dev_cfg->dac_channel, ret);
		return ret;
	}

	/* IBCTLCH0 and IBCTLCH1 are two bit fields sitting side by side */
	uint32_t ibctl_shift = 2U * dev_cfg->dac_channel;

	dacc->DACC_ACR = (dacc->DACC_ACR & ~(DACC_ACR_IBCTLCH0_Msk << ibctl_shift)) |
			 (DACC_ACR_IBCTLCH0(DAC_AUDIO_SAM_IBCTL) << ibctl_shift);

	/* TIOA0, TIOA1 and TIOA2 are trigger sources 1, 2 and 3 */
	uint32_t trigr = dacc->DACC_TRIGR;

	trigr &= ~(DACC_TRIGR_TRGSEL0_Msk << (4U * dev_cfg->dac_channel));
	trigr |= (uint32_t)(dev_cfg->tc_channel + 1U)
		 << (DACC_TRIGR_TRGSEL0_Pos + 4U * dev_cfg->dac_channel);
	trigr |= DACC_TRIGR_TRGEN0 << dev_cfg->dac_channel;
	dacc->DACC_TRIGR = trigr;

	tc_ch->TC_CCR = TC_CCR_CLKDIS;
	tc_ch->TC_CMR = TC_CMR_WAVE | TC_CMR_WAVEFORM_WAVSEL_UP_RC | TC_CMR_WAVEFORM_ACPA_CLEAR |
			TC_CMR_WAVEFORM_ACPC_SET;
	tc_ch->TC_EMR = TC_EMR_NODIVCLK;
	tc_ch->TC_RA = 1;
	tc_ch->TC_RC = ticks - 1U;

	dev_data->block_size = cfg->dai_cfg.pcm.block_size;

	LOG_INF("configured: %u Hz (%u / %u), block size %zu", freq / ticks, freq, ticks,
		dev_data->block_size);

	return 0;
}

static int dac_audio_sam_start(const struct device *dev, uint8_t dir)
{
	const struct dac_audio_sam_config *dev_cfg = dev->config;
	struct dac_audio_sam_data *dev_data = dev->data;
	size_t block_size = dev_data->block_size;

	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_WRN("start: dir 0x%x not supported, TX only", dir);
		return -ENOTSUP;
	}

	if (block_size == 0U) {
		LOG_ERR("start: not configured");
		return -EINVAL;
	}

	if (dev_data->started) {
		LOG_DBG("start: already running");
		return 0;
	}

	for (uint8_t block = 0U; block < ARRAY_SIZE(dev_data->dma_buf); block++) {
		dac_audio_sam_fill_silence(dev_data->dma_buf[block],
					   block_size / DAC_AUDIO_SAM_BYTES_PER_SAMPLE);
		sys_cache_data_flush_range(dev_data->dma_buf[block], block_size);
	}

	dev_data->active_block = 0U;

	struct dma_block_config dma_block = {
		.source_address = (uint32_t)dev_data->dma_buf[0],
		.dest_address = dac_audio_sam_cdr(dev_cfg),
		.block_size = block_size,
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
	};
	struct dma_config dma_cfg = {
		.dma_slot = dev_cfg->dma_slot,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = DAC_AUDIO_SAM_BYTES_PER_SAMPLE,
		.dest_data_size = DAC_AUDIO_SAM_BYTES_PER_SAMPLE,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.block_count = 1,
		.dma_callback = dac_audio_sam_dma_callback,
		.user_data = (void *)dev,
		/* The XDMAC only raises the end of block interrupt on request */
		.complete_callback_en = 1,
		.head_block = &dma_block,
	};

	int ret = dma_config(dev_cfg->dma_dev, dev_cfg->dma_channel, &dma_cfg);

	if (ret < 0) {
		LOG_ERR("start: dma_config ch %u failed (%d)", dev_cfg->dma_channel, ret);
		return ret;
	}

	ret = dma_start(dev_cfg->dma_dev, dev_cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("start: dma_start ch %u failed (%d)", dev_cfg->dma_channel, ret);
		return ret;
	}

	K_SPINLOCK(&dev_data->lock) {
		dev_data->writable = true;
		dev_data->ready = false;
		dev_data->started = true;
	}

	dev_cfg->tc->TcChannel[dev_cfg->tc_channel].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

	if (dev_data->tx_cb != NULL) {
		dev_data->tx_cb(dev, dev_data->tx_cb_user_data);
	} else {
		LOG_WRN("start: no tx callback registered, output stays silent");
	}

	return 0;
}

static void dac_audio_sam_start_output(const struct device *dev)
{
	(void)dac_audio_sam_start(dev, AUDIO_DAI_DIR_TX);
}

static void dac_audio_sam_stop_output(const struct device *dev)
{
	const struct dac_audio_sam_config *dev_cfg = dev->config;
	struct dac_audio_sam_data *dev_data = dev->data;

	if (!dev_data->started) {
		LOG_DBG("stop: already stopped");
		return;
	}

	dev_cfg->tc->TcChannel[dev_cfg->tc_channel].TC_CCR = TC_CCR_CLKDIS;

	int ret = dma_stop(dev_cfg->dma_dev, dev_cfg->dma_channel);

	if (ret < 0) {
		LOG_ERR("stop: dma_stop ch %u failed (%d)", dev_cfg->dma_channel, ret);
	}

	K_SPINLOCK(&dev_data->lock) {
		dev_data->started = false;
		dev_data->writable = false;
		dev_data->ready = false;
	}
}

static int dac_audio_sam_stop(const struct device *dev, uint8_t dir)
{
	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_WRN("stop: dir 0x%x not supported, TX only", dir);
		return -ENOTSUP;
	}

	dac_audio_sam_stop_output(dev);

	return 0;
}

static int dac_audio_sam_set_property(const struct device *dev, audio_property_t property,
				      audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(val);

	LOG_DBG("set_property: property %d (ch %d) not supported", property, channel);

	return -ENOTSUP;
}

static int dac_audio_sam_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int dac_audio_sam_write(const struct device *dev, uint8_t *data, size_t data_size)
{
	struct dac_audio_sam_data *dev_data = dev->data;
	size_t block_size = dev_data->block_size;

	if ((data == NULL) || (data_size == 0U) ||
	    (data_size % DAC_AUDIO_SAM_BYTES_PER_SAMPLE != 0) || (data_size > block_size)) {
		LOG_ERR("write: bad data %p size %zu (block %zu)", (void *)data, data_size,
			block_size);
		return -EINVAL;
	}

	uint16_t *dst = NULL;
	int ret = 0;

	K_SPINLOCK(&dev_data->lock) {
		if (!dev_data->started) {
			ret = -EIO;
			K_SPINLOCK_BREAK;
		}

		if (!dev_data->writable) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		dst = dev_data->dma_buf[dev_data->active_block ^ 1U];
		dev_data->writable = false;
	}

	if (ret != 0) {
		if (ret == -EIO) {
			LOG_ERR("write: output not started");
		} else {
			LOG_WRN_RATELIMIT("write: no free buffer, caller too early");
		}
		return ret;
	}

	size_t src_samples = data_size / DAC_AUDIO_SAM_BYTES_PER_SAMPLE;
	size_t dst_samples = block_size / DAC_AUDIO_SAM_BYTES_PER_SAMPLE;

	for (size_t i = 0; i < src_samples; i++) {
		uint16_t pcm = sys_get_le16(&data[DAC_AUDIO_SAM_BYTES_PER_SAMPLE * i]);

		dst[i] = (pcm ^ DAC_AUDIO_SAM_PCM_SIGN_BIT) >> DAC_AUDIO_SAM_PCM_SHIFT;
	}

	if (src_samples < dst_samples) {
		dac_audio_sam_fill_silence(&dst[src_samples], dst_samples - src_samples);
		LOG_DBG("write: %zu samples, %zu padded with silence", src_samples,
			dst_samples - src_samples);
	}

	sys_cache_data_flush_range(dst, block_size);

	/*
	 * Only hand the block over once it holds the samples. Until then the
	 * completion callback replays the block it already has, so it cannot
	 * leak half of this one to the DACC.
	 */
	K_SPINLOCK(&dev_data->lock) {
		if (dev_data->started) {
			dev_data->ready = true;
		}
	}

	return 0;
}

static int dac_audio_sam_register_done_callback(const struct device *dev,
						audio_codec_tx_done_callback_t tx_cb,
						void *tx_cb_user_data,
						audio_codec_rx_done_callback_t rx_cb,
						void *rx_cb_user_data)
{
	ARG_UNUSED(rx_cb);
	ARG_UNUSED(rx_cb_user_data);

	struct dac_audio_sam_data *dev_data = dev->data;

	dev_data->tx_cb = tx_cb;
	dev_data->tx_cb_user_data = tx_cb_user_data;

	LOG_DBG("tx callback %s", (tx_cb != NULL) ? "registered" : "cleared");

	return 0;
}

static void dac_audio_sam_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				       int status)
{
	const struct device *codec_dev = (const struct device *)user_data;
	const struct dac_audio_sam_config *dev_cfg = codec_dev->config;
	struct dac_audio_sam_data *dev_data = codec_dev->data;

	ARG_UNUSED(dev);

	/* The XDMAC driver reports its error interrupt flags as the status */
	if (status != 0) {
		LOG_ERR("dma ch %u error (0x%x), stopping output", channel, status);
		dac_audio_sam_stop_output(codec_dev);
		return;
	}

	uint16_t *next = NULL;
	bool underrun = false;
	bool notify = false;

	K_SPINLOCK(&dev_data->lock) {
		if (dev_data->ready) {
			dev_data->active_block ^= 1U;
			dev_data->ready = false;
			dev_data->writable = true;
		} else {
			/*
			 * Replay the block that just ended. The other one is
			 * either untouched or still being written, and must
			 * not reach the DACC.
			 */
			underrun = true;
		}

		next = dev_data->dma_buf[dev_data->active_block];
		notify = dev_data->writable;
	}

	if (underrun) {
		LOG_WRN_RATELIMIT("underrun: no block ready, playing the last one again");
	}

	int ret = dma_reload(dev_cfg->dma_dev, dev_cfg->dma_channel, (uint32_t)next,
			     dac_audio_sam_cdr(dev_cfg), dev_data->block_size);

	if (ret == 0) {
		ret = dma_start(dev_cfg->dma_dev, dev_cfg->dma_channel);
	}

	if (ret < 0) {
		LOG_ERR("dma ch %u reload failed (%d), stopping output", channel, ret);
		dac_audio_sam_stop_output(codec_dev);
		return;
	}

	if (notify && (dev_data->tx_cb != NULL)) {
		dev_data->tx_cb(codec_dev, dev_data->tx_cb_user_data);
	}
}

static int dac_audio_sam_init(const struct device *dev)
{
	const struct dac_audio_sam_config *dev_cfg = dev->config;

	if (!device_is_ready(dev_cfg->dacc_dev)) {
		LOG_ERR("init: DACC device %s not ready", dev_cfg->dacc_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(dev_cfg->dma_dev)) {
		LOG_ERR("init: DMA device %s not ready", dev_cfg->dma_dev->name);
		return -ENODEV;
	}

	const struct atmel_sam_pmc_config *tc_clock = &dev_cfg->tc_clock_cfgs[dev_cfg->tc_channel];

	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)tc_clock);

	return 0;
}

static DEVICE_API(audio_codec, dac_audio_sam_api) = {
	.configure = dac_audio_sam_configure,
	.start_output = dac_audio_sam_start_output,
	.stop_output = dac_audio_sam_stop_output,
	.set_property = dac_audio_sam_set_property,
	.apply_properties = dac_audio_sam_apply_properties,
	.start = dac_audio_sam_start,
	.stop = dac_audio_sam_stop,
	.write = dac_audio_sam_write,
	.register_done_callback = dac_audio_sam_register_done_callback,
};

#define DAC_AUDIO_SAM_TC(inst) DT_INST_PARENT(inst)

#define DAC_AUDIO_SAM_DEFINE(inst)                                                                 \
	BUILD_ASSERT(DT_INST_PROP(inst, channel) < TCCHANNEL_NUMBER,                               \
		     "channel out of range for the parent timer counter");                         \
                                                                                                   \
	static const struct dac_audio_sam_config dac_audio_sam_config_##inst = {                   \
		.dacc = (Dacc *)DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR(inst)),                       \
		.dacc_dev = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                         \
		.dac_channel = DT_INST_IO_CHANNELS_OUTPUT(inst),                                   \
		.tc = (Tc *)DT_REG_ADDR(DAC_AUDIO_SAM_TC(inst)),                                   \
		.tc_channel = DT_INST_PROP(inst, channel),                                         \
		.tc_clock_cfgs = SAM_DT_CLOCKS_PMC_CFG(DAC_AUDIO_SAM_TC(inst)),                    \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, tx)),                     \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(inst, tx, channel),                       \
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(inst, tx, perid),                            \
	};                                                                                         \
	static struct dac_audio_sam_data dac_audio_sam_data_##inst;                                \
	DEVICE_DT_INST_DEFINE(inst, dac_audio_sam_init, NULL, &dac_audio_sam_data_##inst,          \
			      &dac_audio_sam_config_##inst, POST_KERNEL,                           \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &dac_audio_sam_api)

DT_INST_FOREACH_STATUS_OKAY(DAC_AUDIO_SAM_DEFINE)
