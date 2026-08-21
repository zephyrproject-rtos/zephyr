/*
 * Copyright (c) 2026 Draeger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dac_audio

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/audio/codec.h>

#include <soc.h>
#include <stm32_ll_dac.h>
#include <stm32_ll_tim.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_dac, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define STM32_DAC_BYTES_PER_SAMPLE 2
#define STM32_DAC_PCM_SIGN_BIT     0x8000U

#define STM32_DAC_SAMPLES_PER_BLOCK                                                                \
	(CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE / STM32_DAC_BYTES_PER_SAMPLE)

BUILD_ASSERT((CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE % STM32_DAC_BYTES_PER_SAMPLE) == 0,
	     "CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE must be even: one sample is two bytes");

BUILD_ASSERT((2 * CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE) <= 0xFFFFU,
	     "CONFIG_AUDIO_DAC_STM32_MAX_BLOCK_SIZE must not exceed 32766");

struct stm32_dac_cfg {
	DAC_TypeDef *dac_base;
	uint32_t dac_ll_channel;
	uint32_t dac_trigger;
	TIM_TypeDef *tim_base;
	const struct device *counter_dev;
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint32_t dma_slot;
};

struct stm32_dac_data {
	struct audio_codec_cfg config;
	audio_codec_tx_done_callback_t tx_cb;
	void *tx_cb_user_data;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block;
	uint16_t dma_buf[2 * STM32_DAC_SAMPLES_PER_BLOCK];
	uint16_t *write_buf;
	bool writable;
	bool started;
	bool configured;
};

static int stm32_dac_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct stm32_dac_cfg *dac_cfg = dev->config;
	struct stm32_dac_data *dac_data = dev->data;

	if (cfg == NULL) {
		LOG_ERR("configure: no config");
		return -EINVAL;
	}

	if (dac_data->started) {
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

	uint32_t freq = counter_get_frequency(dac_cfg->counter_dev);
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

	int ret = counter_set_top_value(dac_cfg->counter_dev, &top_cfg);

	if (ret < 0) {
		LOG_ERR("configure: counter top %" PRIu32 " failed (%d)", top_cfg.ticks, ret);
		return ret;
	}

	LL_TIM_SetUpdateSource(dac_cfg->tim_base, LL_TIM_UPDATESOURCE_COUNTER);
	LL_TIM_SetTriggerOutput(dac_cfg->tim_base, LL_TIM_TRGO_UPDATE);

	LL_DAC_SetOutputBuffer(dac_cfg->dac_base, dac_cfg->dac_ll_channel,
			       LL_DAC_OUTPUT_BUFFER_ENABLE);
	LL_DAC_SetTriggerSource(dac_cfg->dac_base, dac_cfg->dac_ll_channel, dac_cfg->dac_trigger);
	LL_DAC_EnableTrigger(dac_cfg->dac_base, dac_cfg->dac_ll_channel);
	LL_DAC_EnableDMAReq(dac_cfg->dac_base, dac_cfg->dac_ll_channel);
	LL_DAC_Enable(dac_cfg->dac_base, dac_cfg->dac_ll_channel);

	dac_data->config = *cfg;
	dac_data->configured = true;

	LOG_INF("configured: %" PRIu32 " Hz (%" PRIu32 " / %" PRIu32 "), block size %zu",
		freq / ticks, freq, ticks, cfg->dai_cfg.pcm.block_size);

	return 0;
}

static void stm32_dac_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				   int status);

static void stm32_dac_start_output(const struct device *dev)
{
	const struct stm32_dac_cfg *dac_cfg = dev->config;
	struct stm32_dac_data *dac_data = dev->data;

	if (!dac_data->configured) {
		LOG_ERR("start: not configured");
		return;
	}

	if (dac_data->started) {
		LOG_DBG("start: already running");
		return;
	}

	dac_data->dma_block = (struct dma_block_config){
		.source_address = (uint32_t)dac_data->dma_buf,
		.dest_address = LL_DAC_DMA_GetRegAddr(dac_cfg->dac_base, dac_cfg->dac_ll_channel,
						      LL_DAC_DMA_REG_DATA_12BITS_LEFT_ALIGNED),
		.block_size = 2U * dac_data->config.dai_cfg.pcm.block_size,
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.source_reload_en = 1,
		.dest_reload_en = 1,
	};

	dac_data->dma_cfg = (struct dma_config){
		.dma_slot = dac_cfg->dma_slot,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = STM32_DAC_BYTES_PER_SAMPLE,
		.dest_data_size = STM32_DAC_BYTES_PER_SAMPLE,
		.block_count = 1,
		.head_block = &dac_data->dma_block,
		.dma_callback = stm32_dac_dma_callback,
		.user_data = (void *)dev,
	};

	int ret = dma_config(dac_cfg->dma_dev, dac_cfg->dma_channel, &dac_data->dma_cfg);

	if (ret < 0) {
		LOG_ERR("start: dma_config ch %" PRIu32 " failed (%d)", dac_cfg->dma_channel, ret);
		return;
	}

	ret = dma_start(dac_cfg->dma_dev, dac_cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("start: dma_start ch %" PRIu32 " failed (%d)", dac_cfg->dma_channel, ret);
		return;
	}

	ret = counter_start(dac_cfg->counter_dev);
	if (ret < 0) {
		LOG_ERR("start: counter_start failed (%d)", ret);
		dma_stop(dac_cfg->dma_dev, dac_cfg->dma_channel);
		return;
	}

	dac_data->write_buf = dac_data->dma_buf;
	dac_data->writable = true;
	dac_data->started = true;

	if (dac_data->tx_cb != NULL) {
		dac_data->tx_cb(dev, dac_data->tx_cb_user_data);
	} else {
		LOG_WRN("start: no tx callback registered, output stays silent");
	}
}

static void stm32_dac_stop_output(const struct device *dev)
{
	const struct stm32_dac_cfg *dac_cfg = dev->config;
	struct stm32_dac_data *dac_data = dev->data;

	if (!dac_data->started) {
		LOG_DBG("stop: already stopped");
		return;
	}

	int ret = counter_stop(dac_cfg->counter_dev);

	if (ret < 0) {
		LOG_ERR("stop: counter_stop failed (%d)", ret);
	}

	ret = dma_stop(dac_cfg->dma_dev, dac_cfg->dma_channel);
	if (ret < 0) {
		LOG_ERR("stop: dma_stop ch %" PRIu32 " failed (%d)", dac_cfg->dma_channel, ret);
	}

	dac_data->started = false;
	dac_data->writable = false;
}

static int stm32_dac_set_property(const struct device *dev, audio_property_t property,
				  audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(val);

	LOG_WRN("set_property: property %d (ch %d) ignored, none supported", property, channel);

	return 0;
}

static int stm32_dac_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int stm32_dac_start(const struct device *dev, uint8_t dir)
{
	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_ERR("start: dir %" PRIu8 " not supported, TX only", dir);
		return -ENOTSUP;
	}

	stm32_dac_start_output(dev);

	return 0;
}

static int stm32_dac_stop(const struct device *dev, uint8_t dir)
{
	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_ERR("stop: dir %" PRIu8 " not supported, TX only", dir);
		return -ENOTSUP;
	}

	stm32_dac_stop_output(dev);

	return 0;
}

static int stm32_dac_write(const struct device *dev, uint8_t *data, size_t data_size)
{
	struct stm32_dac_data *dac_data = dev->data;

	if ((data == NULL) || (data_size == 0U) || (data_size % STM32_DAC_BYTES_PER_SAMPLE != 0) ||
	    (data_size > dac_data->config.dai_cfg.pcm.block_size)) {
		LOG_ERR("write: bad data %p size %zu (block %zu)", (void *)data, data_size,
			dac_data->config.dai_cfg.pcm.block_size);
		return -EINVAL;
	}

	if (!dac_data->started) {
		LOG_ERR("write: output not started");
		return -EIO;
	}

	if (!dac_data->writable) {
		LOG_WRN_RATELIMIT("write: no free buffer, caller too early");
		return -EBUSY;
	}

	uint16_t *dst = dac_data->write_buf;
	size_t src_samples = data_size / STM32_DAC_BYTES_PER_SAMPLE;
	size_t dst_samples = dac_data->config.dai_cfg.pcm.block_size / STM32_DAC_BYTES_PER_SAMPLE;

	for (size_t i = 0; i < src_samples; i++) {
		dst[i] = sys_get_le16(&data[2 * i]) ^ STM32_DAC_PCM_SIGN_BIT;
	}
	memset(&dst[src_samples], 0, (dst_samples - src_samples) * sizeof(*dst));

	if (src_samples < dst_samples) {
		LOG_DBG("write: %zu samples, %zu padded with silence", src_samples,
			dst_samples - src_samples);
	}

	dac_data->writable = false;

	return 0;
}

static int stm32_dac_register_done_callback(const struct device *dev,
					    audio_codec_tx_done_callback_t tx_cb,
					    void *tx_cb_user_data,
					    audio_codec_rx_done_callback_t rx_cb,
					    void *rx_cb_user_data)
{
	ARG_UNUSED(rx_cb);
	ARG_UNUSED(rx_cb_user_data);

	struct stm32_dac_data *dac_data = dev->data;

	dac_data->tx_cb = tx_cb;
	dac_data->tx_cb_user_data = tx_cb_user_data;

	LOG_DBG("tx callback %s", (tx_cb != NULL) ? "registered" : "cleared");

	return 0;
}

static void stm32_dac_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				   int status)
{
	const struct device *codec_dev = (const struct device *)user_data;
	struct stm32_dac_data *dac_data = codec_dev->data;

	if (status < 0) {
		LOG_ERR("dma ch %" PRIu32 " error (%d), stopping output", channel, status);
		stm32_dac_stop_output(codec_dev);
		return;
	}

	if (dac_data->writable) {
		LOG_WRN_RATELIMIT("underrun: block not written in time, stale samples played");
	}

	size_t write_index = 0;

	if (status == DMA_STATUS_BLOCK) {
		write_index = dac_data->config.dai_cfg.pcm.block_size / STM32_DAC_BYTES_PER_SAMPLE;
	} else if (status != DMA_STATUS_COMPLETE) {
		LOG_WRN_RATELIMIT("dma ch %" PRIu32 " unexpected status %d", channel, status);
	}

	dac_data->write_buf = &(dac_data->dma_buf[write_index]);
	dac_data->writable = true;

	if (dac_data->tx_cb != NULL) {
		dac_data->tx_cb(codec_dev, dac_data->tx_cb_user_data);
	}
}

static int stm32_dac_init(const struct device *dev)
{
	const struct stm32_dac_cfg *dac_cfg = dev->config;
	struct stm32_dac_data *dac_data = dev->data;

	if (!device_is_ready(dac_cfg->dma_dev)) {
		LOG_ERR("init: DMA device %s not ready", dac_cfg->dma_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(dac_cfg->counter_dev)) {
		LOG_ERR("init: counter device %s not ready", dac_cfg->counter_dev->name);
		return -ENODEV;
	}

	memset(dac_data, 0, sizeof(*dac_data));
	return 0;
}

static DEVICE_API(audio_codec, stm32_dac_api) = {
	.configure = stm32_dac_configure,
	.start_output = stm32_dac_start_output,
	.stop_output = stm32_dac_stop_output,
	.set_property = stm32_dac_set_property,
	.apply_properties = stm32_dac_apply_properties,
	.start = stm32_dac_start,
	.stop = stm32_dac_stop,
	.write = stm32_dac_write,
	.register_done_callback = stm32_dac_register_done_callback,
};

#define STM32_DAC_DEFINE(inst)                                                                     \
	static const struct stm32_dac_cfg stm32_dac_cfg_##inst = {                                 \
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
	static struct stm32_dac_data stm32_dac_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, stm32_dac_init, NULL, &stm32_dac_data_##inst,                  \
			      &stm32_dac_cfg_##inst, POST_KERNEL,                                  \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &stm32_dac_api)

DT_INST_FOREACH_STATUS_OKAY(STM32_DAC_DEFINE)
