/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_stm32);

#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_cache.h>
#include <stm32_ll_spi.h>

#include <errno.h>

#include "spi_stm32.h"

#if defined(CONFIG_DCACHE) && !defined(CONFIG_NOCACHE_MEMORY)
/* currently, manual cache coherency management is only done on dummy_rx_tx_buffer */
#define SPI_STM32_MANUAL_CACHE_COHERENCY_REQUIRED	1
#else
#define SPI_STM32_MANUAL_CACHE_COHERENCY_REQUIRED	0
#endif /* defined(CONFIG_DCACHE) && !defined(CONFIG_NOCACHE_MEMORY) */

#define WAIT_1US	1U

/*
 * Check for SPI_SR_FRE to determine support for TI mode frame format
 * error flag, because STM32F1 SoCs do not support it and STM32CUBE
 * for F1 family defines an unused LL_SPI_SR_FRE.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
#define SPI_STM32_ERR_MSK (LL_SPI_SR_UDR | LL_SPI_SR_CRCE | LL_SPI_SR_MODF | \
			   LL_SPI_SR_OVR | LL_SPI_SR_TIFRE)
#else
#if defined(LL_SPI_SR_UDR)
#define SPI_STM32_ERR_MSK (LL_SPI_SR_UDR | LL_SPI_SR_CRCERR | LL_SPI_SR_MODF | \
			   LL_SPI_SR_OVR | LL_SPI_SR_FRE)
#elif defined(SPI_SR_FRE)
#define SPI_STM32_ERR_MSK (LL_SPI_SR_CRCERR | LL_SPI_SR_MODF | \
			   LL_SPI_SR_OVR | LL_SPI_SR_FRE)
#else
#define SPI_STM32_ERR_MSK (LL_SPI_SR_CRCERR | LL_SPI_SR_MODF | LL_SPI_SR_OVR)
#endif
#endif /* CONFIG_SOC_SERIES_STM32MP1X */


#define ANY_SPI_DATA_WIDTH_IS(value) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(IS_EQ_STRING_PROP, \
					   st_spi_data_width,\
					   value, STM32_SPI_DATA_WIDTH_) 0)

#define IS_EQ_STRING_PROP(inst, prop, compare_value, prefix) \
	IS_EQ(CONCAT(prefix, DT_INST_STRING_UPPER_TOKEN(inst, prop)), compare_value) ||

	/* Data width supported */
#define STM32_SPI_DATA_WIDTH_FULL_4_TO_32_BIT	1
#define STM32_SPI_DATA_WIDTH_FULL_4_TO_16_BIT	2
#define STM32_SPI_DATA_WIDTH_LIMITED_8_16_BIT	3

#if ANY_SPI_DATA_WIDTH_IS(STM32_SPI_DATA_WIDTH_FULL_4_TO_32_BIT) || \
	ANY_SPI_DATA_WIDTH_IS(STM32_SPI_DATA_WIDTH_FULL_4_TO_16_BIT)

#define STM32_SPI_DATA_WIDTH_MIN	4

#if ANY_SPI_DATA_WIDTH_IS(STM32_SPI_DATA_WIDTH_FULL_4_TO_32_BIT)
#define STM32_SPI_DATA_WIDTH_MAX	32
#else
#define STM32_SPI_DATA_WIDTH_MAX	16
#endif

/* These macros are used to create a table containing all supported data widths,
 * LL_SPI_DATAWIDTH_x_BIT, with x ranging from 4 to 16 or 32 depending on series.
 */
#define STM32_SPI_DATAWIDTH(i, _)								\
	CONCAT(LL_SPI_DATAWIDTH_, UTIL_INC(UTIL_INC(UTIL_INC(UTIL_INC(i)))), BIT)
static const uint32_t table_datawidth[] = {
	LISTIFY(UTIL_DEC(UTIL_DEC(UTIL_DEC(STM32_SPI_DATA_WIDTH_MAX))), STM32_SPI_DATAWIDTH, (,))
};

#else /* ANY FULL_32 || FULL_16 */

static const uint32_t table_datawidth[] = {
	LL_SPI_DATAWIDTH_8BIT,
	LL_SPI_DATAWIDTH_16BIT,
};

#endif /* ANY FULL_32 || FULL_16 */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
#define STM32_SPI_FIFO_THRESHOLD(i, _)	CONCAT(LL_SPI_FIFO_TH_0, UTIL_INC(i), DATA)
static const uint32_t table_fifo_threshold[] = {
	LISTIFY(8, STM32_SPI_FIFO_THRESHOLD, (,))
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

static bool spi_stm32_is_data_width_supported(const struct device *dev, uint32_t width)
{
	const struct spi_stm32_config *cfg = dev->config;

	switch (cfg->datawidth) {
	case STM32_SPI_DATA_WIDTH_FULL_4_TO_32_BIT:
		return IN_RANGE(width, 4, 32);

	case STM32_SPI_DATA_WIDTH_FULL_4_TO_16_BIT:
		return IN_RANGE(width, 4, 16);

	case STM32_SPI_DATA_WIDTH_LIMITED_8_16_BIT:
		return (width == 8) || (width == 16);

	default:
		LOG_ERR("No data width defined for this instance");
		return false;
	}
}

static void spi_stm32_pm_policy_state_lock_get(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct spi_stm32_data *data = dev->data;

		if (!data->pm_policy_state_on) {
			data->pm_policy_state_on = true;
			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			if (IS_ENABLED(CONFIG_PM_S2RAM)) {
				pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
			}
			pm_device_runtime_get(dev);
		}
	}
}

static void spi_stm32_pm_policy_state_lock_put(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct spi_stm32_data *data = dev->data;

		if (data->pm_policy_state_on) {
			data->pm_policy_state_on = false;
			pm_device_runtime_put(dev);
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			if (IS_ENABLED(CONFIG_PM_S2RAM)) {
				pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
			}
		}
	}
}

static uint8_t bits2bytes(spi_operation_t operation)
{
	uint32_t bits = SPI_WORD_SIZE_GET(operation);

	if (bits <= 8U) {
		return 1U;
	} else if (bits <= 16U) {
		return 2U;
	} else {
		return 4U;
	}
}

#ifdef CONFIG_SPI_STM32_DMA
/* dummy buffer is used for transferring NOP when tx buf is null
 * and used as a dummy sink for when rx buf is null.
 */
/*
 * If Nocache Memory is supported, buffer will be placed in nocache region by
 * the linker to avoid potential DMA cache-coherency problems.
 * If Nocache Memory is not supported, cache coherency might need to be kept
 * manually. See SPI_STM32_MANUAL_CACHE_COHERENCY_REQUIRED.
 */
static __aligned(32) uint32_t dummy_rx_tx_buffer __nocache;

/* This function is executed in the interrupt context */
static void dma_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);

	/* arg holds SPI DMA data
	 * Passed in spi_stm32_dma_tx/rx_load()
	 */
	struct spi_stm32_data *data = arg;

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
		data->status_flags |= SPI_STM32_DMA_ERROR_FLAG;
	} else {
		/* identify the origin of this callback */
		if (channel == data->dma_tx.channel) {
			/* this part of the transfer ends */
			data->status_flags |= SPI_STM32_DMA_TX_DONE_FLAG;
		} else if (channel == data->dma_rx.channel) {
			/* this part of the transfer ends */
			data->status_flags |= SPI_STM32_DMA_RX_DONE_FLAG;
		} else {
			LOG_ERR("DMA callback channel %d is not valid.", channel);
			data->status_flags |= SPI_STM32_DMA_ERROR_FLAG;
		}
	}

	k_sem_give(&data->status_sem);
}

