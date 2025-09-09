/*
 * Copyright (c) 2024-2025 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 * Copyright (c) 2024-2025 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sai

#include <string.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/cache.h>

#include <stm32_ll_dma.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2s_stm32_sai, CONFIG_I2S_LOG_LEVEL);

enum mclk_divider {
	MCLK_NO_DIV,
	MCLK_DIV_256,
	MCLK_DIV_512
};

struct queue_item {
	void *buffer;
	size_t size;
};

struct stream {
	DMA_TypeDef *reg;

	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;

	/* DMA size of a source block transfer in bytes according SAI data size. */
	uint8_t dma_src_size;

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

struct i2s_stm32_sai_data {
	SAI_HandleTypeDef hsai;
	DMA_HandleTypeDef hdma;
	struct stream stream;
};

struct i2s_stm32_sai_cfg {
	const struct pinctrl_dev_config *pincfg;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;

	bool mclk_enable;
	enum mclk_divider mclk_div;
	bool synchronous;
};

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_data *dev_data = CONTAINER_OF(hsai, struct i2s_stm32_sai_data, hsai);
	struct stream *stream = &dev_data->stream;
	int ret;

	/* Exit the callback, Stream is stopped */
	if (stream->state == I2S_STATE_ERROR) {
		goto exit;
	}

	if (stream->mem_block == NULL) {
		if (stream->state != I2S_STATE_READY) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("RX mem_block NULL");
			goto exit;
		} else {
			return;
		}
	}

	struct queue_item item = {.buffer = stream->mem_block, .size = stream->mem_block_len};

	ret = k_msgq_put(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto exit;
	}

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		LOG_DBG("Stopping RX ...");
		goto exit;
	}

	ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto exit;
	}

	stream->mem_block_len = stream->i2s_cfg.block_size;

	if (HAL_SAI_Receive_DMA(hsai, stream->mem_block,
				stream->mem_block_len / stream->dma_src_size) != HAL_OK) {
		LOG_ERR("HAL_SAI_Receive_DMA: <FAILED>");
	}

exit:
	/* EXIT */
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_data *dev_data = CONTAINER_OF(hsai, struct i2s_stm32_sai_data, hsai);
	struct stream *stream = &dev_data->stream;
	void *mem_block_tmp = stream->mem_block;
	struct queue_item item;
	int ret;

	if (stream->state == I2S_STATE_ERROR) {
		LOG_ERR("TX bad status: %d, Stopping...", stream->state);
		goto exit;
	}

	if (stream->mem_block == NULL) {
		if (stream->state != I2S_STATE_READY) {
			stream->state = I2S_STATE_ERROR;
			LOG_ERR("TX mem_block NULL");
			goto exit;
		} else {
			return;
		}
	}

	if (stream->last_block) {
		LOG_DBG("TX Stopped ...");
		stream->state = I2S_STATE_READY;
		stream->mem_block = NULL;
		goto exit;
	}

	/* Exit callback, no more data in the queue */
	/* Reset I2S state */
	if (k_msgq_num_used_get(&stream->queue) == 0) {
		LOG_DBG("Exit TX callback, no more data in the queue");
		stream->state = I2S_STATE_READY;
		stream->mem_block = NULL;
		goto exit;
	}

	ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto exit;
	}

	stream->mem_block = item.buffer;
	stream->mem_block_len = item.size;

	sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);

	if (HAL_SAI_Transmit_DMA(hsai, stream->mem_block,
				 stream->mem_block_len / stream->dma_src_size) != HAL_OK) {
		LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
	}

exit:
	/* Free memory slab & exit */
	k_mem_slab_free(stream->i2s_cfg.mem_slab, mem_block_tmp);
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
	switch (HAL_SAI_GetError(hsai)) {
	case HAL_SAI_ERROR_NONE:
		LOG_INF("No error");
		break;
	case HAL_SAI_ERROR_OVR:
		LOG_WRN("Overrun Error");
		break;
	case HAL_SAI_ERROR_UDR:
		LOG_WRN("Underrun Error");
		break;
	case HAL_SAI_ERROR_AFSDET:
		LOG_WRN("Anticipated Frame synchronisation detection");
		break;
	case HAL_SAI_ERROR_LFSDET:
		LOG_WRN("Late Frame synchronisation detection");
		break;
	case HAL_SAI_ERROR_CNREADY:
		LOG_WRN("codec not ready");
		break;
	case HAL_SAI_ERROR_TIMEOUT:
		LOG_WRN("Timeout error");
		break;
	case HAL_SAI_ERROR_DMA:
		LOG_WRN("DMA error");
		break;
	default:
		LOG_ERR("Unknown error");
	}
}

