/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Realtek Ameba SPI interface
 */

#define DT_DRV_COMPAT realtek_ameba_spi

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ameba_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#if CONFIG_SPI_AMEBA_DMA
#include <zephyr/drivers/dma/dma_ameba_gdma.h>
#include <zephyr/drivers/dma.h>

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};
#endif

struct spi_ameba_data {
	struct spi_context ctx;
	bool initialized;
	uint32_t datasize; /* real dfs */
	uint8_t fifo_diff; /* cannot be bigger than FIFO depth */

#ifdef CONFIG_SPI_AMEBA_DMA
	struct k_sem status_sem;
	volatile uint32_t status_flags;
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
	uint32_t transfer_dir;
#endif
};

struct spi_ameba_config {
	SPI_TypeDef *SPIx;

	/* rcc info */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	/* pinctrl info */
	const struct pinctrl_dev_config *pcfg;

#ifdef CONFIG_SPI_AMEBA_INTERRUPT
	void (*irq_configure)(void);
#endif
};

static int spi_ameba_configure(const struct device *dev, const struct spi_config *spi_cfg);
static inline bool spi_ameba_is_slave(struct spi_ameba_data *data)
{
	return (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(&data->ctx));
}

static bool spi_ameba_transfer_ongoing(struct spi_ameba_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

#if !defined(CONFIG_SPI_AMEBA_INTERRUPT)
static int spi_ameba_get_err(const struct spi_ameba_config *cfg)
{
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;

	if (SSI_GetStatus(spi) & SPI_BIT_DCOL) {
		LOG_ERR("spi%p Data Collision Error status detected", spi);
		return -EIO;
	}

	return 0;
}

static int spi_ameba_frame_exchange(const struct device *dev)
{
	struct spi_ameba_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_ameba_config *cfg = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;

	uint32_t datalen = data->datasize;
	int dfs = ((datalen - 1) >> 3) + 1; /* frame bytes */
	uint16_t tx_frame = 0U, rx_frame = 0U;

	while (!SSI_Writeable(spi)) {
		/* NOP */
	}

	if (spi_context_tx_buf_on(ctx)) {
		if (datalen <= 8) {
			tx_frame = ctx->tx_buf ? *(uint8_t *)(data->ctx.tx_buf) : 0;
		} else if (datalen <= 16) {
			tx_frame = ctx->tx_buf ? *(uint16_t *)(data->ctx.tx_buf) : 0;
		} else { /* if (datalen <= 32)  */
			/* tx_frame = ctx->tx_buf ? *(uint32_t *)(data->ctx.tx_buf) : 0; */
			LOG_ERR("Data Frame Size is supported from 4 ~ 16");
			return -EINVAL;
		}
	}

	SSI_WriteData(spi, tx_frame);
	spi_context_update_tx(ctx, dfs, 1);

	while (!SSI_Readable(spi)) {
		/* NOP */
	}

	rx_frame = SSI_ReadData(spi);

	if (spi_context_rx_buf_on(ctx)) {
		if (datalen <= 8) {
			*(uint8_t *)data->ctx.rx_buf = rx_frame;
		} else if (datalen <= 16) {
			*(uint16_t *)data->ctx.rx_buf = rx_frame;
		} else { /* if (datalen <= 32) */
			/* (uint32_t *)data->ctx.rx_buf = rx_frame; */
			LOG_ERR("Data Frame Size is supported from 4 ~ 16");
			return -EINVAL;
		}
	}
	spi_context_update_rx(ctx, dfs, 1);

	return spi_ameba_get_err(cfg);
}
#endif

#ifdef CONFIG_SPI_AMEBA_INTERRUPT
static void spi_ameba_complete(const struct device *dev, int status)
{
	struct spi_ameba_data *data = dev->data;
	const struct spi_ameba_config *cfg = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;

	uint32_t int_mask = SPI_GetINTConfig(spi);

	SSI_INTConfig(spi, int_mask, DISABLE);

	spi_context_complete(&data->ctx, dev, status);
}

static void spi_ameba_receive_data(const struct device *dev)
{
	struct spi_ameba_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_ameba_config *cfg = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;

	uint32_t rxlevel;
	uint32_t datalen = data->datasize;  /* data bit: 4~16 bits */
	int dfs = ((datalen - 1) >> 3) + 1; /* frame number: 1, 2 bytes */

	volatile uint32_t readable = SSI_Readable(spi);

	while (readable) {
		rxlevel = SSI_GetRxCount(spi);

		while (rxlevel--) {
			if (spi_context_rx_buf_on(ctx)) {
				if (data->ctx.rx_buf != NULL) {
					if (datalen <= 8) {
						/* 8~4 bits mode */
						*(uint8_t *)data->ctx.rx_buf =
							(uint8_t)SSI_ReadData(spi);
					} else {
						/* 16~9 bits mode */
						*(uint16_t *)data->ctx.rx_buf =
							(uint16_t)SSI_ReadData(spi);
					}
				} else {
					/* for Master mode, doing TX also will got RX data,
					 * so drop the dummy data
					 */
					(void)SSI_ReadData(spi);
				}
				/* spi_context_update_rx(ctx, dfs, 1); */
			} else if (spi_context_rx_on(ctx)) {
				/* fix for case: rx half end: buf1 is null
				 * but len1 !=0, skip len1 and rx into buf2
				 */
				(void)SSI_ReadData(spi);
			}

			spi_context_update_rx(ctx, dfs, 1);
			data->fifo_diff--;

			if (!spi_context_rx_on(ctx)) {
				break;
			}
		}

		if (!spi_context_rx_on(ctx)) {
			break;
		}

		readable = SSI_Readable(spi);
	}
}

static void spi_ameba_send_data(const struct device *dev)
{
	const struct spi_ameba_config *cfg = dev->config;
	struct spi_ameba_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t txdata = 0U;
	uint32_t txmax;
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;

	uint32_t datalen = data->datasize;  /* data len: 4 ~ 16 bits */
	int dfs = ((datalen - 1) >> 3) + 1; /* frame size: 1, 2 bytes */

	uint32_t writeable = SSI_Writeable(spi);

	if (spi_context_rx_on(ctx)) { /* fix for rx bigger than tx */
		txmax = SSI_TX_FIFO_DEPTH - SSI_GetTxCount(spi) - SSI_GetRxCount(spi);
		if ((int)txmax < 0) {
			txmax = 0U; /* if rx-fifo is full, hold off tx */
		}
	} else {
		txmax = SSI_TX_FIFO_DEPTH - SSI_GetTxCount(spi);
	}

	if (writeable) {
		/* Disable Tx FIFO Empty IRQ */
		SSI_INTConfig(spi, SPI_BIT_TXEIM, DISABLE);

		while (txmax) {
			if (spi_context_tx_buf_on(ctx)) {
				if (datalen <= 8) {
					/* 4~8 bits mode */
					if (data->ctx.tx_buf != NULL) {
						txdata = *((uint8_t *)(data->ctx.tx_buf));
					} else if (!spi_ameba_is_slave(data)) {
						/* For master mode: Push a dummy to TX FIFO for Read
						 */
						txdata = (uint8_t)0; /* Dummy byte */
					}
				} else {
					/* 9~16 bits mode */
					if (data->ctx.tx_buf != NULL) {
						txdata = *((uint16_t *)(data->ctx.tx_buf));
					} else if (!spi_ameba_is_slave(data)) {
						/* For master mode: Push a dummy to TX FIFO for Read
						 */
						txdata = (uint16_t)0; /* Dummy byte */
					}
				}
				/* spi_context_update_tx(ctx, dfs, 1); */
			} else if (spi_context_rx_on(ctx)) { /* rx bigger than tx */
				/* No need to push more than necessary */
				if ((int)(data->ctx.rx_len - data->fifo_diff) <= 0) {
					break;
				}
				txdata = 0U;

			} else if (spi_context_tx_on(&data->ctx)) {
				/* fix for txbuf is NULL but txlen != 0 */
				txdata = 0U;
			} else {
				/* Nothing to push anymore */
				break;
			}

			SSI_WriteData(spi, txdata);
			spi_context_update_tx(ctx, dfs, 1);
			data->fifo_diff++;

			txmax--;
		}

		/* Enable Tx FIFO Empty IRQ */
		SSI_INTConfig(spi, SPI_BIT_TXEIM, ENABLE);
	}
}

static void spi_ameba_isr(struct device *dev)
{
	const struct spi_ameba_config *cfg = dev->config;
	struct spi_ameba_data *data = dev->data;

	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;
	int err = 0;
	uint32_t int_mask, int_status;

	int_mask = SPI_GetINTConfig(spi);
	LOG_DBG("[ISR] IntMask 0X%x", int_mask);

	int_status = SSI_GetIsr(spi);
	SSI_SetIsrClean(spi, int_status);

	if (int_status & (SPI_BIT_TXOIS | SPI_BIT_RXUIS | SPI_BIT_RXOIS | SPI_BIT_TXUIS)) {
		LOG_DBG("[ISR] IntStatus %x", int_status);
	}

	if (int_status & SPI_BIT_RXFIS) {
		LOG_DBG("[ISR] RXFIS");
		spi_ameba_receive_data(dev);
	}

	if (int_status & SPI_BIT_TXEIS) {
		LOG_DBG("[ISR] TXEIS");
		spi_ameba_send_data(dev);
	}

	if (!spi_ameba_transfer_ongoing(data)) {
		spi_ameba_complete(dev, err);
	}
}
#endif /* CONFIG_SPI_AMEBA_INTERRUPT */

#ifdef CONFIG_SPI_AMEBA_DMA
#define SPI_AMEBA_FULL_DUPLEX_FLAG    0x0
#define SPI_AMEBA_HALF_DUPLEX_TX_FLAG 0x1
#define SPI_AMEBA_HALF_DUPLEX_RX_FLAG 0x2

#define SPI_AMEBA_DMA_ERROR_FLAG   0x01
#define SPI_AMEBA_DMA_RX_DONE_FLAG 0x02
#define SPI_AMEBA_DMA_TX_DONE_FLAG 0x04
#define SPI_AMEBA_DMA_DONE_FLAG    (SPI_AMEBA_DMA_RX_DONE_FLAG | SPI_AMEBA_DMA_TX_DONE_FLAG)

static uint32_t spi_dma_tx_dummy __aligned(64);
static uint32_t spi_dma_rx_dummy __aligned(64);

static bool spi_ameba_dma_enabled(const struct device *dev)
{
	const struct spi_ameba_data *data = dev->data;

	if ((data->dma_tx.dma_dev) && (data->dma_rx.dma_dev)) {
		return true;
	}

	return false;
}

/* This function is executed in the interrupt context */
static void spi_ameba_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				   int status)
{
	ARG_UNUSED(dma_dev);

	/* arg holds SPI device
	 * Passed in spi_ameba_dma_send/receive()
	 */
	struct device *spi_dev = arg;
	struct spi_ameba_data *spi_dma_data = spi_dev->data;
	const struct spi_ameba_config *spi_cfg = spi_dev->config;
	SPI_TypeDef *spi = spi_cfg->SPIx;

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
		spi_dma_data->status_flags |= SPI_AMEBA_DMA_ERROR_FLAG;
	} else {
		/* identify the origin of this callback */
		if (channel == spi_dma_data->dma_tx.dma_channel) {
			LOG_DBG("dma cb tx");
			spi_dma_data->status_flags |= SPI_AMEBA_DMA_TX_DONE_FLAG;

			if (spi_dma_data->transfer_dir == SPI_AMEBA_HALF_DUPLEX_TX_FLAG) {
				LOG_DBG("cb for tx only");
				spi_dma_data->status_flags |= SPI_AMEBA_DMA_RX_DONE_FLAG;
			}
			SSI_SetDmaEnable(spi, DISABLE, SPI_BIT_TDMAE);

			if (dma_stop(spi_dma_data->dma_tx.dma_dev,
				     spi_dma_data->dma_tx.dma_channel) < 0) {
				LOG_ERR("Stop tx ext dma failed !!");
			}
		} else if (channel == spi_dma_data->dma_rx.dma_channel) {
			LOG_DBG("dma cb rx");
			spi_dma_data->status_flags |= SPI_AMEBA_DMA_RX_DONE_FLAG;

			SSI_SetDmaEnable(spi, DISABLE, SPI_BIT_RDMAE);

			if (dma_stop(spi_dma_data->dma_rx.dma_dev,
				     spi_dma_data->dma_rx.dma_channel) < 0) {
				LOG_ERR("Stop tx ext dma failed !!");
			}
		} else {
			LOG_ERR("DMA callback channel %d is not valid.", channel);
			spi_dma_data->status_flags |= SPI_AMEBA_DMA_ERROR_FLAG;
		}
	}

	k_sem_give(&spi_dma_data->status_sem);
}

