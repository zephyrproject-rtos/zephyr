/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_i2s

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/dma.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_infineon);

#include "cy_tdm.h"

#define RX_QUEUE_SIZE CONFIG_I2S_INFINEON_RX_QUEUE_SIZE
#define TX_QUEUE_SIZE CONFIG_I2S_INFINEON_TX_QUEUE_SIZE

static int start_dma_tx_transfer(const struct device *dev);
static int start_dma_rx_transfer(const struct device *dev);
static int ifx_i2s_tx_stream_disable(const struct device *dev, bool drop);
static int ifx_i2s_rx_stream_disable(const struct device *dev, bool drop);

/* Device constant configuration data */
struct ifx_i2s_config {
	TDM_STRUCT_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	cy_stc_tdm_config_t tdm_config;
	uint32_t tx_irq_num;
	uint32_t rx_irq_num;
	void (*irq_config_func)(const struct device *dev);
};

struct queue_item {
	void *buffer;
	size_t size; // bytes
};

struct stream {
	int32_t state;
	struct i2s_config cfg;
	struct k_msgq queue;
	void *mem_block;
	size_t mem_block_len;
	bool xfer_pending;
	bool last_block;
	bool drain;
};

struct dma_channel {
	const struct device *dev_dma;
	uint32_t channel_num;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};

/* Device run time data */
struct ifx_i2s_data {
	struct stream rx;
	struct stream tx;
	struct dma_channel dma_rx;
	struct dma_channel dma_tx;
	struct queue_item rx_queue_buffer[RX_QUEUE_SIZE];
	struct queue_item tx_queue_buffer[TX_QUEUE_SIZE];
	struct ifx_cat1_clock clock;
	struct ifx_cat1_resource_inst resource;
	uint8_t clock_peri_group;
	bool tx_waiting_to_start;
};

/* This function is executed in the interrupt context */
static void dma_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	struct device *dev = (struct device *)arg;
	struct ifx_i2s_data *data = dev->data;
	const struct ifx_i2s_config *config = dev->config;
	TDM_TX_STRUCT_Type *tdm_tx = &((TDM_STRUCT_Type *)config->reg_addr)->TDM_TX_STRUCT;

	k_mem_slab_free(data->tx.cfg.mem_slab, data->tx.mem_block);
	data->tx.mem_block = NULL;

	if (data->tx.last_block && !data->tx.drain) {
		ifx_i2s_tx_stream_disable(dev, false);
		Cy_AudioTDM_DeActivateTx(tdm_tx);
		data->tx.state = I2S_STATE_READY;
		return;
	}

	if (data->tx.xfer_pending) {
		data->tx.xfer_pending = false;
		start_dma_tx_transfer(dev);
	}

	if (data->tx_waiting_to_start) {
		data->tx_waiting_to_start = false;
		Cy_AudioTDM_ClearTxInterrupt(tdm_tx, CY_TDM_INTR_TX_MASK);
		irq_enable(config->tx_irq_num);
		Cy_AudioTDM_ActivateTx(tdm_tx);
	}
}

/* This function is executed in the interrupt context */
static void dma_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	struct device *dev = (struct device *)arg;
	struct ifx_i2s_data *data = dev->data;
	struct queue_item queue_element;

	queue_element.buffer = data->rx.mem_block;
	queue_element.size = data->rx.mem_block_len;

	if (0 != k_msgq_put(&data->rx.queue, &queue_element, K_NO_WAIT)) {
		LOG_ERR("RX overflow, no space in RX queue");
		data->rx.state = I2S_STATE_ERROR;
	}

	data->rx.mem_block = NULL;

	if (data->rx.last_block) {
		ifx_i2s_rx_stream_disable(dev, false);
		data->rx.state = I2S_STATE_READY;
		return;
	}

	if (data->rx.xfer_pending) {
		data->rx.xfer_pending = false;
		start_dma_rx_transfer(dev);
	}
}

