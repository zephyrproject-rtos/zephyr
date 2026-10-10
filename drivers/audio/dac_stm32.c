/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Draeger
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dac_audio

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <soc.h>
#include <stm32_ll_dac.h>
#include <stm32_ll_tim.h>

LOG_MODULE_REGISTER(stm32_dac, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define STM32_DAC_BYTES_PER_SAMPLE 2
#define STM32_DAC_PCM_SIGN_BIT     0x8000U

#define STM32_DAC_SAMPLES_PER_BLOCK                                                                \
	(CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE / STM32_DAC_BYTES_PER_SAMPLE)

BUILD_ASSERT((CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE % STM32_DAC_BYTES_PER_SAMPLE) == 0,
	     "CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE must be even: one sample is two bytes");

BUILD_ASSERT(IN_RANGE(CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE, 2, 32766),
	     "block size must be in range 2, 32766");

struct dac_stm32_cfg {
	DAC_TypeDef *dac_base;
	uint32_t dac_ll_channel;
	uint32_t dac_trigger;
	TIM_TypeDef *tim_base;
	const struct device *counter_dev;
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint32_t dma_slot;
};

struct dac_stm32_data {
	struct audio_codec_cfg config;
	audio_codec_tx_done_callback_t tx_cb;
	void *tx_cb_user_data;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block;
	uint16_t dma_buf[2 * STM32_DAC_SAMPLES_PER_BLOCK];
	uint16_t *write_buf;
	bool writable;
	bool started;
};

static int dac_stm32_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct dac_stm32_cfg *dev_cfg = dev->config;
	struct dac_stm32_data *dev_data = dev->data;

	if (cfg == NULL) {
		LOG_ERR("configure: no config");
		return -EINVAL;
	}

	if (dev_data->started) {
		LOG_ERR("configure: output running");
		return -EBUSY;
	}

	if ((cfg->dai_type != AUDIO_DAI_TYPE_PCM) ||
	    (cfg->dai_cfg.pcm.pcm_width != AUDIO_PCM_WIDTH_16_BITS) ||
	    (cfg->dai_cfg.pcm.channels != 1)) {
		LOG_ERR("configure: need mono 16-bit PCM (dai %d, width %d, ch %" PRIu8 ")",
			cfg->dai_type, cfg->dai_cfg.pcm.pcm_width, cfg->dai_cfg.pcm.channels);
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.pcm.block_size == 0) ||
	    (cfg->dai_cfg.pcm.block_size % STM32_DAC_BYTES_PER_SAMPLE != 0) ||
	    (cfg->dai_cfg.pcm.block_size > CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE)) {
		LOG_ERR("configure: bad block size %zu (max %d, must be even)",
			cfg->dai_cfg.pcm.block_size, CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE);
		return -EINVAL;
	}

	uint32_t freq = counter_get_frequency(dev_cfg->counter_dev);
	uint32_t rate = cfg->dai_cfg.pcm.samplerate;

	if ((rate == 0U) || (rate > freq)) {
		LOG_ERR("configure: samplerate %" PRIu32 " out of range for %" PRIu32 " Hz timer",
			rate, freq);
		return -EINVAL;
	}

	uint32_t ticks = freq / rate;

	if ((freq % rate) != 0U) {
		LOG_WRN("configure: samplerate %" PRIu32 " not exact, running at %" PRIu32 " Hz",
			rate, freq / ticks);
	}

	struct counter_top_cfg top_cfg = {
		.ticks = ticks - 1U,
		.callback = NULL,
		.flags = 0,
	};

	int ret = counter_set_top_value(dev_cfg->counter_dev, &top_cfg);

	if (ret < 0) {
		LOG_ERR("configure: counter top %" PRIu32 " failed (%d)", top_cfg.ticks, ret);
		return ret;
	}

	LL_TIM_SetUpdateSource(dev_cfg->tim_base, LL_TIM_UPDATESOURCE_COUNTER);
	LL_TIM_SetTriggerOutput(dev_cfg->tim_base, LL_TIM_TRGO_UPDATE);

	LL_DAC_SetOutputBuffer(dev_cfg->dac_base, dev_cfg->dac_ll_channel,
			       LL_DAC_OUTPUT_BUFFER_ENABLE);
	LL_DAC_SetTriggerSource(dev_cfg->dac_base, dev_cfg->dac_ll_channel, dev_cfg->dac_trigger);
	LL_DAC_EnableTrigger(dev_cfg->dac_base, dev_cfg->dac_ll_channel);
	LL_DAC_EnableDMAReq(dev_cfg->dac_base, dev_cfg->dac_ll_channel);
	LL_DAC_Enable(dev_cfg->dac_base, dev_cfg->dac_ll_channel);

	dev_data->config = *cfg;

	LOG_INF("configured: %" PRIu32 " Hz (%" PRIu32 " / %" PRIu32 "), block size %zu",
		freq / ticks, freq, ticks, cfg->dai_cfg.pcm.block_size);

	return 0;
}

static void dac_stm32_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				   int status);