static int spi_ameba_wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_ameba_data *data = dev->data;
	int res = -1;
	k_timeout_t timeout;

	/*
	 * In slave mode we do not know when the transaction will start. Hence,
	 * it doesn't make sense to have timeout in this case.
	 */
	if (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(&data->ctx)) {
		timeout = K_FOREVER;
	} else {
		timeout = K_MSEC(1000);
	}

	while (1) {
		res = k_sem_take(&data->status_sem, timeout);
		if (res != 0) {
			return res;
		}

		if (data->status_flags & SPI_AMEBA_DMA_ERROR_FLAG) {
			return -EIO;
		}

		if (data->status_flags & SPI_AMEBA_DMA_DONE_FLAG) {
			return 0;
		}
	}

	return res;
}

static int spi_ameba_dma_receive(const struct device *dev, uint8_t *pdata, size_t length)
{
	const struct spi_ameba_config *cfg = dev->config;
	struct spi_ameba_data *data = dev->data;
	struct spi_dma_stream *spi_dma = &data->dma_rx;
	uint32_t datalen = data->datasize; /* data bit: 4~16 bits */
	uint32_t handshake_index = spi_dma->dma_cfg.dma_slot;
	int ret = 0;

	memset(&spi_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&spi_dma->blk_cfg, 0, sizeof(struct dma_block_config));

	spi_dma->dma_cfg.dma_slot = handshake_index;
	spi_dma->dma_cfg.dma_callback = spi_ameba_dma_callback;
	spi_dma->dma_cfg.user_data = (void *)dev;
	spi_dma->dma_cfg.channel_priority = 1; /* ? */

	spi_dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	spi_dma->dma_cfg.error_callback_dis = 1;
	spi_dma->dma_cfg.source_handshake = 0;
	spi_dma->dma_cfg.dest_handshake = 1;

	spi_dma->dma_cfg.block_count = 1;
	spi_dma->dma_cfg.head_block = &spi_dma->blk_cfg;

	spi_dma->blk_cfg.flow_control_mode = 0;

	spi_dma->blk_cfg.source_address = (uint32_t)&cfg->SPIx->SPI_DRx;
	spi_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	if (spi_context_rx_buf_on(&data->ctx)) {
		spi_dma->blk_cfg.dest_address = (uint32_t)data->ctx.rx_buf;
		spi_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		spi_dma->blk_cfg.dest_address = (uint32_t)&spi_dma_rx_dummy;
		spi_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		pdata = (uint8_t *)spi_dma->blk_cfg.dest_address;
		LOG_DBG("spi dma rx dummy: %p", &spi_dma_rx_dummy);
	}

	spi_dma->blk_cfg.block_size = length;
	LOG_DBG("length:%u datalen:%u", length, datalen);
	if (datalen > 8) {
		spi_dma->dma_cfg.source_burst_length = 4;
		spi_dma->dma_cfg.source_data_size = 2;

		if (((uint32_t)(length) & 0x03) == 0 && (((uint32_t)(pdata) & 0x03) == 0)) {
			LOG_DBG("4-bytes aligned");
			/* 4-bytes aligned, move 4 bytes each transfer */
			spi_dma->dma_cfg.dest_burst_length = 4;
			spi_dma->dma_cfg.dest_data_size = 4;
		} else if (((length & 0x01) == 0) && (((uint32_t)(pdata) & 0x01) == 0)) {
			LOG_DBG("not 4-bytes aligned");
			/* 2-bytes aligned, move 2 bytes each transfer */
			spi_dma->dma_cfg.dest_burst_length = 8;
			spi_dma->dma_cfg.dest_data_size = 2;
		} else {
			LOG_ERR("pTxData=%p,  Length=%u", pdata, length);
			return -EINVAL;
		}
	} else {
		spi_dma->dma_cfg.source_burst_length = 4;
		spi_dma->dma_cfg.source_data_size = 1;

		if (((uint32_t)(length) & 0x03) == 0 && (((uint32_t)(pdata) & 0x03) == 0)) {
			LOG_DBG("4-bytes aligned");
			/* 4-bytes aligned, move 4 bytes each transfer */
			spi_dma->dma_cfg.dest_burst_length = 1;
			spi_dma->dma_cfg.dest_data_size = 4;
		} else {
			LOG_DBG("not 4-bytes aligned");
			/* 2-bytes aligned, move 2 bytes each transfer */
			spi_dma->dma_cfg.dest_burst_length = 4;
			spi_dma->dma_cfg.dest_data_size = 1;
		}
	}

	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &(spi_dma->dma_cfg));
	if (ret < 0) {
		LOG_ERR("dma_config %p failed %d", data->dma_rx.dma_dev, ret);
		return ret;
	}

	ret = dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
	if (ret < 0) {
		LOG_ERR("dma_start %p failed %d", data->dma_rx.dma_dev, ret);
		return ret;
	}

	return 0;
}