/* This function is executed in the interrupt context */
static void tx_fifo_trigger_handler(const struct device *dev)
{
	struct ifx_i2s_data *data = dev->data;
	struct stream *stream = &data->tx;

	switch(stream->state) {
	case I2S_STATE_RUNNING:
	case I2S_STATE_STOPPING:
		/* Continue transmission */
		if (stream->mem_block == NULL) {
			if (stream->last_block) {
				/* Don't start the next DMA transfer if we are on the last block.
				Just wait for the remaining data in the HW FIFO to be sent */
				stream->drain = true;
			} else {
				start_dma_tx_transfer(dev);
			}
		} else {
			/* Previous DMA transfer still in progress */
			data->tx.xfer_pending = true;
		}
		break;
	case I2S_STATE_ERROR:
		ifx_i2s_tx_stream_disable(dev, false);
		return;
	}
}

/* This function is executed in the interrupt context */
static void rx_fifo_trigger_handler(const struct device *dev)
{
	struct ifx_i2s_data *data = dev->data;
	struct stream *stream = &data->rx;

	switch(stream->state) {
	case I2S_STATE_RUNNING:
	case I2S_STATE_STOPPING:
		/* Continue transmission */
		if (stream->mem_block == NULL) {
			start_dma_rx_transfer(dev);
		} else {
			/* Previous DMA transfer still in progress */
			data->rx.xfer_pending = true;
		}
		break;
	case I2S_STATE_ERROR:
		ifx_i2s_rx_stream_disable(dev, false);
		return;
	}
}

static int start_dma_tx_transfer(const struct device *dev)
{
	int ret = 0;
	struct ifx_i2s_data *data = dev->data;
	struct stream *stream = &data->tx;
	struct dma_channel *dma_ch = &data->dma_tx;
	struct queue_item queue_element;

	ret = k_msgq_get(&stream->queue, &queue_element, K_NO_WAIT);
	if (ret != 0) {
		if (stream->state == I2S_STATE_STOPPING) {
			stream->last_block = true;
			stream->drain = true;
			stream->state = I2S_STATE_READY;
		} else {
			LOG_ERR("No data available to start transmission");
			stream->state = I2S_STATE_ERROR;
		}
		return ret;
	}

	stream->mem_block = queue_element.buffer;
	stream->mem_block_len = queue_element.size;
	dma_ch->blk_cfg.source_address = (uint32_t)queue_element.buffer;
	dma_ch->blk_cfg.block_size = (uint32_t)queue_element.size / dma_ch->dma_cfg.source_data_size;

	ret = dma_config(dma_ch->dev_dma, dma_ch->channel_num, &dma_ch->dma_cfg);
	if (ret < 0) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	ret = dma_start(dma_ch->dev_dma, dma_ch->channel_num);
	if (ret < 0) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	return 0;
}

static int start_dma_rx_transfer(const struct device *dev)
{
	int ret = 0;
	struct ifx_i2s_data *data = dev->data;
	struct stream *stream = &data->rx;
	struct dma_channel *dma_ch = &data->dma_rx;

	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret != 0) {
		LOG_DBG("No free memory block available for reception");
		ifx_i2s_rx_stream_disable(dev, false);
		stream->state = I2S_STATE_ERROR;
		return ret;
	}
	stream->mem_block_len = stream->cfg.block_size;

	dma_ch->blk_cfg.dest_address = (uint32_t)stream->mem_block;
	dma_ch->blk_cfg.block_size = (uint32_t)stream->mem_block_len / dma_ch->dma_cfg.source_data_size;

	ret = dma_config(dma_ch->dev_dma, dma_ch->channel_num, &dma_ch->dma_cfg);
	if (ret < 0) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	ret = dma_start(dma_ch->dev_dma, dma_ch->channel_num);
	if (ret < 0) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return ret;
	}

	return 0;
}

static int ifx_i2s_tx_stream_start(const struct device *dev)
{
	int ret = 0;
	struct ifx_i2s_data *const data = dev->data;

	data->tx_waiting_to_start = true;
	ret = start_dma_tx_transfer(dev);
	if (ret != 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", ret);
		return ret;
	}

	return 0;
}

