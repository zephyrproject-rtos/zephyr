/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_ll_stm32);

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_spi.h>
#include <errno.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/toolchain.h>
#ifdef CONFIG_SPI_STM32_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#endif
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "spi_ll_stm32.h"

/*
 * Check for SPI_SR_FRE to determine support for TI mode frame format
 * error flag, because STM32F1 SoCs do not support it and  STM32CUBE
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

#ifdef CONFIG_SPI_STM32_DMA
/* dummy value used for transferring NOP when tx buf is null
 * and use as dummy sink for when rx buf is null
 */
uint32_t dummy_rx_tx_buffer;

/* This function is executed in the interrupt context */
static void dma_callback(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	/* arg directly holds the spi device */
	struct spi_stm32_data *data = arg;

	if (status != 0) {
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
			LOG_ERR("DMA callback channel %d is not valid.",
								channel);
			data->status_flags |= SPI_STM32_DMA_ERROR_FLAG;
		}
	}

	k_sem_give(&data->status_sem);
}

static int spi_stm32_dma_tx_load(const struct device *dev, const uint8_t *buf,
				 size_t len)
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
		dummy_rx_tx_buffer = 0;
		/* if tx buff is null, then sends NOP on the line. */
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

	blk_cfg->dest_address = ll_func_dma_get_reg_addr(cfg->spi, SPI_STM32_DMA_TX);
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
	/* give the client dev as arg, as the callback comes from the dma */
	stream->dma_cfg.user_data = data;
	/* pass our client origin to the dma: data->dma_tx.dma_channel */
	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.channel,
			&stream->dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		return ret;
	}

	/* gives the request ID to the dma mux */
	return dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
}

static int spi_stm32_dma_rx_load(const struct device *dev, uint8_t *buf,
				 size_t len)
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

	blk_cfg->source_address = ll_func_dma_get_reg_addr(cfg->spi, SPI_STM32_DMA_RX);
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
	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.channel,
			&stream->dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		return ret;
	}

	/* gives the request ID to the dma mux */
	return dma_start(data->dma_rx.dma_dev, data->dma_rx.channel);
}

static int spi_dma_move_buffers(const struct device *dev, size_t len)
{
	struct spi_stm32_data *data = dev->data;
	int ret;
	size_t dma_segment_len;

	dma_segment_len = len * data->dma_rx.dma_cfg.dest_data_size;
	ret = spi_stm32_dma_rx_load(dev, data->ctx.rx_buf, dma_segment_len);

	if (ret != 0) {
		return ret;
	}

	dma_segment_len = len * data->dma_tx.dma_cfg.source_data_size;
	ret = spi_stm32_dma_tx_load(dev, data->ctx.tx_buf, dma_segment_len);

	return ret;
}

#endif /* CONFIG_SPI_STM32_DMA */

/* Value to shift out when no application data needs transmitting. */
#define SPI_STM32_TX_NOP 0x00

static bool spi_stm32_transfer_ongoing(struct spi_stm32_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_stm32_get_err(SPI_TypeDef *spi)
{
	uint32_t sr = LL_SPI_ReadReg(spi, SR);

	if (sr & SPI_STM32_ERR_MSK) {
		LOG_ERR("%s: err=%d", __func__,
			    sr & (uint32_t)SPI_STM32_ERR_MSK);

		/* OVR error must be explicitly cleared */
		if (LL_SPI_IsActiveFlag_OVR(spi)) {
			LL_SPI_ClearFlag_OVR(spi);
		}

		return -EIO;
	}

	return 0;
}

/* Shift a SPI frame as master. */
static void spi_stm32_shift_m(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	uint16_t tx_frame = SPI_STM32_TX_NOP;
	uint16_t rx_frame;

	while (!ll_func_tx_is_empty(spi)) {
		/* NOP */
	}

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
#endif

	if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
		if (spi_context_tx_buf_on(&data->ctx)) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		}
		LL_SPI_TransmitData8(spi, tx_frame);
		/* The update is ignored if TX is off. */
		spi_context_update_tx(&data->ctx, 1, 1);
	} else {
		if (spi_context_tx_buf_on(&data->ctx)) {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
		}
		LL_SPI_TransmitData16(spi, tx_frame);
		/* The update is ignored if TX is off. */
		spi_context_update_tx(&data->ctx, 2, 1);
	}

	while (!ll_func_rx_is_not_empty(spi)) {
		/* NOP */
	}

	if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
		rx_frame = LL_SPI_ReceiveData8(spi);
		if (spi_context_rx_buf_on(&data->ctx)) {
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
		}
		spi_context_update_rx(&data->ctx, 1, 1);
	} else {
		rx_frame = LL_SPI_ReceiveData16(spi);
		if (spi_context_rx_buf_on(&data->ctx)) {
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
		}
		spi_context_update_rx(&data->ctx, 2, 1);
	}
}