static int spi_ameba_dma_send(const struct device *dev, const uint8_t *pdata, size_t length)
{
	const struct spi_ameba_config *cfg = dev->config;

	struct spi_ameba_data *data = dev->data;
	struct spi_dma_stream *spi_dma = &data->dma_tx;
	uint32_t datalen = data->datasize; /* data bit: 4~16 bits */
	uint32_t handshake_index = spi_dma->dma_cfg.dma_slot;
	int ret = 0;

	memset(&spi_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&spi_dma->blk_cfg, 0, sizeof(struct dma_block_config));

	spi_dma->dma_cfg.dma_slot = handshake_index;
	spi_dma->dma_cfg.dma_callback = spi_ameba_dma_callback;
	spi_dma->dma_cfg.user_data = (void *)dev;
	spi_dma->dma_cfg.channel_priority = 1; /* ? */

	spi_dma->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	spi_dma->dma_cfg.source_handshake = 1;
	spi_dma->dma_cfg.dest_handshake = 0;

	spi_dma->dma_cfg.block_count = 1;
	spi_dma->dma_cfg.head_block = &spi_dma->blk_cfg;

	spi_dma->blk_cfg.flow_control_mode = 0;

	spi_dma->blk_cfg.dest_address = (uint32_t)&cfg->SPIx->SPI_DRx;
	spi_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	spi_dma->blk_cfg.block_size = length;

	if (spi_context_tx_buf_on(&data->ctx)) {
		spi_dma->blk_cfg.source_address = (uint32_t)pdata;
		spi_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		spi_dma->blk_cfg.source_address = (uint32_t)&spi_dma_tx_dummy;
		spi_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		pdata = (uint8_t *)spi_dma->blk_cfg.source_address;
		LOG_DBG("spi dma tx dummy: %p", &spi_dma_tx_dummy);
	}

	spi_dma->blk_cfg.block_size = length;

	LOG_DBG("length:%u datalen:%u", length, datalen);
	if (datalen > 8) {
		/* 16~9 bits mode */
		if (((length & 0x03) == 0) && (((uint32_t)(pdata) & 0x03) == 0)) {
			LOG_DBG("4-bytes aligned");
			/* 4-bytes aligned, move 4 bytes each transfer */
			spi_dma->dma_cfg.source_burst_length = 4;
			spi_dma->dma_cfg.source_data_size = 4;
		} else if (((length & 0x01) == 0) && (((uint32_t)(pdata) & 0x01) == 0)) {
			LOG_DBG("not 4-bytes aligned");
			/* 2-bytes aligned, move 2 bytes each transfer */
			spi_dma->dma_cfg.source_burst_length = 8;
			spi_dma->dma_cfg.source_data_size = 2;
		} else {
			LOG_ERR("pTxData=%p,  Length=%u", pdata, length);
			return -EINVAL;
		}

		spi_dma->dma_cfg.dest_data_size = 2;
		spi_dma->dma_cfg.dest_burst_length = 8;

	} else {
		/*  8~4 bits mode */
		if (((length & 0x03) == 0) && (((uint32_t)(pdata) & 0x03) == 0)) {
			LOG_DBG("4-bytes aligned");
			/* 4-bytes aligned, move 4 bytes each transfer */
			spi_dma->dma_cfg.source_burst_length = 1;
			spi_dma->dma_cfg.source_data_size = 4;
		} else {
			LOG_DBG("not 4-bytes aligned");
			/* 2-bytes aligned, move 2 bytes each transfer */
			spi_dma->dma_cfg.source_burst_length = 4;
			spi_dma->dma_cfg.source_data_size = 1;
		}

		spi_dma->dma_cfg.dest_data_size = 1;
		spi_dma->dma_cfg.dest_burst_length = 4;
	}

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &(spi_dma->dma_cfg));
	if (ret < 0) {
		LOG_ERR("dma_config %p failed %d", data->dma_tx.dma_dev, ret);
		return ret;
	}

	ret = dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	if (ret < 0) {
		LOG_ERR("dma_start %p failed %d", data->dma_tx.dma_dev, ret);
		return ret;
	}

	return 0;
}

