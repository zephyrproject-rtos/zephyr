/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_spi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#ifdef CONFIG_SPI_BEE_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_bee.h>
#endif
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(spi_bee, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_spi.h>
#include <rtl_rcc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_spi.h>
#include <rtl876x_rcc.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#define SPI_DATASIZE_TO_BYTE(size) ((size) <= 16 ? ((((size) - 1) >> 3) + 1) : 4)
#define SPI_SRC_CLOCK_HZ           40000000U

#ifdef CONFIG_SPI_SLAVE
#define SPI_SLAVE_ENABLED 1
#else
#define SPI_SLAVE_ENABLED 0
#endif
BUILD_ASSERT(!(DT_NODE_HAS_STATUS(DT_NODELABEL(spi0_slave), okay) && !SPI_SLAVE_ENABLED),
	     "Error: CONFIG_SPI_SLAVE should be set 'y' if spi0_slave node is enabled");
BUILD_ASSERT(
	DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay) +
			DT_NODE_HAS_STATUS(DT_NODELABEL(spi0_slave), okay) <=
		1,
	"Error: Both 'spi0' and 'spi0_slave' nodes are enabled. Only one can be used at a time.");

#ifdef CONFIG_SPI_BEE_DMA

K_MEM_SLAB_DEFINE(spi_dma_slab, CONFIG_SPI_BEE_DMA_BUF_SIZE, CONFIG_SPI_BEE_DMA_BUF_COUNT, 4);

struct spi_bee_dma_data {
	struct dma_config config;
	struct dma_block_config block;
	uint32_t count;
};

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t src_addr_increment;
	uint8_t dst_addr_increment;
	int fifo_threshold;
	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_length;
	size_t offset;
	volatile size_t counter;
	int32_t timeout;
	struct k_work_delayable timeout_work;
	bool enabled;
	void *dma_tmp_buf;
};
#endif

struct spi_bee_data {
	struct spi_context ctx;
	const struct device *dev;
	bool initialized;
	uint32_t datasize;
#ifdef CONFIG_SPI_BEE_DMA
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
	struct spi_bee_dma_data dma_rx_data;
	struct spi_bee_dma_data dma_tx_data;
#endif
};

struct spi_bee_config {
	uint32_t reg;
	uint16_t clkid;
	const struct pinctrl_dev_config *pcfg;
	const bool is_slave;
#ifdef CONFIG_SPI_BEE_INTERRUPT
	void (*irq_configure)(void);
#endif
};

static bool spi_bee_tx_fifo_not_full(const struct spi_bee_config *cfg)
{
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	return SPI_GetTxFIFOLen((SPI_TypeDef *)cfg->reg) <
	       (cfg->is_slave ? SPI0_SLAVE_TX_FIFO_SIZE : SPI_TX_FIFO_SIZE);
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
	if ((SPI_TypeDef *)cfg->reg == SPI1) {
		return SPI_GetTxFIFOLen((SPI_TypeDef *)cfg->reg) < 16;
	} else {
		return SPI_GetTxFIFOLen((SPI_TypeDef *)cfg->reg) < 64;
	}
#endif
}

static bool spi_bee_rx_fifo_not_full(const struct spi_bee_config *cfg)
{
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	return (SPI_GetRxFIFOLen((SPI_TypeDef *)cfg->reg) +
			SPI_GetTxFIFOLen((SPI_TypeDef *)cfg->reg) <
		(cfg->is_slave ? SPI0_SLAVE_RX_FIFO_SIZE : SPI_RX_FIFO_SIZE));
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
	if ((SPI_TypeDef *)cfg->reg == SPI1) {
		return (SPI_GetRxFIFOLen((SPI_TypeDef *)cfg->reg) +
				SPI_GetTxFIFOLen((SPI_TypeDef *)cfg->reg) <
			16);
	} else {
		return (SPI_GetRxFIFOLen((SPI_TypeDef *)cfg->reg) +
				SPI_GetTxFIFOLen((SPI_TypeDef *)cfg->reg) <
			64);
	}
#endif
}

