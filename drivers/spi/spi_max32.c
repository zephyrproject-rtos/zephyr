/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_spi

#include <string.h>
#include <errno.h>
#if CONFIG_SPI_MAX32_DMA
#include <zephyr/drivers/dma.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/spi/rtio.h>

#include <wrap_max32_spi.h>

LOG_MODULE_REGISTER(spi_max32, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

#ifdef CONFIG_SPI_MAX32_DMA
struct max32_spi_dma_config {
	const struct device *dev;
	const uint32_t channel;
	const uint32_t slot;
};
#endif /* CONFIG_SPI_MAX32_DMA */

struct max32_spi_config {
	mxc_spi_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
#ifdef CONFIG_SPI_MAX32_INTERRUPT
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_SPI_MAX32_INTERRUPT */
#ifdef CONFIG_SPI_MAX32_DMA
	struct max32_spi_dma_config tx_dma;
	struct max32_spi_dma_config rx_dma;
#endif /* CONFIG_SPI_MAX32_DMA */
};

/* Device run time data */
struct max32_spi_data {
	struct spi_context ctx;
	const struct device *dev;
	mxc_spi_req_t req;
	uint8_t dummy[2];

#ifdef CONFIG_SPI_MAX32_DMA
	volatile uint8_t dma_stat;
#endif /* CONFIG_SPI_MAX32_DMA */

#ifdef CONFIG_SPI_ASYNC
	struct k_work async_work;
#endif /* CONFIG_SPI_ASYNC */

#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif
};

#ifdef CONFIG_SPI_MAX32_DMA
#define SPI_MAX32_DMA_ERROR_FLAG   0x01U
#define SPI_MAX32_DMA_RX_DONE_FLAG 0x02U
#define SPI_MAX32_DMA_TX_DONE_FLAG 0x04U
#define SPI_MAX32_DMA_DONE_FLAG    (SPI_MAX32_DMA_RX_DONE_FLAG | SPI_MAX32_DMA_TX_DONE_FLAG)
#endif /* CONFIG_SPI_MAX32_DMA */

#ifdef CONFIG_SPI_MAX32_INTERRUPT
static void spi_max32_callback(mxc_spi_req_t *req, int error);
#endif /* CONFIG_SPI_MAX32_INTERRUPT */

static int spi_configure(const struct device *dev, const struct spi_config *config)
{
	int ret = 0;
	const struct max32_spi_config *cfg = dev->config;
	mxc_spi_regs_t *regs = cfg->regs;
	struct max32_spi_data *data = dev->data;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) & SPI_OP_MODE_SLAVE) {
		return -ENOTSUP;
	}

	int master_mode = 1;
	int quad_mode = 0;
	int num_slaves = 1;
	int ss_polarity = (config->operation & SPI_CS_ACTIVE_HIGH) ? 1 : 0;
	unsigned int spi_speed = (unsigned int)config->frequency;

	ret = Wrap_MXC_SPI_Init(regs, master_mode, quad_mode, num_slaves, ss_polarity, spi_speed);
	if (ret) {
		return ret;
	}

	int cpol = (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0;
	int cpha = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0;

	if (cpol && cpha) {
		ret = MXC_SPI_SetMode(regs, SPI_MODE_3);
	} else if (cpha) {
		ret = MXC_SPI_SetMode(regs, SPI_MODE_2);
	} else if (cpol) {
		ret = MXC_SPI_SetMode(regs, SPI_MODE_1);
	} else {
		ret = MXC_SPI_SetMode(regs, SPI_MODE_0);
	}
	if (ret) {
		return ret;
	}

	ret = MXC_SPI_SetDataSize(regs, SPI_WORD_SIZE_GET(config->operation));
	if (ret) {
		return ret;
	}

#if defined(CONFIG_SPI_EXTENDED_MODES)
	switch (config->operation & SPI_LINES_MASK) {
	case SPI_LINES_QUAD:
		ret = MXC_SPI_SetWidth(regs, SPI_WIDTH_QUAD);
		break;
	case SPI_LINES_DUAL:
		ret = MXC_SPI_SetWidth(regs, SPI_WIDTH_DUAL);
		break;
	case SPI_LINES_OCTAL:
		ret = -ENOTSUP;
		break;
	case SPI_LINES_SINGLE:
	default:
		ret = MXC_SPI_SetWidth(regs, SPI_WIDTH_STANDARD);
		break;
	}

	if (ret) {
		return ret;
	}
#endif

	data->ctx.config = config;

	return ret;
}