static int ifx_i2s_rx_stream_start(const struct device *dev)
{
	const struct ifx_i2s_config *config = dev->config;
	TDM_RX_STRUCT_Type *tdm_rx = &(((TDM_STRUCT_Type *)config->reg_addr)->TDM_RX_STRUCT);

	Cy_AudioTDM_ClearRxInterrupt(tdm_rx, CY_TDM_INTR_RX_MASK);
	irq_enable(config->rx_irq_num);
	Cy_AudioTDM_ActivateRx(tdm_rx);

	return 0;
}

static int ifx_i2s_tx_stream_disable(const struct device *dev, bool drop)
{
	const struct ifx_i2s_config *config = dev->config;
	struct ifx_i2s_data *data = dev->data;
	struct stream *stream = &data->tx;
	struct queue_item queue_element;
	struct dma_channel *dma_ch = &data->dma_tx;

	irq_disable(config->tx_irq_num);

	dma_stop(dma_ch->dev_dma, dma_ch->channel_num);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}

	if (drop) {
		/* free all queued buffers */
		while (k_msgq_get(&stream->queue, &queue_element, K_NO_WAIT) == 0) {
			k_mem_slab_free(stream->cfg.mem_slab, queue_element.buffer);
		}
	}
	
	return 0;
}

static int ifx_i2s_rx_stream_disable(const struct device *dev, bool drop)
{
	const struct ifx_i2s_config *config = dev->config;
	struct ifx_i2s_data *data = dev->data;
	struct stream *stream = &data->rx;
	TDM_RX_STRUCT_Type *tdm_rx = &(((TDM_STRUCT_Type *)config->reg_addr)->TDM_RX_STRUCT);
	struct queue_item queue_element;
	struct dma_channel *dma_ch = &data->dma_rx;

	irq_disable(config->rx_irq_num);

	dma_stop(dma_ch->dev_dma, dma_ch->channel_num);
	Cy_AudioTDM_DeActivateRx(tdm_rx);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}

	if (drop) {
		/* free all queued buffers */
		while (k_msgq_get(&stream->queue, &queue_element, K_NO_WAIT) == 0) {
			k_mem_slab_free(stream->cfg.mem_slab, queue_element.buffer);
		}

		/* empty the RX hardware FIFO */
		while (Cy_AudioTDM_GetNumInRxFifo(tdm_rx) > 0) {
			(void)Cy_AudioTDM_ReadRxData(tdm_rx);
		}
	}

	return 0;
}

static int configiure_i2s_clock(const struct device *dev, enum i2s_dir dir)
{
	const struct ifx_i2s_config *config = dev->config;
	struct ifx_i2s_data *const data = dev->data;
	struct stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else {
		stream = &data->tx;
	}

	uint32_t clk_dest = PCLK_TDM0_CLK_IF_SRSS0 + data->clock.channel;
	uint32_t peri_freq = ifx_cat1_utils_peri_pclk_get_frequency((en_clk_dst_t)clk_dest, &data->clock);

	/* SCK = Fs * CH_SIZE * CH_NR */
	uint32_t i2s_sck = stream->cfg.frame_clk_freq * stream->cfg.word_size * stream->cfg.channels;
	/* F_peri / clkDiv = SCK
	   The valid range for clkDiv is 2 to 256 (even numbers should be selected to ensure a 50% duty cycle) */
	uint16_t clkDiv = peri_freq / i2s_sck;
	clkDiv = (clkDiv + 1) & ~1; // round up to the next even number
	if (clkDiv < 2) {
		clkDiv = 2;
	} else if (clkDiv > 256) {
		clkDiv = 256;
	}
	config->tdm_config.tx_config->clkDiv = clkDiv;
	LOG_DBG("I2S divider set to %u", config->tdm_config.tx_config->clkDiv);

	return 0;
}