/* Shift a SPI frame as slave. */
static void spi_stm32_shift_s(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	if (ll_func_tx_is_empty(spi) && spi_context_tx_on(&data->ctx)) {
		uint16_t tx_frame;

		if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
			LL_SPI_TransmitData8(spi, tx_frame);
			spi_context_update_tx(&data->ctx, 1, 1);
		} else {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
			LL_SPI_TransmitData16(spi, tx_frame);
			spi_context_update_tx(&data->ctx, 2, 1);
		}
	} else {
		ll_func_disable_int_tx_empty(spi);
	}

	if (ll_func_rx_is_not_empty(spi) &&
	    spi_context_rx_buf_on(&data->ctx)) {
		uint16_t rx_frame;

		if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
			rx_frame = LL_SPI_ReceiveData8(spi);
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
			spi_context_update_rx(&data->ctx, 1, 1);
		} else {
			rx_frame = LL_SPI_ReceiveData16(spi);
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
			spi_context_update_rx(&data->ctx, 2, 1);
		}
	}
}

/*
 * Without a FIFO, we can only shift out one frame's worth of SPI
 * data, and read the response back.
 *
 * TODO: support 16-bit data frames.
 */
static int spi_stm32_shift_frames(SPI_TypeDef *spi, struct spi_stm32_data *data)
{
	uint16_t operation = data->ctx.config->operation;

	if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_MASTER) {
		spi_stm32_shift_m(spi, data);
	} else {
		spi_stm32_shift_s(spi, data);
	}

	return spi_stm32_get_err(spi);
}

static void spi_stm32_cs_control(const struct device *dev, bool on)
{
	struct spi_stm32_data *data = dev->data;

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
#endif
}

static void spi_stm32_complete(const struct device *dev, int status)
{
	const struct spi_stm32_config *cfg = dev->config;
	SPI_TypeDef *spi = cfg->spi;
#ifdef CONFIG_SPI_STM32_INTERRUPT
	struct spi_stm32_data *data = dev->data;

	ll_func_disable_int_tx_empty(spi);
	ll_func_disable_int_rx_not_empty(spi);
	ll_func_disable_int_errors(spi);
#endif

	spi_stm32_cs_control(dev, false);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
	/* Flush RX buffer */
	while (ll_func_rx_is_not_empty(spi)) {
		(void) LL_SPI_ReceiveData8(spi);
	}
#endif

	if (LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
		while (ll_func_spi_is_busy(spi)) {
			/* NOP */
		}
	}
	/* BSY flag is cleared when MODF flag is raised */
	if (LL_SPI_IsActiveFlag_MODF(spi)) {
		LL_SPI_ClearFlag_MODF(spi);
	}

	ll_func_disable_spi(spi);

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

	err = spi_stm32_get_err(spi);
	if (err) {
		spi_stm32_complete(dev, err);
		return;
	}

	if (spi_stm32_transfer_ongoing(data)) {
		err = spi_stm32_shift_frames(spi, data);
	}

	if (err || !spi_stm32_transfer_ongoing(data)) {
		spi_stm32_complete(dev, err);
	}
}
#endif