static inline int spi_max32_get_dfs_shift(const struct spi_context *ctx)
{
	if (SPI_WORD_SIZE_GET(ctx->config->operation) < 9) {
		return 0;
	}

	return 1;
}

static void spi_max32_setup(mxc_spi_regs_t *spi, mxc_spi_req_t *req)
{
	req->rxCnt = 0;
	req->txCnt = 0;

	if (spi->ctrl0 & ADI_MAX32_SPI_CTRL_MASTER_MODE) {
		MXC_SPI_SetSlave(spi, req->ssIdx);
	}

	if (req->rxData && req->rxLen) {
		MXC_SETFIELD(spi->ctrl1, MXC_F_SPI_CTRL1_RX_NUM_CHAR,
			     req->rxLen << MXC_F_SPI_CTRL1_RX_NUM_CHAR_POS);
		spi->dma |= MXC_F_SPI_DMA_RX_FIFO_EN;
	} else {
		spi->ctrl1 &= ~MXC_F_SPI_CTRL1_RX_NUM_CHAR;
		spi->dma &= ~MXC_F_SPI_DMA_RX_FIFO_EN;
	}

	if (req->txLen) {
		MXC_SETFIELD(spi->ctrl1, MXC_F_SPI_CTRL1_TX_NUM_CHAR,
			     req->txLen << MXC_F_SPI_CTRL1_TX_NUM_CHAR_POS);
		spi->dma |= MXC_F_SPI_DMA_TX_FIFO_EN;
	} else {
		spi->ctrl1 &= ~MXC_F_SPI_CTRL1_TX_NUM_CHAR;
		spi->dma &= ~MXC_F_SPI_DMA_TX_FIFO_EN;
	}

	spi->dma |= (ADI_MAX32_SPI_DMA_TX_FIFO_CLEAR | ADI_MAX32_SPI_DMA_RX_FIFO_CLEAR);
	spi->ctrl0 |= MXC_F_SPI_CTRL0_EN;
	MXC_SPI_ClearFlags(spi);
}

#ifndef CONFIG_SPI_MAX32_INTERRUPT
static int spi_max32_transceive_sync(mxc_spi_regs_t *spi, struct max32_spi_data *data,
				     uint8_t dfs_shift)
{
	int ret = 0;
	mxc_spi_req_t *req = &data->req;
	uint32_t remain, flags, tx_len, rx_len;

	MXC_SPI_ClearTXFIFO(spi);
	MXC_SPI_ClearRXFIFO(spi);

	tx_len = req->txLen << dfs_shift;
	rx_len = req->rxLen << dfs_shift;
	do {
		remain = tx_len - req->txCnt;
		if (remain > 0) {
			if (!data->req.txData) {
				req->txCnt += MXC_SPI_WriteTXFIFO(spi, data->dummy,
								  MIN(remain, sizeof(data->dummy)));
			} else {
				req->txCnt +=
					MXC_SPI_WriteTXFIFO(spi, &req->txData[req->txCnt], remain);
			}
			if (!(spi->ctrl0 & MXC_F_SPI_CTRL0_START)) {
				spi->ctrl0 |= MXC_F_SPI_CTRL0_START;
			}
		}

		if (req->rxCnt < rx_len) {
			req->rxCnt += MXC_SPI_ReadRXFIFO(spi, &req->rxData[req->rxCnt],
							 rx_len - req->rxCnt);
		}
	} while ((req->txCnt < tx_len) || (req->rxCnt < rx_len));

	do {
		flags = MXC_SPI_GetFlags(spi);
	} while (!(flags & ADI_MAX32_SPI_INT_FL_MST_DONE));
	MXC_SPI_ClearFlags(spi);

	return ret;
}
#endif /* CONFIG_SPI_MAX32_INTERRUPT */