static int spi_stm32_dma_tx_load(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct spi_stm32_config *cfg = dev->config;
	struct spi_stm32_data *data = dev->data;
	struct dma_block_config *blk_cfg;
	int ret;

	/* remember active TX DMA channel (used in callback) */
	struct stream *stream = &data->dma_tx;

	blk_cfg = &stream->dma_blk_cfg;

	/* prepare the block for this TX DMA channel */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));
	blk_cfg->block_size = len;

	/* tx direction has memory as source and periph as dest. */
	if (buf == NULL) {
		/* if tx buff is null, then sends NOP on the line. */
		dummy_rx_tx_buffer = 0;
#if SPI_STM32_MANUAL_CACHE_COHERENCY_REQUIRED
		sys_cache_data_flush_range((void *)&dummy_rx_tx_buffer, sizeof(uint32_t));
#endif /* SPI_STM32_MANUAL_CACHE_COHERENCY_REQUIRED */
		blk_cfg->source_address = (uint32_t)&dummy_rx_tx_buffer;
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->source_address = (uint32_t)buf;
		if (data->dma_tx.src_addr_increment) {
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	blk_cfg->dest_address = ll_dma_get_reg_addr(cfg->spi, SPI_STM32_DMA_TX);
	/* fifo mode NOT USED there */
	if (data->dma_tx.dst_addr_increment) {
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* give the fifo mode from the DT */
	blk_cfg->fifo_mode_control = data->dma_tx.fifo_threshold;

	/* direction is given by the DT */
	stream->dma_cfg.head_block = blk_cfg;
	/* give the dma channel data as arg, as the callback comes from the dma */
	stream->dma_cfg.user_data = data;
	/* pass our client origin to the dma: data->dma_tx.dma_channel */
	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.channel, &stream->dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		return ret;
	}

	/* gives the request ID to the dma mux */
	return dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
}

static int spi_stm32_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct spi_stm32_config *cfg = dev->config;
	struct spi_stm32_data *data = dev->data;
	struct dma_block_config *blk_cfg;
	int ret;

	/* retrieve active RX DMA channel (used in callback) */
	struct stream *stream = &data->dma_rx;

	blk_cfg = &stream->dma_blk_cfg;

	/* prepare the block for this RX DMA channel */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));
	blk_cfg->block_size = len;


	/* rx direction has periph as source and mem as dest. */
	if (buf == NULL) {
		/* if rx buff is null, then write data to dummy address. */
		blk_cfg->dest_address = (uint32_t)&dummy_rx_tx_buffer;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->dest_address = (uint32_t)buf;
		if (data->dma_rx.dst_addr_increment) {
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	blk_cfg->source_address = ll_dma_get_reg_addr(cfg->spi, SPI_STM32_DMA_RX);
	if (data->dma_rx.src_addr_increment) {
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* give the fifo mode from the DT */
	blk_cfg->fifo_mode_control = data->dma_rx.fifo_threshold;

	/* direction is given by the DT */
	stream->dma_cfg.head_block = blk_cfg;
	stream->dma_cfg.user_data = data;


	/* pass our client origin to the dma: data->dma_rx.channel */
	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.channel, &stream->dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		return ret;
	}

	/* gives the request ID to the dma mux */
	return dma_start(data->dma_rx.dma_dev, data->dma_rx.channel);
}

static int spi_dma_move_rx_buffers(const struct device *dev, size_t len)
{
	struct spi_stm32_data *data = dev->data;
	size_t dma_segment_len;

	dma_segment_len = len * data->dma_rx.dma_cfg.dest_data_size;
	return spi_stm32_dma_rx_load(dev, data->ctx.rx_buf, dma_segment_len);
}

static int spi_dma_move_tx_buffers(const struct device *dev, size_t len)
{
	struct spi_stm32_data *data = dev->data;
	size_t dma_segment_len;

	dma_segment_len = len * data->dma_tx.dma_cfg.source_data_size;
	return spi_stm32_dma_tx_load(dev, data->ctx.tx_buf, dma_segment_len);
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

static void spi_dma_enable_requests(SPI_TypeDef *spi)
{
	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);

	if (transfer_dir == LL_SPI_FULL_DUPLEX) {
		LL_SPI_EnableDMAReq_RX(spi);
		LL_SPI_EnableDMAReq_TX(spi);
	} else if (transfer_dir == LL_SPI_HALF_DUPLEX_TX) {
		LL_SPI_EnableDMAReq_TX(spi);
	} else {
		LL_SPI_EnableDMAReq_RX(spi);
	}
}
#endif /* CONFIG_SPI_STM32_DMA */

/* Value to shift out when no application data needs transmitting. */
#define SPI_STM32_TX_NOP 0x00

static uint8_t spi_stm32_send_next_frame(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	const uint8_t dfs = bits2bytes(data->ctx.config->operation);
	uint32_t tx_frame = SPI_STM32_TX_NOP;
	uint8_t len;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (dfs == 4U ||
	    (dfs == 2U && data->fifo_threshold >= 2U && data->tx_len >= 2) ||
	    (dfs == 1U && data->fifo_threshold >= 4U && data->tx_len >= 4)) {
		if (spi_context_tx_buf_on(&data->ctx)) {
			tx_frame = UNALIGNED_GET((uint32_t *)(data->ctx.tx_buf));
		}
		LL_SPI_TransmitData32(spi, tx_frame);
		len = 4U / dfs;
	} else {
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
		if (dfs == 2U ||
		    (dfs == 1U && data->fifo_threshold >= 2U && data->tx_len >= 2)) {
			if (spi_context_tx_buf_on(&data->ctx)) {
				tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
			}
			LL_SPI_TransmitData16(spi, tx_frame);
			len = 2U / dfs;
		} else {
			if (spi_context_tx_buf_on(&data->ctx)) {
				tx_frame = *((uint8_t *)(data->ctx.tx_buf));
			}
			LL_SPI_TransmitData8(spi, tx_frame);
			len = 1U;
		}
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

	spi_context_update_tx(&data->ctx, dfs, len);
	return len;
}

static uint8_t spi_stm32_read_next_frame(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	const uint8_t dfs = bits2bytes(data->ctx.config->operation);
	uint32_t rx_frame = 0;
	uint8_t len;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (dfs == 4U ||
	    (dfs == 2U && data->fifo_threshold >= 2U && data->rx_len >= 2) ||
	    (dfs == 1U && data->fifo_threshold >= 4U && data->rx_len >= 4)) {
		rx_frame = LL_SPI_ReceiveData32(spi);
		if (spi_context_rx_buf_on(&data->ctx)) {
			UNALIGNED_PUT(rx_frame, (uint32_t *)data->ctx.rx_buf);
		}
		len = 4U / dfs;
	} else {
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
		if (dfs == 2U ||
		    (dfs == 1U && data->fifo_threshold >= 2U && data->rx_len >= 2)) {
			rx_frame = LL_SPI_ReceiveData16(spi);
			if (spi_context_rx_buf_on(&data->ctx)) {
				UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
			}
			len = 2U / dfs;
		} else {
			rx_frame = LL_SPI_ReceiveData8(spi);
			if (spi_context_rx_buf_on(&data->ctx)) {
				UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
			}
			len = 1U;
		}
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

	spi_context_update_rx(&data->ctx, dfs, len);
	return len;
}

static bool spi_stm32_transfer_ongoing(struct spi_stm32_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_stm32_get_err(SPI_TypeDef *spi)
{
	uint32_t sr = stm32_reg_read(&spi->SR);

	if ((sr & SPI_STM32_ERR_MSK) != 0U) {
		LOG_ERR("%s: err=%d", __func__, sr & (uint32_t)SPI_STM32_ERR_MSK);

		/* OVR error must be explicitly cleared */
		if (LL_SPI_IsActiveFlag_OVR(spi)) {
			LL_SPI_ClearFlag_OVR(spi);
		}

		return -EIO;
	}

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
static void spi_stm32_send_fifo(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	uint8_t nb_sent;
	uint8_t nb_fifo = 0U;

	do {
		nb_sent = spi_stm32_send_next_frame(spi, data);
		data->tx_len -= nb_sent;
		nb_fifo += nb_sent;
	} while (nb_fifo < data->fifo_threshold && data->tx_len);
}

static void spi_stm32_read_fifo(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	uint8_t nb_read;
	uint8_t nb_fifo = 0U;

	do {
		nb_read = spi_stm32_read_next_frame(spi, data);
		data->rx_len -= nb_read;
		nb_fifo += nb_read;
	} while (nb_fifo < data->fifo_threshold && data->rx_len);
}

static int spi_stm32_shift_fifo(const struct spi_stm32_config *cfg, struct spi_stm32_data *data)
{
	SPI_TypeDef *spi = cfg->spi;
	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);

	if (transfer_dir == LL_SPI_FULL_DUPLEX && LL_SPI_IsActiveFlag_DXP(spi) &&
	    data->tx_len != 0U && data->rx_len != 0U) {
		/* Complete data packet is available and complete data packet can be sent.
		 * Fill the TxFIFO until threshold is reached or all data for the transfer are sent.
		 * Read the RxFIFO until threshold is reached or all data for the transfer are read.
		 */
		spi_stm32_send_fifo(spi, data);
		spi_stm32_read_fifo(spi, data);
	} else if (transfer_dir != LL_SPI_HALF_DUPLEX_TX && ll_rx_is_not_empty(spi) &&
		   data->rx_len != 0U) {
		/* Complete data packet is available.
		 * Read the RxFIFO until threshold is reached or all data for the transfer are read.
		 */
		spi_stm32_read_fifo(spi, data);
	} else if (transfer_dir != LL_SPI_HALF_DUPLEX_RX && ll_tx_is_not_full(spi) &&
		   data->tx_len != 0U && data->tx_len == data->rx_len) {
		/* First complete data packet can be sent (Tx len == Rx len). After that, use DXP.
		 * Fill the TxFIFO until threshold is reached or all data for the transfer are sent.
		 */
		spi_stm32_send_fifo(spi, data);
#ifdef CONFIG_SPI_STM32_INTERRUPT
		/* After first transmit, disable TXP. The goal is to use DXP from then on.
		 * This keeps Tx and Rx synchronized and avoids overruns.
		 */
		LL_SPI_DisableIT_TXP(spi);
#endif /* CONFIG_SPI_STM32_INTERRUPT */
	}

	if (LL_SPI_IsActiveFlag_EOT(spi)) {
		while (data->rx_len != 0U) {
			/* Some data may remain in the RxFIFO at the end of transfer if transfer
			 * size is not a multiple of FIFO threshold. Read them here.
			 */
			data->rx_len -= spi_stm32_read_next_frame(spi, data);
		}
		data->tx_len = data->rx_len = spi_context_max_continuous_chunk(&data->ctx);
		if (data->tx_len != 0U) {
			/* If another transfer should take place, set the new transfer size.
			 * SPI instance should be disabled to change the TSIZE register.
			 */
			LL_SPI_ClearFlag_TXTF(spi);
			LL_SPI_ClearFlag_EOT(spi);
			ll_disable_spi(spi);
			if (data->tx_len > cfg->fifo_max_transfer_size) {
				LOG_ERR("Buffer size exceeds maximal supported value");
				return -EINVAL;
			}
			LL_SPI_SetTransferSize(spi, data->tx_len);
			LL_SPI_Enable(spi);
			LL_SPI_StartMasterTransfer(spi);
#ifdef CONFIG_SPI_STM32_INTERRUPT
			/* These flags are cleared when TXTF is set. Set them again. */
			LL_SPI_EnableIT_DXP(spi);
			LL_SPI_EnableIT_TXP(spi);
#endif /* CONFIG_SPI_STM32_INTERRUPT */
			spi_stm32_send_fifo(spi, data);
		}
	}
	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

/* Shift a SPI frame as master. */
static int spi_stm32_shift_m(const struct spi_stm32_config *cfg, struct spi_stm32_data *data)
{
	if (cfg->fifo_enabled) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		return spi_stm32_shift_fifo(cfg, data);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
	} else {
		uint32_t transfer_dir = LL_SPI_GetTransferDirection(cfg->spi);

		if (transfer_dir != LL_SPI_HALF_DUPLEX_RX) {
			while (!ll_tx_is_not_full(cfg->spi)) {
				/* NOP */
			}

			spi_stm32_send_next_frame(cfg->spi, data);
		}

		if (transfer_dir != LL_SPI_HALF_DUPLEX_TX) {
			while (!ll_rx_is_not_empty(cfg->spi)) {
				/* NOP */
			}

			spi_stm32_read_next_frame(cfg->spi, data);
		}
	}
	return 0;
}

/* Shift a SPI frame as slave. */
static void spi_stm32_shift_s(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	if (ll_tx_is_not_full(spi) && spi_context_tx_on(&data->ctx)) {
		spi_stm32_send_next_frame(spi, data);
	} else {
		ll_disable_int_tx_empty(spi);
	}

	if (ll_rx_is_not_empty(spi) && spi_context_rx_buf_on(&data->ctx)) {
		spi_stm32_read_next_frame(spi, data);
	}
}

/*
 * Without a FIFO, we can only shift out one frame's worth of SPI
 * data, and read the response back.
 */
static int spi_stm32_shift_frames(const struct spi_stm32_config *cfg, struct spi_stm32_data *data)
{
	spi_operation_t operation = data->ctx.config->operation;
	int ret;

	if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_MASTER) {
		ret = spi_stm32_shift_m(cfg, data);
		if (ret != 0) {
			return ret;
		}
	} else {
		spi_stm32_shift_s(cfg->spi, data);
	}

	return spi_stm32_get_err(cfg->spi);
}

static void spi_stm32_cs_control(const struct device *dev, bool on __maybe_unused)
{
	__maybe_unused struct spi_stm32_data *data = dev->data;

	spi_context_cs_control(&data->ctx, on);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz)
	const struct spi_stm32_config *cfg = dev->config;

	if (cfg->use_subghzspi_nss) {
		if (on) {
			LL_PWR_SelectSUBGHZSPI_NSS();
		} else {
			LL_PWR_UnselectSUBGHZSPI_NSS();
		}
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz) */
}

static void spi_stm32_msg_start(const struct device *dev, bool is_rx_empty)
{
	const struct spi_stm32_config *cfg = dev->config;
	SPI_TypeDef *spi = cfg->spi;

	ARG_UNUSED(is_rx_empty);

#if defined(CONFIG_SPI_STM32_INTERRUPT) && defined(CONFIG_SOC_SERIES_STM32H7X)
	/* Make sure IRQ is disabled to avoid any spurious IRQ to happen */
	irq_disable(cfg->irq_line);
#endif /* CONFIG_SPI_STM32_INTERRUPT && CONFIG_SOC_SERIES_STM32H7X */

	LL_SPI_Enable(spi);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	/* With the STM32MP1, STM32U5 and the STM32H7,
	 * if the device is the SPI master,
	 * we need to enable the start of the transfer with
	 * LL_SPI_StartMasterTransfer(spi)
	 */
	if (LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
		LL_SPI_StartMasterTransfer(spi);
		while (!LL_SPI_IsActiveMasterTransfer(spi)) {
			/* NOP */
		}
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

#ifdef CONFIG_SOC_SERIES_STM32H7X
	/*
	 * Add a small delay after enabling to prevent transfer stalling at high
	 * system clock frequency (see errata sheet ES0392).
	 */
	k_busy_wait(WAIT_1US);
#endif /* CONFIG_SOC_SERIES_STM32H7X */

	/* This is turned off in spi_stm32_complete(). */
	spi_stm32_cs_control(dev, true);

#ifdef CONFIG_SPI_STM32_INTERRUPT
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (cfg->fifo_enabled) {
		LL_SPI_EnableIT_EOT(spi);
		LL_SPI_EnableIT_DXP(spi);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
	ll_enable_int_errors(spi);

	if (!is_rx_empty) {
		ll_enable_int_rx_not_empty(spi);
	}

	ll_enable_int_tx_empty(spi);

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	irq_enable(cfg->irq_line);
#endif /* CONFIG_SOC_SERIES_STM32H7X */
#endif /* CONFIG_SPI_STM32_INTERRUPT */
}

#ifdef CONFIG_SPI_RTIO
/* Forward declaration for RTIO handlers conveniance */
static void spi_stm32_iodev_complete(const struct device *dev, int status);
static int spi_stm32_configure(const struct device *dev,
			       const struct spi_config *config,
			       bool write);
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
static int32_t spi_stm32_set_transfer_size(const struct device *dev,
					   const struct spi_config *config,
					   const struct spi_buf_set *tx_bufs,
					   const struct spi_buf_set *rx_bufs);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

static void spi_stm32_iodev_msg_start(const struct device *dev, struct spi_config *config,
				      const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t buf_len)
{
	struct spi_stm32_data *data = dev->data;
	uint32_t size = buf_len / bits2bytes(config->operation);

	const struct spi_buf current_tx = {.buf = NULL, .len = size};
	const struct spi_buf current_rx = {.buf = NULL, .len = size};

	data->ctx.current_tx = &current_tx;
	data->ctx.current_rx = &current_rx;

	data->ctx.tx_buf = tx_buf;
	data->ctx.rx_buf = rx_buf;
	data->ctx.tx_len = tx_buf != NULL ? size : 0;
	data->ctx.rx_len = rx_buf != NULL ? size : 0;
	data->ctx.tx_count = tx_buf != NULL ? 1 : 0;
	data->ctx.rx_count = rx_buf != NULL ? 1 : 0;

	data->ctx.sync_status = 0;

#ifdef CONFIG_SPI_SLAVE
	ctx->recv_frames = 0;
#endif /* CONFIG_SPI_SLAVE */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	const struct spi_stm32_config *cfg = dev->config;
	SPI_TypeDef *spi = cfg->spi;

	if (cfg->fifo_enabled && SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		if (LL_SPI_IsEnabled(spi)) {
			/* SPI needs to be disabled to set the transfer size */
			ll_disable_spi(spi);
		}

		if (spi_stm32_set_transfer_size(dev, NULL, NULL, NULL) != 0) {
			spi_stm32_iodev_complete(dev, -EINVAL);
		}
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

	spi_stm32_msg_start(dev, rx_buf == NULL);
}

static void spi_stm32_iodev_start(const struct device *dev)
{
	struct spi_stm32_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct spi_dt_spec *spi_dt_spec = rtio_ctx->txn_curr->sqe.iodev->data;
	struct spi_config *spi_config = &spi_dt_spec->config;
	struct rtio_sqe *sqe = &rtio_ctx->txn_curr->sqe;

	switch (sqe->op) {
	case RTIO_OP_RX:
		spi_stm32_iodev_msg_start(dev, spi_config, NULL, sqe->rx.buf, sqe->rx.buf_len);
		break;
	case RTIO_OP_TX:
		spi_stm32_iodev_msg_start(dev, spi_config, sqe->tx.buf, NULL, sqe->tx.buf_len);
		break;
	case RTIO_OP_TINY_TX:
		spi_stm32_iodev_msg_start(dev, spi_config, sqe->tiny_tx.buf, NULL,
					  sqe->tiny_tx.buf_len);
		break;
	case RTIO_OP_TXRX:
		spi_stm32_iodev_msg_start(dev, spi_config, sqe->txrx.tx_buf, sqe->txrx.rx_buf,
					  sqe->txrx.buf_len);
		break;
	default:
		LOG_ERR("Invalid op code %d for submission %p", sqe->op, (void *)sqe);
		spi_stm32_iodev_complete(dev, -EINVAL);
		break;
	}
}

static inline int spi_stm32_iodev_prepare_start(const struct device *dev)
{
	struct spi_stm32_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct spi_dt_spec *spi_dt_spec = rtio_ctx->txn_curr->sqe.iodev->data;
	struct spi_config *spi_config = &spi_dt_spec->config;
	uint8_t op_code = rtio_ctx->txn_curr->sqe.op;
	bool write = (op_code == RTIO_OP_TX) ||
		     (op_code == RTIO_OP_TINY_TX) ||
		     (op_code == RTIO_OP_TXRX);

	return spi_stm32_configure(dev, spi_config, write);
}

static void spi_stm32_iodev_complete(const struct device *dev, int status)
{
	struct spi_stm32_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (status == 0 && (rtio_ctx->txn_curr->sqe.flags & RTIO_SQE_TRANSACTION) != 0) {
		rtio_ctx->txn_curr = rtio_txn_next(rtio_ctx->txn_curr);
		spi_stm32_iodev_start(dev);
	} else {
		spi_stm32_cs_control(dev, false);
		while (spi_rtio_complete(rtio_ctx, status)) {
			status = spi_stm32_iodev_prepare_start(dev);
			if (status == 0) {
				spi_stm32_iodev_start(dev);
				break;
			}

			/* Clear chip select and loop to mark transfer completed with an error */
			spi_stm32_cs_control(dev, false);
		}
	}
}

static void spi_stm32_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_stm32_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	int err;

	if (spi_rtio_submit(rtio_ctx, iodev_sqe)) {
		err = spi_stm32_iodev_prepare_start(dev);
		if (err == 0) {
			spi_stm32_iodev_start(dev);
		} else {
			spi_stm32_iodev_complete(dev, err);
		}
	}
}
#endif /* CONFIG_SPI_RTIO */

static void spi_stm32_complete(const struct device *dev, int status)
{
	const struct spi_stm32_config *cfg = dev->config;
	SPI_TypeDef *spi = cfg->spi;
	struct spi_stm32_data *data = dev->data;

#ifdef CONFIG_SPI_RTIO
	if (data->rtio_ctx->txn_head != NULL) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		if (cfg->fifo_enabled) {
			LL_SPI_ClearFlag_TXTF(spi);
			LL_SPI_ClearFlag_OVR(spi);
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
		spi_stm32_iodev_complete(dev, status);
		return;
	}
#endif /* CONFIG_SPI_RTIO */

#ifdef CONFIG_SPI_STM32_INTERRUPT
	ll_disable_int_tx_empty(spi);
	ll_disable_int_rx_not_empty(spi);
	ll_disable_int_errors(spi);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (cfg->fifo_enabled) {
		LL_SPI_DisableIT_EOT(spi);
		LL_SPI_DisableIT_DXP(spi);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

#endif /* CONFIG_SPI_STM32_INTERRUPT */


#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
	/* Flush RX buffer */
	while (ll_rx_is_not_empty(spi)) {
		(void) LL_SPI_ReceiveData8(spi);
	}
#endif /* compat st_stm32_spi_fifo*/

	if (LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
		while (ll_spi_is_busy(spi) && LL_SPI_IsEnabled(spi)) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
			uint32_t width = SPI_WORD_SIZE_GET(data->ctx.config->operation);
			/* In non-FIFO mode, the TXC flag is not raised at the end of 9, 17 or 25
			 * bit transfer, so disable the SPI in these cases to avoid being stuck.
			 */
			if (!cfg->fifo_enabled &&
			    ((width == 9U) || (width == 17U) || (width == 25U))) {
				ll_disable_spi(spi);
			}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
			if (LL_SPI_HALF_DUPLEX_RX == LL_SPI_GetTransferDirection(spi)) {
				ll_disable_spi(spi);
			}
		}

		spi_stm32_cs_control(dev, false);
	}

	/* BSY flag is cleared when MODF flag is raised */
	if (LL_SPI_IsActiveFlag_MODF(spi)) {
		LL_SPI_ClearFlag_MODF(spi);
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);

	if (cfg->fifo_enabled) {
		LL_SPI_ClearFlag_TXTF(spi);
		LL_SPI_ClearFlag_OVR(spi);
		LL_SPI_ClearFlag_EOT(spi);
		LL_SPI_SetTransferSize(spi, 0);
	} else if (transfer_dir == LL_SPI_HALF_DUPLEX_RX) {
		LL_SPI_SetTransferSize(spi, 0);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

#ifdef CONFIG_SPI_SLAVE
	bool slave_hd_tx = spi_context_is_slave(&data->ctx) &&
			   (data->ctx.config->operation & SPI_HALF_DUPLEX) &&
			   (LL_SPI_GetTransferDirection(spi) == LL_SPI_HALF_DUPLEX_TX);
#else
	bool slave_hd_tx = false;
#endif

	/*
	 * Keep SPE enabled for SPI_HOLD_ON_CS or Slave Half-Duplex TX.
	 * Application must call spi_release() to disable SPE.
	 */
	if (!slave_hd_tx && !(data->ctx.config->operation & SPI_HOLD_ON_CS)) {
		ll_disable_spi(spi);
#if defined(CONFIG_SPI_STM32_INTERRUPT) && defined(CONFIG_SOC_SERIES_STM32H7X)
	} else {
		/* On H7, with SPI still enabled, the ISR is always called, even if the interrupt
		 * enable register is cleared.
		 * As a workaround, disable the IRQ at NVIC level.
		 */
		irq_disable(cfg->irq_line);
#endif /* CONFIG_SPI_STM32_INTERRUPT && CONFIG_SOC_SERIES_STM32H7X */
	}

#ifdef CONFIG_SPI_STM32_INTERRUPT
	spi_context_complete(&data->ctx, dev, status);
#endif
}

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_isr(const struct device *dev)
{
	const struct spi_stm32_config *cfg = dev->config;
	struct spi_stm32_data *data = dev->data;
	SPI_TypeDef *spi = cfg->spi;
	int err;

#if defined(CONFIG_SPI_RTIO)
	/* With RTIO, an interrupt can occur even though they
	 * are all previously disabled. Ignore it then.
	 */
	if (ll_are_int_disabled(spi)) {
		return;
	}
#endif /* CONFIG_SPI_RTIO */

	/* Some spurious interrupts are triggered when SPI is not enabled; ignore them.
	 * Do it only when fifo is enabled to leave non-fifo functionality untouched for now
	 */
	if (cfg->fifo_enabled) {
		if (!LL_SPI_IsEnabled(spi)) {
			return;
		}
	}

	err = spi_stm32_get_err(spi);
	if (err) {
		spi_stm32_complete(dev, err);
		return;
	}

	if (spi_stm32_transfer_ongoing(data)) {
		err = spi_stm32_shift_frames(cfg, data);
		if (err) {
			spi_stm32_complete(dev, err);
			return;
		}
	}

	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);

	if (((transfer_dir == LL_SPI_FULL_DUPLEX) && !spi_stm32_transfer_ongoing(data)) ||
	    ((transfer_dir == LL_SPI_HALF_DUPLEX_TX) && !spi_context_tx_on(&data->ctx)) ||
	    ((transfer_dir == LL_SPI_HALF_DUPLEX_RX) && !spi_context_rx_on(&data->ctx))) {
		spi_stm32_complete(dev, err);
	}
}
#endif /* CONFIG_SPI_STM32_INTERRUPT */

static int spi_stm32_configure(const struct device *dev,
			       const struct spi_config *config,
			       bool write)
{
	const struct spi_stm32_config *cfg = dev->config;
	struct spi_stm32_data *data = dev->data;
	static const uint32_t scaler[] = {
#ifdef LL_SPI_BAUDRATEPRESCALER_BYPASS
		LL_SPI_BAUDRATEPRESCALER_BYPASS,
#endif /* LL_SPI_BAUDRATEPRESCALER_BYPASS */
		LL_SPI_BAUDRATEPRESCALER_DIV2,
		LL_SPI_BAUDRATEPRESCALER_DIV4,
		LL_SPI_BAUDRATEPRESCALER_DIV8,
		LL_SPI_BAUDRATEPRESCALER_DIV16,
		LL_SPI_BAUDRATEPRESCALER_DIV32,
		LL_SPI_BAUDRATEPRESCALER_DIV64,
		LL_SPI_BAUDRATEPRESCALER_DIV128,
		LL_SPI_BAUDRATEPRESCALER_DIV256
	};
	SPI_TypeDef *spi = cfg->spi;
	uint32_t clock;
	int br;
	const int shift = (scaler[0] == LL_SPI_BAUDRATEPRESCALER_DIV2) ? 1 : 0;

#ifndef CONFIG_SPI_RTIO
	if (spi_context_configured(&data->ctx, config)) {
		if (config->operation & SPI_HALF_DUPLEX) {
			if (write) {
				LL_SPI_SetTransferDirection(spi, LL_SPI_HALF_DUPLEX_TX);
			} else {
				LL_SPI_SetTransferDirection(spi, LL_SPI_HALF_DUPLEX_RX);
			}
		}
		return 0;
	}
#endif /* CONFIG_SPI_RTIO */

	if (!spi_stm32_is_data_width_supported(dev, SPI_WORD_SIZE_GET(config->operation))) {
		return -ENOTSUP;
	}

	/* configure the frame format Motorola (default) or TI */
	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
#ifdef LL_SPI_PROTOCOL_TI
		LL_SPI_SetStandard(spi, LL_SPI_PROTOCOL_TI);
#else
		LOG_ERR("Frame Format TI not supported");
		/* on stm32F1 or some stm32L1 (cat1,2) without SPI_CR2_FRF */
		return -ENOTSUP;
#endif
#if defined(LL_SPI_PROTOCOL_MOTOROLA) && defined(SPI_CR2_FRF)
	} else {
		LL_SPI_SetStandard(spi, LL_SPI_PROTOCOL_MOTOROLA);
#endif
	}

	if (IS_ENABLED(STM32_SPI_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					   (clock_control_subsys_t) &cfg->pclken[1], &clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclk[1])");
			return -EIO;
		}
	} else {
		if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					   (clock_control_subsys_t) &cfg->pclken[0], &clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclk[0])");
			return -EIO;
		}
	}

	/* When the scaler table does not have the LL_SPI_BAUDRATEPRESCALER_BYPASS value, the n-th
	 * value of the table corresponds to a prescaler of value 2^(n+1). So in this case, it is
	 * necessary to add 1 (= shift variable) to the index.
	 * When the scaler table has the LL_SPI_BAUDRATEPRESCALER_BYPASS value, the n-th
	 * value of the table corresponds to a prescaler of value 2^n. So in this case, there is
	 * nothing to add (shift = 0).
	 */
	for (br = 0; br < ARRAY_SIZE(scaler); ++br) {
		uint32_t clk = clock >> (br + shift);

		if (clk <= config->frequency) {
			break;
		}
	}

	if (br >= ARRAY_SIZE(scaler)) {
		LOG_ERR("Unsupported frequency %uHz, max %uHz, min %uHz",
			config->frequency, clock >> shift,
			clock >> (ARRAY_SIZE(scaler) - 1 + shift));
		return -EINVAL;
	}

	LL_SPI_Disable(spi);
	LL_SPI_SetBaudRatePrescaler(spi, scaler[br]);

#if defined(SPI_CFG2_IOSWP)
	if (cfg->ioswp) {
		LL_SPI_EnableIOSwap(cfg->spi);
	}
#endif

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		LL_SPI_SetClockPolarity(spi, LL_SPI_POLARITY_HIGH);
	} else {
		LL_SPI_SetClockPolarity(spi, LL_SPI_POLARITY_LOW);
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		LL_SPI_SetClockPhase(spi, LL_SPI_PHASE_2EDGE);
	} else {
		LL_SPI_SetClockPhase(spi, LL_SPI_PHASE_1EDGE);
	}

	if ((config->operation & SPI_HALF_DUPLEX) != 0U) {
		if (write) {
			LL_SPI_SetTransferDirection(spi, LL_SPI_HALF_DUPLEX_TX);
		} else {
			LL_SPI_SetTransferDirection(spi, LL_SPI_HALF_DUPLEX_RX);
		}
	} else {
		LL_SPI_SetTransferDirection(spi, LL_SPI_FULL_DUPLEX);
	}

	if ((config->operation & SPI_TRANSFER_LSB) != 0) {
		LL_SPI_SetTransferBitOrder(spi, LL_SPI_LSB_FIRST);
	} else {
		LL_SPI_SetTransferBitOrder(spi, LL_SPI_MSB_FIRST);
	}

	LL_SPI_DisableCRC(spi);

	if (spi_cs_is_gpio(config) || cfg->soft_nss) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		if ((SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) &&
		    (LL_SPI_GetNSSPolarity(spi) == LL_SPI_NSS_POLARITY_LOW)) {
			LL_SPI_SetInternalSSLevel(spi, LL_SPI_SS_LEVEL_HIGH);
		}