static int spi_dma_move_rx_buffers(const struct device *dev, size_t len)
{
	struct spi_ameba_data *data = dev->data;
	uint32_t dma_segment_len;
	uint8_t dfs;

	if (data->datasize <= 8) {
		dfs = 1;
	} else if (data->datasize <= 16) {
		dfs = 2;
	} else {
		LOG_ERR("err datalen:%u", data->datasize);
		return -ENOTSUP;
	}

	dma_segment_len = len * dfs;
	LOG_DBG("%s %u rx%p len%u dfs%u", __func__, __LINE__, data->ctx.rx_buf, len, dfs);

	return spi_ameba_dma_receive(dev, data->ctx.rx_buf, dma_segment_len);
}

static int spi_dma_move_tx_buffers(const struct device *dev, size_t len)
{
	struct spi_ameba_data *data = dev->data;
	uint32_t dma_segment_len;
	uint8_t dfs;

	if (data->datasize <= 8) {
		dfs = 1;
	} else if (data->datasize <= 16) {
		dfs = 2;
	} else {
		LOG_ERR("err datalen:%u", data->datasize);
		return -ENOTSUP;
	}

	dma_segment_len = len * dfs;
	LOG_DBG("%s %u tx%p len%u dfs%u", __func__, __LINE__, data->ctx.tx_buf, len, dfs);
	return spi_ameba_dma_send(dev, data->ctx.tx_buf, dma_segment_len);
}

