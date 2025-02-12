/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_spi

#include <string.h>
#include <errno.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <wrap_max32_spi.h>

LOG_MODULE_REGISTER(spi_max32, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

struct max32_spi_config {
	mxc_spi_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
#ifdef CONFIG_SPI_MAX32_INTERRUPT
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_SPI_MAX32_INTERRUPT */
};

/* Device run time data */
struct max32_spi_data {
	struct spi_context ctx;
	const struct device *dev;
	mxc_spi_req_t req;
	uint8_t dummy[2];
#ifdef CONFIG_SPI_ASYNC
	struct k_work async_work;
#endif /* CONFIG_SPI_ASYNC */
};

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
	uint32_t len;
	uint8_t dfs_shift;

	MXC_SPI_ClearTXFIFO(cfg->regs);

	dfs_shift = spi_max32_get_dfs_shift(ctx);

	len = spi_context_max_continuous_chunk(ctx);
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
	const struct max32_spi_config *cfg = dev->config;
	struct max32_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool hw_cs_ctrl = true;

#ifndef CONFIG_SPI_MAX32_INTERRUPT
	if (async) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(ctx, async, cb, userdata, config);

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

	spi_context_release(ctx, ret);

	return ret;
}

static int api_transceive(const struct device *dev, const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
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

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

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
static const struct spi_driver_api spi_max32_api = {
	.transceive = api_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = api_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
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

#define DEFINE_SPI_MAX32(_num)                                                                     \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	SPI_MAX32_IRQ_CONFIG_FUNC(_num)                                                            \
	static const struct max32_spi_config max32_spi_config_##_num = {                           \
		.regs = (mxc_spi_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		SPI_MAX32_CONFIG_IRQ_FUNC(_num)};                                                  \
	static struct max32_spi_data max32_spi_data_##_num = {                                     \
		SPI_CONTEXT_INIT_LOCK(max32_spi_data_##_num, ctx),                                 \
		SPI_CONTEXT_INIT_SYNC(max32_spi_data_##_num, ctx),                                 \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(_num), ctx)};                          \
	DEVICE_DT_INST_DEFINE(_num, spi_max32_init, NULL, &max32_spi_data_##_num,                  \
			      &max32_spi_config_##_num, PRE_KERNEL_2, CONFIG_SPI_INIT_PRIORITY,    \
			      &spi_max32_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SPI_MAX32)
