/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tdm_i2s

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2s_infineon_tdm, CONFIG_I2S_LOG_LEVEL);

#define TDM_CTL         0x00U
#define TDM_IF_CTL      0x10U
#define TDM_CH_CTL      0x14U
#define TDM_TEST_CTL    0x20U
#define TDM_FIFO_CTL    0x80U
#define TDM_FIFO_DATA   0x88U
#define TDM_INTR        0xC0U
#define TDM_INTR_MASK   0xC8U
#define TDM_INTR_MASKED 0xCCU
#define TDM_RX          0x100U

#define TDM_CTL_MS      BIT(16)
#define TDM_CTL_ENABLED BIT(31)

#define TDM_IF_CTL_CLOCK_SEL GENMASK(10, 8)
#define TDM_IF_CTL_CH_SIZE   GENMASK(28, 24)
#define TDM_IF_CTL_I2S       (BIT(31) | BIT(16) | BIT(15) | BIT(13))

#define TDM_CH_CTL_STEREO    0x3U
#define TDM_TEST_CTL_ENABLED BIT(31)
#define TDM_FIFO_CTL_ACTIVE  BIT(18)

#define TDM_INTR_TRIGGER   BIT(0)
#define TDM_INTR_OVERFLOW  BIT(1)
#define TDM_INTR_UNDERFLOW BIT(2)
#define TDM_INTR_IF        BIT(8)
#define TDM_INTR_ALL       (TDM_INTR_TRIGGER | TDM_INTR_OVERFLOW | TDM_INTR_UNDERFLOW | TDM_INTR_IF)

#define TDM_XFER_MAX_WORDS   64U
#define TDM_TX_TRIGGER_LEVEL 48U
#define TDM_TX_DUMMY_WORDS   6U
#define TDM_CLK_HF_SRSS0     5U

struct ifx_tdm_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	const struct device *dma_tx;
	const struct device *dma_rx;
	uint32_t irq_tx;
	uint32_t irq_rx;
	void (*irq_config)(void);
	struct ifx_clk clk;
	uint8_t dma_tx_ch;
	uint8_t dma_rx_ch;
};

struct ifx_tdm_stream {
	struct i2s_config cfg;
	struct k_msgq queue;
	struct dma_config dma_cfg;
	struct dma_block_config blk;
	void *mem_block;
	uint8_t *next;
	uint32_t remaining;
	uint16_t max_xfer;
	uint8_t state;
	bool last_block;
	bool drain;
	bool waiting_to_start;
};

struct ifx_tdm_tx_item {
	void *buffer;
	size_t size;
};

struct ifx_tdm_data {
	struct ifx_tdm_stream rx;
	struct ifx_tdm_stream tx;
	void *rx_items[CONFIG_I2S_INFINEON_TDM_RX_QUEUE_SIZE];
	struct ifx_tdm_tx_item tx_items[CONFIG_I2S_INFINEON_TDM_TX_QUEUE_SIZE];
};

static void ifx_tdm_flush_queue(struct ifx_tdm_stream *stream)
{
	struct ifx_tdm_tx_item item;

	while (k_msgq_get(&stream->queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, item.buffer);
	}
}