static int spi_stm32_configure(const struct device *dev,
			       const struct spi_config *config)
{
	const struct spi_stm32_config *cfg = dev->config;
	struct spi_stm32_data *data = dev->data;
	const uint32_t scaler[] = {
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

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if ((SPI_WORD_SIZE_GET(config->operation) != 8)
	    && (SPI_WORD_SIZE_GET(config->operation) != 16)) {
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

	for (br = 1 ; br <= ARRAY_SIZE(scaler) ; ++br) {
		uint32_t clk = clock >> br;

		if (clk <= config->frequency) {
			break;
		}
	}

	if (br > ARRAY_SIZE(scaler)) {
		LOG_ERR("Unsupported frequency %uHz, max %uHz, min %uHz",
			    config->frequency,
			    clock >> 1,
			    clock >> ARRAY_SIZE(scaler));
		return -EINVAL;
	}

	LL_SPI_Disable(spi);
	LL_SPI_SetBaudRatePrescaler(spi, scaler[br - 1]);

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

	LL_SPI_SetTransferDirection(spi, LL_SPI_FULL_DUPLEX);

	if (config->operation & SPI_TRANSFER_LSB) {
		LL_SPI_SetTransferBitOrder(spi, LL_SPI_LSB_FIRST);
	} else {
		LL_SPI_SetTransferBitOrder(spi, LL_SPI_MSB_FIRST);
	}

	LL_SPI_DisableCRC(spi);

	if (config->cs || !IS_ENABLED(CONFIG_SPI_STM32_USE_HW_SS)) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			if (LL_SPI_GetNSSPolarity(spi) == LL_SPI_NSS_POLARITY_LOW)
				LL_SPI_SetInternalSSLevel(spi, LL_SPI_SS_LEVEL_HIGH);
		}
#endif
		LL_SPI_SetNSSMode(spi, LL_SPI_NSS_SOFT);
	} else {
		if (config->operation & SPI_OP_MODE_SLAVE) {
			LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_INPUT);
		} else {
			LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_OUTPUT);
		}
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LL_SPI_SetMode(spi, LL_SPI_MODE_SLAVE);
	} else {
		LL_SPI_SetMode(spi, LL_SPI_MODE_MASTER);
	}

	if (SPI_WORD_SIZE_GET(config->operation) ==  8) {
		LL_SPI_SetDataWidth(spi, LL_SPI_DATAWIDTH_8BIT);
	} else {
		LL_SPI_SetDataWidth(spi, LL_SPI_DATAWIDTH_16BIT);
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
	ll_func_set_fifo_threshold_8bit(spi);
#endif

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = config;

	LOG_DBG("Installed config %p: freq %uHz (div = %u),"
		    " mode %u/%u/%u, slave %u",
		    config, clock >> br, 1 << br,
		    (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0,
		    (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0,
		    (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) ? 1 : 0,
		    config->slave);

	return 0;
}

static int spi_stm32_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct spi_stm32_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int transceive(const struct device *dev,
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

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_STM32_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	ret = spi_stm32_configure(dev, config);
	if (ret) {
		goto end;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
	/* Flush RX buffer */
	while (ll_func_rx_is_not_empty(spi)) {
		(void) LL_SPI_ReceiveData8(spi);
	}
#endif

	LL_SPI_Enable(spi);

	/* This is turned off in spi_stm32_complete(). */
	spi_stm32_cs_control(dev, true);

#ifdef CONFIG_SPI_STM32_INTERRUPT
	ll_func_enable_int_errors(spi);

	if (rx_bufs) {
		ll_func_enable_int_rx_not_empty(spi);
	}

	ll_func_enable_int_tx_empty(spi);

	ret = spi_context_wait_for_completion(&data->ctx);
#else
	do {
		ret = spi_stm32_shift_frames(spi, data);
	} while (!ret && spi_stm32_transfer_ongoing(data));

	spi_stm32_complete(dev, ret);

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

#endif

end:
	spi_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_STM32_DMA

static int wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_stm32_data *data = dev->data;
	int res = -1;

	while (1) {
		res = k_sem_take(&data->status_sem, K_MSEC(1000));
		if (res != 0) {
			return res;
		}

		if (data->status_flags & SPI_STM32_DMA_ERROR_FLAG) {
			return -EIO;
		}

		if (data->status_flags & SPI_STM32_DMA_DONE_FLAG) {
			return 0;
		}
	}

	return res;
}

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

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	if (asynchronous) {
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	k_sem_reset(&data->status_sem);

	ret = spi_stm32_configure(dev, config);
	if (ret) {
		goto end;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	/* set request before enabling (else SPI CFG1 reg is write protected) */
	LL_SPI_EnableDMAReq_RX(spi);
	LL_SPI_EnableDMAReq_TX(spi);

	LL_SPI_Enable(spi);
	if (LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
		LL_SPI_StartMasterTransfer(spi);
	}
#else
	LL_SPI_Enable(spi);
#endif /* st_stm32h7_spi */

	/* This is turned off in spi_stm32_complete(). */
	spi_stm32_cs_control(dev, true);

	while (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
		size_t dma_len;

		if (data->ctx.rx_len == 0) {
			dma_len = data->ctx.tx_len;
		} else if (data->ctx.tx_len == 0) {
			dma_len = data->ctx.rx_len;
		} else {
			dma_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
		}

		data->status_flags = 0;

		ret = spi_dma_move_buffers(dev, dma_len);
		if (ret != 0) {
			break;
		}

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)

		/* toggle the DMA request to restart the transfer */
		LL_SPI_EnableDMAReq_RX(spi);
		LL_SPI_EnableDMAReq_TX(spi);
#endif /* ! st_stm32h7_spi */

		ret = wait_dma_rx_tx_done(dev);
		if (ret != 0) {
			break;
		}

#ifdef SPI_SR_FTLVL
		while (LL_SPI_GetTxFIFOLevel(spi) > 0) {
		}
#endif

		/* wait until spi is no more busy (spi TX fifo is really empty) */
		while (ll_func_spi_dma_busy(spi) == 0) {
		}

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
		/* toggle the DMA transfer request */
		LL_SPI_DisableDMAReq_TX(spi);
		LL_SPI_DisableDMAReq_RX(spi);
#endif /* ! st_stm32h7_spi */

		spi_context_update_tx(&data->ctx, 1, dma_len);
		spi_context_update_rx(&data->ctx, 1, dma_len);
	}

	/* spi complete relies on SPI Status Reg which cannot be disabled */
	spi_stm32_complete(dev, ret);
	/* disable spi instance after completion */
	LL_SPI_Disable(spi);
	/* The Config. Reg. on some mcus is write un-protected when SPI is disabled */
	LL_SPI_DisableDMAReq_TX(spi);
	LL_SPI_DisableDMAReq_RX(spi);

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.channel);
	dma_stop(data->dma_tx.dma_dev, data->dma_tx.channel);

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

end:
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

	if ((data->dma_tx.dma_dev != NULL)
	 && (data->dma_rx.dma_dev != NULL)) {
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

static const struct spi_driver_api api_funcs = {
	.transceive = spi_stm32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_stm32_transceive_async,
#endif
	.release = spi_stm32_release,
};

static inline bool spi_stm32_is_subghzspi(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz)
	const struct spi_stm32_config *cfg = dev->config;

	return cfg->use_subghzspi_nss;
#else
	ARG_UNUSED(dev);
	return false;
#endif
}

static int spi_stm32_init(const struct device *dev)
{
	struct spi_stm32_data *data __attribute__((unused)) = dev->data;
	const struct spi_stm32_config *cfg = dev->config;
	int err;

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t) &cfg->pclken[0]);
	if (err < 0) {
		LOG_ERR("Could not enable SPI clock");
		return err;
	}

	if (IS_ENABLED(STM32_SPI_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		err = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					      (clock_control_subsys_t) &cfg->pclken[1],
					      NULL);
		if (err < 0) {
			LOG_ERR("Could not select SPI domain clock");
			return err;
		}
	}

	if (!spi_stm32_is_subghzspi(dev)) {
		/* Configure dt provided device signals when available */
		err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			LOG_ERR("SPI pinctrl setup failed (%d)", err);
			return err;
		}
	}

#ifdef CONFIG_SPI_STM32_INTERRUPT
	cfg->irq_config(dev);
#endif

#ifdef CONFIG_SPI_STM32_DMA
	if ((data->dma_rx.dma_dev != NULL) &&
				!device_is_ready(data->dma_rx.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma_rx.dma_dev->name);
		return -ENODEV;
	}

	if ((data->dma_tx.dma_dev != NULL) &&
				!device_is_ready(data->dma_tx.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma_tx.dma_dev->name);
		return -ENODEV;
	}

	LOG_INF(" SPI with DMA transfer");

#endif /* CONFIG_SPI_STM32_DMA */

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_STM32_INTERRUPT
#define STM32_SPI_IRQ_HANDLER_DECL(id)					\
	static void spi_stm32_irq_config_func_##id(const struct device *dev)
#define STM32_SPI_IRQ_HANDLER_FUNC(id)					\
	.irq_config = spi_stm32_irq_config_func_##id,
#define STM32_SPI_IRQ_HANDLER(id)					\
static void spi_stm32_irq_config_func_##id(const struct device *dev)		\
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
#endif

#define SPI_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)	\
	.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(index, dir)),			\
	.channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),	\
	.dma_cfg = {							\
		.dma_slot = STM32_DMA_SLOT(index, dir, slot),\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(	\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),       \
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(    \
					STM32_DMA_CHANNEL_CONFIG(index, dir)),       \
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(     \
				STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.source_burst_length = 1, /* SINGLE transfer */		\
		.dest_burst_length = 1, /* SINGLE transfer */		\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),\
		.dma_callback = dma_callback,				\
		.block_count = 2,					\
	},								\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(	\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(	\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(		\
				STM32_DMA_FEATURES(index, dir)),		\


#if CONFIG_SPI_STM32_DMA
#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)			\
	.dma_##dir = {							\
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, dir),		\
			(SPI_DMA_CHANNEL_INIT(id, dir, DIR, src, dest)),\
			(NULL))						\
		},