static int ifx_i2s_configure(const struct device *dev, enum i2s_dir dir,
				  const struct i2s_config *i2s_cfg)
{
	const struct ifx_i2s_config *config = dev->config;
	struct ifx_i2s_data *const data = dev->data;
	struct stream *stream;
	struct queue_item queue_element;
	cy_en_tdm_ws_t tdm_word_size = 0;
	uint32_t dma_data_size_bytes = 0;
	bool bit_clk_slave = (0 != (I2S_OPT_BIT_CLK_SLAVE & i2s_cfg->options));
	bool frame_clk_slave = (0 != (I2S_OPT_FRAME_CLK_SLAVE & i2s_cfg->options));
	cy_en_tdm_device_cfg_t master_mode;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}
	
	if (stream->state != I2S_STATE_NOT_READY &&
	    stream->state != I2S_STATE_READY) {
		LOG_ERR("CFG ERR: %d", stream->state);
		return -EINVAL;
	}

	if (bit_clk_slave != frame_clk_slave) {
		LOG_ERR("Both bit clock and frame clock must be set to either master or slave");
		return -EINVAL;
	} else if (bit_clk_slave && frame_clk_slave) {
		master_mode = CY_TDM_DEVICE_SLAVE;
	} else {
		master_mode = CY_TDM_DEVICE_MASTER;
	}

	if (i2s_cfg->channels != 2) {
		LOG_ERR("Only stereo mode (2 channels) is supported");
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	switch (i2s_cfg->word_size) {
	case 8:
		tdm_word_size = CY_TDM_SIZE_8;
		dma_data_size_bytes = 1;
		break;
	case 10:
		tdm_word_size = CY_TDM_SIZE_10;
		dma_data_size_bytes = 2;
		break;
	case 12:
		tdm_word_size = CY_TDM_SIZE_12;
		dma_data_size_bytes = 2;
		break;
	case 14:
		tdm_word_size = CY_TDM_SIZE_14;
		dma_data_size_bytes = 2;
		break;
	case 16:
		tdm_word_size = CY_TDM_SIZE_16;
		dma_data_size_bytes = 2;
		break;
	case 18:
		tdm_word_size = CY_TDM_SIZE_18;
		dma_data_size_bytes = 3;
		break;
	case 20:
		tdm_word_size = CY_TDM_SIZE_20;
		dma_data_size_bytes = 3;
		break;
	case 24:
		tdm_word_size = CY_TDM_SIZE_24;
		dma_data_size_bytes = 3;
		break;
	case 32:
		tdm_word_size = CY_TDM_SIZE_32;
		dma_data_size_bytes = 4;
		break;
	default:
		LOG_ERR("Invalid word size %u", i2s_cfg->word_size);
		return -EINVAL;
	}

	/* Only the I2S data format is supported, so other format parameters are ignored */
	if (I2S_FMT_DATA_FORMAT_I2S != (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK)) {
		LOG_ERR("Only I2S data format is supported");
		return -ENOTSUP;
	} else {
		/* make sure no invalid format options are set */
		if (i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) {
			return -ENOTSUP;
		}
	}

	/* decode options (i2s_opt_t) */
	if (I2S_OPT_BIT_CLK_GATED & i2s_cfg->options) {
		LOG_ERR("Gated bit clock is not supported");
		return -ENOTSUP;
	}

	if (I2S_OPT_PINGPONG & i2s_cfg->options) {
		LOG_ERR("Ping-pong mode is not supported");
		return -ENOTSUP;
	}

	if (dir == I2S_DIR_RX) {
		/* prevent the tx block from being initialized */
		config->tdm_config.tx_config->enable = false;

		config->tdm_config.rx_config->enable = true;
		config->tdm_config.rx_config->wordSize = tdm_word_size;
		config->tdm_config.rx_config->masterMode = master_mode;
		/* Rx Trigger level should simply be the size of one mem_slab block */
		config->tdm_config.rx_config->fifoTriggerLevel = i2s_cfg->block_size / dma_data_size_bytes;

		/* configure DMA data sizes */
		data->dma_rx.dma_cfg.source_data_size = dma_data_size_bytes;
		data->dma_rx.dma_cfg.dest_data_size = dma_data_size_bytes;
		Cy_AudioTDM_DisableRx(&config->reg_addr->TDM_RX_STRUCT);
	} else {
		/* prevent the rx block from being initialized */
		config->tdm_config.rx_config->enable = false;

		config->tdm_config.tx_config->enable = true;
		config->tdm_config.tx_config->wordSize = tdm_word_size;
		config->tdm_config.tx_config->masterMode = master_mode;
		/* The hw fifo size is 128 elements */
		if (i2s_cfg->block_size / dma_data_size_bytes > 84) {
			LOG_ERR("TX block size is too large, must be less than or equal to 64 samples");
			return -EINVAL;
		}
		config->tdm_config.tx_config->fifoTriggerLevel = i2s_cfg->block_size / dma_data_size_bytes / 2;
		LOG_DBG("TX FIFO Trigger Level set to %u",
			config->tdm_config.tx_config->fifoTriggerLevel);
		
		/* configure DMA data sizes */
		data->dma_tx.dma_cfg.source_data_size = dma_data_size_bytes;
		data->dma_tx.dma_cfg.dest_data_size = dma_data_size_bytes;
		Cy_AudioTDM_DisableTx(&config->reg_addr->TDM_TX_STRUCT);
	}

	/* empty the queue in case of re-configure */
	while (k_msgq_get(&stream->queue, &queue_element, K_NO_WAIT) == 0) {
		/* free all queued buffers */
		k_mem_slab_free(stream->cfg.mem_slab, queue_element.buffer);
	}

	/* Save configuration for get_config */
	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	configiure_i2s_clock(dev, dir);

	if (CY_TDM_SUCCESS == Cy_AudioTDM_Init(config->reg_addr, &config->tdm_config)) {
		if (dir == I2S_DIR_RX) {
			Cy_AudioTDM_EnableRx(&config->reg_addr->TDM_RX_STRUCT);
			if (I2S_OPT_LOOPBACK & i2s_cfg->options) {
				Cy_AudioTDM_EnableRxTestMode(&config->reg_addr->TDM_RX_STRUCT);
			}

		} else {
			Cy_AudioTDM_EnableTx(&config->reg_addr->TDM_TX_STRUCT);
		}
	} else {
		LOG_ERR("TDM Init failed");
		return -EINVAL;
	}

	stream->state = I2S_STATE_READY;
	return 0;
}