static int ifx_tdm_word_code(uint8_t word_size, uint32_t *ctl)
{
	switch (word_size) {
	case 8:
	case 10:
	case 12:
	case 14:
	case 16:
	case 18:
	case 20:
		*ctl = (word_size - 8U) / 2U;
		return 0;
	case 24:
		*ctl = 7U;
		return 0;
	case 32:
		*ctl = 8U;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ifx_tdm_configure(const struct device *dev, enum i2s_dir dir,
			     const struct i2s_config *i2s_cfg)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream;
	mm_reg_t reg = config->base;
	uint8_t ws = i2s_cfg->word_size;
	uint32_t div = 2U;
	uint32_t ctl;
	uint32_t if_ctl;
	uint32_t words;
	uint32_t level;
	uint32_t rate;
	uint8_t dsize;
	int ret;

	switch (dir) {
	case I2S_DIR_RX:
		stream = &data->rx;
		reg += TDM_RX;
		break;
	case I2S_DIR_TX:
		stream = &data->tx;
		break;
	case I2S_DIR_BOTH:
		LOG_ERR("I2S_DIR_BOTH not supported");
		return -ENOSYS;
	default:
		LOG_ERR("Invalid direction");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		LOG_ERR("CFG ERR: %d", stream->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	if (((i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET) == 0U) !=
	    ((i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET) == 0U)) {
		LOG_ERR("Both bit clock and frame clock must be set to either master or slave");
		return -EINVAL;
	}

	if (i2s_cfg->channels != 2U) {
		LOG_ERR("Only stereo mode (2 channels) is supported");
		return -EINVAL;
	}

	if (ifx_tdm_word_code(ws, &ctl) != 0) {
		LOG_ERR("Invalid word size %u", ws);
		return -EINVAL;
	}

	if ((i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) != I2S_FMT_DATA_FORMAT_I2S) {
		LOG_ERR("Only I2S data format is supported");
		return -ENOTSUP;
	}

	if ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		return -ENOTSUP;
	}

	if ((i2s_cfg->options & I2S_OPT_BIT_CLK_GATED) != 0U) {
		LOG_ERR("Gated bit clock is not supported");
		return -ENOTSUP;
	}

	if ((i2s_cfg->options & I2S_OPT_PINGPONG) != 0U) {
		LOG_ERR("Ping-pong mode is not supported");
		return -ENOTSUP;
	}

	dsize = (ws <= 8U) ? 1U : ((ws <= 16U) ? 2U : 4U);
	if (i2s_cfg->block_size < dsize) {
		LOG_ERR("Block size %zu too small", i2s_cfg->block_size);
		return -EINVAL;
	}

	if ((i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET) == 0U) {
		ctl |= TDM_CTL_MS;
		ret = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clk,
					     &rate);
		if (ret < 0) {
			return ret;
		}
		div = rate / (i2s_cfg->frame_clk_freq * ws * 2U);
		div = CLAMP((div + 1U) & ~1U, 2U, 256U);
	}

	words = i2s_cfg->block_size / dsize;
	if (words < TDM_XFER_MAX_WORDS) {
		stream->max_xfer = (uint16_t)i2s_cfg->block_size;
		level = (dir == I2S_DIR_TX) ? MIN(words - 1U, TDM_TX_TRIGGER_LEVEL) : words;
	} else {
		stream->max_xfer = (uint16_t)(TDM_XFER_MAX_WORDS * dsize);
		level = (dir == I2S_DIR_TX) ? TDM_TX_TRIGGER_LEVEL : TDM_XFER_MAX_WORDS;
	}

	ifx_tdm_flush_queue(stream);

	stream->cfg = *i2s_cfg;
	stream->dma_cfg.source_data_size = dsize;
	stream->dma_cfg.dest_data_size = dsize;

	if_ctl = TDM_IF_CTL_I2S | FIELD_PREP(TDM_IF_CTL_CH_SIZE, ws - 1U) |
		 FIELD_PREP(TDM_IF_CTL_CLOCK_SEL, config->clk.clk_id - TDM_CLK_HF_SRSS0) |
		 (div - 1U);

	sys_write32(ctl, reg + TDM_CTL);
	sys_write32(if_ctl, reg + TDM_IF_CTL);
	sys_write32(TDM_CH_CTL_STEREO, reg + TDM_CH_CTL);
	sys_write32(level, reg + TDM_FIFO_CTL);
	if (dir == I2S_DIR_RX) {
		sys_write32(((i2s_cfg->options & I2S_OPT_LOOPBACK) != 0U) ? TDM_TEST_CTL_ENABLED
									  : 0U,
			    reg + TDM_TEST_CTL);
	}
	sys_write32(ctl | TDM_CTL_ENABLED, reg + TDM_CTL);

	stream->state = I2S_STATE_READY;

	return 0;
}

static const struct i2s_config *ifx_tdm_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream;

	switch (dir) {
	case I2S_DIR_RX:
		stream = &data->rx;
		break;
	case I2S_DIR_TX:
		stream = &data->tx;
		break;
	case I2S_DIR_BOTH:
		LOG_ERR("I2S_DIR_BOTH not supported");
		return NULL;
	default:
		LOG_ERR("Invalid direction");
		return NULL;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &stream->cfg;
}

static void ifx_tdm_tx_fill_zeros(mm_reg_t base)
{
	sys_write32(TDM_INTR_ALL & ~TDM_INTR_TRIGGER, base + TDM_INTR_MASK);
	for (uint32_t i = 0U; i < TDM_TX_DUMMY_WORDS; i++) {
		sys_write32(0U, base + TDM_FIFO_DATA);
	}
}

static int ifx_tdm_tx_next(const struct device *dev)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->tx;
	struct ifx_tdm_tx_item item;
	uint32_t len;
	int ret;

	if (stream->remaining == 0U) {
		ret = k_msgq_get(&stream->queue, &item, K_NO_WAIT);
		if (ret != 0) {
			if (stream->state == I2S_STATE_STOPPING) {
				stream->last_block = true;
				stream->drain = true;
			}
			ifx_tdm_tx_fill_zeros(config->base);
			return ret;
		}
		stream->mem_block = item.buffer;
		stream->next = item.buffer;
		stream->remaining = item.size;
	}

	len = MIN(stream->remaining, stream->max_xfer);
	stream->blk.source_address = (uint32_t)stream->next;
	stream->blk.block_size = len;
	stream->next += len;
	stream->remaining -= len;

	ret = dma_config(config->dma_tx, config->dma_tx_ch, &stream->dma_cfg);
	if (ret == 0) {
		ret = dma_start(config->dma_tx, config->dma_tx_ch);
	}
	if (ret < 0) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}

	return ret;
}