static bool spi_bee_rx_fifo_empty(const struct spi_bee_config *cfg)
{
	return SPI_GetRxFIFOLen((SPI_TypeDef *)cfg->reg) == 0;
}

static bool spi_bee_context_ongoing(const struct device *dev)
{
	struct spi_bee_data *data = dev->data;

	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static bool spi_bee_hw_ongoing(const struct device *dev)
{
	const struct spi_bee_config *config = dev->config;

	return !SPI_GetFlagState((SPI_TypeDef *)config->reg, SPI_FLAG_TFE) ||
	       (!config->is_slave && SPI_GetFlagState((SPI_TypeDef *)config->reg, SPI_FLAG_BUSY));
}

static bool spi_bee_transfer_ongoing(const struct device *dev)
{
	return spi_bee_context_ongoing(dev) || spi_bee_hw_ongoing(dev);
}

static int spi_bee_get_err(const struct spi_bee_config *cfg)
{
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->reg;

	if (SPI_GetFlagState(spi, SPI_FLAG_DCOL)) {
		LOG_ERR("spi%p Data Collision Error status detected", spi);

		return -EIO;
	}

	return 0;
}

static inline void spi_bee_tx(struct spi_bee_data *data, SPI_TypeDef *spi, int dfs)
{
	uint32_t tx_frame = 0U;
	uint32_t datalen = data->datasize;

	if (datalen <= 8) {
		tx_frame = data->ctx.tx_buf ? *(const uint8_t *)(data->ctx.tx_buf) : 0;
	} else if (datalen <= 16) {
		tx_frame = data->ctx.tx_buf ? *(const uint16_t *)(data->ctx.tx_buf) : 0;
	} else {
		tx_frame = data->ctx.tx_buf ? *(const uint32_t *)(data->ctx.tx_buf) : 0;
	}

	SPI_SendData(spi, tx_frame);
	spi_context_update_tx(&data->ctx, dfs, 1);
}

static inline void spi_bee_rx(struct spi_bee_data *data, SPI_TypeDef *spi, int dfs)
{
	uint32_t rx_frame = SPI_ReceiveData(spi);
	uint32_t datalen = data->datasize;

	if (spi_context_rx_buf_on(&data->ctx)) {
		if (datalen <= 8) {
			*(uint8_t *)data->ctx.rx_buf = rx_frame;
		} else if (datalen <= 16) {
			*(uint16_t *)data->ctx.rx_buf = rx_frame;
		} else {
			*(uint32_t *)data->ctx.rx_buf = rx_frame;
		}
	}

	spi_context_update_rx(&data->ctx, dfs, MIN(data->ctx.rx_len, 1));
}

static int spi_bee_frame_exchange(const struct device *dev)
{
	struct spi_bee_data *data = dev->data;
	const struct spi_bee_config *dev_config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)dev_config->reg;
	struct spi_context *ctx = &data->ctx;
	int dfs = SPI_DATASIZE_TO_BYTE(data->datasize);

	while (spi_bee_tx_fifo_not_full(dev_config) && spi_bee_rx_fifo_not_full(dev_config)) {
		if (ctx->tx_len) {
			spi_bee_tx(data, spi, dfs);
		} else if (SPI_GetTxFIFOLen(spi) + SPI_GetRxFIFOLen(spi) < ctx->rx_len) {
			SPI_SendData(spi, 0);
		} else {
			break;
		}
	}

	while (!spi_bee_rx_fifo_empty(dev_config)) {
		spi_bee_rx(data, spi, dfs);
	}

	return spi_bee_get_err(dev_config);
}

#ifdef CONFIG_SPI_BEE_INTERRUPT
static void spi_bee_complete(const struct device *dev, int status)
{
	struct spi_bee_data *dev_data = dev->data;
	const struct spi_bee_config *dev_config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)dev_config->reg;

	SPI_INTConfig(spi, SPI_INT_TXE | SPI_INT_RXF, DISABLE);