static int spi_max32_transceive(const struct device *dev)
{
	int ret = 0;
	const struct max32_spi_config *cfg = dev->config;
	struct max32_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct rtio_sqe *sqe = &rtio_ctx->txn_curr->sqe;
#endif
	uint32_t len;
	uint8_t dfs_shift;

	MXC_SPI_ClearTXFIFO(cfg->regs);

	dfs_shift = spi_max32_get_dfs_shift(ctx);

	len = spi_context_max_continuous_chunk(ctx);

#ifdef CONFIG_SPI_RTIO
	switch (sqe->op) {
	case RTIO_OP_RX:
		len = sqe->rx.buf_len;
		data->req.rxData = sqe->rx.buf;
		data->req.rxLen = sqe->rx.buf_len;
		data->req.txData = NULL;
		data->req.txLen = len >> dfs_shift;
		break;
	case RTIO_OP_TX:
		len = sqe->tx.buf_len;
		data->req.rxLen = 0;
		data->req.rxData = data->dummy;
		data->req.txData = (uint8_t *)sqe->tx.buf;
		data->req.txLen = len >> dfs_shift;
		break;
	case RTIO_OP_TINY_TX:
		len = sqe->tiny_tx.buf_len;
		data->req.txData = (uint8_t *)sqe->tiny_tx.buf;
		data->req.rxData = data->dummy;
		data->req.txLen = len >> dfs_shift;
		data->req.rxLen = 0;
		break;
	case RTIO_OP_TXRX:
		len = sqe->txrx.buf_len;
		data->req.txData = (uint8_t *)sqe->txrx.tx_buf;
		data->req.rxData = sqe->txrx.rx_buf;
		data->req.txLen = len >> dfs_shift;
		data->req.rxLen = len >> dfs_shift;
		break;
	default:
		break;
	}
#else
	data->req.txLen = len >> dfs_shift;
	data->req.txData = (uint8_t *)ctx->tx_buf;
	data->req.rxLen = len >> dfs_shift;
	data->req.rxData = ctx->rx_buf;

	data->req.rxData = ctx->rx_buf;

	data->req.rxLen = len >> dfs_shift;
	if (!data->req.rxData) {
		/* Pass a dummy buffer to HAL if receive buffer is NULL, otherwise
		 * corrupt data is read during subsequent transactions.
		 */
		data->req.rxData = data->dummy;
		data->req.rxLen = 0;
	}
#endif
	data->req.spi = cfg->regs;
	data->req.ssIdx = ctx->config->slave;
	data->req.ssDeassert = 0;
	data->req.txCnt = 0;
	data->req.rxCnt = 0;
	spi_max32_setup(cfg->regs, &data->req);
#ifdef CONFIG_SPI_MAX32_INTERRUPT
	MXC_SPI_SetTXThreshold(cfg->regs, 1);
	if (data->req.rxLen) {
		MXC_SPI_SetRXThreshold(cfg->regs, 2);
		MXC_SPI_EnableInt(cfg->regs, ADI_MAX32_SPI_INT_EN_RX_THD);
	}
	MXC_SPI_EnableInt(cfg->regs, ADI_MAX32_SPI_INT_EN_TX_THD | ADI_MAX32_SPI_INT_EN_MST_DONE);

	if (!data->req.txData) {
		data->req.txCnt =
			MXC_SPI_WriteTXFIFO(cfg->regs, data->dummy, MIN(len, sizeof(data->dummy)));
	} else {
		data->req.txCnt = MXC_SPI_WriteTXFIFO(cfg->regs, data->req.txData, len);
	}

	MXC_SPI_StartTransmission(cfg->regs);
#else
	ret = spi_max32_transceive_sync(cfg->regs, data, dfs_shift);
	if (ret) {
		ret = -EIO;
	} else {
		spi_context_update_tx(ctx, 1, len);
		spi_context_update_rx(ctx, 1, len);
	}
#endif

	return ret;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool async, spi_callback_t cb, void *userdata)
{
	int ret = 0;
	struct max32_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
#ifndef CONFIG_SPI_RTIO
	const struct max32_spi_config *cfg = dev->config;
	bool hw_cs_ctrl = true;
#endif

#ifndef CONFIG_SPI_MAX32_INTERRUPT
	if (async) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(ctx, async, cb, userdata, config);

#ifndef CONFIG_SPI_RTIO
	ret = spi_configure(dev, config);
	if (ret != 0) {
		spi_context_release(ctx, ret);
		return -EIO;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Check if CS GPIO exists */
	if (spi_cs_is_gpio(config)) {
		hw_cs_ctrl = false;
	}
	MXC_SPI_HWSSControl(cfg->regs, hw_cs_ctrl);

	/* Assert the CS line if HW control disabled */
	if (!hw_cs_ctrl) {
		spi_context_cs_control(ctx, true);
	} else {
		cfg->regs->ctrl0 =
			(cfg->regs->ctrl0 & ~MXC_F_SPI_CTRL0_START) | MXC_F_SPI_CTRL0_SS_CTRL;
	}

#ifdef CONFIG_SPI_MAX32_INTERRUPT
	do {
		ret = spi_max32_transceive(dev);
		if (!ret) {
			ret = spi_context_wait_for_completion(ctx);
			if (ret || async) {
				break;
			}
		} else {
			break;
		}
	} while ((spi_context_tx_on(ctx) || spi_context_rx_on(ctx)));
#else
	do {
		ret = spi_max32_transceive(dev);
		if (ret) {
			break;
		}
	} while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx));