static int ifx_tdm_tx_start(const struct device *dev)
{
	struct ifx_tdm_data *data = dev->data;
	int ret;

	data->tx.waiting_to_start = true;
	data->tx.remaining = 0U;

	ret = ifx_tdm_tx_next(dev);
	if (ret != 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", ret);
	}

	return ret;
}

static void ifx_tdm_tx_stop(const struct device *dev, bool drop)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->tx;
	mm_reg_t base = config->base;
	uint32_t ctl;

	sys_write32(sys_read32(base + TDM_FIFO_CTL) & ~TDM_FIFO_CTL_ACTIVE, base + TDM_FIFO_CTL);
	ctl = sys_read32(base + TDM_CTL);
	sys_write32(ctl & ~TDM_CTL_ENABLED, base + TDM_CTL);
	sys_write32(ctl | TDM_CTL_ENABLED, base + TDM_CTL);
	irq_disable(config->irq_tx);
	(void)dma_stop(config->dma_tx, config->dma_tx_ch);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}

	if (drop) {
		ifx_tdm_flush_queue(stream);
	}
}

static void ifx_tdm_tx_dma_done(const struct device *dma_dev, void *arg, uint32_t channel,
				int status)
{
	const struct device *dev = arg;
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->tx;
	mm_reg_t base = config->base;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	if (stream->remaining == 0U) {
		if (stream->mem_block == NULL) {
			LOG_ERR("TX block buffer is NULL in DMA callback");
			stream->state = I2S_STATE_ERROR;
		} else {
			k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
			stream->mem_block = NULL;
		}
	}

	if (stream->waiting_to_start) {
		stream->waiting_to_start = false;
		sys_write32(TDM_INTR_ALL, base + TDM_INTR_MASK);
		sys_write32(TDM_INTR_ALL, base + TDM_INTR);
		(void)sys_read32(base + TDM_INTR);
		irq_enable(config->irq_tx);
		sys_write32(sys_read32(base + TDM_FIFO_CTL) | TDM_FIFO_CTL_ACTIVE,
			    base + TDM_FIFO_CTL);
	} else {
		sys_write32(TDM_INTR_TRIGGER, base + TDM_INTR);
		(void)sys_read32(base + TDM_INTR);
		sys_write32(TDM_INTR_ALL, base + TDM_INTR_MASK);
	}
}