static const struct i2s_config *ifx_i2s_config_get(const struct device *dev,
							enum i2s_dir dir)
{
	struct ifx_i2s_data *const data = dev->data;
	struct stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		LOG_ERR("I2S_DIR_BOTH not supported");
		return NULL;
	} else {
		LOG_ERR("Invalid direction");
		return NULL;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->cfg;
}

static int ifx_i2s_read(const struct device *dev, void **mem_block,
			size_t *size)
{
	struct ifx_i2s_data *const data = dev->data;
	struct stream *stream = &data->rx;
	struct queue_item queue_element;
	int ret = 0;

	if (stream->state == I2S_STATE_NOT_READY) {
		LOG_ERR("invalid state %d", stream->state);
		return -EIO;
	}

	ret = k_msgq_get(&stream->queue, &queue_element,
			 SYS_TIMEOUT_MS(stream->cfg.timeout));

	if (ret != 0) {
		if (stream->state == I2S_STATE_ERROR) {
			return -EIO;
		} else {
			return ret;
		}
	}

	*mem_block = queue_element.buffer;
	*size = queue_element.size;
	return 0;
}

static int ifx_i2s_write(const struct device *dev, void *mem_block,
			 size_t size)
{
	int ret;
	struct ifx_i2s_data *const data = dev->data;
	struct stream *stream = &data->tx;
	struct queue_item queue_element = {
		.buffer = mem_block,
		.size = size,
	};

	if (stream->state != I2S_STATE_RUNNING &&
		stream->state != I2S_STATE_READY) {
		LOG_ERR("invalid state (%d)", stream->state);
		return -EIO;
	}

	ret = k_msgq_put(&stream->queue, &queue_element,
			 SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (ret) {
		LOG_ERR("k_msgq_put failed %d", ret);
	}

	return ret;
}

static int ifx_i2s_trigger(const struct device *dev, enum i2s_dir dir,
			   enum i2s_trigger_cmd cmd)
{
	struct ifx_i2s_data *const data = dev->data;
	struct stream *stream;
	unsigned int key;
	int ret = 0;

	if (dir == I2S_DIR_RX){
		stream = &data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	key = irq_lock();

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d",
				    stream->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			ret = ifx_i2s_tx_stream_start(dev);
		} else {
			ret = ifx_i2s_rx_stream_start(dev);
		}

		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			break;
		}

		stream->xfer_pending = false;
		stream->last_block = false;
		stream->drain = false;
		stream->state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->last_block = true;
		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			stream->drain = true;
		} else {
			/* I2S_TRIGGER_DRAIN has the same effect as I2S_TRIGGER_STOP for RX */
			stream->last_block = true;
		}
		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		
		if (dir == I2S_DIR_TX) {
			ifx_i2s_tx_stream_disable(dev, true);
		} else {
			ifx_i2s_rx_stream_disable(dev, true);
		}
		
		stream->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}
		
		if (dir == I2S_DIR_TX) {
			ifx_i2s_tx_stream_disable(dev, true);
		} else {
			ifx_i2s_rx_stream_disable(dev, true);
		}

		stream->state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		ret = -EINVAL;
	}

	irq_unlock(key);

	return ret;
}