#ifdef CONFIG_SPI_BEE_DMA
	if (dev_data->dma_rx.dma_dev && dev_data->dma_tx.dma_dev) {
		dma_stop(dev_data->dma_tx.dma_dev, dev_data->dma_tx.dma_channel);
		dma_stop(dev_data->dma_rx.dma_dev, dev_data->dma_rx.dma_channel);

		if (dev_data->dma_tx.dma_tmp_buf) {
			k_mem_slab_free(&spi_dma_slab, dev_data->dma_tx.dma_tmp_buf);
			dev_data->dma_tx.dma_tmp_buf = NULL;
		}

		if (dev_data->dma_rx.dma_tmp_buf) {
			k_mem_slab_free(&spi_dma_slab, dev_data->dma_rx.dma_tmp_buf);
			dev_data->dma_rx.dma_tmp_buf = NULL;
		}
	}
#endif
	spi_context_complete(&dev_data->ctx, dev, status);

	if (!dev_config->is_slave) {
		spi_context_cs_control(&dev_data->ctx, false);
	}

	SPI_Cmd(spi, DISABLE);

	spi_context_release(&dev_data->ctx, status);
}

static void spi_bee_isr(const struct device *dev)
{
	const struct spi_bee_config *cfg = dev->config;
	int err = 0;

	err = spi_bee_get_err(cfg);
	if (err) {
		spi_bee_complete(dev, err);
		return;
	}

	if (spi_bee_transfer_ongoing(dev)) {
		err = spi_bee_frame_exchange(dev);
	}

	if (err || !spi_bee_transfer_ongoing(dev)) {
		spi_bee_complete(dev, err);
	}
}

#endif /* CONFIG_SPI_BEE_INTERRUPT */

#ifdef CONFIG_SPI_BEE_DMA
static int spi_bee_dma_buf_alloc(struct spi_bee_data *data, size_t chunk_len)
{
	int ret;

	if (data->dma_tx.dma_tmp_buf) {
		k_mem_slab_free(&spi_dma_slab, data->dma_tx.dma_tmp_buf);
		data->dma_tx.dma_tmp_buf = NULL;
	}
	if (data->dma_rx.dma_tmp_buf) {
		k_mem_slab_free(&spi_dma_slab, data->dma_rx.dma_tmp_buf);
		data->dma_rx.dma_tmp_buf = NULL;
	}

	if (!(uintptr_t)data->ctx.rx_buf) {
		ret = k_mem_slab_alloc(&spi_dma_slab, &data->dma_rx.dma_tmp_buf, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("spi rx dma slab alloc fail");
			return -ENOMEM;
		}
		memset(data->dma_rx.dma_tmp_buf, 0, chunk_len);
	}

	if (!(uintptr_t)data->ctx.tx_buf) {
		ret = k_mem_slab_alloc(&spi_dma_slab, &data->dma_tx.dma_tmp_buf, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("spi tx dma slab alloc fail");
			if (data->dma_rx.dma_tmp_buf) {
				k_mem_slab_free(&spi_dma_slab, data->dma_rx.dma_tmp_buf);
				data->dma_rx.dma_tmp_buf = NULL;
			}
			return -ENOMEM;
		}
		memset(data->dma_tx.dma_tmp_buf, 0, chunk_len);
	}
	return 0;
}