#define SPI_DMA_STATUS_SEM(id)						\
	.status_sem = Z_SEM_INITIALIZER(				\
		spi_stm32_dev_data_##id.status_sem, 0, 1),
#else
#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)
#define SPI_DMA_STATUS_SEM(id)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz)
#define STM32_SPI_USE_SUBGHZSPI_NSS_CONFIG(id)				\
	.use_subghzspi_nss = DT_INST_PROP_OR(				\
			id, use_subghzspi_nss, false),
#else
#define STM32_SPI_USE_SUBGHZSPI_NSS_CONFIG(id)
#endif



#define STM32_SPI_INIT(id)						\
STM32_SPI_IRQ_HANDLER_DECL(id);						\
									\
PINCTRL_DT_INST_DEFINE(id);						\
									\
static const struct stm32_pclken pclken_##id[] =			\
					       STM32_DT_INST_CLOCKS(id);\
									\
static const struct spi_stm32_config spi_stm32_cfg_##id = {		\
	.spi = (SPI_TypeDef *) DT_INST_REG_ADDR(id),			\
	.pclken = pclken_##id,						\
	.pclk_len = DT_INST_NUM_CLOCKS(id),				\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
	STM32_SPI_IRQ_HANDLER_FUNC(id)					\
	STM32_SPI_USE_SUBGHZSPI_NSS_CONFIG(id)				\
};									\
									\
static struct spi_stm32_data spi_stm32_dev_data_##id = {		\
	SPI_CONTEXT_INIT_LOCK(spi_stm32_dev_data_##id, ctx),		\
	SPI_CONTEXT_INIT_SYNC(spi_stm32_dev_data_##id, ctx),		\
	SPI_DMA_CHANNEL(id, rx, RX, PERIPHERAL, MEMORY)			\
	SPI_DMA_CHANNEL(id, tx, TX, MEMORY, PERIPHERAL)			\
	SPI_DMA_STATUS_SEM(id)						\
	SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)		\
};									\
									\
DEVICE_DT_INST_DEFINE(id, &spi_stm32_init, NULL,			\
		    &spi_stm32_dev_data_##id, &spi_stm32_cfg_##id,	\
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,		\
		    &api_funcs);					\
									\
STM32_SPI_IRQ_HANDLER(id)

DT_INST_FOREACH_STATUS_OKAY(STM32_SPI_INIT)
