/*
 * Copyright (c) 2024-2025 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 * Copyright (c) 2024-2026 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sai
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/cache.h>

#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <stm32_ll_dma.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_stm32_sai, CONFIG_I2S_LOG_LEVEL);

enum mclk_divider {
	MCLK_NO_DIV,
	MCLK_DIV_256,
	MCLK_DIV_512
};

static const uint32_t dma_priority[] = {
#if defined(CONFIG_DMA_STM32U5)
	DMA_LOW_PRIORITY_LOW_WEIGHT,
	DMA_LOW_PRIORITY_MID_WEIGHT,
	DMA_LOW_PRIORITY_HIGH_WEIGHT,
	DMA_HIGH_PRIORITY,
#else
	DMA_PRIORITY_LOW,
	DMA_PRIORITY_MEDIUM,
	DMA_PRIORITY_HIGH,
	DMA_PRIORITY_VERY_HIGH,
#endif
};

#if defined(CONFIG_DMA_STM32U5)
static const uint32_t dma_src_size[] = {
	DMA_SRC_DATAWIDTH_BYTE,
	DMA_SRC_DATAWIDTH_HALFWORD,
	DMA_SRC_DATAWIDTH_WORD,
};
static const uint32_t dma_dest_size[] = {
	DMA_DEST_DATAWIDTH_BYTE,
	DMA_DEST_DATAWIDTH_HALFWORD,
	DMA_DEST_DATAWIDTH_WORD,
};
#else
static const uint32_t dma_p_size[] = {
	DMA_PDATAALIGN_BYTE,
	DMA_PDATAALIGN_HALFWORD,
	DMA_PDATAALIGN_WORD,
};
static const uint32_t dma_m_size[] = {
	DMA_MDATAALIGN_BYTE,
	DMA_MDATAALIGN_HALFWORD,
	DMA_MDATAALIGN_WORD,
};
#endif

static const uint32_t sai_fifo_threshold[] = {
	SAI_FIFOTHRESHOLD_EMPTY,
	SAI_FIFOTHRESHOLD_1QF,
	SAI_FIFOTHRESHOLD_HF,
	SAI_FIFOTHRESHOLD_3QF,
	SAI_FIFOTHRESHOLD_FULL,
};

struct queue_item {
	void *buffer;
	size_t size;
};

struct i2s_stm32_sai_cfg {
	const struct stm32_pclken *pclken;
	size_t pclk_len;
};

struct i2s_stm32_sai_sub_cfg {
	const struct pinctrl_dev_config *pincfg;
	bool mclk_enable;
	enum mclk_divider mclk_div;
	bool synchronous;

	const struct device *parent;
};

struct stream {
	DMA_TypeDef *reg;

	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;

	/* DMA size of a source block transfer in bytes according SAI data size. */
	uint8_t dma_block_size;

	struct i2s_config i2s_cfg;
	void *mem_block;
	size_t mem_block_len;

	bool master;
	bool last_block;

	int32_t state;
	struct k_msgq queue;

	int (*stream_start)(const struct device *dev, enum i2s_dir dir);
	void (*queue_drop)(const struct device *dev);
};