#endif
		LL_SPI_SetNSSMode(spi, LL_SPI_NSS_SOFT);
	} else {
		if ((config->operation & SPI_OP_MODE_SLAVE) != 0U) {
			LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_INPUT);
		} else {
			LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_OUTPUT);
		}
	}
	if ((config->operation & SPI_OP_MODE_SLAVE) != 0U) {
		LL_SPI_SetMode(spi, LL_SPI_MODE_SLAVE);
	} else {
		LL_SPI_SetMode(spi, LL_SPI_MODE_MASTER);
	}

#if ANY_SPI_DATA_WIDTH_IS(STM32_SPI_DATA_WIDTH_FULL_4_TO_32_BIT) || \
	ANY_SPI_DATA_WIDTH_IS(STM32_SPI_DATA_WIDTH_FULL_4_TO_16_BIT)
	LL_SPI_SetDataWidth(spi, table_datawidth[SPI_WORD_SIZE_GET(config->operation) -
						 STM32_SPI_DATA_WIDTH_MIN]);
#else
	LL_SPI_SetDataWidth(spi, table_datawidth[bits2bytes(config->operation) - 1U]);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_SetMasterSSIdleness(spi, cfg->mssi_clocks);
	LL_SPI_SetInterDataIdleness(spi, cfg->midi_clocks << SPI_CFG2_MIDI_Pos);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (cfg->fifo_enabled) {
		/* Do not use bits2bytes here, we want to get 3 for 24-bit operation */
		uint8_t dfs = DIV_ROUND_UP(SPI_WORD_SIZE_GET(config->operation), 8U);

		/* FIFO threshold should not exceed half the FIFO size */
		data->fifo_threshold = cfg->fifo_size / (dfs * 2U);
	} else {
		data->fifo_threshold = 1;
	}
	LL_SPI_SetFIFOThreshold(spi, table_fifo_threshold[data->fifo_threshold - 1]);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
	data->fifo_threshold = 1;
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
#else
	data->fifo_threshold = 1;
