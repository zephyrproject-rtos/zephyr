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

struct stream {
	DMA_TypeDef *reg;
	const struct device *dma_dev;
	struct dma_config dma_cfg;
	uint32_t dma_channel;

	struct i2s_config i2s_cfg;
	int32_t state;
	bool master;

	struct k_msgq queue;
	int (*start_transfer)(const struct device *dev, enum i2s_dir i2s_dir);
	void (*stop_transfer)(const struct device *dev, enum i2s_dir i2s_dir);
	void (*drop_queue)(struct stream *stream);
};

struct i2s_stm32_sai_data {
	const struct device *dev;
	SAI_HandleTypeDef hsai;

	struct stream dma;
	void *data_queue[CONFIG_I2S_STM32_SAI_BLOCK_COUNT];
};

struct i2s_stm32_sai_config {
	const struct pinctrl_dev_config *pincfg;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(const struct device *dev);
	bool mclk_sel;
	bool mclk_nodiv;
};

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

#ifdef CONFIG_I2S_STM32_SAI_DMA
static int stm32_dma_init(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;
	struct dma_config dma_cfg = dev_data->dma.dma_cfg;
	DMA_HandleTypeDef hdma;
	int ret;

	if (!device_is_ready(dev_data->dma.dma_dev)) {
		LOG_ERR("%s DMA device not ready", dev_data->dma.dma_dev->name);
		return -ENODEV;
	}
	LOG_DBG("%s DMA device: <OK>", dev_data->dma.dma_dev->name);

	/* Proceed to the minimum Zephyr DMA driver init */
	dma_cfg.user_data = &hdma;
	/* HACK: This field is used to inform driver that it is overridden */
	dma_cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;
	/* Because of the STREAM OFFSET, the DMA channel given here is from 1 -  */
	ret = dma_config(dev_data->dma.dma_dev, dev_data->dma.dma_channel + STM32_DMA_STREAM_OFFSET,
			 &dma_cfg);

	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d",
			dev_data->dma.dma_channel + STM32_DMA_STREAM_OFFSET);
		return ret;
	}
	LOG_DBG("DMA channel %d configured", dev_data->dma.dma_channel + STM32_DMA_STREAM_OFFSET);

	hdma.Instance = LL_DMA_GET_CHANNEL_INSTANCE(dev_data->dma.reg, dev_data->dma.dma_channel);
	hdma.Init.Request = dma_cfg.dma_slot;

	hdma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;

	hdma.Init.SrcInc = DMA_SINC_FIXED;
	hdma.Init.DestInc = DMA_DINC_FIXED;
	hdma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
	hdma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;

	hdma.Init.Priority = DMA_HIGH_PRIORITY;

	hdma.Init.SrcBurstLength = 1;
	hdma.Init.DestBurstLength = 1;
	hdma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
	hdma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
	hdma.Init.Mode = DMA_NORMAL;

	if (dev_data->dma.dma_cfg.channel_direction ==
	    (enum dma_channel_direction)MEMORY_TO_PERIPHERAL) {
		hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
		__HAL_LINKDMA(hsai, hdmatx, hdma);
	} else {
		__HAL_LINKDMA(hsai, hdmarx, hdma);
	}

	if (HAL_DMA_Init(&hdma) != HAL_OK) {
		LOG_ERR("HAL_DMA_Init: <FAILED>");
		return -EIO;
	}
	LOG_INF("HAL_DMA_Init: <OK>");

	if (HAL_DMA_ConfigChannelAttributes(&hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
		LOG_INF("HAL_DMA_ConfigChannelAttributes: <Failed>");
	}

	return 0;
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
	LOG_WRN("%s", __func__);
}

#endif

static int stm32_sai_enable_clock(const struct device *dev)
{
	const struct i2s_stm32_sai_config *cfg = dev->config;
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

static int i2s_stm32_initialize(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	const struct i2s_stm32_sai_config *cfg = dev->config;
	int ret = 0;

	/* Initialize the buffer queue */
	k_msgq_init(&dev_data->dma.queue, (char *)dev_data->data_queue, sizeof(void *),
		    CONFIG_I2S_STM32_SAI_BLOCK_COUNT);

	/* Configure DT provided pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2S pinctrl setup: <FAILED>");
		return ret;
	}
	LOG_DBG("I2S pinctrl setup: <OK>");

#ifdef CONFIG_I2S_STM32_SAI_DMA
	/* Initialize DMA peripheral */
	ret = stm32_dma_init(dev);
	if (ret < 0) {
		LOG_ERR("DMA initialization failed.");
		return ret;
	}
#endif

	/* Enable SAI clock */
	ret = stm32_sai_enable_clock(dev);
	if (ret < 0) {
		LOG_ERR("Clock enabling failed.");
		return -EIO;
	}

	/* Run IRQ init */
	cfg->irq_config(dev);

	k_sleep(K_MSEC(100));
	LOG_DBG("%s Init: <OK>", dev->name);

	return 0;
}

static void i2s_stm32_isr(const struct device *dev)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	HAL_SAI_IRQHandler(&dev_data->hsai);
}