#endif /* CONFIG_SPI_MAX32_INTERRUPT */

	/* Deassert the CS line if hw control disabled */
	if (!async) {
		if (!hw_cs_ctrl) {
			spi_context_cs_control(ctx, false);
		} else {
			cfg->regs->ctrl0 &= ~(MXC_F_SPI_CTRL0_START | MXC_F_SPI_CTRL0_SS_CTRL |
					      MXC_F_SPI_CTRL0_EN);
			cfg->regs->ctrl0 |= MXC_F_SPI_CTRL0_EN;
		}
	}
#else
		struct spi_rtio *rtio_ctx = data->rtio_ctx;

		ret = spi_rtio_transceive(rtio_ctx, config, tx_bufs, rx_bufs);
#endif
	spi_context_release(ctx, ret);
	return ret;
}

#ifdef CONFIG_SPI_MAX32_DMA
static void spi_max32_dma_callback(const struct device *dev, void *arg, uint32_t channel,
				   int status)
{
	struct max32_spi_data *data = arg;
	const struct device *spi_dev = data->dev;
	const struct max32_spi_config *config = spi_dev->config;
	uint32_t len;

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
	} else {
		/* identify the origin of this callback */
		if (channel == config->tx_dma.channel) {
			data->dma_stat |= SPI_MAX32_DMA_TX_DONE_FLAG;
		} else if (channel == config->rx_dma.channel) {
			data->dma_stat |= SPI_MAX32_DMA_RX_DONE_FLAG;
		}
	}
	if ((data->dma_stat & SPI_MAX32_DMA_DONE_FLAG) == SPI_MAX32_DMA_DONE_FLAG) {
		len = spi_context_max_continuous_chunk(&data->ctx);
		spi_context_update_tx(&data->ctx, 1, len);
		spi_context_update_rx(&data->ctx, 1, len);
		spi_context_complete(&data->ctx, spi_dev, status == 0 ? 0 : -EIO);
	}
}

static int spi_max32_tx_dma_load(const struct device *dev, const uint8_t *buf, uint32_t len,
				 uint8_t word_shift)
{
	int ret;
	const struct max32_spi_config *config = dev->config;
	struct max32_spi_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_callback = spi_max32_dma_callback;
	dma_cfg.user_data = (void *)data;
	dma_cfg.dma_slot = config->tx_dma.slot;
	dma_cfg.block_count = 1;
	dma_cfg.source_data_size = 1U << word_shift;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_data_size = 1U << word_shift;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;
	if (buf) {
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_blk.source_address = (uint32_t)buf;
	} else {
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_blk.source_address = (uint32_t)data->dummy;
	}

	ret = dma_config(config->tx_dma.dev, config->tx_dma.channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Error configuring Tx DMA (%d)", ret);
	}

	return dma_start(config->tx_dma.dev, config->tx_dma.channel);
}