static int spi_bee_start_dma_transceive(const struct device *dev)
{
	const struct spi_bee_config *cfg = dev->config;
	struct spi_bee_data *data = dev->data;
	SPI_TypeDef *spi = (SPI_TypeDef *)cfg->reg;
	struct dma_status status;
	int ret = 0;
	uint8_t dma_datasize = SPI_DATASIZE_TO_BYTE(data->datasize);
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx) * dma_datasize;

	if (chunk_len == 0) {
		spi_bee_complete(dev, 0);
		return 0;
	}

	if (chunk_len > CONFIG_SPI_BEE_DMA_BUF_SIZE) {
		LOG_ERR("DMA chunk length %d exceeds slab size %d", chunk_len,
			CONFIG_SPI_BEE_DMA_BUF_SIZE);
		return -EINVAL;
	}

	ret = spi_bee_dma_buf_alloc(data, chunk_len);
	if (ret < 0) {
		return ret;
	}

	dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &status);
	if (chunk_len != data->dma_rx.counter && !status.busy) {
		data->dma_rx.blk_cfg.dest_address =
			data->ctx.rx_buf ? (uint32_t)(uintptr_t)data->ctx.rx_buf
					 : (uint32_t)(uintptr_t)data->dma_rx.dma_tmp_buf;
		data->dma_rx.blk_cfg.block_size = chunk_len;
		ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
				 &data->dma_rx.dma_cfg);
		if (ret < 0) {
			goto error;
		}
		ret = dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		if (ret < 0) {
			goto error;
		}
	}

	dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &status);
	if (chunk_len != data->dma_tx.counter && !status.busy) {
		data->dma_tx.blk_cfg.source_address =
			data->ctx.tx_buf ? (uint32_t)(uintptr_t)data->ctx.tx_buf
					 : (uint32_t)(uintptr_t)data->dma_tx.dma_tmp_buf;
		data->dma_tx.blk_cfg.block_size = chunk_len;

		ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel,
				 &data->dma_tx.dma_cfg);
		if (ret < 0) {
			goto error;
		}
		ret = dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
		if (ret < 0) {
			goto error;
		}
	}

	SPI_Cmd(spi, ENABLE);

error:
	if (ret < 0) {
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

		if (data->dma_rx.dma_tmp_buf) {
			k_mem_slab_free(&spi_dma_slab, data->dma_rx.dma_tmp_buf);
			data->dma_rx.dma_tmp_buf = NULL;
		}

		if (data->dma_tx.dma_tmp_buf) {
			k_mem_slab_free(&spi_dma_slab, data->dma_tx.dma_tmp_buf);
			data->dma_tx.dma_tmp_buf = NULL;
		}
	}
	return ret;
}

static bool spi_bee_chunk_transfer_finished(const struct device *dev)
{
	struct spi_bee_data *data = dev->data;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);

	return (MIN(data->dma_tx.counter, data->dma_rx.counter) >= chunk_len);
}

void spi_bee_dma_rx_cb(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	const struct device *dma_dev = (const struct device *)dev;
	const struct device *spi_dev = (const struct device *)user_data;
	struct spi_bee_data *data = spi_dev->data;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);
	uint32_t datalen = data->datasize;
	int dfs = SPI_DATASIZE_TO_BYTE(datalen);
	int err = 0;

	if (status < 0) {
		LOG_ERR("dma:%p ch:%d callback gets error: %d", dma_dev, channel, status);
		spi_bee_complete(spi_dev, status);
		return;
	}

	data->dma_tx.counter += chunk_len;
	data->dma_rx.counter += chunk_len;

	if (spi_bee_chunk_transfer_finished(spi_dev)) {
		spi_context_update_tx(&data->ctx, dfs, MIN(data->ctx.tx_len, chunk_len));
		spi_context_update_rx(&data->ctx, dfs, MIN(data->ctx.rx_len, chunk_len));
		if (spi_bee_context_ongoing(spi_dev)) {
			/* Next chunk is available, reset the count and
			 * continue processing
			 */
			data->dma_tx.counter = 0;
			data->dma_rx.counter = 0;
		} else {
			/* All data is processed, complete the process */
			spi_bee_complete(spi_dev, 0);
			return;
		}
	}

	err = spi_bee_start_dma_transceive(spi_dev);
	if (err) {
		spi_bee_complete(spi_dev, err);
	}
}

void spi_bee_dma_tx_cb(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	/* TX callback is intentionally left empty. Handled in RX callback. */
}

#endif /* CONFIG_SPI_BEE_DMA */

static int spi_bee_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_bee_data *data = dev->data;
	const struct spi_bee_config *config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)config->reg;
	struct spi_context *ctx = &data->ctx;
	SPI_InitTypeDef spi_init_struct;
#ifdef CONFIG_SPI_BEE_DMA
	int dma_datasize;