static void dac_stm32_start_output(const struct device *dev)
{
	const struct dac_stm32_cfg *dev_cfg = dev->config;
	struct dac_stm32_data *dev_data = dev->data;

	if (!dev_data->config.dai_type != AUDIO_DAI_TYPE_PCM) {
		LOG_ERR("start: not configured");
		return;
	}

	if (dev_data->started) {
		LOG_DBG("start: already running");
		return;
	}

	dev_data->dma_block = (struct dma_block_config){
		.source_address = (uint32_t)dev_data->dma_buf,
		.dest_address = LL_DAC_DMA_GetRegAddr(dev_cfg->dac_base, dev_cfg->dac_ll_channel,
						      LL_DAC_DMA_REG_DATA_12BITS_LEFT_ALIGNED),
		.block_size = 2U * dev_data->config.dai_cfg.pcm.block_size,
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		/*
		 * for historical reasons, the STM32 driver
		 * needs this in addition to below cyclic=true
		 */
		.source_reload_en = 1,
		.dest_reload_en = 1,
	};

	dev_data->dma_cfg.head_block = &dev_data->dma_block;

	int ret = dma_config(dev_cfg->dma_dev, dev_cfg->dma_channel, &dev_data->dma_cfg);

	if (ret < 0) {
		LOG_ERR("start: dma_config ch %" PRIu32 " failed (%d)", dev_cfg->dma_channel, ret);
		return;
	}

	ret = dma_start(dev_cfg->dma_dev, dev_cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("start: dma_start ch %" PRIu32 " failed (%d)", dev_cfg->dma_channel, ret);
		return;
	}

	ret = counter_start(dev_cfg->counter_dev);
	if (ret < 0) {
		LOG_ERR("start: counter_start failed (%d)", ret);
		dma_stop(dev_cfg->dma_dev, dev_cfg->dma_channel);
		return;
	}

	dev_data->write_buf = dev_data->dma_buf;
	dev_data->writable = true;
	dev_data->started = true;

	if (dev_data->tx_cb != NULL) {
		dev_data->tx_cb(dev, dev_data->tx_cb_user_data);
	} else {
		LOG_WRN("start: no tx callback registered, output stays silent");
	}
}

static void dac_stm32_stop_output(const struct device *dev)
{
	const struct dac_stm32_cfg *dev_cfg = dev->config;
	struct dac_stm32_data *dev_data = dev->data;

	if (!dev_data->started) {
		LOG_DBG("stop: already stopped");
		return;
	}

	int ret = counter_stop(dev_cfg->counter_dev);

	if (ret < 0) {
		LOG_ERR("stop: counter_stop failed (%d)", ret);
	}

	ret = dma_stop(dev_cfg->dma_dev, dev_cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("stop: dma_stop ch %" PRIu32 " failed (%d)", dev_cfg->dma_channel, ret);
	}

	dev_data->started = false;
	dev_data->writable = false;
}

static int dac_stm32_set_property(const struct device *dev, audio_property_t property,
				  audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(val);

	LOG_WRN("set_property: property %d (ch %d) ignored, none supported", property, channel);

	return 0;
}

static int dac_stm32_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int dac_stm32_start(const struct device *dev, uint8_t dir)
{
	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_ERR("start: dir %" PRIu8 " not supported, TX only", dir);
		return -ENOTSUP;
	}

	dac_stm32_start_output(dev);

	return 0;
}

static int dac_stm32_stop(const struct device *dev, uint8_t dir)
{
	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_ERR("stop: dir %" PRIu8 " not supported, TX only", dir);
		return -ENOTSUP;
	}

	dac_stm32_stop_output(dev);

	return 0;
}

static int dac_stm32_write(const struct device *dev, uint8_t *data, size_t data_size)
{
	struct dac_stm32_data *dev_data = dev->data;

	if ((data == NULL) || (data_size == 0U) || (data_size % STM32_DAC_BYTES_PER_SAMPLE != 0) ||
	    (data_size > dev_data->config.dai_cfg.pcm.block_size)) {
		LOG_ERR("write: bad data %p size %zu (block %zu)", (void *)data, data_size,
			dev_data->config.dai_cfg.pcm.block_size);
		return -EINVAL;
	}

	if (!dev_data->started) {
		LOG_ERR("write: output not started");
		return -EIO;
	}

	if (!dev_data->writable) {
		LOG_WRN_RATELIMIT("write: no free buffer, caller too early");
		return -EBUSY;
	}

	uint16_t *dst = dev_data->write_buf;
	size_t src_samples = data_size / STM32_DAC_BYTES_PER_SAMPLE;
	size_t dst_samples = dev_data->config.dai_cfg.pcm.block_size / STM32_DAC_BYTES_PER_SAMPLE;

	for (size_t i = 0; i < src_samples; i++) {
		dst[i] = sys_get_le16(&data[2 * i]) ^ STM32_DAC_PCM_SIGN_BIT;
	}

	if (src_samples < dst_samples) {
		memset(&dst[src_samples], 0, (dst_samples - src_samples) * sizeof(*dst));
		LOG_DBG("write: %zu samples, %zu padded with silence", src_samples,
			dst_samples - src_samples);
	}

	dev_data->writable = false;

	return 0;
}