static int spi_max32_rx_dma_load(const struct device *dev, const uint8_t *buf, uint32_t len,
				 uint8_t word_shift)
{
	int ret;
	const struct max32_spi_config *config = dev->config;
	struct max32_spi_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback = spi_max32_dma_callback;
	dma_cfg.user_data = (void *)data;
	dma_cfg.dma_slot = config->rx_dma.slot;
	dma_cfg.block_count = 1;
	dma_cfg.source_data_size = 1U << word_shift;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_data_size = 1U << word_shift;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;
	if (buf) {
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_blk.dest_address = (uint32_t)buf;
	} else {
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_blk.dest_address = (uint32_t)data->dummy;
	}
	ret = dma_config(config->rx_dma.dev, config->rx_dma.channel, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Error configuring Rx DMA (%d)", ret);
	}

	return dma_start(config->rx_dma.dev, config->rx_dma.channel);
}

static int transceive_dma(const struct device *dev, const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			  bool async, spi_callback_t cb, void *userdata)
{
	int ret = 0;
	const struct max32_spi_config *cfg = dev->config;
	struct max32_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	mxc_spi_regs_t *spi = cfg->regs;
	struct dma_status status;
	uint32_t len, word_count;
	uint8_t dfs_shift;

	bool hw_cs_ctrl = true;

	spi_context_lock(ctx, async, cb, userdata, config);

	ret = dma_get_status(cfg->tx_dma.dev, cfg->tx_dma.channel, &status);
	if (ret < 0 || status.busy) {
		ret = ret < 0 ? ret : -EBUSY;
		goto unlock;
	}

	ret = dma_get_status(cfg->rx_dma.dev, cfg->rx_dma.channel, &status);
	if (ret < 0 || status.busy) {
		ret = ret < 0 ? ret : -EBUSY;
		goto unlock;
	}

	ret = spi_configure(dev, config);
	if (ret != 0) {
		ret = -EIO;
		goto unlock;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Check if CS GPIO exists */
	if (spi_cs_is_gpio(config)) {
		hw_cs_ctrl = false;
	}
	MXC_SPI_HWSSControl(cfg->regs, hw_cs_ctrl);

	/* Assert the CS line if HW control disabled */
	if (!hw_cs_ctrl) {
		spi_context_cs_control(ctx, true);
	}

	MXC_SPI_SetSlave(cfg->regs, ctx->config->slave);

	do {
		spi->ctrl0 &= ~(MXC_F_SPI_CTRL0_EN);

		len = spi_context_max_continuous_chunk(ctx);
		dfs_shift = spi_max32_get_dfs_shift(ctx);
		word_count = len >> dfs_shift;

		MXC_SETFIELD(spi->ctrl1, MXC_F_SPI_CTRL1_RX_NUM_CHAR,
			     word_count << MXC_F_SPI_CTRL1_RX_NUM_CHAR_POS);
		spi->dma |= ADI_MAX32_SPI_DMA_RX_FIFO_CLEAR;
		spi->dma |= MXC_F_SPI_DMA_RX_FIFO_EN;
		spi->dma |= ADI_MAX32_SPI_DMA_RX_DMA_EN;
		MXC_SPI_SetRXThreshold(spi, 0);

		ret = spi_max32_rx_dma_load(dev, ctx->rx_buf, len, dfs_shift);
		if (ret < 0) {
			goto unlock;
		}

		MXC_SETFIELD(spi->ctrl1, MXC_F_SPI_CTRL1_TX_NUM_CHAR,
			     word_count << MXC_F_SPI_CTRL1_TX_NUM_CHAR_POS);
		spi->dma |= ADI_MAX32_SPI_DMA_TX_FIFO_CLEAR;
		spi->dma |= MXC_F_SPI_DMA_TX_FIFO_EN;
		spi->dma |= ADI_MAX32_SPI_DMA_TX_DMA_EN;
		MXC_SPI_SetTXThreshold(spi, 1);

		ret = spi_max32_tx_dma_load(dev, ctx->tx_buf, len, dfs_shift);
		if (ret < 0) {
			goto unlock;
		}

		spi->ctrl0 |= MXC_F_SPI_CTRL0_EN;

		data->dma_stat = 0;
		MXC_SPI_StartTransmission(spi);
		ret = spi_context_wait_for_completion(ctx);
	} while (!ret && (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)));

unlock:
	/* Deassert the CS line if hw control disabled */
	if (!hw_cs_ctrl) {
		spi_context_cs_control(ctx, false);
	}

	spi_context_release(ctx, ret);

	return ret;
}
#endif /* CONFIG_SPI_MAX32_DMA */