#endif
	if (data->initialized && spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER && config->is_slave) {
		LOG_ERR("Master mode is not supported on %s", dev->name);
		return -EINVAL;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE && !config->is_slave) {
		LOG_ERR("Slave mode is not supported on %s", dev->name);
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB mode is not supported");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	if (data->initialized) {
		SPI_Cmd(spi, DISABLE);
		data->initialized = false;
	}

	SPI_StructInit(&spi_init_struct);
	if (!config->is_slave) {
		spi_init_struct.SPI_BaudRatePrescaler = SPI_SRC_CLOCK_HZ / spi_cfg->frequency;
	}
#if defined(CONFIG_SOC_SERIES_RTL8752H)
	else {
		spi_init_struct.SPI_Mode = SPI_Mode_Slave;
	}
#endif

	spi_init_struct.SPI_DataSize = SPI_WORD_SIZE_GET(spi_cfg->operation) - 1;
	spi_init_struct.SPI_CPOL =
		spi_cfg->operation & SPI_MODE_CPOL ? SPI_CPOL_High : SPI_CPOL_Low;
	spi_init_struct.SPI_CPHA =
		spi_cfg->operation & SPI_MODE_CPHA ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
	spi_init_struct.SPI_TxThresholdLevel = 0;
	spi_init_struct.SPI_RxThresholdLevel = 0;
#ifdef CONFIG_SPI_BEE_DMA
	dma_datasize = SPI_DATASIZE_TO_BYTE(SPI_WORD_SIZE_GET(spi_cfg->operation));
	if (data->dma_rx.dma_dev != NULL) {
		spi_init_struct.SPI_RxDmaEn = ENABLE;
		spi_init_struct.SPI_RxWaterlevel = data->dma_rx.dma_cfg.source_burst_length;
		data->dma_rx.dma_cfg.source_data_size = dma_datasize;
		data->dma_rx.dma_cfg.dest_data_size = dma_datasize;
	}

	if (data->dma_tx.dma_dev != NULL) {
		spi_init_struct.SPI_TxDmaEn = ENABLE;
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
		spi_init_struct.SPI_TxWaterlevel =
			(config->is_slave ? SPI0_SLAVE_TX_FIFO_SIZE : SPI_TX_FIFO_SIZE) -
			data->dma_tx.dma_cfg.dest_burst_length;
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
		if ((SPI_TypeDef *)config->reg == SPI1) {
			spi_init_struct.SPI_TxWaterlevel =
				16 - data->dma_tx.dma_cfg.dest_burst_length;
		} else {
			spi_init_struct.SPI_TxWaterlevel =
				64 - data->dma_tx.dma_cfg.dest_burst_length;
		}
#endif
		data->dma_tx.dma_cfg.source_data_size = dma_datasize;
		data->dma_tx.dma_cfg.dest_data_size = dma_datasize;
	}
#endif
	SPI_Init(spi, &spi_init_struct);

	data->datasize = SPI_WORD_SIZE_GET(spi_cfg->operation);
	data->initialized = true;

	ctx->config = spi_cfg;

	return 0;
}

static int spi_bee_transceive_impl(const struct device *dev, const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs, bool asynchronous,
				   spi_callback_t cb, void *userdata)
{
	struct spi_bee_data *data = dev->data;
	const struct spi_bee_config *config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)config->reg;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);
	ret = spi_bee_configure(dev, spi_cfg);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	SPI_Cmd(spi, ENABLE);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs,
				  SPI_DATASIZE_TO_BYTE(data->datasize));

	if (!config->is_slave) {
		spi_context_cs_control(&data->ctx, true);
	}

	if (!asynchronous) {
#if !defined(CONFIG_SPI_ASYNC)
		do {
			ret = spi_bee_frame_exchange(dev);
			if (ret < 0) {
				break;
			}
		} while (spi_bee_transfer_ongoing(dev));

		if (!config->is_slave) {
			spi_context_cs_control(&data->ctx, false);
		}

		SPI_Cmd(spi, DISABLE);

		spi_context_release(&data->ctx, ret);
		return ret;

#else
#if defined(CONFIG_SPI_BEE_DMA)
		if (data->dma_rx.dma_dev && data->dma_tx.dma_dev) {
			data->dma_rx.counter = 0;
			data->dma_tx.counter = 0;

			ret = spi_bee_start_dma_transceive(dev);
			if (ret < 0) {
				if (!config->is_slave) {
					spi_context_cs_control(&data->ctx, false);
				}
				SPI_Cmd(spi, DISABLE);
				spi_context_release(&data->ctx, ret);
				return ret;
			}
		} else {
			SPI_INTConfig(spi, SPI_INT_TXE | SPI_INT_RXF, ENABLE);
		}
#else
		{
			SPI_INTConfig(spi, SPI_INT_TXE | SPI_INT_RXF, ENABLE);
		}
#endif /* CONFIG_SPI_BEE_DMA */

		ret = spi_context_wait_for_completion(&data->ctx);
		return ret;
#endif /* CONFIG_SPI_ASYNC */
	}