static int dac_stm32_register_done_callback(const struct device *dev,
					    audio_codec_tx_done_callback_t tx_cb,
					    void *tx_cb_user_data,
					    audio_codec_rx_done_callback_t rx_cb,
					    void *rx_cb_user_data)
{
	ARG_UNUSED(rx_cb);
	ARG_UNUSED(rx_cb_user_data);

	struct dac_stm32_data *dev_data = dev->data;

	dev_data->tx_cb = tx_cb;
	dev_data->tx_cb_user_data = tx_cb_user_data;

	LOG_DBG("tx callback %s", (tx_cb != NULL) ? "registered" : "cleared");

	return 0;
}

static void dac_stm32_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				   int status)
{
	const struct device *codec_dev = (const struct device *)user_data;
	struct dac_stm32_data *dev_data = codec_dev->data;

	if (status < 0) {
		LOG_ERR("dma ch %" PRIu32 " error (%d), stopping output", channel, status);
		dac_stm32_stop_output(codec_dev);
		return;
	}

	if (dev_data->writable) {
		LOG_WRN_RATELIMIT("underrun: block not written in time, stale samples played");
	}

	size_t write_index = 0;

	if (status == DMA_STATUS_BLOCK) {
		write_index = dev_data->config.dai_cfg.pcm.block_size / STM32_DAC_BYTES_PER_SAMPLE;
	} else if (status != DMA_STATUS_COMPLETE) {
		LOG_WRN_RATELIMIT("dma ch %" PRIu32 " unexpected status %d", channel, status);
	}

	dev_data->write_buf = &(dev_data->dma_buf[write_index]);
	dev_data->writable = true;

	if (dev_data->tx_cb != NULL) {
		dev_data->tx_cb(codec_dev, dev_data->tx_cb_user_data);
	}
}

static int dac_stm32_init(const struct device *dev)
{
	const struct dac_stm32_cfg *dev_cfg = dev->config;
	struct dac_stm32_data *dev_data = dev->data;

	if (!device_is_ready(dev_cfg->dma_dev)) {
		LOG_ERR("init: DMA device %s not ready", dev_cfg->dma_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(dev_cfg->counter_dev)) {
		LOG_ERR("init: counter device %s not ready", dev_cfg->counter_dev->name);
		return -ENODEV;
	}

	memset(dev_data, 0, sizeof(*dev_data));

	dev_data->dma_cfg = (struct dma_config){
		.dma_slot = dev_cfg->dma_slot,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = STM32_DAC_BYTES_PER_SAMPLE,
		.dest_data_size = STM32_DAC_BYTES_PER_SAMPLE,
		.block_count = 1,
		.head_block = NULL,
		.dma_callback = dac_stm32_dma_callback,
		.user_data = (void *)dev,
		.cyclic = true,
	};

	return 0;
}

static DEVICE_API(audio_codec, dac_stm32_api) = {
	.configure = dac_stm32_configure,
	.start_output = dac_stm32_start_output,
	.stop_output = dac_stm32_stop_output,
	.set_property = dac_stm32_set_property,
	.apply_properties = dac_stm32_apply_properties,
	.start = dac_stm32_start,
	.stop = dac_stm32_stop,
	.write = dac_stm32_write,
	.register_done_callback = dac_stm32_register_done_callback,
};

#define STM32_DAC_DEFINE(inst)                                                                     \
	static const struct dac_stm32_cfg dac_stm32_cfg_##inst = {                                 \
		.dac_base = (DAC_TypeDef *)DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR_BY_IDX(inst, 0)),  \
		.dac_ll_channel = UTIL_CAT(LL_DAC_CHANNEL_, DT_INST_IO_CHANNELS_OUTPUT(inst)),     \
		.dac_trigger = UTIL_CAT(LL_DAC_TRIG_EXT_, DT_STRING_TOKEN(DT_DRV_INST(inst),       \
									  st_dac_trigger_source)), \
		.tim_base =                                                                        \
			(TIM_TypeDef *)DT_REG_ADDR(DT_INST_PHANDLE(inst, st_dac_trigger_timer)),   \
		.counter_dev = DEVICE_DT_GET(                                                      \
			DT_CHILD(DT_INST_PHANDLE(inst, st_dac_trigger_timer), counter)),           \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(inst, 0)),                       \
		.dma_channel = DT_INST_DMAS_CELL_BY_IDX(inst, 0, channel),                         \
		.dma_slot = STM32_DMA_SLOT_BY_IDX(inst, 0, slot),                                  \
	};                                                                                         \
	static struct dac_stm32_data dac_stm32_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, dac_stm32_init, NULL, &dac_stm32_data_##inst,                  \
			      &dac_stm32_cfg_##inst, POST_KERNEL,                                  \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &dac_stm32_api)

DT_INST_FOREACH_STATUS_OKAY(STM32_DAC_DEFINE)
