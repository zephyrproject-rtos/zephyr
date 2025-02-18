/*
 * Copyright (c) 2024 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 * Copyright (c) 2024 Mario Paja
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

#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <stm32_ll_dma.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2s_stm32_sai, CONFIG_I2S_LOG_LEVEL);

#define SAI_WORD_SIZE_BITS_MIN 8
#define SAI_WORD_SIZE_BITS_MAX 32

#define SAI_WORD_PER_FRAME_MIN 0
#define SAI_WORD_PER_FRAME_MAX 32

struct queue_item {
	void *buffer;
	size_t size;
};

struct stream {
	DMA_TypeDef *reg;

	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;

	uint8_t priority;

	struct i2s_config i2s_cfg;
	void *mem_block;

	bool master;
	size_t mem_block_len;

	int32_t state;
	struct k_msgq queue;

	int (*stream_start)(struct stream *, const struct device *dev, enum i2s_dir dir);
	void (*stream_stop)(struct stream *);
	void (*queue_drop)(struct stream *);
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
	//void (*irq_config)(const struct device *dev);

	bool mclk_sel;
	bool mclk_nodiv;
};

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_data *dev_data = CONTAINER_OF(hsai, struct i2s_stm32_sai_data, hsai);
	struct stream *stream = &dev_data->stream;
	int ret;
	LOG_WRN("%s", __func__);

	/* Exit the callback, Stream is stopped */
	if (stream->state == I2S_STATE_READY || stream->mem_block == NULL) {
		goto exit;
	}

	struct queue_item item = {.buffer = stream->mem_block, .size = stream->mem_block_len};

	ret = k_msgq_put(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto exit;
	}

	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		goto exit;
	}

	ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto exit;
	}

	stream->mem_block_len = stream->i2s_cfg.block_size;

	if (HAL_SAI_Receive_DMA(hsai, stream->mem_block, stream->mem_block_len) != HAL_OK) {
		LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
		return -EIO;
	}

exit:
	/* EXIT */
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_data *dev_data = CONTAINER_OF(hsai, struct i2s_stm32_sai_data, hsai);
	struct stream *stream = &dev_data->stream;
	struct queue_item item;
	void *mem_block_tmp;
	int ret;

	//LOG_WRN("%s", __func__);

	/* Exit the callback, Stream is stopped */
	if (stream->state == I2S_STATE_READY || stream->mem_block == NULL) {
		goto exit;
	}

	/* Exit callback, no more data in the queue */
	if (stream->state == I2S_STATE_STOPPING) {
		if (k_msgq_num_used_get(&stream->queue) == 0) {
			stream->state = I2S_STATE_READY;
			goto exit;
		}
	}

	ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto exit;
	}

	mem_block_tmp = stream->mem_block;

	stream->mem_block = item.buffer;
	stream->mem_block_len = item.size;

	sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);

#ifdef CONFIG_I2S_STM32_SAI_USE_DMA
	if (HAL_SAI_Transmit_DMA(hsai, stream->mem_block, stream->mem_block_len) != HAL_OK) {
		LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
	}
#else
	if (HAL_SAI_Transmit_IT(hsai, stream->mem_block, stream->mem_block_len) != HAL_OK) {
		LOG_ERR("HAL_SAI_Transmit_IT: <FAILED>");
	}
#endif

	k_mem_slab_free(stream->i2s_cfg.mem_slab, mem_block_tmp);
exit:
	/* EXIT */
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