#ifdef CONFIG_SPI_RTIO
static void spi_max32_iodev_complete(const struct device *dev, int status);

static void spi_max32_iodev_start(const struct device *dev)
{
	struct max32_spi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct rtio_sqe *sqe = &rtio_ctx->txn_curr->sqe;
	int ret = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
	case RTIO_OP_TX:
	case RTIO_OP_TINY_TX:
	case RTIO_OP_TXRX:
		ret = spi_max32_transceive(dev);
		break;
	default:
		spi_max32_iodev_complete(dev, -EINVAL);
		break;
	}
	if (ret != 0) {
		spi_max32_iodev_complete(dev, -EIO);
	}
}

static inline void spi_max32_iodev_prepare_start(const struct device *dev)
{
	struct max32_spi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct spi_dt_spec *spi_dt_spec = rtio_ctx->txn_curr->sqe.iodev->data;
	struct spi_config *spi_config = &spi_dt_spec->config;
	struct max32_spi_config *cfg = (struct max32_spi_config *)dev->config;
	int ret;
	bool hw_cs_ctrl = true;

	ret = spi_configure(dev, spi_config);
	__ASSERT(!ret, "%d", ret);

	/* Check if CS GPIO exists */
	if (spi_cs_is_gpio(spi_config)) {
		hw_cs_ctrl = false;
	}
	MXC_SPI_HWSSControl(cfg->regs, hw_cs_ctrl);

	/* Assert the CS line if HW control disabled */
	if (!hw_cs_ctrl) {
		spi_context_cs_control(&data->ctx, true);
	} else {
		cfg->regs->ctrl0 = (cfg->regs->ctrl0 & ~MXC_F_SPI_CTRL0_START) |
					MXC_F_SPI_CTRL0_SS_CTRL;
	};
}

static void spi_max32_iodev_complete(const struct device *dev, int status)
{
	struct max32_spi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (!status && rtio_ctx->txn_curr->sqe.flags & RTIO_SQE_TRANSACTION) {
		rtio_ctx->txn_curr = rtio_txn_next(rtio_ctx->txn_curr);
		spi_max32_iodev_start(dev);
	} else {
		struct max32_spi_config *cfg = (struct max32_spi_config *)dev->config;
		bool hw_cs_ctrl = true;

		if (!hw_cs_ctrl) {
			spi_context_cs_control(&data->ctx, false);
		} else {
			cfg->regs->ctrl0 &= ~(MXC_F_SPI_CTRL0_START | MXC_F_SPI_CTRL0_SS_CTRL |
					      MXC_F_SPI_CTRL0_EN);
			cfg->regs->ctrl0 |= MXC_F_SPI_CTRL0_EN;
		}

		if (spi_rtio_complete(rtio_ctx, status)) {
			spi_max32_iodev_prepare_start(dev);
			spi_max32_iodev_start(dev);
		}
	}
}

static void api_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct max32_spi_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (spi_rtio_submit(rtio_ctx, iodev_sqe)) {
		spi_max32_iodev_prepare_start(dev);
		spi_max32_iodev_start(dev);
	}
}
#endif