static int stm32_sai_enable_clock(const struct device *dev)
{
	const struct i2s_stm32_sai_cfg *cfg = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}
	LOG_DBG("Clock Control Device: <OK>");

	/* Turn on SAI peripheral clock */
	err = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (err != 0) {
		LOG_ERR("I2S clock Enable: <FAILED>");
		return -EIO;
	}
	LOG_DBG("I2S clock Enable: <OK>");

	if (cfg->pclk_len > 1) {
		/* Enable I2S clock source */
		err = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
		if (err < 0) {
			LOG_ERR("I2S domain clock configuration: <FAILED>");
			return -EIO;
		}
	}
	LOG_DBG("I2S domain clock configuration: <OK>");

	return 0;
}

static int i2s_stm32_sai_dma_init(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;
	DMA_HandleTypeDef *hdma = &dev_data->hdma;

	struct stream *stream = &dev_data->stream;

	struct dma_config dma_cfg = dev_data->stream.dma_cfg;

	int ret;

	if (!device_is_ready(stream->dma_dev)) {
		LOG_ERR("%s DMA device not ready", stream->dma_dev->name);
		return -ENODEV;
	}

	/* Proceed to the minimum Zephyr DMA driver init */
	dma_cfg.user_data = hdma;

	/* HACK: This field is used to inform driver that it is overridden */
	dma_cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;

	ret = dma_config(stream->dma_dev, stream->dma_channel, &dma_cfg);

	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d", stream->dma_channel);
		return ret;
	}

	hdma->Instance = STM32_DMA_GET_INSTANCE(stream->reg, stream->dma_channel);
	hdma->Init.Request = dma_cfg.dma_slot;
	hdma->Init.Mode = DMA_NORMAL;

#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32L4X)
	hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma->Init.Priority = DMA_PRIORITY_HIGH;
	hdma->Init.PeriphInc = DMA_PINC_DISABLE;
	hdma->Init.MemInc = DMA_MINC_ENABLE;
#else
	hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
	hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
	hdma->Init.Priority = DMA_HIGH_PRIORITY;
	hdma->Init.SrcBurstLength = 1;
	hdma->Init.DestBurstLength = 1;
	hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
	hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
#endif

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	hdma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
#endif

	if (stream->dma_cfg.channel_direction == (enum dma_channel_direction)MEMORY_TO_PERIPHERAL) {
		hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;

#if !defined(CONFIG_SOC_SERIES_STM32H7X) && !defined(CONFIG_SOC_SERIES_STM32L4X)
		hdma->Init.SrcInc = DMA_SINC_INCREMENTED;
		hdma->Init.DestInc = DMA_DINC_FIXED;
#endif

		__HAL_LINKDMA(hsai, hdmatx, dev_data->hdma);
	} else {
		hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;

#if !defined(CONFIG_SOC_SERIES_STM32H7X) && !defined(CONFIG_SOC_SERIES_STM32L4X)
		hdma->Init.SrcInc = DMA_SINC_FIXED;
		hdma->Init.DestInc = DMA_DINC_INCREMENTED;
#endif

		__HAL_LINKDMA(hsai, hdmarx, dev_data->hdma);
	}

	if (HAL_DMA_Init(&dev_data->hdma) != HAL_OK) {
		LOG_ERR("HAL_DMA_Init: <FAILED>");
		return -EIO;
	}

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	if (HAL_DMA_ConfigChannelAttributes(&dev_data->hdma, DMA_CHANNEL_SEC | DMA_CHANNEL_PRIV |
					    DMA_CHANNEL_SRC_SEC | DMA_CHANNEL_DEST_SEC) != HAL_OK) {
		LOG_ERR("HAL_DMA_ConfigChannelAttributes: <Failed>");
		return -EIO;
	}