#ifdef CONFIG_I2S_STM32_SAI_USE_DMA
static int i2s_stm32_sai_dma_init(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;
	DMA_HandleTypeDef *hdma = &dev_data->hdma;

	struct stream *stream = &dev_data->stream;

	struct dma_config dma_cfg = dev_data->stream.dma_cfg;
	// static DMA_HandleTypeDef hdma;

	int ret;

	if (!device_is_ready(stream->dma_dev)) {
		LOG_ERR("%s DMA device not ready", stream->dma_dev->name);
		return -ENODEV;
	}
	LOG_DBG("%s DMA device: <OK>", stream->dma_dev->name);

	/* Proceed to the minimum Zephyr DMA driver init */
	dma_cfg.user_data = hdma;

	/* HACK: This field is used to inform driver that it is overridden */
	dma_cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;
	/* Because of the STREAM OFFSET, the DMA channel given here is from 1 -  */
	ret = dma_config(stream->dma_dev, stream->dma_channel, &dma_cfg);

	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d",
			stream->dma_channel + STM32_DMA_STREAM_OFFSET);
		return ret;
	}
	LOG_DBG("DMA channel %d configured", stream->dma_channel);

	hdma->Instance = LL_DMA_GET_CHANNEL_INSTANCE(stream->reg, stream->dma_channel);
	LOG_WRN("HDMA Instance: 0x%x", hdma->Instance);

	hdma->Init.Request = dma_cfg.dma_slot;
	LOG_WRN("HDMA Requenst: %d", hdma->Init.Request);

	hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
	hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
	hdma->Init.Priority = DMA_HIGH_PRIORITY;
	hdma->Init.SrcBurstLength = 1;
	hdma->Init.DestBurstLength = 1;
	hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
	hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
	hdma->Init.Mode = DMA_NORMAL;

	if (stream->dma_cfg.channel_direction == (enum dma_channel_direction)MEMORY_TO_PERIPHERAL) {
		hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma->Init.SrcInc = DMA_SINC_INCREMENTED;
		hdma->Init.DestInc = DMA_DINC_FIXED;
		__HAL_LINKDMA(hsai, hdmatx, dev_data->hdma);
		LOG_WRN("TX");
	} else {
		hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma->Init.SrcInc = DMA_SINC_FIXED;
		hdma->Init.DestInc = DMA_DINC_INCREMENTED;
		__HAL_LINKDMA(hsai, hdmarx, dev_data->hdma);
		LOG_WRN("RX");
	}

	if (HAL_DMA_Init(&dev_data->hdma) != HAL_OK) {
		LOG_ERR("HAL_DMA_Init: <FAILED>");
		return -EIO;
	}
	LOG_INF("HAL_DMA_Init: <OK>");

	if (HAL_DMA_ConfigChannelAttributes(&dev_data->hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
		LOG_INF("HAL_DMA_ConfigChannelAttributes: <Failed>");
	}

	return 0;
}
#endif

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

	/* Run IRQ init */
	//cfg->irq_config(dev);

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

#ifdef CONFIG_I2S_STM32_SAI_USE_DMA
	/* Initialize DMA */
	ret = i2s_stm32_sai_dma_init(dev);
	if (ret < 0) {
		LOG_ERR("i2s_stm32_sai_dma_init(): <FAILED>");
		return ret;
	}
#endif

	LOG_INF("%s inited", dev->name);

	return 0;
}

/*static void i2s_stm32_isr(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	HAL_SAI_IRQHandler(&dev_data->hsai);
}*/

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
	// bool mclk_en;
	uint8_t protocol;
	uint8_t word_size;

	if (dir == I2S_DIR_RX) {
		LOG_DBG("I2S_DIR_RX - SAI_MODESLAVE_RX");
		hsai->Init.AudioMode = SAI_MODESLAVE_RX;
	} else if (dir == I2S_DIR_TX) {
		LOG_DBG("I2S_DIR_TX - SAI_MODEMASTER_TX");
		hsai->Init.AudioMode = SAI_MODEMASTER_TX;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	memcpy(&stream->i2s_cfg, i2s_cfg, sizeof(struct i2s_config));
	stream->state = I2S_STATE_READY;

	/* AudioFrequency */
	switch (stream->i2s_cfg.frame_clk_freq) {
	case 192000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_192K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_192K");
		break;
	case 96000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_96K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_96K");
		break;
	case 48000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_48K");
		break;
	case 44100U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_44K");
		break;
	case 32000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_32K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_32K");
		break;
	case 22050U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_22K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_22K");
		break;
	case 16000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_16K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_16K");
		break;
	case 11025U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_11K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_11K");
		break;
	case 8000U:
		hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_8K;
		LOG_DBG("SAI_AUDIO_FREQUENCY_8K");
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
		LOG_DBG("SAI_PROTOCOL_DATASIZE_16BIT");
		break;
	case 24:
		word_size = SAI_PROTOCOL_DATASIZE_24BIT;
		LOG_DBG("SAI_PROTOCOL_DATASIZE_24BIT");
		break;
	case 32:
		word_size = SAI_PROTOCOL_DATASIZE_32BIT;
		LOG_DBG("SAI_PROTOCOL_DATASIZE_32BIT");
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

	switch (stream->i2s_cfg.format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		protocol = SAI_I2S_STANDARD;
		LOG_DBG("SAI_I2S_STANDARD");
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		protocol = SAI_I2S_MSBJUSTIFIED;
		LOG_DBG("SAI_I2S_MSBJUSTIFIED");
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		protocol = SAI_PCM_SHORT;
		LOG_DBG("SAI_PCM_SHORT");
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		protocol = SAI_PCM_LONG;
		LOG_DBG("SAI_PCM_LONG");
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		protocol = SAI_I2S_LSBJUSTIFIED;
		LOG_DBG("SAI_I2S_LSBJUSTIFIED");
		break;
	default:
		LOG_ERR("Unsupported I2S data format");
		stream->state = I2S_STATE_NOT_READY;
		return -EINVAL;
	}

	/* Initialize SAI peripheral */
	if (HAL_SAI_InitProtocol(hsai, protocol, word_size, 2) != HAL_OK) {
		LOG_ERR("HAL_SAI_InitProtocol: <FAILED>");
	}
	LOG_DBG("HAL_SAI_InitProtocol: <OK>");

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
		return ret;
	}

	return 0;
}