#endif

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = config;

	LOG_DBG("Installed config %p: freq %uHz (div = %u), mode %u/%u/%u, slave %u",
		config, clock >> br, 1 << br,
		(SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0,
		(SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0,
		(SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) ? 1 : 0,
		config->slave);

	return 0;
}

static int spi_stm32_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_stm32_data *data = dev->data;
	const struct spi_stm32_config *cfg = dev->config;

	spi_context_unlock_unconditionally(&data->ctx);
	ll_disable_spi(cfg->spi);

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
static int32_t spi_stm32_count_bufset_frames(const struct spi_config *config,
					     const struct spi_buf_set *bufs)
{
	if (bufs == NULL) {
		return 0;
	}

	uint32_t num_bytes = 0;

	for (size_t i = 0; i < bufs->count; i++) {
		num_bytes += bufs->buffers[i].len;
	}

	uint8_t dfs = bits2bytes(config->operation);

	if ((num_bytes % dfs) != 0) {
		return -EINVAL;
	}

	int frames = num_bytes / dfs;

	if (frames > UINT16_MAX) {
		return -EMSGSIZE;
	}

	return frames;
}

static int32_t spi_stm32_set_transfer_size(const struct device *dev,
					   const struct spi_config *config,
					   const struct spi_buf_set *tx_bufs,
					   const struct spi_buf_set *rx_bufs)
{
	struct spi_stm32_data *data = dev->data;
	const struct spi_stm32_config *cfg = dev->config;
	SPI_TypeDef *spi = cfg->spi;
	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);
	int32_t frames;

	if (transfer_dir == LL_SPI_FULL_DUPLEX) {
		frames = spi_context_max_continuous_chunk(&data->ctx);
	} else if (transfer_dir == LL_SPI_HALF_DUPLEX_TX) {
		frames = spi_stm32_count_bufset_frames(config, tx_bufs);
	} else {
		frames = spi_stm32_count_bufset_frames(config, rx_bufs);
	}

	if (frames < 0) {
		return frames;
	}

	if (frames <= cfg->fifo_max_transfer_size) {
		data->tx_len = data->rx_len = frames;
		LL_SPI_SetTransferSize(spi, (uint32_t)frames);
	} else {
		LOG_ERR("Buffer size exceeds maximal supported value");
		return -EINVAL;
	}

	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

#ifndef CONFIG_SPI_RTIO
static int spi_stm32_half_duplex_switch_to_receive(const struct spi_stm32_config *cfg,
						   struct spi_stm32_data *data)
{
	SPI_TypeDef *spi = cfg->spi;

	if (!spi_context_tx_on(&data->ctx) && spi_context_rx_on(&data->ctx)) {
#ifndef CONFIG_SPI_STM32_INTERRUPT
		while (ll_spi_is_busy(spi)) {
			/* NOP */
		}
		LL_SPI_Disable(spi);
#endif /* CONFIG_SPI_STM32_INTERRUPT*/

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		const struct spi_config *config = data->ctx.config;

		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			int num_bytes = spi_context_total_rx_len(&data->ctx);
			uint8_t dfs = bits2bytes(config->operation);

			if ((num_bytes % dfs) != 0) {
				return -EINVAL;
			}

			LL_SPI_SetTransferSize(spi, (uint32_t) num_bytes / dfs);
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

		LL_SPI_SetTransferDirection(spi, LL_SPI_HALF_DUPLEX_RX);

		LL_SPI_Enable(spi);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		/* With the STM32MP1, STM32U5 and the STM32H7,
		 * if the device is the SPI master,
		 * we need to enable the start of the transfer with
		 * LL_SPI_StartMasterTransfer(spi).
		 */
		if (LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
			LL_SPI_StartMasterTransfer(spi);
			while (!LL_SPI_IsActiveMasterTransfer(spi)) {
				/*
				 * Check for the race condition where the transfer completes
				 * before this loop is entered. If EOT is set, the transfer
				 * is done and we can break.
				 */
				if (LL_SPI_IsActiveFlag_EOT(spi)) {
					break;
				}
			}
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

#if CONFIG_SOC_SERIES_STM32H7X
		/*
		 * Add a small delay after enabling to prevent transfer stalling at high
		 * system clock frequency (see errata sheet ES0392).
		 */
		k_busy_wait(WAIT_1US);
#endif

#ifdef CONFIG_SPI_STM32_INTERRUPT
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		if (cfg->fifo_enabled) {
			LL_SPI_EnableIT_EOT(spi);
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

		ll_enable_int_errors(spi);
		ll_enable_int_rx_not_empty(spi);
#endif /* CONFIG_SPI_STM32_INTERRUPT */
	}

	return 0;
}
#endif /* !CONFIG_SPI_RTIO */

static int transceive(const struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	struct spi_stm32_data *data = dev->data;
	int ret;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return 0;
	}

	if (!IS_ENABLED(CONFIG_SPI_STM32_INTERRUPT) && asynchronous) {
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	spi_stm32_pm_policy_state_lock_get(dev);

#ifdef CONFIG_SPI_RTIO
	ret = spi_rtio_transceive(data->rtio_ctx, config, tx_bufs, rx_bufs);
#else /* CONFIG_SPI_RTIO */
	const struct spi_stm32_config *cfg = dev->config;
	SPI_TypeDef *spi = cfg->spi;

	ret = spi_stm32_configure(dev, config, tx_bufs != NULL);
	if (ret != 0) {
		goto end;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, bits2bytes(config->operation));

	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (cfg->fifo_enabled && SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		ret = spi_stm32_set_transfer_size(dev, config, tx_bufs, rx_bufs);
		if (ret != 0) {
			goto end;
		}
	}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

	spi_stm32_msg_start(dev, rx_bufs == NULL);

#ifdef CONFIG_SPI_STM32_INTERRUPT
	do {
		ret = spi_context_wait_for_completion(&data->ctx);

		if (ret == 0 && transfer_dir == LL_SPI_HALF_DUPLEX_TX) {
			ret = spi_stm32_half_duplex_switch_to_receive(cfg, data);
			transfer_dir = LL_SPI_GetTransferDirection(spi);
		}
	} while (ret == 0 && spi_stm32_transfer_ongoing(data) &&
		 transfer_dir != LL_SPI_FULL_DUPLEX);
#else /* CONFIG_SPI_STM32_INTERRUPT */
	while (ret == 0 && spi_stm32_transfer_ongoing(data)) {
		ret = spi_stm32_shift_frames(cfg, data);

		if (ret == 0 && transfer_dir == LL_SPI_HALF_DUPLEX_TX) {
			ret = spi_stm32_half_duplex_switch_to_receive(cfg, data);
			transfer_dir = LL_SPI_GetTransferDirection(spi);
		}
	}

	spi_stm32_complete(dev, ret);

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && ret == 0) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

#endif /* CONFIG_SPI_STM32_INTERRUPT */

end:
#endif /* CONFIG_SPI_RTIO */
	spi_stm32_pm_policy_state_lock_put(dev);

	spi_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_STM32_DMA

static int wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_stm32_data *data = dev->data;
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

		if ((data->status_flags & SPI_STM32_DMA_ERROR_FLAG) != 0U) {
			return -EIO;
		}

		if ((data->status_flags & SPI_STM32_DMA_DONE_FLAG) != 0U) {
			return 0;
		}
	}

	return res;
}

#ifdef CONFIG_DCACHE
static bool is_dummy_buffer(const struct spi_buf *buf)
{
	return buf->buf == NULL;
}

static bool spi_buf_set_in_nocache(const struct spi_buf_set *bufs)
{
	for (size_t i = 0; i < bufs->count; i++) {
		const struct spi_buf *buf = &bufs->buffers[i];

		if (!is_dummy_buffer(buf) && !stm32_buf_in_nocache((uintptr_t)buf->buf, buf->len)) {
			return false;
		}
	}
	return true;
}
#endif /* CONFIG_DCACHE */

static int transceive_dma(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs,
			  bool asynchronous,
			  spi_callback_t cb,
			  void *userdata)
{
	const struct spi_stm32_config *cfg = dev->config;
	struct spi_stm32_data *data = dev->data;
	SPI_TypeDef *spi = cfg->spi;
	int ret;
	int err;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return 0;
	}

	if (asynchronous) {
		return -ENOTSUP;
	}

#ifdef CONFIG_DCACHE
	if ((tx_bufs != NULL && !spi_buf_set_in_nocache(tx_bufs)) ||
	    (rx_bufs != NULL && !spi_buf_set_in_nocache(rx_bufs))) {
		LOG_ERR("SPI DMA transfers not supported on cached memory");
		return -ENOTSUP;
	}
#endif /* CONFIG_DCACHE */

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	spi_stm32_pm_policy_state_lock_get(dev);

	k_sem_reset(&data->status_sem);

	ret = spi_stm32_configure(dev, config, tx_bufs != NULL);
	if (ret != 0) {
		goto end;
	}

	uint32_t transfer_dir = LL_SPI_GetTransferDirection(spi);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (transfer_dir == LL_SPI_HALF_DUPLEX_RX) {
		ret = spi_stm32_set_transfer_size(dev, config, tx_bufs, rx_bufs);

		if (ret < 0) {
			goto end;
		}
	}
#endif /* st_stm32h7_spi */

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, bits2bytes(config->operation));

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	/* set request before enabling (else SPI CFG1 reg is write protected) */
	spi_dma_enable_requests(spi);

	if (!cfg->fifo_enabled || transfer_dir != LL_SPI_FULL_DUPLEX ||
	    SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LL_SPI_Enable(spi);
	}

	/* In half-duplex rx mode, start transfer after
	 * setting DMA configurations
	 */
	if (transfer_dir != LL_SPI_HALF_DUPLEX_RX && LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
		LL_SPI_StartMasterTransfer(spi);
	}