#elif !defined(CONFIG_SOC_SERIES_STM32H7X) && !defined(CONFIG_SOC_SERIES_STM32L4X)
	if (HAL_DMA_ConfigChannelAttributes(&dev_data->hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
		LOG_ERR("HAL_DMA_ConfigChannelAttributes: <Failed>");
		return -EIO;
	}
#endif

	return 0;
}

static int i2s_stm32_sai_initialize(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	const struct i2s_stm32_sai_cfg *cfg = dev->config;
	int ret = 0;

	/* Enable SAI clock */
	ret = stm32_sai_enable_clock(dev);
	if (ret < 0) {
		LOG_ERR("Clock enabling failed.");
		return -EIO;
	}

	/* Configure DT provided pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2S pinctrl setup: <FAILED>");
		return ret;
	}

	if (!device_is_ready(dev_data->stream.dma_dev)) {
		LOG_ERR("%s device not ready", dev_data->stream.dma_dev->name);
		return -ENODEV;
	}

	ret = k_msgq_alloc_init(&dev_data->stream.queue, sizeof(struct queue_item),
				CONFIG_I2S_STM32_SAI_BLOCK_COUNT);
	if (ret < 0) {
		LOG_ERR("k_msgq_alloc_init(): <FAILED>");
		return ret;
	}

	/* Initialize DMA */
	ret = i2s_stm32_sai_dma_init(dev);
	if (ret < 0) {
		LOG_ERR("i2s_stm32_sai_dma_init(): <FAILED>");
		return ret;
	}

	LOG_INF("%s inited", dev->name);

	return 0;
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

static int i2s_stm32_sai_configure(const struct device *dev, enum i2s_dir dir,
				   const struct i2s_config *i2s_cfg)
{
	const struct i2s_stm32_sai_cfg *const cfg = dev->config;
	struct i2s_stm32_sai_data *const dev_data = dev->data;
	struct stream *stream = &dev_data->stream;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;
	uint8_t protocol;
	uint8_t word_size;

	memcpy(&stream->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));

	stream->master = true;
	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		stream->master = false;
	}

	hsai->Init.Synchro = SAI_ASYNCHRONOUS;

	if (dir == I2S_DIR_RX) {
		hsai->Init.AudioMode = SAI_MODEMASTER_RX;

		if (stream->master == false) {
			hsai->Init.AudioMode = SAI_MODESLAVE_RX;
			if (cfg->synchronous) {
				hsai->Init.Synchro = SAI_SYNCHRONOUS;
			}
		}

	} else if (dir == I2S_DIR_TX) {
		hsai->Init.AudioMode = SAI_MODEMASTER_TX;

		if (stream->master == false) {
			hsai->Init.AudioMode = SAI_MODESLAVE_TX;
			if (cfg->synchronous) {
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

	/* Control of MCLK output from SAI configuration is not possible on STM32L4xx MCUs */
#if !defined(CONFIG_SOC_SERIES_STM32L4X)
	if (cfg->mclk_enable && stream->master) {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
	} else {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
	}
#endif

	if (cfg->mclk_div == (enum mclk_divider)MCLK_NO_DIV) {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_DISABLED;
	} else {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;

		/* MckOverSampling is not supported by all STM32L4xx MCUs */
#if !defined(CONFIG_SOC_SERIES_STM32L4X)
		if (cfg->mclk_div == (enum mclk_divider)MCLK_DIV_256) {
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
		stream->dma_src_size = 2;
		break;
	case 24:
		word_size = SAI_PROTOCOL_DATASIZE_24BIT;
		stream->dma_src_size = 4;
		break;
	case 32:
		word_size = SAI_PROTOCOL_DATASIZE_32BIT;
		stream->dma_src_size = 4;
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
		LOG_DBG("SAI_MONOMODE");
		break;
	case 2:
		hsai->Init.MonoStereoMode = SAI_STEREOMODE;
		LOG_DBG("SAI_STEREOMODE");
		break;
	default:
		LOG_ERR("NOT VALID CHANNEL NUMBER %u", stream->i2s_cfg.channels);
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

	stream->state = I2S_STATE_READY;

	return 0;
}

static int i2s_stm32_sai_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->stream;
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

static int i2s_stm32_sai_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct queue_item item;
	int ret;

	if (dev_data->stream.state == I2S_STATE_NOT_READY ||
	    dev_data->stream.state == I2S_STATE_ERROR) {
		LOG_ERR("RX invalid state: %d", (int)dev_data->stream.state);
		return -EIO;
	}

	ret = k_msgq_get(&dev_data->stream.queue, &item, K_MSEC(dev_data->stream.i2s_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("RX queue: %d", k_msgq_num_used_get(&dev_data->stream.queue));
		return ret;
	}

	*mem_block = item.buffer;
	*size = item.size;

	return 0;
}

static int stream_start(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->stream;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;
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

		if (HAL_SAI_Transmit_DMA(hsai, stream->mem_block,
					 stream->mem_block_len / stream->dma_src_size) != HAL_OK) {
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
					stream->mem_block_len / stream->dma_src_size) != HAL_OK) {
			LOG_ERR("HAL_SAI_Receive_DMA: <FAILED>");
			return -EIO;
		}
	}

	return 0;
}

static void queue_drop(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->stream;
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

static int i2s_stm32_sai_trigger(const struct device *dev, enum i2s_dir dir,
				 enum i2s_trigger_cmd cmd)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->stream;
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

static DEVICE_API(i2s, i2s_stm32_driver_api) = {
	.configure = i2s_stm32_sai_configure,
	.trigger = i2s_stm32_sai_trigger,
	.write = i2s_stm32_sai_write,
	.read = i2s_stm32_sai_read,
};

#define SAI_DMA_CHANNEL_INIT(index, dir, src_dev, dest_dev)                                        \
	.stream = {                                                                                \
		.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(index, dir)),                              \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                     \
		.reg = (DMA_TypeDef *)DT_REG_ADDR(                                                 \
			DT_PHANDLE_BY_NAME(DT_DRV_INST(index), dmas, dir)),                        \
		.dma_cfg =                                                                         \
			{                                                                          \
				.dma_slot = STM32_DMA_SLOT(index, dir, slot),                      \
				.channel_direction = src_dev##_TO_##dest_dev,                      \
				.dma_callback = dma_callback,                                      \
			},                                                                         \
		.stream_start = stream_start,                                                      \
		.queue_drop = queue_drop,                                                          \
	}

#define I2S_STM32_SAI_INIT(index)                                                                  \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct stm32_pclken clk_##index[] = STM32_DT_INST_CLOCKS(index);              \
                                                                                                   \
	struct i2s_stm32_sai_data sai_data_##index = {                                             \
		.hsai =                                                                            \
			{                                                                          \
				.Instance = (SAI_Block_TypeDef *)DT_INST_REG_ADDR(index),          \
				.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE,                       \
				.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_FULL,                      \
				.Init.SynchroExt = SAI_SYNCEXT_DISABLE,                            \
				.Init.CompandingMode = SAI_NOCOMPANDING,                           \
				.Init.TriState = SAI_OUTPUT_NOTRELEASED,                           \
			},                                                                         \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),                                     \
			(SAI_DMA_CHANNEL_INIT(index, tx, MEMORY, PERIPHERAL)),                  \
		(SAI_DMA_CHANNEL_INIT(index, rx, PERIPHERAL, MEMORY)))};                     \
                                                                                                   \
	struct i2s_stm32_sai_cfg sai_config_##index = {                                            \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                   \
		.pclken = clk_##index,                                                             \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.mclk_enable = DT_INST_PROP(index, mclk_enable),                                   \
		.mclk_div = (enum mclk_divider)DT_ENUM_IDX(DT_DRV_INST(index), mclk_divider),      \
		.synchronous = DT_INST_PROP(index, synchronous),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_stm32_sai_initialize, NULL, &sai_data_##index,           \
			      &sai_config_##index, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,          \
			      &i2s_stm32_driver_api);                                              \
                                                                                                   \
	K_MSGQ_DEFINE(queue_##index, sizeof(struct queue_item), CONFIG_I2S_STM32_SAI_BLOCK_COUNT,  \
		      4);

DT_INST_FOREACH_STATUS_OKAY(I2S_STM32_SAI_INIT)