static int i2s_stm32_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	const struct i2s_stm32_sai_config *cfg = dev->config;
	SAI_HandleTypeDef *hsai = &dev_data->hsai;

	struct stream *stream = &dev_data->dma;
	bool mclk_en;
	uint8_t protocol;
	uint8_t word_size;

	if (dir == I2S_DIR_RX) {
		LOG_DBG("I2S_DIR_RX");
	} else if (dir == I2S_DIR_TX) {
		LOG_DBG("I2S_DIR_TX");
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

	/* MckOutput */
	stream->master = true;
	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		stream->master = false;
	}

	mclk_en = stream->master && cfg->mclk_sel;
	if (mclk_en) {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
		LOG_DBG("MCLK ENABLED");
	} else {
		hsai->Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
		LOG_DBG("MCLK DISABLED");
	}

	/* NoDivider */
	if (cfg->mclk_nodiv) {
		hsai->Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
		LOG_DBG("SAI_MASTERDIVIDER_ENABLE");
	}

	/* FIFOThreshold */

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

static int i2s_stm32_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->dma;
	int ret;

	if (stream->state != I2S_STATE_RUNNING && stream->state != I2S_STATE_READY) {
		LOG_ERR("TX Invalid state: %d", stream->state);
		return -EIO;
	}

	ret = k_msgq_put(&stream->queue, &mem_block, SYS_TIMEOUT_MS(stream->i2s_cfg.timeout));
	if (ret) {
		LOG_DBG("k_msgq_put returned code %d", ret);
		return ret;
	}

	return ret;
}

static int i2s_stm32_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->dma;
	int ret;

	if (dir == I2S_DIR_BOTH) {
		LOG_ERR("Either RX or TX direction must be selected");
		return -ENOSYS;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %u", stream->state);
			ret = -EIO;
			break;
		}

		stream->state = I2S_STATE_RUNNING;

		ret = stream->start_transfer(dev, dir);
		if (ret < 0) {
			LOG_DBG("I2S_TRIGGER_START: <FAILED> %d", ret);
			ret = -EIO;
			break;
		}
		LOG_DBG("I2S_TRIGGER_START: <OK>");

		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->stop_transfer(dev, dir);
		stream->drop_queue(stream);
		stream->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->stop_transfer(dev, dir);
		break;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN/STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		stream->drop_queue(stream);
		stream->state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		return -EINVAL;
	}

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

static const struct i2s_driver_api i2s_stm32_driver_api = {
	.configure = i2s_stm32_configure,
	.write = i2s_stm32_write,
	.trigger = i2s_stm32_trigger,
};

static int sai_transmit(SAI_HandleTypeDef *hsai, struct stream *stream)
{
	void *buffer;
	int ret = 0;

	if (stream->state == I2S_STATE_RUNNING || stream->state == I2S_STATE_STOPPING) {
		ret = k_msgq_get(&stream->queue, &buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_DBG("No buffer in input queue to start");
			stream->state = I2S_STATE_READY;
			return 0;
		}
		/* dont free the slab, just in case we want to play the audio in repeat */
		k_mem_slab_free(stream->i2s_cfg.mem_slab, buffer);

#ifdef CONFIG_I2S_STM32_SAI_DMA
		if (HAL_SAI_Transmit_DMA(hsai, buffer, stream->i2s_cfg.block_size) != HAL_OK) {
			LOG_ERR("HAL_SAI_Transmit_DMA: <FAILED>");
			return -EIO;
		}
#else
		if (HAL_SAI_Transmit_IT(hsai, buffer, stream->i2s_cfg.block_size) != HAL_OK) {
			LOG_ERR("HAL_SAI_Transmit_IT: <FAILED>");
			return -EIO;
		}
#endif
	}

	return 0;
}

static int sai_start_transfer(const struct device *dev, enum i2s_dir i2s_dir)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->dma;

	if (i2s_dir == I2S_DIR_TX) {
		return sai_transmit(&dev_data->hsai, stream);
	} else if (i2s_dir == I2S_DIR_RX) {
		/* TODO */
	}

	return 0;
}