#else
	LL_SPI_Enable(spi);
#endif /* st_stm32h7_spi */

	/* This is turned off in spi_stm32_complete(). */
	spi_stm32_cs_control(dev, true);

	uint8_t dfs = bits2bytes(config->operation);
	struct dma_config *rx_cfg = &data->dma_rx.dma_cfg, *tx_cfg = &data->dma_tx.dma_cfg;

	rx_cfg->source_data_size = rx_cfg->source_burst_length = dfs;
	rx_cfg->dest_data_size = rx_cfg->dest_burst_length = dfs;
	tx_cfg->source_data_size = tx_cfg->source_burst_length = dfs;
	tx_cfg->dest_data_size = tx_cfg->dest_burst_length = dfs;

	while (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
		size_t dma_len;

		data->status_flags = 0;

		if (transfer_dir == LL_SPI_FULL_DUPLEX) {
			dma_len = spi_context_max_continuous_chunk(&data->ctx);
			ret = spi_dma_move_buffers(dev, dma_len);
		} else if (transfer_dir == LL_SPI_HALF_DUPLEX_TX) {
			dma_len = data->ctx.tx_len;
			ret = spi_dma_move_tx_buffers(dev, dma_len);
		} else {
			dma_len = data->ctx.rx_len;
			ret = spi_dma_move_rx_buffers(dev, dma_len);
		}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		if (cfg->fifo_enabled && transfer_dir == LL_SPI_FULL_DUPLEX &&
		    SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			ret = spi_stm32_set_transfer_size(dev, config, tx_bufs, rx_bufs);
			if (ret != 0) {
				break;
			}
			LL_SPI_Enable(spi);
			LL_SPI_StartMasterTransfer(spi);
		}

		if (transfer_dir == LL_SPI_HALF_DUPLEX_RX &&
		    LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
			LL_SPI_StartMasterTransfer(spi);
		}
#endif /* st_stm32h7_spi */

		if (ret != 0) {
			break;
		}

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		/* toggle the DMA request to restart the transfer */
		spi_dma_enable_requests(spi);
#endif /* ! st_stm32h7_spi */

		ret = wait_dma_rx_tx_done(dev);
		if (ret != 0) {
			break;
		}

#ifdef SPI_SR_FTLVL
		while (LL_SPI_GetTxFIFOLevel(spi) > 0) {
		}
#endif /* SPI_SR_FTLVL */

#ifdef CONFIG_SPI_STM32_ERRATA_BUSY
		WAIT_FOR(!ll_spi_dma_busy(spi), CONFIG_SPI_STM32_BUSY_FLAG_TIMEOUT, k_yield());
#else
		/* wait until spi is no more busy (spi TX fifo is really empty) */
		while (ll_spi_dma_busy(spi) && LL_SPI_IsEnabled(spi)) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
			uint32_t width = SPI_WORD_SIZE_GET(data->ctx.config->operation);
			/* The TXC flag is not raised at the end of 9, 17 or 25
			 * bit transfer, so disable the SPI in these cases to avoid being stuck.
			 */
			if (!cfg->fifo_enabled &&
			    ((width == 9U) || (width == 17U) || (width == 25U))) {
				k_usleep(1000);
				ll_disable_spi(spi);
			}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */
		}