static int ifx_i2s_init(const struct device *dev)
{
	int ret = 0;
	struct ifx_i2s_data *const data = dev->data;
	const struct ifx_i2s_config *const config = dev->config;
	TDM_TX_STRUCT_Type *tdm_tx = &((TDM_STRUCT_Type *)config->reg_addr)->TDM_TX_STRUCT;
	TDM_RX_STRUCT_Type *tdm_rx = &((TDM_STRUCT_Type *)config->reg_addr)->TDM_RX_STRUCT;

	k_msgq_init(&data->tx.queue, (char*)data->tx_queue_buffer, sizeof(struct queue_item),
		    TX_QUEUE_SIZE);
	k_msgq_init(&data->rx.queue, (char*)data->rx_queue_buffer, sizeof(struct queue_item),
		    RX_QUEUE_SIZE);

	if (data->dma_rx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_rx.dev_dma)) {
			return -ENODEV;
		}
		data->dma_rx.blk_cfg.source_address = (uint32_t)(&tdm_rx->RX_FIFO_RD);
		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
		data->dma_rx.dma_cfg.user_data = (void *)dev;
		data->dma_rx.dma_cfg.dma_callback = dma_rx_callback;
	}

	if (data->dma_tx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_tx.dev_dma)) {
			return -ENODEV;
		}
		data->dma_tx.blk_cfg.dest_address = (uint32_t)(&tdm_tx->TX_FIFO_WR);
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
		data->dma_tx.dma_cfg.user_data = (void *)dev;
		data->dma_tx.dma_cfg.dma_callback = dma_tx_callback;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	config->irq_config_func(dev);
	Cy_AudioTDM_SetTxInterruptMask(tdm_tx, CY_TDM_INTR_TX_MASK & ~CY_TDM_INTR_TX_IF_UNDERFLOW);
	Cy_AudioTDM_SetRxInterruptMask(tdm_rx, CY_TDM_INTR_RX_MASK & ~CY_TDM_INTR_RX_IF_UNDERFLOW);

	data->rx.state = I2S_STATE_NOT_READY;
	data->tx.state = I2S_STATE_NOT_READY;

	/* make sure the rx fifo is empty (after soft reset) */
	while (Cy_AudioTDM_GetNumInRxFifo(tdm_rx) > 0) {
		(void)Cy_AudioTDM_ReadRxData(tdm_rx);
	}

	LOG_DBG("Device %s inited", dev->name);

	return 0;
}