struct i2s_stm32_sai_sub_data {
	SAI_HandleTypeDef hsai;
	DMA_HandleTypeDef hdma;
	struct stream stream;
};

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_sub_data *sub_data =
		CONTAINER_OF(hsai, struct i2s_stm32_sai_sub_data, hsai);
	struct stream *stream = &sub_data->stream;
	int ret;

	/* Exit the callback, Stream is stopped */
	if (stream->state == I2S_STATE_ERROR) {
		goto exit;
	}

	if (stream->mem_block == NULL) {
		if (stream->state != I2S_STATE_READY) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("RX mem_block NULL");
			__HAL_SAI_DISABLE(hsai);
			goto exit;
		} else {
			return;
		}
	}

	struct queue_item item = {.buffer = stream->mem_block, .size = stream->mem_block_len};

	ret = k_msgq_put(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		LOG_DBG("Stopping RX ...");
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	stream->mem_block_len = stream->i2s_cfg.block_size;

	if (HAL_SAI_Receive_DMA(hsai, stream->mem_block,
				stream->mem_block_len / stream->dma_block_size) != HAL_OK) {
		LOG_ERR("HAL_SAI_Receive_DMA: <FAILED>");
	}

exit:
	/* EXIT */
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_sub_data *sub_data =
		CONTAINER_OF(hsai, struct i2s_stm32_sai_sub_data, hsai);
	struct stream *stream = &sub_data->stream;
	void *mem_block_tmp = stream->mem_block;
	struct queue_item item;
	int ret;

	if (stream->state == I2S_STATE_ERROR) {
		LOG_ERR("TX bad status: %d, Stopping...", stream->state);
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	if (stream->mem_block == NULL) {
		if (stream->state != I2S_STATE_READY) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("TX mem_block NULL");
			__HAL_SAI_DISABLE(hsai);
			goto exit;
		} else {
			return;
		}
	}

	if (stream->last_block) {
		LOG_DBG("TX Stopped ...");
		stream->state = I2S_STATE_READY;
		stream->mem_block = NULL;
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	/* Exit callback, no more data in the queue */
	/* Reset I2S state */
	if (k_msgq_num_used_get(&stream->queue) == 0) {
		LOG_DBG("Exit TX callback, no more data in the queue");
		stream->state = I2S_STATE_READY;
		stream->mem_block = NULL;
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		__HAL_SAI_DISABLE(hsai);
		goto exit;
	}

	stream->mem_block = item.buffer;
	stream->mem_block_len = item.size;

	sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);

	if (HAL_SAI_Transmit_DMA(hsai, stream->mem_block,
				 stream->mem_block_len / stream->dma_block_size) != HAL_OK) {
		LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
	}

exit:
	/* Free memory slab & exit */
	k_mem_slab_free(stream->i2s_cfg.mem_slab, mem_block_tmp);
}

static int sub_dma_init(const struct device *dev)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	struct dma_config *dma_cfg = &sub_data->stream.dma_cfg;
	int ret;

	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	DMA_HandleTypeDef *hdma = &sub_data->hdma;

	if (!device_is_ready(stream->dma_dev)) {
		LOG_ERR("%s DMA device not ready", stream->dma_dev->name);
		return -ENODEV;
	}

	/* Proceed to the minimum Zephyr DMA driver init */
	dma_cfg->user_data = hdma;

	/* HACK: This field is used to inform driver that it is overridden */
	dma_cfg->linked_channel = STM32_DMA_HAL_OVERRIDE;

	ret = dma_config(stream->dma_dev, stream->dma_channel, dma_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d", stream->dma_channel);
		return ret;
	}

	hdma->Instance = STM32_DMA_GET_INSTANCE(stream->reg, stream->dma_channel);
	hdma->Init.Mode = DMA_NORMAL;

	if (dma_cfg->channel_priority >= ARRAY_SIZE(dma_priority)) {
		LOG_ERR("Invalid DMA channel priority");
		return -EINVAL;
	}
	hdma->Init.Priority = dma_priority[dma_cfg->channel_priority];

#if defined(DMA_CHANNEL_1)
	hdma->Init.Channel = dma_cfg->dma_slot * DMA_CHANNEL_1;
#else
	hdma->Init.Request = dma_cfg->dma_slot;
#endif

	if (dma_cfg->source_data_size != dma_cfg->dest_data_size) {
		LOG_ERR("Source and destination data sizes are not aligned");
		return -EINVAL;
	}

	int idx = find_lsb_set(dma_cfg->source_data_size) - 1;

#if defined(CONFIG_DMA_STM32U5)
	if (idx >= ARRAY_SIZE(dma_src_size)) {
		LOG_ERR("Invalid source and destination DMA data size");
		return -EINVAL;
	}

	hdma->Init.SrcDataWidth = dma_src_size[idx];
	hdma->Init.DestDataWidth = dma_dest_size[idx];
	hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma->Init.SrcBurstLength = 1;
	hdma->Init.DestBurstLength = 1;
	hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
	hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