static int spi_dma_move_buffers(const struct device *dev, size_t len)
{
	int ret;

	ret = spi_dma_move_rx_buffers(dev, len);

	if (ret != 0) {
		return ret;
	}

	return spi_dma_move_tx_buffers(dev, len);
}
#endif /* CONFIG_SPI_AMEBA_DMA */

#ifdef CONFIG_SPI_AMEBA_DMA
static int transceive_dma(const struct device *dev, const struct spi_config *spi_cfg,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			  bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_ameba_data *data = dev->data;
	const struct spi_ameba_config *cfg = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->SPIx;
	uint32_t transfer_dir;
	uint32_t dma_len;
	uint8_t frame_size_bytes;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	if (asynchronous) {
		LOG_ERR("Not support dma mode for ASYNC");
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);
	ret = spi_ameba_configure(dev, spi_cfg);
	if (ret < 0) {
		goto end;
	}

	SSI_Cmd(spi, ENABLE);
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, ((data->datasize - 1) >> 3) + 1);
	spi_context_cs_control(&data->ctx, true);

	k_sem_reset(&data->status_sem);

	if (tx_bufs && rx_bufs) {
		data->transfer_dir = SPI_AMEBA_FULL_DUPLEX_FLAG;
	} else if (tx_bufs) {
		data->transfer_dir = SPI_AMEBA_HALF_DUPLEX_TX_FLAG;
	} else {
		LOG_ERR("Not support rx only mode for dma");
		ret = -ENOTSUP;
		goto end;
	}

	LOG_DBG("dma mode");

	transfer_dir = data->transfer_dir;
	if (spi_ameba_dma_enabled(dev)) {
		while (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
			data->status_flags = 0;

			if (transfer_dir == SPI_AMEBA_FULL_DUPLEX_FLAG) {
				dma_len = spi_context_max_continuous_chunk(&data->ctx);
				LOG_DBG("%s dma trx frame number:%u", __func__, dma_len);
				ret = spi_dma_move_buffers(dev, dma_len);
				if (ret != 0) {
					goto dma_error;
				}
			} else if (transfer_dir == SPI_AMEBA_HALF_DUPLEX_TX_FLAG) {
				LOG_DBG("%s dma tx frame number:%u", __func__, dma_len);
				dma_len = data->ctx.tx_len;
				ret = spi_dma_move_tx_buffers(dev, dma_len);
			} else {
				LOG_ERR("Not support rx only mode for dma");
				ret = -ENOTSUP;
				goto end;
			}

			if (ret != 0) {
				break;
			}

			if (transfer_dir == SPI_AMEBA_FULL_DUPLEX_FLAG) {
				SSI_SetDmaEnable(spi, ENABLE, SPI_BIT_RDMAE);
				SSI_SetDmaEnable(spi, ENABLE, SPI_BIT_TDMAE);
			} else if (transfer_dir == SPI_AMEBA_HALF_DUPLEX_TX_FLAG) {
				SSI_SetDmaEnable(spi, ENABLE, SPI_BIT_TDMAE);
			} else {
				LOG_ERR("Not support rx only mode for dma");
				ret = -ENOTSUP;
				goto end;
			}

			ret = spi_ameba_wait_dma_rx_tx_done(dev);
			if (ret != 0) {
				break;
			}
			LOG_DBG("%s dma trx done", __func__);

			/* wait until spi is no more busy (spi TX fifo is really empty) */
			while ((!(SSI_GetStatus(spi) & SPI_BIT_TFE)) ||
			       (SSI_GetStatus(spi) & SPI_BIT_BUSY)) {
				/* Wait until last frame transfer complete. */
			}

			LOG_DBG("%s trx done: tfe and bus idle", __func__);

			frame_size_bytes =
				(SPI_WORD_SIZE_GET(data->ctx.config->operation)) <= 8 ? 1 : 2;
			if (transfer_dir == SPI_AMEBA_FULL_DUPLEX_FLAG) {
				spi_context_update_tx(&data->ctx, frame_size_bytes, dma_len);
				spi_context_update_rx(&data->ctx, frame_size_bytes, dma_len);
			} else if (transfer_dir == SPI_AMEBA_HALF_DUPLEX_TX_FLAG) {
				spi_context_update_tx(&data->ctx, frame_size_bytes, dma_len);
			} else {
				LOG_ERR("Not support rx only mode for dma");
				ret = -ENOTSUP;
				goto dma_error;
				/* spi_context_update_rx(&data->ctx, frame_size_bytes,
				 * dma_len);
				 */
			}

			if (transfer_dir == SPI_AMEBA_HALF_DUPLEX_TX_FLAG &&
			    !spi_context_tx_on(&data->ctx) && spi_context_rx_on(&data->ctx)) {
				LOG_ERR("dma dev is not enabled");
				ret = -ENOTSUP;
				goto dma_error;
			}
		}

		if (ret < 0) {
			goto dma_error;
		}
	} else {
		LOG_ERR("dma dev is not enabled");
		ret = -ENOTSUP;
		goto end;
	}

	/* check after trx cb done
	 *	while ((!(SSI_GetStatus(spi) & SPI_BIT_TFE)) || (SSI_GetStatus(spi) &
	 *  SPI_BIT_BUSY));
	 */