static void ifx_tdm_tx_isr(const struct device *dev)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->tx;
	mm_reg_t base = config->base;
	uint32_t status = sys_read32(base + TDM_INTR_MASKED);

	if ((status & TDM_INTR_OVERFLOW) != 0U) {
		stream->state = I2S_STATE_ERROR;
	}

	if ((status & TDM_INTR_UNDERFLOW) != 0U) {
		ifx_tdm_tx_stop(dev, false);
		if (stream->last_block && stream->drain) {
			stream->state = I2S_STATE_READY;
		} else {
			LOG_DBG("I2S TX FIFO underflow");
			stream->state = I2S_STATE_ERROR;
		}
	}

	if ((status & TDM_INTR_IF) != 0U) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("I2S TX IF underflow - indicates clocking issues");
		__ASSERT(false, "I2S TX IF underflow - indicates clocking issues");
	}

	if ((status & TDM_INTR_TRIGGER) != 0U) {
		switch (stream->state) {
		case I2S_STATE_RUNNING:
		case I2S_STATE_STOPPING:
			if (stream->last_block && stream->remaining == 0U) {
				stream->drain = true;
				ifx_tdm_tx_fill_zeros(base);
			} else {
				sys_write32(TDM_INTR_ALL & ~TDM_INTR_TRIGGER, base + TDM_INTR_MASK);
				(void)ifx_tdm_tx_next(dev);
			}
			break;
		case I2S_STATE_ERROR:
			ifx_tdm_tx_stop(dev, false);
			break;
		default:
			LOG_ERR("Tx trigger handler: unhandled state: %d", stream->state);
			break;
		}
	}

	sys_write32(status, base + TDM_INTR);
	(void)sys_read32(base + TDM_INTR);
}

static int ifx_tdm_write(const struct device *dev, void *mem_block, size_t size)
{
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->tx;
	struct ifx_tdm_tx_item item = {
		.buffer = mem_block,
		.size = size,
	};
	int ret;

	if (stream->state != I2S_STATE_RUNNING && stream->state != I2S_STATE_READY) {
		LOG_DBG("Write: invalid state (%d)", stream->state);
		return -EIO;
	}

	ret = k_msgq_put(&stream->queue, &item, SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (ret != 0) {
		LOG_ERR("k_msgq_put failed %d", ret);
	}

	return ret;
}

static void ifx_tdm_rx_stop(const struct device *dev, bool drop)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->rx;
	mm_reg_t reg = config->base + TDM_RX;

	sys_write32(sys_read32(reg + TDM_FIFO_CTL) & ~TDM_FIFO_CTL_ACTIVE, reg + TDM_FIFO_CTL);
	sys_write32(sys_read32(reg + TDM_CTL) & ~TDM_CTL_ENABLED, reg + TDM_CTL);
	irq_disable(config->irq_rx);
	(void)dma_stop(config->dma_rx, config->dma_rx_ch);

	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
	}

	if (drop) {
		ifx_tdm_flush_queue(stream);
	}
}

static void ifx_tdm_rx_next(const struct device *dev)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->rx;
	uint32_t len;
	int ret;

	if (stream->remaining == 0U) {
		ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret != 0) {
			LOG_WRN("No free memory block available for reception");
			ifx_tdm_rx_stop(dev, false);
			stream->state = I2S_STATE_ERROR;
			return;
		}
		stream->next = stream->mem_block;
		stream->remaining = stream->cfg.block_size;
	}

	len = MIN(stream->remaining, stream->max_xfer);
	stream->blk.dest_address = (uint32_t)stream->next;
	stream->blk.block_size = len;
	stream->next += len;
	stream->remaining -= len;

	ret = dma_config(config->dma_rx, config->dma_rx_ch, &stream->dma_cfg);
	if (ret == 0) {
		ret = dma_start(config->dma_rx, config->dma_rx_ch);
	}
	if (ret < 0) {
		k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
		stream->mem_block = NULL;
		return;
	}

	sys_write32(TDM_INTR_ALL & ~TDM_INTR_TRIGGER, config->base + TDM_RX + TDM_INTR_MASK);
}

static int ifx_tdm_rx_start(const struct device *dev)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	mm_reg_t reg = config->base + TDM_RX;

	data->rx.remaining = 0U;

	sys_write32(TDM_INTR_ALL, reg + TDM_INTR);
	(void)sys_read32(reg + TDM_INTR);
	sys_write32(TDM_INTR_ALL, reg + TDM_INTR_MASK);
	irq_enable(config->irq_rx);
	sys_write32(sys_read32(reg + TDM_CTL) | TDM_CTL_ENABLED, reg + TDM_CTL);
	sys_write32(sys_read32(reg + TDM_FIFO_CTL) | TDM_FIFO_CTL_ACTIVE, reg + TDM_FIFO_CTL);

	return 0;
}