static int api_transceive(const struct device *dev, const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
#ifdef CONFIG_SPI_MAX32_DMA
	const struct max32_spi_config *cfg = dev->config;

	if (cfg->tx_dma.channel != 0xFF && cfg->rx_dma.channel != 0xFF) {
		return transceive_dma(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
	}
#endif /* CONFIG_SPI_MAX32_DMA */
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int api_transceive_async(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

#ifdef CONFIG_SPI_MAX32_INTERRUPT
static void spi_max32_callback(mxc_spi_req_t *req, int error)
{
	struct max32_spi_data *data = CONTAINER_OF(req, struct max32_spi_data, req);
	struct spi_context *ctx = &data->ctx;
	const struct device *dev = data->dev;
	uint32_t len;

#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (rtio_ctx->txn_head != NULL) {
		spi_max32_iodev_complete(data->dev, 0);
	}
#endif
	len = spi_context_max_continuous_chunk(ctx);
	spi_context_update_tx(ctx, 1, len);
	spi_context_update_rx(ctx, 1, len);
#ifdef CONFIG_SPI_ASYNC
	if (ctx->asynchronous && ((spi_context_tx_on(ctx) || spi_context_rx_on(ctx)))) {
		k_work_submit(&data->async_work);
	} else {
		if (spi_cs_is_gpio(ctx->config)) {
			spi_context_cs_control(ctx, false);
		} else {
			req->spi->ctrl0 &= ~(MXC_F_SPI_CTRL0_START | MXC_F_SPI_CTRL0_SS_CTRL |
					     MXC_F_SPI_CTRL0_EN);
			req->spi->ctrl0 |= MXC_F_SPI_CTRL0_EN;
		}
		spi_context_complete(ctx, dev, error == E_NO_ERROR ? 0 : -EIO);
	}
#else
	spi_context_complete(ctx, dev, error == E_NO_ERROR ? 0 : -EIO);
#endif
}

#ifdef CONFIG_SPI_ASYNC
void spi_max32_async_work_handler(struct k_work *work)
{
	struct max32_spi_data *data = CONTAINER_OF(work, struct max32_spi_data, async_work);
	const struct device *dev = data->dev;
	int ret;

	ret = spi_max32_transceive(dev);
	if (ret) {
		spi_context_complete(&data->ctx, dev, -EIO);
	}
}
#endif /* CONFIG_SPI_ASYNC */

static void spi_max32_isr(const struct device *dev)
{
	const struct max32_spi_config *cfg = dev->config;
	struct max32_spi_data *data = dev->data;
	mxc_spi_req_t *req = &data->req;
	mxc_spi_regs_t *spi = cfg->regs;
	uint32_t flags, remain;
	uint8_t dfs_shift = spi_max32_get_dfs_shift(&data->ctx);

	flags = MXC_SPI_GetFlags(spi);
	MXC_SPI_ClearFlags(spi);

	remain = (req->txLen << dfs_shift) - req->txCnt;
	if (flags & ADI_MAX32_SPI_INT_FL_TX_THD) {
		if (remain) {
			if (!data->req.txData) {
				req->txCnt += MXC_SPI_WriteTXFIFO(cfg->regs, data->dummy,
								  MIN(remain, sizeof(data->dummy)));
			} else {
				req->txCnt +=
					MXC_SPI_WriteTXFIFO(spi, &req->txData[req->txCnt], remain);
			}
		} else {
			MXC_SPI_DisableInt(spi, ADI_MAX32_SPI_INT_EN_TX_THD);
		}
	}

	remain = (req->rxLen << dfs_shift) - req->rxCnt;
	if (remain) {
		req->rxCnt += MXC_SPI_ReadRXFIFO(spi, &req->rxData[req->rxCnt], remain);
		remain = (req->rxLen << dfs_shift) - req->rxCnt;
		if (remain >= MXC_SPI_FIFO_DEPTH) {
			MXC_SPI_SetRXThreshold(spi, 2);
		} else {
			MXC_SPI_SetRXThreshold(spi, remain);
		}
	} else {
		MXC_SPI_DisableInt(spi, ADI_MAX32_SPI_INT_EN_RX_THD);
	}

	if ((req->txLen == req->txCnt) && (req->rxLen == req->rxCnt)) {
		MXC_SPI_DisableInt(spi, ADI_MAX32_SPI_INT_EN_TX_THD | ADI_MAX32_SPI_INT_EN_RX_THD);
		if (flags & ADI_MAX32_SPI_INT_FL_MST_DONE) {
			MXC_SPI_DisableInt(spi, ADI_MAX32_SPI_INT_EN_MST_DONE);
			spi_max32_callback(req, 0);
		}
	}
}
#endif /* CONFIG_SPI_MAX32_INTERRUPT */

static int api_release(const struct device *dev, const struct spi_config *config)
{
	struct max32_spi_data *data = dev->data;

#ifndef CONFIG_SPI_RTIO
	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}
#endif
	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static int spi_max32_init(const struct device *dev)
{
	int ret = 0;
	const struct max32_spi_config *const cfg = dev->config;
	mxc_spi_regs_t *regs = cfg->regs;
	struct max32_spi_data *data = dev->data;

	if (!device_is_ready(cfg->clock)) {
		return -ENODEV;
	}

	MXC_SPI_Shutdown(regs);

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

#ifdef CONFIG_SPI_RTIO
	spi_rtio_init(data->rtio_ctx, dev);
#endif

#ifdef CONFIG_SPI_MAX32_INTERRUPT
	cfg->irq_config_func(dev);
#ifdef CONFIG_SPI_ASYNC
	k_work_init(&data->async_work, spi_max32_async_work_handler);
#endif
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

/* SPI driver APIs structure */
static DEVICE_API(spi, spi_max32_api) = {
	.transceive = api_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = api_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = api_iodev_submit,
#endif /* CONFIG_SPI_RTIO */
	.release = api_release,
};

/* SPI driver registration */
#ifdef CONFIG_SPI_MAX32_INTERRUPT
#define SPI_MAX32_CONFIG_IRQ_FUNC(n) .irq_config_func = spi_max32_irq_config_func_##n,

#define SPI_MAX32_IRQ_CONFIG_FUNC(n)                                                               \
	static void spi_max32_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_max32_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#else
#define SPI_MAX32_CONFIG_IRQ_FUNC(n)
#define SPI_MAX32_IRQ_CONFIG_FUNC(n)
#endif /* CONFIG_SPI_MAX32_INTERRUPT */

#if CONFIG_SPI_MAX32_DMA
#define MAX32_DT_INST_DMA_CTLR(n, name)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                                                \
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))), (NULL))