dma_error:
	SSI_SetDmaEnable(spi, DISABLE, SPI_BIT_TDMAE);
	SSI_SetDmaEnable(spi, DISABLE, SPI_BIT_RDMAE);

	(void)dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	(void)dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	/* spi_context_complete(&data->ctx, dev, ret); */

end:
	spi_context_cs_control(&data->ctx, false);
	SSI_Cmd(spi, DISABLE);

	spi_context_release(&data->ctx, ret);
	return ret;
}
#endif /* CONFIG_SPI_AMEBA_DMA */

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_ameba_data *data = dev->data;
	const struct spi_ameba_config *config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)config->SPIx;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_AMEBA_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif /* CONFIG_SPI_AMEBA_INTERRUPT */

	/* fix for take sync sema timeout
	 * spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);
	 */
	spi_context_lock(&data->ctx, (cb != NULL), cb, userdata, spi_cfg);

	ret = spi_ameba_configure(dev, spi_cfg);
	if (ret < 0) {
		goto end;
	}

	SSI_Cmd(spi, ENABLE);
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, ((data->datasize <= 8) ? 1 : 2));
	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_AMEBA_INTERRUPT
	{
		uint32_t int_mask = 0; /* fix warning */

		data->fifo_diff = 0U;

		if (data->datasize > 16 || data->datasize < 4) {
			LOG_ERR("Data Frame Size is supported from 4 ~ 16");
			ret = -EINVAL;
			goto end;
		}

		if (spi_ameba_is_slave(data)) {
			if (tx_bufs) { /* && tx_bufs->buffers*/
				int_mask = SPI_BIT_TXEIM;
			}
			if (rx_bufs) { /* && rx_bufs->buffers*/
				int_mask |= SPI_BIT_RXFIM;
			}
		} else {
			int_mask = (SPI_BIT_TXEIM | SPI_BIT_RXFIM);
		}

		LOG_DBG("Set int_mask 0x%x", int_mask);
		SSI_INTConfig(spi, int_mask, ENABLE);
	}

	ret = spi_context_wait_for_completion(&data->ctx);