static void ifx_tdm_rx_dma_done(const struct device *dma_dev, void *arg, uint32_t channel,
				int status)
{
	const struct device *dev = arg;
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->rx;
	mm_reg_t reg = config->base + TDM_RX;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	if (stream->remaining == 0U) {
		if (k_msgq_put(&stream->queue, &stream->mem_block, K_NO_WAIT) != 0) {
			LOG_ERR("RX overflow, no space in RX queue");
			stream->state = I2S_STATE_ERROR;
			return;
		}

		stream->mem_block = NULL;

		if (stream->last_block) {
			ifx_tdm_rx_stop(dev, false);
			stream->state = I2S_STATE_READY;
			return;
		}
	}

	sys_write32(TDM_INTR_TRIGGER, reg + TDM_INTR);
	(void)sys_read32(reg + TDM_INTR);
	sys_write32(TDM_INTR_ALL, reg + TDM_INTR_MASK);
}

static void ifx_tdm_rx_isr(const struct device *dev)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->rx;
	mm_reg_t reg = config->base + TDM_RX;
	uint32_t status = sys_read32(reg + TDM_INTR_MASKED);

	if ((status & (TDM_INTR_OVERFLOW | TDM_INTR_UNDERFLOW)) != 0U) {
		stream->state = I2S_STATE_ERROR;
	}

	if ((status & TDM_INTR_IF) != 0U) {
		stream->state = I2S_STATE_ERROR;
		LOG_ERR("I2S RX IF underflow - indicates clocking issues");
		__ASSERT(false, "I2S RX IF underflow - indicates clocking issues");
	}

	if ((status & TDM_INTR_TRIGGER) != 0U) {
		switch (stream->state) {
		case I2S_STATE_RUNNING:
		case I2S_STATE_STOPPING:
			ifx_tdm_rx_next(dev);
			break;
		case I2S_STATE_ERROR:
			ifx_tdm_rx_stop(dev, false);
			break;
		default:
			LOG_ERR("Rx trigger handler: unhandled state: %d", stream->state);
			break;
		}
	}

	sys_write32(status, reg + TDM_INTR);
	(void)sys_read32(reg + TDM_INTR);
}

static int ifx_tdm_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream = &data->rx;
	int ret;

	if (stream->state == I2S_STATE_NOT_READY) {
		LOG_DBG("Read: invalid state %d", stream->state);
		return -EIO;
	}

	ret = k_msgq_get(&stream->queue, mem_block, SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (ret != 0) {
		LOG_DBG("I2S Read: k_msgq_get failed %d", ret);
		return (stream->state == I2S_STATE_ERROR) ? -EIO : ret;
	}

	*size = stream->cfg.block_size;

	return 0;
}

static int ifx_tdm_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct ifx_tdm_data *data = dev->data;
	struct ifx_tdm_stream *stream;
	unsigned int key;
	int ret = 0;

	switch (dir) {
	case I2S_DIR_RX:
		stream = &data->rx;
		break;
	case I2S_DIR_TX:
		stream = &data->tx;
		break;
	case I2S_DIR_BOTH:
		LOG_ERR("I2S_DIR_BOTH not supported");
		return -ENOSYS;
	default:
		return -EINVAL;
	}

	key = irq_lock();

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_DBG("START trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->last_block = false;
		stream->drain = false;
		ret = (dir == I2S_DIR_TX) ? ifx_tdm_tx_start(dev) : ifx_tdm_rx_start(dev);
		if (ret < 0) {
			LOG_DBG("START trigger failed %d", ret);
			break;
		}

		stream->state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_DBG("STOP trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		stream->last_block = true;
		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_DBG("DRAIN trigger: invalid state %d", stream->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			stream->drain = true;
		} else {
			stream->last_block = true;
		}
		stream->state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_DROP:
	case I2S_TRIGGER_PREPARE:
		if ((cmd == I2S_TRIGGER_DROP) ? (stream->state == I2S_STATE_NOT_READY)
					      : (stream->state != I2S_STATE_ERROR)) {
			LOG_DBG("%s trigger: invalid state %d",
				(cmd == I2S_TRIGGER_DROP) ? "DROP" : "PREPARE", stream->state);
			ret = -EIO;
			break;
		}

		if (dir == I2S_DIR_TX) {
			ifx_tdm_tx_stop(dev, true);
		} else {
			ifx_tdm_rx_stop(dev, true);
		}
		stream->state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		ret = -EINVAL;
		break;
	}

	irq_unlock(key);

	return ret;
}