#endif /* CONFIG_SPI_STM32_ERRATA_BUSY */

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		/* toggle the DMA transfer request */
		LL_SPI_DisableDMAReq_TX(spi);
		LL_SPI_DisableDMAReq_RX(spi);
#endif /* ! st_stm32h7_spi */

		if (transfer_dir == LL_SPI_FULL_DUPLEX) {
			spi_context_update_tx(&data->ctx, dfs, dma_len);
			spi_context_update_rx(&data->ctx, dfs, dma_len);
		} else if (transfer_dir == LL_SPI_HALF_DUPLEX_TX) {
			spi_context_update_tx(&data->ctx, dfs, dma_len);
		} else {
			spi_context_update_rx(&data->ctx, dfs, dma_len);
		}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		if (cfg->fifo_enabled && transfer_dir == LL_SPI_FULL_DUPLEX &&
		    SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			LL_SPI_ClearFlag_TXTF(spi);
			LL_SPI_Disable(spi);
		}
#endif /* st_stm32h7_spi */

		if (transfer_dir == LL_SPI_HALF_DUPLEX_TX &&
		    !spi_context_tx_on(&data->ctx) &&
		    spi_context_rx_on(&data->ctx)) {
			LL_SPI_Disable(spi);
			LL_SPI_SetTransferDirection(spi, LL_SPI_HALF_DUPLEX_RX);

			transfer_dir = LL_SPI_HALF_DUPLEX_RX;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
			ret = spi_stm32_set_transfer_size(dev, config, tx_bufs, rx_bufs);

			if (ret < 0) {
				break;
			}

			spi_dma_enable_requests(spi);
#endif /* st_stm32h7_spi */

			LL_SPI_Enable(spi);
		}
	}

	/* spi complete relies on SPI Status Reg which cannot be disabled */
	spi_stm32_complete(dev, ret);