#else
	if (idx >= ARRAY_SIZE(dma_m_size)) {
		LOG_ERR("Invalid peripheral and memory DMA data size");
		return -EINVAL;
	}

	hdma->Init.PeriphDataAlignment = dma_p_size[idx];
	hdma->Init.MemDataAlignment = dma_m_size[idx];
	hdma->Init.PeriphInc = DMA_PINC_DISABLE;
	hdma->Init.MemInc = DMA_MINC_ENABLE;
#endif

#if defined(DMA_FIFOMODE_DISABLE)
	hdma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
#endif

	if (dma_cfg->channel_direction == (enum dma_channel_direction)MEMORY_TO_PERIPHERAL) {
		hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;

#if defined(CONFIG_DMA_STM32U5)
		hdma->Init.SrcInc = DMA_SINC_INCREMENTED;
		hdma->Init.DestInc = DMA_DINC_FIXED;
#endif

		__HAL_LINKDMA(hsai, hdmatx, sub_data->hdma);
	} else {
		hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;

#if defined(CONFIG_DMA_STM32U5)
		hdma->Init.SrcInc = DMA_SINC_FIXED;
		hdma->Init.DestInc = DMA_DINC_INCREMENTED;
#endif

		__HAL_LINKDMA(hsai, hdmarx, sub_data->hdma);
	}

	if (HAL_DMA_Init(&sub_data->hdma) != HAL_OK) {
		LOG_ERR("HAL_DMA_Init: <FAILED>");
		return -EIO;
	}

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	if (HAL_DMA_ConfigChannelAttributes(&dev_data->hdma, DMA_CHANNEL_SEC | DMA_CHANNEL_PRIV |
					    DMA_CHANNEL_SRC_SEC | DMA_CHANNEL_DEST_SEC) != HAL_OK) {
		LOG_ERR("HAL_DMA_ConfigChannelAttributes: <Failed>");
		return -EIO;
	}
#elif defined(CONFIG_DMA_STM32U5)
	if (HAL_DMA_ConfigChannelAttributes(&sub_data->hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
		LOG_ERR("HAL_DMA_ConfigChannelAttributes: <Failed>");
		return -EIO;
	}
#endif

	LOG_INF("dma@%08x Init: <OK>", (uint32_t)hdma->Instance);

	return 0;
}

static int sai_sub_init(const struct device *dev)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	const struct i2s_stm32_sai_sub_cfg *sub_cfg = dev->config;
	int ret;

	/* Configure DT provided pins */
	ret = pinctrl_apply_state(sub_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SAI Sub-Block pinctrl setup: <FAILED>, ret: %d", ret);
		return ret;
	}

	if (!device_is_ready(sub_data->stream.dma_dev)) {
		LOG_ERR("%s device not ready", sub_data->stream.dma_dev->name);
		return -ENODEV;
	}

	ret = k_msgq_alloc_init(&sub_data->stream.queue, sizeof(struct queue_item),
				CONFIG_I2S_STM32_SAI_BLOCK_COUNT);
	if (ret < 0) {
		LOG_ERR("k_msgq_alloc_init(): <FAILED>, ret: %d", ret);
		return ret;
	}

	/* Initialize DMA */
	ret = sub_dma_init(dev);
	if (ret < 0) {
		LOG_ERR("SAI Sub-Block DMA Init <FAILED>, ret: %d", ret);
		return ret;
	}

	return 0;
}

static int stream_start(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	struct queue_item item;
	int ret;

	if (dir == I2S_DIR_TX) {
		ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}

		stream->mem_block = item.buffer;
		stream->mem_block_len = item.size;

		sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);

		ret = HAL_SAI_Transmit_DMA(hsai, stream->mem_block,
					   stream->mem_block_len / stream->dma_block_size);

		if (ret != HAL_OK) {
			LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
			return -EIO;
		}
	} else {

		ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}

		stream->mem_block_len = stream->i2s_cfg.block_size;

		if (HAL_SAI_Receive_DMA(hsai, stream->mem_block,
					stream->mem_block_len / stream->dma_block_size) != HAL_OK) {
			LOG_ERR("HAL_SAI_Receive_DMA: <FAILED>");
			return -EIO;
		}
	}

	return 0;
}