#else

	do {
		ret = spi_ameba_frame_exchange(dev);
		if (ret < 0) {
			break;
		}
	} while (spi_ameba_transfer_ongoing(data));

#ifdef CONFIG_SPI_ASYNC
	spi_context_complete(&data->ctx, dev, ret);
#endif
#endif

	while ((!(SSI_GetStatus(spi) & SPI_BIT_TFE)) || (SSI_GetStatus(spi) & SPI_BIT_BUSY)) {
		/* NOP */
	}

end:
	spi_context_cs_control(&data->ctx, false);
	SSI_Cmd(spi, DISABLE);

	spi_context_release(&data->ctx, ret);
	return ret;
}

static int spi_ameba_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_ameba_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_ameba_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_ameba_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_ameba_config *config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)config->SPIx;

	SSI_InitTypeDef spi_init_struct;
	uint32_t bus_freq;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enables SPI peripheral */
	if (clock_control_on(config->clock_dev, config->clock_subsys)) {
		LOG_ERR("Could not enable SPI clock");
		return -EIO;
	}

	if (data->initialized && spi_context_configured(ctx, spi_cfg)) {
		LOG_DBG("Already configured. No need to do it again");
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported on %s", dev->name);
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB mode is supported");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	if (data->initialized) {
		SSI_Cmd(spi, DISABLE);
		data->initialized = false;
	}

	SSI_StructInit(&spi_init_struct);

	if (spi_ameba_is_slave(data)) {
		SSI_SetRole(spi, SSI_SLAVE);
		spi_init_struct.SPI_Role = SSI_SLAVE;
		LOG_DBG("SPI ROLE: SSI_SLAVE");
	} else {
		SSI_SetRole(spi, SSI_MASTER);
		spi_init_struct.SPI_Role = SSI_MASTER;
		LOG_DBG(" SPI ROLE: SSI_MASTER");
	}

	SSI_Init(spi, &spi_init_struct);

	/* set format */
	SSI_SetSclkPhase(spi, (((spi_cfg->operation) & SPI_MODE_CPHA) ? SCPH_TOGGLES_AT_START
								      : SCPH_TOGGLES_IN_MIDDLE));
	SSI_SetSclkPolarity(spi, (((spi_cfg->operation) & SPI_MODE_CPOL) ? SCPOL_INACTIVE_IS_HIGH
									 : SCPOL_INACTIVE_IS_LOW));
	SSI_SetDataFrameSize(spi, (SPI_WORD_SIZE_GET(spi_cfg->operation) - 1)); /* DataFrameSize */

	if (!(spi_ameba_is_slave(data))) {
		/* set frequency */
		bus_freq = 100000000;
		LOG_DBG("%s %u freq%u", __func__, __LINE__, spi_cfg->frequency);
		SSI_SetBaudDiv(spi, bus_freq / spi_cfg->frequency);
	}

	data->datasize = SPI_WORD_SIZE_GET(spi_cfg->operation);
	data->initialized = true;

	ctx->config = spi_cfg;
	return 0;
}