static void sai_stop_transfer(const struct device *dev, enum i2s_dir i2s_dir)
{
	struct i2s_stm32_sai_data *dev_data = dev->data;
	struct stream *stream = &dev_data->dma;
	void *buffer;
	int ret = 0;

	if (i2s_dir == I2S_DIR_TX) {
/* TODO */
#ifdef CONFIG_I2S_STM32_SAI_DMA
// DMAStop
#endif
		stream->state = I2S_STATE_READY;
	} else if (i2s_dir == I2S_DIR_RX) {
		/* TODO */
	}
}

static void sai_drop_queue(struct stream *stream)
{
	void *buffer;

	while (k_msgq_get(&stream->queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->i2s_cfg.mem_slab, buffer);
	}
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	struct i2s_stm32_sai_data *dev_data = CONTAINER_OF(hsai, struct i2s_stm32_sai_data, hsai);
	struct stream *stream = &dev_data->dma;

	sai_transmit(hsai, stream);
}

#define SAI_DMA_CHANNEL_INIT(index, dir, src_dev, dest_dev)                                        \
	.dma = {                                                                                   \
		.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(index, dir)),                              \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                     \
		.reg = (DMA_TypeDef *)DT_REG_ADDR(                                                 \
			DT_PHANDLE_BY_IDX(DT_DRV_INST(index), dmas, index)),                       \
		.dma_cfg =                                                                         \
			{                                                                          \
				.block_count = 2,                                                  \
				.dma_slot = STM32_DMA_SLOT(index, dir, slot),                      \
				.channel_direction = src_dev##_TO_##dest_dev,                      \
				.source_data_size = 2,    /* 16bit default */                      \
				.dest_data_size = 2,      /* 16bit default */                      \
				.source_burst_length = 1, /* SINGLE transfer */                    \
				.dest_burst_length = 1,   /* SINGLE transfer */                    \
				.channel_priority = STM32_DMA_CONFIG_PRIORITY(                     \
					STM32_DMA_CHANNEL_CONFIG(index, dir)),                     \
				.dma_callback = dma_callback,                                      \
			},                                                                         \
		.start_transfer = sai_start_transfer,                                              \
		.stop_transfer = sai_stop_transfer,                                                \
		.drop_queue = sai_drop_queue,                                                      \
	}

#define I2S_STM32_SAI_INIT(index)                                                                  \
                                                                                                   \
	static void i2s_stm32_irq_config_func_##index(const struct device *dev);                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct stm32_pclken clk_##index[] = STM32_DT_INST_CLOCKS(index);              \
                                                                                                   \
	struct i2s_stm32_sai_data sai##index_data = {                                              \
		.hsai =                                                                            \
			{                                                                          \
				.Instance = (SAI_Block_TypeDef *)DT_INST_REG_ADDR(index),          \
				.Init.AudioMode = SAI_MODEMASTER_TX,                               \
				.Init.Synchro = SAI_ASYNCHRONOUS,                                  \
				.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE,                       \
				.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE,                        \
				.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_FULL,                      \
				.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_32K,                    \
				.Init.SynchroExt = SAI_SYNCEXT_DISABLE,                            \
				.Init.MckOutput = SAI_MCK_OUTPUT_ENABLE,                           \
				.Init.MonoStereoMode = SAI_STEREOMODE,                             \
				.Init.CompandingMode = SAI_NOCOMPANDING,                           \
				.Init.TriState = SAI_OUTPUT_NOTRELEASED,                           \
			},                                                                         \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),                                       \
			(SAI_DMA_CHANNEL_INIT(index, tx, MEMORY, PERIPHERAL)),                     \
			(SAI_DMA_CHANNEL_INIT(index, rx, PERIPHERAL, MEMORY))),                      \
	};                                                                                         \
                                                                                                   \
	struct i2s_stm32_sai_config sai##index_config = {                                          \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                   \
		.pclken = clk_##index,                                                             \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_config = i2s_stm32_irq_config_func_##index,                                   \
		.mclk_sel = DT_INST_PROP(index, mck_enabled),                                      \
		.mclk_nodiv = DT_INST_PROP(index, no_divider),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_stm32_initialize, NULL, &sai##index_data,                \
			      &sai##index_config, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,           \
			      &i2s_stm32_driver_api);                                              \
                                                                                                   \
	static void i2s_stm32_irq_config_func_##index(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2s_stm32_isr,      \
			    DEVICE_DT_INST_GET(index), index);                                     \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

DT_INST_FOREACH_STATUS_OKAY(I2S_STM32_SAI_INIT)