#ifdef CONFIG_SPI_SLAVE
	bool slave_hd_tx = spi_context_is_slave(&data->ctx) &&
			   (config->operation & SPI_HALF_DUPLEX) &&
			   (LL_SPI_GetTransferDirection(spi) == LL_SPI_HALF_DUPLEX_TX);
#else
	bool slave_hd_tx = false;
#endif

	/* Keep SPE enabled for SPI_HOLD_ON_CS or Slave Half-Duplex TX */
	if (!slave_hd_tx && !(config->operation & SPI_HOLD_ON_CS)) {
		LL_SPI_Disable(spi);
	}
	/* The Config. Reg. on some mcus is write un-protected when SPI is disabled */
	LL_SPI_DisableDMAReq_TX(spi);
	LL_SPI_DisableDMAReq_RX(spi);

	err = dma_stop(data->dma_rx.dma_dev, data->dma_rx.channel);
	if (err != 0) {
		LOG_DBG("Rx dma_stop failed with error %d", err);
	}
	err = dma_stop(data->dma_tx.dma_dev, data->dma_tx.channel);
	if (err != 0) {
		LOG_DBG("Tx dma_stop failed with error %d", err);
	}

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && ret == 0) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

end:
	spi_stm32_pm_policy_state_lock_put(dev);

	spi_context_release(&data->ctx, ret);

	return ret;
}
#endif /* CONFIG_SPI_STM32_DMA */