#if defined(CONFIG_SPI_BEE_DMA)
	if (data->dma_rx.dma_dev && data->dma_tx.dma_dev) {
		data->dma_rx.counter = 0;
		data->dma_tx.counter = 0;

		ret = spi_bee_start_dma_transceive(dev);
		if (ret < 0) {
			if (!config->is_slave) {
				spi_context_cs_control(&data->ctx, false);
			}
			SPI_Cmd(spi, DISABLE);
			spi_context_release(&data->ctx, ret);
			return ret;
		}

	} else {
		SPI_INTConfig(spi, SPI_INT_TXE | SPI_INT_RXF, ENABLE);
	}
#else
	{
		SPI_INTConfig(spi, SPI_INT_TXE | SPI_INT_RXF, ENABLE);
	}
#endif /* CONFIG_SPI_BEE_DMA */

	return 0;
}

static int spi_bee_transceive(const struct device *dev, const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return spi_bee_transceive_impl(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_bee_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				    void *userdata)
{
	return spi_bee_transceive_impl(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int spi_bee_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_bee_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_BEE_DMA
static int spi_bee_dma_init(const struct device *dev)
{
	struct spi_bee_data *data = dev->data;
	const struct spi_bee_config *config = dev->config;
	SPI_TypeDef *spi = (SPI_TypeDef *)config->reg;
	int ret = 0;

	if (data->dma_rx.dma_dev != NULL && !device_is_ready(data->dma_rx.dma_dev)) {
		return -ENODEV;
	}

	if (data->dma_tx.dma_dev != NULL && !device_is_ready(data->dma_tx.dma_dev)) {
		return -ENODEV;
	}

	/* Configure dma rx config */
	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));

	data->dma_rx.blk_cfg.source_address = SPI_RX_FIFO_ADDR(spi);

	/* dest not ready */
	data->dma_rx.blk_cfg.dest_address = 0;
	data->dma_rx.blk_cfg.source_addr_adj = data->dma_rx.src_addr_increment;
	data->dma_rx.blk_cfg.dest_addr_adj = data->dma_rx.dst_addr_increment;

	data->dma_rx.blk_cfg.source_reload_en = 0;
	data->dma_rx.blk_cfg.dest_reload_en = 0;

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
	data->dma_rx.dma_cfg.user_data = (void *)(uintptr_t)dev;

	/* Configure dma tx config */
	memset(&data->dma_tx.blk_cfg, 0, sizeof(data->dma_tx.blk_cfg));

	data->dma_tx.blk_cfg.dest_address = SPI_TX_FIFO_ADDR(spi);

	data->dma_tx.blk_cfg.source_address = 0; /* not ready */

	data->dma_tx.blk_cfg.source_addr_adj = data->dma_tx.src_addr_increment;

	data->dma_tx.blk_cfg.dest_addr_adj = data->dma_tx.dst_addr_increment;

	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)(uintptr_t)dev;

	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);

	return ret;
}
#endif