/* This function is executed in the interrupt context */
static void ifx_i2s_tx_isr(const struct device *dev)
{
	const struct ifx_i2s_config *const config = dev->config;
	struct ifx_i2s_data *const data = dev->data;
	TDM_TX_STRUCT_Type *tdm_tx = &((TDM_STRUCT_Type *)config->reg_addr)->TDM_TX_STRUCT;
	struct stream *stream = &data->tx;

	uint32_t tx_int_status = Cy_AudioTDM_GetTxInterruptStatusMasked(tdm_tx);
	if (tx_int_status & CY_TDM_INTR_TX_FIFO_OVERFLOW) {
		stream->state = I2S_STATE_ERROR;
	}
	if (tx_int_status & CY_TDM_INTR_TX_FIFO_UNDERFLOW) {
		if (stream->last_block && stream->drain) {
			/* This indicates that we're done sending the last block */
			Cy_AudioTDM_DeActivateTx(tdm_tx);
			ifx_i2s_tx_stream_disable(dev, true);
			data->tx.state = I2S_STATE_READY;
		} else {
			stream->state = I2S_STATE_ERROR;
		}
	}
	if (tx_int_status & CY_TDM_INTR_TX_IF_UNDERFLOW) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("I2S TX IF underflow - indicates clocking issues");
		__ASSERT(false, "I2S TX IF underflow - indicates clocking issues");
	}
	if (tx_int_status & CY_TDM_INTR_TX_FIFO_TRIGGER) {
		tx_fifo_trigger_handler(dev);
	}
	Cy_AudioTDM_ClearTxInterrupt(tdm_tx, tx_int_status);
}

/* This function is executed in the interrupt context */
static void ifx_i2s_rx_isr(const struct device *dev)
{
	const struct ifx_i2s_config *const config = dev->config;
	struct ifx_i2s_data *const data = dev->data;
	TDM_RX_STRUCT_Type *tdm_rx = &((TDM_STRUCT_Type *)config->reg_addr)->TDM_RX_STRUCT;
	struct stream *stream = &data->rx;

	uint32_t rx_int_status = Cy_AudioTDM_GetRxInterruptStatusMasked(tdm_rx);
	if (rx_int_status & CY_TDM_INTR_RX_FIFO_OVERFLOW) {
		stream->state = I2S_STATE_ERROR;
	}
	if (rx_int_status & CY_TDM_INTR_RX_FIFO_UNDERFLOW) {
		stream->state = I2S_STATE_ERROR;
	}
	if (rx_int_status & CY_TDM_INTR_RX_IF_UNDERFLOW) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("I2S RX IF underflow - indicates clocking issues");
		__ASSERT(false, "I2S RX IF underflow - indicates clocking issues");
	}
	if (rx_int_status & CY_TDM_INTR_RX_FIFO_TRIGGER) {
		if (stream->state == I2S_STATE_STOPPING) {
			/* stop receiving new data but allow DMA to pull 
			   one more block off the hw fifo */
			Cy_AudioTDM_DeActivateRx(tdm_rx);
		}
		rx_fifo_trigger_handler(dev);
	}
	Cy_AudioTDM_ClearRxInterrupt(tdm_rx, rx_int_status);
}

static DEVICE_API(i2s, ifx_i2s_api) = {
	.configure = ifx_i2s_configure,
	.config_get = ifx_i2s_config_get,
	.read = ifx_i2s_read,
	.write = ifx_i2s_write,
	.trigger = ifx_i2s_trigger,
};

#define I2S_PERI_CLOCK_INFO(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),                 \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	},                                                                                         \
	.resource = {                                                                              \
		.type = DT_INST_PROP_BY_PHANDLE(n, clocks, resource_type),                         \
		.block_num = DT_INST_PROP_BY_PHANDLE(n, clocks, resource_instance),                \
		.channel_num = DT_INST_PROP_BY_PHANDLE(n, clocks, resource_channel),               \
	},                                                                                         \
	.clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),

#define I2S_DMA_CHANNEL_INIT(index, dir, ch_dir)                                                   \
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.channel_num = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = {                                                                               \
		.channel_direction = ch_dir,                                                       \
		.source_burst_length = 0,                                                          \
		.dest_burst_length = 0,                                                            \
		.block_count = 1,                                                                  \
		.complete_callback_en = 1,                                                         \
	},

#define I2S_DMA_CHANNEL(index, dir, ch_dir)                                                        \
	.dma_##dir = {COND_CODE_1(                                                                 \
		DT_INST_DMAS_HAS_NAME(index, dir),                                                 \
		(I2S_DMA_CHANNEL_INIT(index, dir, ch_dir)),                                        \
		(NULL))},