static int spi_stm32_transceive(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
#ifdef CONFIG_SPI_STM32_DMA
	struct spi_stm32_data *data = dev->data;

	if ((data->dma_tx.dma_dev != NULL) && (data->dma_rx.dma_dev != NULL)) {
		return transceive_dma(dev, config, tx_bufs, rx_bufs,
				      false, NULL, NULL);
	}
#endif /* CONFIG_SPI_STM32_DMA */
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_stm32_transceive_async(const struct device *dev,
				      const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      spi_callback_t cb,
				      void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static DEVICE_API(spi, api_funcs) = {
	.transceive = spi_stm32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_stm32_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_stm32_iodev_submit,
#endif
	.release = spi_stm32_release,
};

static bool spi_stm32_is_subghzspi(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz)
	const struct spi_stm32_config *cfg = dev->config;

	return cfg->use_subghzspi_nss;
#else
	ARG_UNUSED(dev);
	return false;
#endif /* st_stm32_spi_subghz */
}

static int spi_stm32_pinctrl_apply(const struct device *dev, uint8_t id)
{
	const struct spi_stm32_config *config = dev->config;
	int err;

	if (spi_stm32_is_subghzspi(dev)) {
		return 0;
	}

	/* Move pins to requested state */
	err = pinctrl_apply_state(config->pcfg, id);
	if ((id == PINCTRL_STATE_SLEEP) && (err == -ENOENT)) {
		/* Sleep state is optional */
		err = 0;
	}
	return err;
}

static int spi_stm32_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused struct spi_stm32_data *data = dev->data;
	const struct spi_stm32_config *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Configure pins for active mode */
		err = spi_stm32_pinctrl_apply(dev, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}
		/* enable clock */
		err = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
		if (err != 0) {
			LOG_ERR("Could not enable SPI clock");
			return err;
		}
		/* (re-)init SPI context and all CS configuration */
		err = spi_context_cs_configure_all(&data->ctx);
		if (err < 0) {
			return err;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Stop device clock. */
		err = clock_control_off(clk, (clock_control_subsys_t)&config->pclken[0]);
		if (err != 0) {
			LOG_ERR("Could not disable SPI clock");
			return err;
		}
		/* Configure pins for sleep mode */
		return spi_stm32_pinctrl_apply(dev, PINCTRL_STATE_SLEEP);
	case PM_DEVICE_ACTION_TURN_ON:
		/* Configure pins for sleep mode */
		return spi_stm32_pinctrl_apply(dev, PINCTRL_STATE_SLEEP);
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int spi_stm32_init(const struct device *dev)
{
	struct spi_stm32_data *data __attribute__((unused)) = dev->data;
	const struct spi_stm32_config *cfg = dev->config;
	int err;

	if (IS_ENABLED(STM32_SPI_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		err = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					      (clock_control_subsys_t) &cfg->pclken[1],
					      NULL);
		if (err < 0) {
			LOG_ERR("Could not select SPI domain clock");
			return err;
		}
	}

#ifdef CONFIG_SPI_STM32_INTERRUPT
	cfg->irq_config(dev);
#endif /* CONFIG_SPI_STM32_INTERRUPT */

#ifdef CONFIG_SPI_STM32_DMA
	if ((data->dma_rx.dma_dev != NULL) && !device_is_ready(data->dma_rx.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma_rx.dma_dev->name);
		return -ENODEV;
	}

	if ((data->dma_tx.dma_dev != NULL) && !device_is_ready(data->dma_tx.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma_tx.dma_dev->name);
		return -ENODEV;
	}

	LOG_DBG("SPI with DMA transfer");

#endif /* CONFIG_SPI_STM32_DMA */

#ifdef CONFIG_SPI_RTIO
	spi_rtio_init(data->rtio_ctx, dev);
#endif /* CONFIG_SPI_RTIO */

	spi_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, spi_stm32_pm_action);
}

#ifdef CONFIG_SPI_STM32_INTERRUPT
#define STM32_SPI_IRQ_HANDLER_DECL(id)						\
	static void spi_stm32_irq_config_func_##id(const struct device *dev)

#define STM32_SPI_IRQ_HANDLER_FUNC(id)						\
	.irq_config = spi_stm32_irq_config_func_##id,				\
	IF_ENABLED(CONFIG_SOC_SERIES_STM32H7X,					\
		   (.irq_line = DT_INST_IRQN(id),))

#define STM32_SPI_IRQ_HANDLER(id)						\
	static void spi_stm32_irq_config_func_##id(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(id),					\
			    DT_INST_IRQ(id, priority),				\
			    spi_stm32_isr, DEVICE_DT_INST_GET(id), 0);		\
		irq_enable(DT_INST_IRQN(id));					\
	}
#else
#define STM32_SPI_IRQ_HANDLER_DECL(id)
#define STM32_SPI_IRQ_HANDLER_FUNC(id)
#define STM32_SPI_IRQ_HANDLER(id)
#endif /* CONFIG_SPI_STM32_INTERRUPT */

#define SPI_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)		\
	.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(index, dir)),			\
	.channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),		\
	.dma_cfg = {								\
		.dma_slot = STM32_DMA_SLOT(index, dir, slot),			\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(		\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(	\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(	\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		/* use single transfers (burst length = data size) */		\
		.source_burst_length = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(	\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.dest_burst_length = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(	\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(			\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.dma_callback = dma_callback,					\
		.block_count = 2,						\
	},									\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(		\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),		\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(		\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),		\
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(			\
				STM32_DMA_FEATURES(index, dir)),		\


#ifdef CONFIG_SPI_STM32_DMA
#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)				\
	.dma_##dir = {								\
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, dir),			\
			    (SPI_DMA_CHANNEL_INIT(id, dir, DIR, src, dest)),	\
			    (NULL))						\
		},
#define SPI_DMA_STATUS_SEM(id)							\
	.status_sem = Z_SEM_INITIALIZER(spi_stm32_dev_data_##id.status_sem, 0, 1),
#else
#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)
#define SPI_DMA_STATUS_SEM(id)
#endif /* CONFIG_SPI_STM32_DMA */

#define SPI_SUPPORTS_FIFO(id)	DT_INST_NODE_HAS_PROP(id, fifo_enable)
#define SPI_GET_FIFO_PROP(id)	DT_INST_PROP(id, fifo_enable)
#define SPI_FIFO_ENABLED(id)	COND_CODE_1(SPI_SUPPORTS_FIFO(id), (SPI_GET_FIFO_PROP(id)), (0))

#define STM32_SPI_INIT(id)							\
	STM32_SPI_IRQ_HANDLER_DECL(id);						\
										\
	PINCTRL_DT_INST_DEFINE(id);						\
										\
	static const struct stm32_pclken pclken_##id[] =			\
						       STM32_DT_INST_CLOCKS(id);\
										\
	static const struct spi_stm32_config spi_stm32_cfg_##id = {		\
		.spi = (SPI_TypeDef *)DT_INST_REG_ADDR(id),			\
		.pclken = pclken_##id,						\
		.pclk_len = DT_INST_NUM_CLOCKS(id),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
		.datawidth = CONCAT(STM32_SPI_DATA_WIDTH_,			\
			DT_INST_STRING_UPPER_TOKEN(id, st_spi_data_width)),	\
		.fifo_enabled = SPI_FIFO_ENABLED(id),				\
		.ioswp = DT_INST_PROP(id, ioswp),				\
		STM32_SPI_IRQ_HANDLER_FUNC(id)					\
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz),	\
			   (.use_subghzspi_nss =				\
				DT_INST_PROP_OR(id, use_subghzspi_nss, false),))\
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi), (		\
			.midi_clocks = DT_INST_PROP(id, midi_clock),		\
			.mssi_clocks = DT_INST_PROP(id, mssi_clock),		\
			.fifo_size = DT_INST_PROP(id, fifo_size),		\
			.fifo_max_transfer_size = DT_INST_PROP(id, fifo_max_transfer_size),	\
		))								\
		.soft_nss = DT_INST_PROP(id, st_soft_nss),			\
	};									\
										\
	IF_ENABLED(CONFIG_SPI_RTIO,						\
		   (SPI_RTIO_DEFINE(spi_stm32_rtio_##id,			\
				    CONFIG_SPI_STM32_RTIO_SQ_SIZE,		\
				    CONFIG_SPI_STM32_RTIO_CQ_SIZE)))		\
										\
	static struct spi_stm32_data spi_stm32_dev_data_##id = {		\
		SPI_CONTEXT_INIT_LOCK(spi_stm32_dev_data_##id, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_stm32_dev_data_##id, ctx),		\
		SPI_DMA_CHANNEL(id, rx, RX, PERIPHERAL, MEMORY)			\
		SPI_DMA_CHANNEL(id, tx, TX, MEMORY, PERIPHERAL)			\
		SPI_DMA_STATUS_SEM(id)						\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)		\
		IF_ENABLED(CONFIG_SPI_RTIO, (.rtio_ctx = &spi_stm32_rtio_##id,))\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(id, spi_stm32_pm_action);			\
										\
	SPI_DEVICE_DT_INST_DEFINE(id, spi_stm32_init,				\
				  PM_DEVICE_DT_INST_GET(id),			\
				  &spi_stm32_dev_data_##id,			\
				  &spi_stm32_cfg_##id,				\
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,	\
				  &api_funcs);					\
										\
	STM32_SPI_IRQ_HANDLER(id)

DT_INST_FOREACH_STATUS_OKAY(STM32_SPI_INIT)