static int spi_ameba_init(const struct device *dev)
{
	struct spi_ameba_data *data = dev->data;
	const struct spi_ameba_config *cfg = dev->config;
	int ret;

	/* spi@40121000 or spi@40122000 */
	LOG_DBG("Initializing spi: %s", dev->name);

	/* check clock device */
	if (!cfg->clock_dev) {
		return -EINVAL;
	}

	/* pinmux config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

#ifdef CONFIG_SPI_AMEBA_INTERRUPT
	/* register ISR and enable IRQn */
	cfg->irq_configure(dev);
#endif

#ifdef CONFIG_SPI_AMEBA_DMA
	if ((data->dma_rx.dma_dev != NULL) && !device_is_ready(data->dma_rx.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma_rx.dma_dev->name);
		return -ENODEV;
	}

	if ((data->dma_tx.dma_dev != NULL) && !device_is_ready(data->dma_tx.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma_tx.dma_dev->name);
		return -ENODEV;
	}

	LOG_DBG("SPI with DMA transfer");

#endif /* CONFIG_SPI_AMEBA_DMA */

	ret = spi_context_cs_configure_all(&data->ctx);

	if (ret < 0) {
		LOG_ERR("Failed to spi_context_cs_configure_all");
		return ret;
	}
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_ameba_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
#ifdef CONFIG_SPI_AMEBA_DMA
	struct spi_ameba_data *data = dev->data;

	if ((data->dma_tx.dma_dev != NULL) && (data->dma_rx.dma_dev != NULL)) {
		return transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
	}
#endif /* CONFIG_SPI_AMEBA_DMA */

	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_ameba_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				      void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static DEVICE_API(spi, ameba_spi_api) = {
	.transceive = spi_ameba_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_ameba_transceive_async,
#endif /* CONFIG_SPI_ASYNC */

#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif /* CONFIG_SPI_RTIO */
	.release = spi_ameba_release,
};

#define AMEBA_SPI_IRQ_CONFIGURE(n)                                                                 \
	static void spi_ameba_irq_configure_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_ameba_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* .dma_cfg = AMEBA_DMA_CONFIG(index, dir, 1, spi_ameba_dma_##dir##_cb), */
#ifdef CONFIG_SPI_AMEBA_DMA
#define SPI_DMA_STATUS_SEM(index)                                                                  \
	.status_sem = Z_SEM_INITIALIZER(spi_ameba_data_##index.status_sem, 0, 1),
#define SPI_DMA_CHANNEL_INIT(index, dir)                                                           \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = AMEBA_DMA_CONFIG(index, dir, 1, spi_ameba_dma_callback),

/* define macro for dma_rx or dma_tx */
#define SPI_DMA_CHANNEL(index, dir)                                                                \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                               \
					(SPI_DMA_CHANNEL_INIT(index, dir)),                        \
					(NULL)) },

#else
#define SPI_DMA_CHANNEL(index, dir)
#define SPI_DMA_STATUS_SEM(index)
#endif

#define AMEBA_SPI_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	IF_ENABLED(CONFIG_SPI_AMEBA_INTERRUPT, \
				(AMEBA_SPI_IRQ_CONFIGURE(n)));                                     \
                                                                                                   \
	static struct spi_ameba_data spi_ameba_data_##n = {                                        \
		SPI_CONTEXT_INIT_LOCK(spi_ameba_data_##n, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(spi_ameba_data_##n, ctx),                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx).initialized = false,          \
		SPI_DMA_CHANNEL(n, rx) SPI_DMA_CHANNEL(n, tx) SPI_DMA_STATUS_SEM(n)};              \
                                                                                                   \
	static struct spi_ameba_config spi_ameba_config_##n = {                                    \
		.SPIx = (SPI_TypeDef *)DT_INST_REG_ADDR(n),                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		IF_ENABLED(CONFIG_SPI_AMEBA_INTERRUPT, \
			(.irq_configure = spi_ameba_irq_configure_##n,)) };                        \
	DEVICE_DT_INST_DEFINE(n, &spi_ameba_init, NULL, &spi_ameba_data_##n,                       \
			      &spi_ameba_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
			      &ameba_spi_api);

DT_INST_FOREACH_STATUS_OKAY(AMEBA_SPI_INIT)