#define MAX32_DT_INST_DMA_CELL(n, name, cell)                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas), (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),    \
		    (0xff))

#define MAX32_SPI_DMA_INIT(n)                                                                      \
	.tx_dma.dev = MAX32_DT_INST_DMA_CTLR(n, tx),                                               \
	.tx_dma.channel = MAX32_DT_INST_DMA_CELL(n, tx, channel),                                  \
	.tx_dma.slot = MAX32_DT_INST_DMA_CELL(n, tx, slot),                                        \
	.rx_dma.dev = MAX32_DT_INST_DMA_CTLR(n, rx),                                               \
	.rx_dma.channel = MAX32_DT_INST_DMA_CELL(n, rx, channel),                                  \
	.rx_dma.slot = MAX32_DT_INST_DMA_CELL(n, rx, slot),
#else
#define MAX32_SPI_DMA_INIT(n)
#endif

#define DEFINE_SPI_MAX32_RTIO(_num) SPI_RTIO_DEFINE(max32_spi_rtio_##_num,                 \
			CONFIG_SPI_MAX32_RTIO_SQ_SIZE,                              \
			CONFIG_SPI_MAX32_RTIO_CQ_SIZE)

#define DEFINE_SPI_MAX32(_num)                                                                     \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	SPI_MAX32_IRQ_CONFIG_FUNC(_num)                                                            \
	COND_CODE_1(CONFIG_SPI_RTIO, (DEFINE_SPI_MAX32_RTIO(_num)), ());                           \
	static const struct max32_spi_config max32_spi_config_##_num = {                           \
		.regs = (mxc_spi_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		MAX32_SPI_DMA_INIT(_num) SPI_MAX32_CONFIG_IRQ_FUNC(_num)};                         \
	static struct max32_spi_data max32_spi_data_##_num = {                                     \
		SPI_CONTEXT_INIT_LOCK(max32_spi_data_##_num, ctx),                                 \
		SPI_CONTEXT_INIT_SYNC(max32_spi_data_##_num, ctx),                                 \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(_num), ctx)                            \
		IF_ENABLED(CONFIG_SPI_RTIO, (.rtio_ctx = &max32_spi_rtio_##_num))};                \
	SPI_DEVICE_DT_INST_DEFINE(_num, spi_max32_init, NULL, &max32_spi_data_##_num,              \
			      &max32_spi_config_##_num, PRE_KERNEL_2, CONFIG_SPI_INIT_PRIORITY,    \
			      &spi_max32_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SPI_MAX32)