static int i2s_stm32_sai_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct queue_item item;
	int ret;

	LOG_WRN("sai read");

	if (dev_data->stream.state == I2S_STATE_NOT_READY ||
	    dev_data->stream.state == I2S_STATE_ERROR) {
		LOG_ERR("RX invalid state: %d", (int)dev_data->stream.state);
		return -EIO;
	}

	ret = k_msgq_get(&dev_data->stream.queue, &item, K_MSEC(dev_data->stream.i2s_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("RX queue empty");
		return ret;
	}

	*mem_block = item.buffer;
	*size = item.size;

	return 0;
}

static int stream_start(struct stream *stream, const struct device *dev, enum i2s_dir dir)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;
	struct queue_item item;
	int ret;

	if (dir == I2S_DIR_TX) {
		ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}

		LOG_DBG("TX Stream Started");

		stream->mem_block = item.buffer;
		stream->mem_block_len = item.size;

		sys_cache_data_flush_range(stream->mem_block, stream->mem_block_len);

#ifdef CONFIG_I2S_STM32_SAI_USE_DMA

		if (HAL_SAI_Transmit_DMA(hsai, stream->mem_block, stream->mem_block_len) !=
		    HAL_OK) {
			LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
			return -EIO;
		}
#else
		if (HAL_SAI_Transmit_IT(hsai, stream->mem_block, stream->mem_block_len) != HAL_OK) {
			LOG_ERR("HAL_SAI_Transmit_IT: <FAILED>");
			return -EIO;
		}
#endif
	} else {

		ret = k_mem_slab_alloc(stream->i2s_cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}

		stream->mem_block_len = stream->i2s_cfg.block_size;

		if (HAL_SAI_Receive_DMA(hsai, stream->mem_block, stream->mem_block_len) != HAL_OK) {
			LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
			return -EIO;
		}
	}

	return 0;
}

static void stream_stop(struct stream *stream)
{
	LOG_DBG("Stream Stopped");

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		stream->mem_block_len = 0;
	}
}

static void queue_drop(struct stream *stream)
{
	struct queue_item item;

	while (k_msgq_get(&stream->queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, item.buffer);
		LOG_DBG("Queue Dropped");
	}
}