static int spi_bee_init(const struct device *dev)
{
	struct spi_bee_data *data = dev->data;
	const struct spi_bee_config *cfg = dev->config;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

#ifdef CONFIG_SPI_BEE_DMA
	if ((data->dma_rx.dma_dev && !data->dma_tx.dma_dev) ||
	    (data->dma_tx.dma_dev && !data->dma_rx.dma_dev)) {
		LOG_ERR("dma must be enabled for both tx and rx channels");
		return -ENODEV;
	}

	if (data->dma_rx.dma_dev && data->dma_tx.dma_dev) {
		ret = spi_bee_dma_init(dev);
		if (ret < 0) {
			LOG_ERR("dma not ready");
		}
	}

#endif

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_SPI_BEE_INTERRUPT
	cfg->irq_configure();
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_bee_driver_api) = {
	.transceive = spi_bee_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_bee_transceive_async,
#endif
	.release = spi_bee_release,
};

#define SPI_DMA_CHANNEL_INIT(index, dir)                                                           \
	.dma_dev = DEVICE_DT_GET(BEE_DMA_CTLR(index, dir)),                                        \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg =                                                                                 \
		{                                                                                  \
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),                   \
			.channel_direction =                                                       \
				BEE_DMA_CONFIG_DIRECTION(BEE_DMA_CHANNEL_CONFIG(index, dir)),      \
			.channel_priority =                                                        \
				BEE_DMA_CONFIG_PRIORITY(BEE_DMA_CHANNEL_CONFIG(index, dir)),       \
			.source_data_size = BEE_DMA_CONFIG_SOURCE_DATA_SIZE(                       \
				BEE_DMA_CHANNEL_CONFIG(index, dir)),                               \
			.dest_data_size = BEE_DMA_CONFIG_DESTINATION_DATA_SIZE(                    \
				BEE_DMA_CHANNEL_CONFIG(index, dir)),                               \
			.source_burst_length =                                                     \
				BEE_DMA_CONFIG_SOURCE_MSIZE(BEE_DMA_CHANNEL_CONFIG(index, dir)),   \
			.dest_burst_length = BEE_DMA_CONFIG_DESTINATION_MSIZE(                     \
				BEE_DMA_CHANNEL_CONFIG(index, dir)),                               \
			.block_count = 1,                                                          \
			.dma_callback = spi_bee_dma_##dir##_cb,                                    \
	},                                                                                         \
	.src_addr_increment = BEE_DMA_CONFIG_SOURCE_ADDR_INC(BEE_DMA_CHANNEL_CONFIG(index, dir)),  \
	.dst_addr_increment =                                                                      \
		BEE_DMA_CONFIG_DESTINATION_ADDR_INC(BEE_DMA_CHANNEL_CONFIG(index, dir)),

#if defined(CONFIG_SPI_BEE_DMA)
#define SPI_DMA_CHANNEL(index, dir)                                                                \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),                               \
				  (SPI_DMA_CHANNEL_INIT(index, dir)), (NULL))},
#else
#define SPI_DMA_CHANNEL(index, dir)
#endif

#define SPI_BEE_IRQ_CONFIGURE(index)                                                               \
	static void spi_bee_irq_config_##index(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), spi_bee_isr,        \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define BEE_SPI_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	IF_ENABLED(CONFIG_SPI_BEE_INTERRUPT,                                                       \
		   (SPI_BEE_IRQ_CONFIGURE(index)));                       \
	static struct spi_bee_data spi_bee_data_##index = {                                        \
		SPI_CONTEXT_INIT_LOCK(spi_bee_data_##index, ctx),                                  \
		SPI_CONTEXT_INIT_SYNC(spi_bee_data_##index, ctx),                                  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx).dev =                     \
			DEVICE_DT_INST_GET(index),                                                 \
		.initialized = false, SPI_DMA_CHANNEL(index, rx) SPI_DMA_CHANNEL(index, tx)};      \
	static const struct spi_bee_config spi_bee_config_##index = {                              \
		.reg = DT_INST_REG_ADDR(index),                                                    \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.is_slave = DT_INST_PROP_OR(index, is_slave, false),                               \
		IF_ENABLED(CONFIG_SPI_BEE_INTERRUPT,                                               \
			   (.irq_configure = spi_bee_irq_config_##index))}; \
	DEVICE_DT_INST_DEFINE(index, &spi_bee_init, NULL, &spi_bee_data_##index,                   \
			      &spi_bee_config_##index, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,      \
			      &spi_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEE_SPI_INIT)