static void queue_drop(const struct device *dev)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	struct queue_item item;

	if (stream->mem_block != NULL) {
		stream->mem_block = NULL;
		stream->mem_block_len = 0;
	}

	while (k_msgq_get(&stream->queue, &item, K_NO_WAIT) == 0) {
		LOG_DBG("Dropping item from queue");
		k_mem_slab_free(stream->i2s_cfg.mem_slab, item.buffer);
	}
}

static void dma_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	ARG_UNUSED(dma_dev);

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
	}
	HAL_DMA_IRQHandler(hdma);
}

#if defined(CONFIG_SOC_SERIES_STM32F4X)
static int sai_sub_f4_clk_src_conf(const struct device *dev)
{
	const struct i2s_stm32_sai_sub_cfg *sub_cfg = dev->config;
	const struct i2s_stm32_sai_cfg *sai_cfg = sub_cfg->parent->config;
	struct i2s_stm32_sai_sub_data *const sub_data = dev->data;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	uint32_t clock_source = 0U;

	if (sai_cfg->pclk_len > 1) {
		clock_source = sai_cfg->pclken[1].bus;
	}

	switch (clock_source) {
#if defined(STM32F413xx) || defined(STM32F423xx)
	case STM32_SRC_PLLI2S_POST_R:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
		break;

	case STM32_SRC_PLL_POST_R:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLR;
		break;

	case STM32_SRC_HSI:
		hsai->Init.ClockSource = SAI_CLKSOURCE_HS;
		break;
#else /* STM32F413xx || STM32F423xx */
	case STM32_SRC_PLLSAI_POST_Q:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
		break;

	case STM32_SRC_PLLI2S_POST_Q:
		hsai->Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
		break;
#endif /* STM32F413xx || STM32F423xx */
	case 0U:
		/* No source clock defined. Do nothing. */
		break;

	default:
		LOG_ERR("Wrong source clock defined.");
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_SOC_SERIES_STM32F4X */

static int sai_sub_conf(const struct device *dev, enum i2s_dir dir,
			const struct i2s_config *i2s_cfg)
{
	const struct i2s_stm32_sai_sub_cfg *const sub_cfg = dev->config;
	struct i2s_stm32_sai_sub_data *const sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	SAI_HandleTypeDef *hsai = &sub_data->hsai;
	uint8_t protocol;
	uint8_t word_size;

	memcpy(&stream->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->master = true;
	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET) {
		stream->master = false;
	}

#if defined(CONFIG_SOC_SERIES_STM32F4X)
	int err;

	/* ON F4x, the HAL modifies the RCC to set the source clock, so it is necessary to define
	 * the ClockSource Init parameter.
	 */
	err = sai_sub_f4_clk_src_conf(dev);
	if (err != 0) {
		return err;
	}
#endif /* CONFIG_SOC_SERIES_STM32F4X */

	hsai->Init.Synchro = SAI_ASYNCHRONOUS;

	if (dir == I2S_DIR_RX) {
		hsai->Init.AudioMode = SAI_MODEMASTER_RX;

		if (stream->master == false) {
			hsai->Init.AudioMode = SAI_MODESLAVE_RX;
			if (sub_cfg->synchronous) {
				hsai->Init.Synchro = SAI_SYNCHRONOUS;
			}
		}

	} else if (dir == I2S_DIR_TX) {
		hsai->Init.AudioMode = SAI_MODEMASTER_TX;

		if (stream->master == false) {
			hsai->Init.AudioMode = SAI_MODESLAVE_TX;
			if (sub_cfg->synchronous) {
				hsai->Init.Synchro = SAI_SYNCHRONOUS;
			}
		}
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		LOG_ERR("Invalid state: %d", (int)stream->state);
		return -EINVAL;
	}

	/* MckOutput is not supported by all MCU series */
#if defined(SAI_MCK_OUTPUT_ENABLE)
	if (sub_cfg->mclk_enable && stream->master) {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
	} else {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
	}
#endif

	if (sub_cfg->mclk_div == (enum mclk_divider)MCLK_NO_DIV) {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_DISABLED;
	} else {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;

		/* MckOverSampling is not supported by all MCU series */
#if defined(SAI_MCK_OVERSAMPLING_DISABLE)
		if (sub_cfg->mclk_div == (enum mclk_divider)MCLK_DIV_256) {
			hsai->Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;
		} else {
			hsai->Init.MckOverSampling = SAI_MCK_OVERSAMPLING_ENABLE;
		}
#endif
	}

	/* AudioFrequency */
	switch (stream->i2s_cfg.frame_clk_freq) {
	case 192000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_192K;
		break;
	case 96000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_96K;
		break;
	case 48000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
		break;
	case 44100U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
		break;
	case 32000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_32K;
		break;
	case 22050U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_22K;
		break;
	case 16000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_16K;
		break;
	case 11025U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_11K;
		break;
	case 8000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_8K;
		break;
	default:
		LOG_ERR("Invalid frame_clk_freq %u", stream->i2s_cfg.frame_clk_freq);
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	/* WordSize */
	switch (stream->i2s_cfg.word_size) {
	case 16:
		word_size = SAI_PROTOCOL_DATASIZE_16BIT;
		stream->dma_block_size = 2;
		break;
	case 24:
		word_size = SAI_PROTOCOL_DATASIZE_24BIT;
		stream->dma_block_size = 4;
		break;
	case 32:
		word_size = SAI_PROTOCOL_DATASIZE_32BIT;
		stream->dma_block_size = 4;
		break;
	default:
		LOG_ERR("Invalid wordsize %u", stream->i2s_cfg.word_size);
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	/* MonoStereoMode */
	switch (stream->i2s_cfg.channels) {
	case 1:
		hsai->Init.MonoStereoMode = SAI_MONOMODE;
		break;
	case 2:
		hsai->Init.MonoStereoMode = SAI_STEREOMODE;
		break;
	default:
		LOG_ERR("Invalid channel number: %u", stream->i2s_cfg.channels);
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	if (stream->i2s_cfg.options & I2S_OPT_PINGPONG) {
		LOG_ERR("Ping-pong mode not supported");
		stream->state = I2S_STATE_NOT_READY;
		return -ENOTSUP;
	}

	if ((stream->i2s_cfg.format & I2S_FMT_DATA_ORDER_LSB) ||
	    (stream->i2s_cfg.format & I2S_FMT_BIT_CLK_INV) ||
	    (stream->i2s_cfg.format & I2S_FMT_FRAME_CLK_INV)) {
		LOG_ERR("Unsupported stream format");
		return -EINVAL;
	}

	switch (stream->i2s_cfg.format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		protocol = SAI_I2S_STANDARD;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		protocol = SAI_I2S_MSBJUSTIFIED;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		protocol = SAI_PCM_SHORT;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		protocol = SAI_PCM_LONG;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		protocol = SAI_I2S_LSBJUSTIFIED;
		break;
	default:
		LOG_ERR("Unsupported I2S data format");
		return -EINVAL;
	}

	/* Initialize SAI peripheral */
	if (HAL_SAI_InitProtocol(hsai, protocol, word_size, 2) != HAL_OK) {
		LOG_ERR("HAL_SAI_InitProtocol: <FAILED>");
		return -EIO;
	}

	LOG_INF("%s Init: <OK>", dev->name);

	stream->state = I2S_STATE_READY;

	return 0;
}

static int sai_sub_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_BOTH) {
		LOG_ERR("Unsupported direction: %d", (int)dir);
		return -ENOSYS;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		LOG_DBG("I2S_TRIGGER_START");

		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d", stream->state);
			return -EIO;
		}

		ret = stream->stream_start(dev, dir);
		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			return ret;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;

		break;
	case I2S_TRIGGER_STOP:
		key = irq_lock();
		LOG_DBG("I2S_TRIGGER_STOP");

		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP - Invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->last_block = true;
		stream->state = I2S_STATE_STOPPING;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		LOG_DBG("I2S_TRIGGER_DRAIN");

		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN - Invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->state = I2S_STATE_STOPPING;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_DROP:
		key = irq_lock();
		LOG_DBG("I2S_TRIGGER_DROP");

		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->queue_drop(dev);
		stream->state = I2S_STATE_READY;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_PREPARE:
		key = irq_lock();
		LOG_DBG("I2S_TRIGGER_PREPARE");

		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE - invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}
		stream->queue_drop(dev);
		stream->state = I2S_STATE_READY;

		irq_unlock(key);
		break;
	default:
		LOG_ERR("Unsupported trigger command");
		return -EINVAL;
	}
	return 0;
}

static int sai_sub_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	struct stream *stream = &sub_data->stream;
	int ret;

	if (stream->state != I2S_STATE_RUNNING && stream->state != I2S_STATE_READY) {
		LOG_ERR("TX Invalid state: %d", (int)stream->state);
		return -EIO;
	}

	if (size > stream->i2s_cfg.block_size) {
		LOG_ERR("Max write size is: %u", (unsigned int)stream->i2s_cfg.block_size);
		return -EINVAL;
	}

	struct queue_item item = {.buffer = mem_block, .size = size};

	ret = k_msgq_put(&stream->queue, &item, K_MSEC(stream->i2s_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("TX queue full");
	}

	return 0;
}

static int sai_sub_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_stm32_sai_sub_data *sub_data = dev->data;
	struct queue_item item;
	int ret;

	if (sub_data->stream.state == I2S_STATE_NOT_READY ||
	    sub_data->stream.state == I2S_STATE_ERROR) {
		LOG_ERR("RX invalid state: %d", (int)sub_data->stream.state);
		return -EIO;
	}

	ret = k_msgq_get(&sub_data->stream.queue, &item, K_MSEC(sub_data->stream.i2s_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("RX queue: %d", k_msgq_num_used_get(&sub_data->stream.queue));
		return ret;
	}

	*mem_block = item.buffer;
	*size = item.size;

	return 0;
}

static int stm32_sai_clock_en(const struct device *dev)
{
	const struct i2s_stm32_sai_cfg *sai_cfg = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	/* Turn on SAI peripheral clock */
	ret = clock_control_on(clk, (clock_control_subsys_t)&sai_cfg->pclken[0]);
	if (ret != 0) {
		return -EIO;
	}

	if (sai_cfg->pclk_len > 1) {
		/* Enable SAI clock source */
		ret = clock_control_configure(clk, (clock_control_subsys_t)&sai_cfg->pclken[1],
					      NULL);
		if (ret < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int sai_init(const struct device *dev)
{
	int ret = 0;

	/* Enable SAI clock */
	ret = stm32_sai_clock_en(dev);
	if (ret < 0) {
		LOG_ERR("SAI clock enable <FAILED>, ret: %d.", ret);
		return -EIO;
	}

	LOG_INF("%s Init: <OK>", dev->name);

	return 0;
}

static DEVICE_API(i2s, i2s_stm32_sai_sub_api) = {
	.configure = sai_sub_conf,
	.trigger = sai_sub_trigger,
	.write = sai_sub_write,
	.read = sai_sub_read,
};

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2bis)
#define DMA_SLOT_BY_IDX(id, idx, slot) 0
#else
#define DMA_SLOT_BY_IDX(id, idx, slot) DT_DMAS_CELL_BY_IDX(id, idx, slot)
#endif

#define DMA_CHANNEL_CONFIG_BY_IDX(id, idx) DT_DMAS_CELL_BY_IDX(id, idx, channel_config)

#define SAI_FIFO_THRESHOLD(idx) \
	sai_fifo_threshold[DT_ENUM_IDX_OR(idx, fifo_threshold, 0)]

#define SAI_DMA_CHANNEL_INIT(node, dir, src, dest)                                                 \
	.stream = {                                                                                \
		.dma_dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),                                      \
		.dma_channel = DT_DMAS_CELL_BY_IDX(node, 0, channel),                              \
		.reg = (DMA_TypeDef *)DT_REG_ADDR(DT_PHANDLE_BY_IDX(node, dmas, 0)),               \
		.dma_cfg = {                                                                       \
			.dma_slot = DMA_SLOT_BY_IDX(node, 0, slot),                                \
			.channel_direction = src##_TO_##dest,                                      \
			.dma_callback = dma_callback,                                              \
			.channel_priority = STM32_DMA_CONFIG_PRIORITY(                             \
				DMA_CHANNEL_CONFIG_BY_IDX(node, 0)),                               \
			.source_data_size = STM32_DMA_CONFIG_##src##_DATA_SIZE(                    \
				DMA_CHANNEL_CONFIG_BY_IDX(node, 0)),                               \
			.dest_data_size = STM32_DMA_CONFIG_##dest##_DATA_SIZE(                     \
				DMA_CHANNEL_CONFIG_BY_IDX(node, 0)),                               \
		},                                                                                 \
		.stream_start = stream_start,                                                      \
		.queue_drop = queue_drop,                                                          \
	}