static int i2s_stm32_sai_trigger(const struct device *dev, enum i2s_dir dir,
				 enum i2s_trigger_cmd cmd)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_RX || dir == I2S_DIR_TX) {
		stream = &dev_data->stream;
	} else if (dir == I2S_DIR_BOTH) {
		LOG_ERR("Unsupported direction: %d", (int)dir);
		return -ENOSYS;
	} else {
		LOG_ERR("Invalid direction: %d", (int)dir);
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d", stream->state);
			return -EIO;
		}

		__ASSERT_NO_MSG(stream->mem_block == NULL);

		stream->state = I2S_STATE_RUNNING;

		ret = stream->stream_start(stream, dev, dir);
		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			return ret;
		}

		break;
	case I2S_TRIGGER_STOP:
		key = irq_lock();

		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP - Invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->state = I2S_STATE_READY;
		stream->stream_stop(stream);

		irq_unlock(key);
		break;
	case I2S_TRIGGER_DRAIN:
		key = irq_lock();

		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN - Invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		if (dir == I2S_DIR_TX) {
			stream->state = I2S_STATE_STOPPING;
		} else if (dir == I2S_DIR_RX) {
			stream->state = I2S_STATE_READY;
		}

		irq_unlock(key);
		break;
	case I2S_TRIGGER_DROP:
		key = irq_lock();

		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}

		stream->stream_stop(stream);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;

		irq_unlock(key);
		break;
	case I2S_TRIGGER_PREPARE:
		key = irq_lock();

		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE - invalid state: %d", (int)stream->state);
			irq_unlock(key);
			return -EIO;
		}
		stream->queue_drop(stream);
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
				.block_count = 2,                                                  \
				.dma_slot = STM32_DMA_SLOT(index, dir, slot),                      \
				.channel_direction = src_dev##_TO_##dest_dev,                      \
				.source_data_size = 2,    /* 16bit default  - IS IT NEEDED? */     \
				.dest_data_size = 2,      /* 16bit default   - IS IT NEEDED? */    \
				.source_burst_length = 1, /* SINGLE transfer - IS IT NEEDED? */    \
				.dest_burst_length = 1,   /* SINGLE transfer - IS IT NEEDED?  */   \
				.channel_priority = STM32_DMA_CONFIG_PRIORITY(                     \
					STM32_DMA_CHANNEL_CONFIG(index, dir)),                     \
				.dma_callback = dma_callback,                                      \
			},                                                                         \
		.stream_start = stream_start,                                                      \
		.stream_stop = stream_stop,                                                        \
		.queue_drop = queue_drop,                                                          \
	}

#define I2S_STM32_SAI_INIT(index)                                                                  \
                                                                                                   \
	/*static void i2s_stm32_irq_config_func_##index(const struct device *dev); */                  \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct stm32_pclken clk_##index[] = STM32_DT_INST_CLOCKS(index);              \
                                                                                                   \
	struct i2s_stm32_sai_data sai_data_##index = {                                             \
		.hsai =                                                                            \
			{                                                                          \
				.Instance = (SAI_Block_TypeDef *)DT_INST_REG_ADDR(index),          \
				.Init.Synchro = SAI_ASYNCHRONOUS,                                  \
				.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE,                       \
				.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE,                        \
				.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_FULL,                     \
				.Init.SynchroExt = SAI_SYNCEXT_DISABLE,                            \
				.Init.MckOutput = SAI_MCK_OUTPUT_ENABLE,                           \
				.Init.CompandingMode = SAI_NOCOMPANDING,                           \
				.Init.TriState = SAI_OUTPUT_NOTRELEASED,                           \
			},                                                                         \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),                                       \
			(SAI_DMA_CHANNEL_INIT(index, tx, MEMORY, PERIPHERAL)),                     \
			(SAI_DMA_CHANNEL_INIT(index, rx, PERIPHERAL, MEMORY))),                      \
	};                                                                                         \
                                                                                                   \
	struct i2s_stm32_sai_cfg sai_config_##index = {                                            \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                   \
		.pclken = clk_##index,                                                             \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		/*.irq_config = i2s_stm32_irq_config_func_##index, */                                  \
		.mclk_sel = DT_INST_PROP(index, mck_enabled),                                      \
		.mclk_nodiv = DT_INST_PROP(index, no_divider),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_stm32_sai_initialize, NULL, &sai_data_##index,           \
			      &sai_config_##index, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,          \
			      &i2s_stm32_driver_api);                                              \
                                                                                                   \
	K_MSGQ_DEFINE(queue_##index, sizeof(struct queue_item), CONFIG_I2S_STM32_SAI_BLOCK_COUNT,  \
		      4);                                                                          \
                                                                                                   \
/*	static void i2s_stm32_irq_config_func_##index(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2s_stm32_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}*/

DT_INST_FOREACH_STATUS_OKAY(I2S_STM32_SAI_INIT)