#define ifx_I2S_INIT(index)                                                                        \
                                                                                                   \
	static void ifx_i2s_irq_config_func_##index(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 0, priority), ifx_i2s_rx_isr,                \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 1, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 1, priority), ifx_i2s_tx_isr,                \
			    DEVICE_DT_INST_GET(index), 0);                                         \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static cy_stc_tdm_config_tx_t tx_config_##index = {                                        \
		.enable = true,                                                                    \
		.masterMode = CY_TDM_DEVICE_MASTER,                                                \
		.wordSize = CY_TDM_SIZE_16,                                                        \
		.format = CY_TDM_LEFT_DELAYED,         /* fixed for i2s mode */                    \
		.clkDiv = 2,                                                                       \
		.clkSel = (0 == DT_INST_PROP_BY_PHANDLE(index, clocks, channel) ?                  \
			CY_TDM_SEL_SRSS_CLK0 : CY_TDM_SEL_SRSS_CLK1),                              \
		.sckPolarity = CY_TDM_CLK,                                                         \
		.fsyncPolarity = CY_TDM_SIGN_INVERTED, /* fixed for i2s mode */                    \
		.fsyncFormat = CY_TDM_CH_PERIOD,       /* fixed for i2s mode */                    \
		.channelNum = 2,                       /* fixed for i2s mode */                    \
		.channelSize = 16,                                                                 \
		.fifoTriggerLevel = 32,                                                            \
		.chEn = 0x3,                                                                       \
		.signalInput = 0,                                                                  \
		.i2sMode = true};                      /* fixed for i2s mode */                    \
                                                                                                   \
	static cy_stc_tdm_config_rx_t rx_config_##index = {                                        \
		.enable = false,                                                                   \
		.masterMode = CY_TDM_DEVICE_SLAVE,                                                 \
		.wordSize = CY_TDM_SIZE_16,                                                        \
		.signExtend = CY_ZERO_EXTEND,                                                      \
		.format = CY_TDM_LEFT_DELAYED,         /* fixed for i2s mode */                    \
		.clkDiv = 2,                                                                       \
		.clkSel = (0 == DT_INST_PROP_BY_PHANDLE(index, clocks, channel) ?                  \
			CY_TDM_SEL_SRSS_CLK0 : CY_TDM_SEL_SRSS_CLK1),                              \
		.sckPolarity = CY_TDM_CLK,                                                         \
		.fsyncPolarity = CY_TDM_SIGN_INVERTED, /* fixed for i2s mode */                    \
		.lateSample = false,                                                               \
		.fsyncFormat = CY_TDM_CH_PERIOD,       /* fixed for i2s mode */                    \
		.channelNum = 2,                       /* fixed for i2s mode */                    \
		.channelSize = 16,                                                                 \
		.fifoTriggerLevel = 32,                                                            \
		.chEn = 0x3,                                                                       \
		.signalInput = 0,                                                                  \
		.i2sMode = true};                      /* fixed for i2s mode */                    \
                                                                                                   \
	static struct ifx_i2s_data i2s_data_##index = {                                            \
		I2S_DMA_CHANNEL(index, tx, MEMORY_TO_PERIPHERAL)                                   \
		I2S_DMA_CHANNEL(index, rx, PERIPHERAL_TO_MEMORY)                                   \
		I2S_PERI_CLOCK_INFO(index)                                                         \
		.tx_waiting_to_start = false,                                                      \
	};                                                                                         \
                                                                                                   \
	static struct ifx_i2s_config i2s_config_##index = {                                        \
		.reg_addr = (TDM_STRUCT_Type *)DT_INST_REG_ADDR(index),                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.tdm_config =                                                                      \
			{.tx_config = &tx_config_##index,                                          \
			 .rx_config = &rx_config_##index},                                         \
                                                                                                   \
		.rx_irq_num = DT_INST_IRQN_BY_IDX(index, 0),                                       \
		.tx_irq_num = DT_INST_IRQN_BY_IDX(index, 1),                                       \
		.irq_config_func = ifx_i2s_irq_config_func_##index                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &ifx_i2s_init, NULL, &i2s_data_##index,                       \
			      &i2s_config_##index, POST_KERNEL,                                    \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_i2s_api);

DT_INST_FOREACH_STATUS_OKAY(ifx_I2S_INIT)