#define SAI_SUB_INIT(node)                                                                         \
	PINCTRL_DT_DEFINE(node);                                                                   \
                                                                                                   \
	struct i2s_stm32_sai_sub_data sub_data_##node = {                                          \
		.hsai = {                                                                          \
			.Instance = (SAI_Block_TypeDef *)DT_REG_ADDR(node),                        \
			.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE,                               \
			.Init.FIFOThreshold = SAI_FIFO_THRESHOLD(node),                            \
			.Init.SynchroExt = SAI_SYNCEXT_DISABLE,                                    \
			.Init.CompandingMode = SAI_NOCOMPANDING,                                   \
			.Init.TriState = SAI_OUTPUT_NOTRELEASED,                                   \
		},                                                                                 \
		COND_CODE_1(DT_DMAS_HAS_NAME(node, tx),                                            \
		    (SAI_DMA_CHANNEL_INIT(node, tx, MEMORY, PERIPHERAL)),                          \
		    (SAI_DMA_CHANNEL_INIT(node, rx, PERIPHERAL, MEMORY))),                         \
	};                                                                                         \
                                                                                                   \
	static struct i2s_stm32_sai_sub_cfg sub_cfg_##node = {                                     \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(node),                                         \
		.mclk_enable = DT_PROP(node, mclk_enable),                                         \
		.mclk_div = DT_ENUM_IDX(node, mclk_divider),                                       \
		.synchronous = DT_PROP(node, synchronous),                                         \
		.parent = DEVICE_DT_GET(DT_PARENT(node)),                                          \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, &sai_sub_init, NULL, &sub_data_##node, &sub_cfg_##node,             \
			 POST_KERNEL, CONFIG_I2S_INIT_PRIORITY, &i2s_stm32_sai_sub_api);           \
	K_MSGQ_DEFINE(queue_##node, sizeof(struct queue_item), CONFIG_I2S_STM32_SAI_BLOCK_COUNT, 4);

/* Parent Node */
#define SAI_INIT(inst)                                                                             \
	static const struct stm32_pclken clk_##inst[] = STM32_DT_INST_CLOCKS(inst);                \
	struct i2s_stm32_sai_cfg sai_cfg_##inst = {                                                \
		.pclken = clk_##inst,                                                              \
		.pclk_len = DT_INST_NUM_CLOCKS(inst),                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &sai_init, NULL, NULL, &sai_cfg_##inst, POST_KERNEL,           \
			      CONFIG_I2S_INIT_PRIORITY, NULL);                                     \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SAI_SUB_INIT)

DT_INST_FOREACH_STATUS_OKAY(SAI_INIT)