static int ifx_tdm_init(const struct device *dev)
{
	const struct ifx_tdm_config *config = dev->config;
	struct ifx_tdm_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->dma_tx) || !device_is_ready(config->dma_rx)) {
		return -ENODEV;
	}

	k_msgq_init(&data->tx.queue, (char *)data->tx_items, sizeof(data->tx_items[0]),
		    CONFIG_I2S_INFINEON_TDM_TX_QUEUE_SIZE);
	k_msgq_init(&data->rx.queue, (char *)data->rx_items, sizeof(data->rx_items[0]),
		    CONFIG_I2S_INFINEON_TDM_RX_QUEUE_SIZE);

	data->tx.dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	data->tx.dma_cfg.block_count = 1U;
	data->tx.dma_cfg.complete_callback_en = 1U;
	data->tx.dma_cfg.source_handshake = 1U;
	data->tx.dma_cfg.head_block = &data->tx.blk;
	data->tx.dma_cfg.user_data = (void *)dev;
	data->tx.dma_cfg.dma_callback = ifx_tdm_tx_dma_done;
	data->tx.blk.dest_address = config->base + TDM_FIFO_DATA;
	data->tx.blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	data->rx.dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	data->rx.dma_cfg.block_count = 1U;
	data->rx.dma_cfg.complete_callback_en = 1U;
	data->rx.dma_cfg.source_handshake = 1U;
	data->rx.dma_cfg.head_block = &data->rx.blk;
	data->rx.dma_cfg.user_data = (void *)dev;
	data->rx.dma_cfg.dma_callback = ifx_tdm_rx_dma_done;
	data->rx.blk.source_address = config->base + TDM_RX + TDM_FIFO_DATA;
	data->rx.blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	config->irq_config();

	return 0;
}

static DEVICE_API(i2s, ifx_tdm_api) = {
	.configure = ifx_tdm_configure,
	.config_get = ifx_tdm_config_get,
	.read = ifx_tdm_read,
	.write = ifx_tdm_write,
	.trigger = ifx_tdm_trigger,
};

#define IFX_TDM_INIT(n)                                                                            \
	BUILD_ASSERT(IN_RANGE(DT_REG_ADDR(DT_INST_CLOCKS_CTLR(n)), TDM_CLK_HF_SRSS0,               \
			      TDM_CLK_HF_SRSS0 + 2),                                               \
		     "clocks must be clk_hf5, clk_hf6 or clk_hf7");                                \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void ifx_tdm_irq_config_##n(void)                                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN_BY_NAME(n, rx), DT_INST_IRQ_BY_NAME(n, rx, priority),     \
			    ifx_tdm_rx_isr, DEVICE_DT_INST_GET(n), 0);                             \
		IRQ_CONNECT(DT_INST_IRQN_BY_NAME(n, tx), DT_INST_IRQ_BY_NAME(n, tx, priority),     \
			    ifx_tdm_tx_isr, DEVICE_DT_INST_GET(n), 0);                             \
	}                                                                                          \
	static struct ifx_tdm_data ifx_tdm_data_##n;                                               \
	static const struct ifx_tdm_config ifx_tdm_config_##n = {                                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_GPARENT(DT_INST_CLOCKS_CTLR(n))),                      \
		.dma_tx = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),                         \
		.dma_rx = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),                         \
		.irq_tx = DT_INST_IRQN_BY_NAME(n, tx),                                             \
		.irq_rx = DT_INST_IRQN_BY_NAME(n, rx),                                             \
		.irq_config = ifx_tdm_irq_config_##n,                                              \
		.clk = {.clk = IFX_CLK_HF, .clk_id = DT_REG_ADDR(DT_INST_CLOCKS_CTLR(n))},         \
		.dma_tx_ch = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),                            \
		.dma_rx_ch = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ifx_tdm_init, NULL, &ifx_tdm_data_##n, &ifx_tdm_config_##n,       \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_tdm_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_TDM_INIT)
