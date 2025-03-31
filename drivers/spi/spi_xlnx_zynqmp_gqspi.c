/*
 * Copyright (c) 2025 Calian Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_zynqmp_qspi_1_0

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(xlnx_zynqmp_gqspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/**
 * TODO: DMA mode not yet implemented
 */

enum xlnx_zynqmp_gqspi_registers {
	GQSPI_CFG = 0x100,
	GQSPI_ISR = 0x104,
	GQSPI_IER = 0x108,
	GQSPI_IDR = 0x10C,
	GQSPI_IMASK = 0x110,
	GQSPI_EN = 0x114,
	GQSPI_TXD = 0x11C,
	GQSPI_RXD = 0x120,
	GQSPI_TX_THRESH = 0x128,
	GQSPI_RX_THRESH = 0x12C,
	GQSPI_GPIO = 0x130,
	GQSPI_LPBK_DLY_ADJ = 0x138,
	GQSPI_GEN_FIFO = 0x140,
	GQSPI_SEL = 0x144,
	GQSPI_FIFO_CTRL = 0x14C,
	GQSPI_GF_THRESH = 0x150,
	GQSPI_POLL_CFG = 0x154,
	GQSPI_POLL_TIMEOUT = 0x158,
	GQSPI_DATA_DLY_ADJ = 0x1F8,
	GQSPI_MOD_ID = 0x1FC,
	GQSPIDMA_DST_ADDR = 0x800,
	GQSPIDMA_DST_SIZE = 0x804,
	GQSPIDMA_DST_STS = 0x808,
	GQSPIDMA_DST_CTRL = 0x80C,
	GQSPIDMA_DST_I_STS = 0x814,
	GQSPIDMA_DST_I_EN = 0x818,
	GQSPIDMA_DST_I_DIS = 0x81C,
	GQSPIDMA_DST_I_MASK = 0x820,
	GQSPIDMA_DST_CTRL2 = 0x824,
	GQSPIDMA_DST_ADDR_MSB = 0x828,
};

#define GQSPI_CFG_MODE_EN_MASK               BIT_MASK(2)
#define GQSPI_CFG_MODE_EN_SHIFT              30
#define GQSPI_CFG_MODE_IO                    0
#define GQSPI_CFG_MODE_DMA                   2
#define GQSPI_CFG_GEN_FIFO_START_MANUAL_MASK BIT(29)
#define GQSPI_CFG_START_GEN_FIFO_MASK        BIT(28)
#define GQSPI_CFG_ENDIAN_BE_MASK             BIT(26)
#define GQSPI_CFG_EN_POLL_TIMEOUT_MASK       BIT(20)
#define GQSPI_CFG_WP_HOLD_MASK               BIT(19)
#define GQSPI_CFG_BAUD_RATE_DIV_MASK         BIT_MASK(3)
#define GQSPI_CFG_BAUD_RATE_DIV_SHIFT        3
#define GQSPI_CFG_CLK_PH_MASK                BIT(2)
#define GQSPI_CFG_CLK_POL_MASK               BIT(1)

#define GQSPI_EN_ENABLE_MASK BIT(0)

#define GQSPI_SEL_GQSPI_MASK BIT(0)

#define GQSPI_GEN_FIFO_POLL_MASK       BIT(19)
#define GQSPI_GEN_FIFO_STRIPE_MASK     BIT(18)
#define GQSPI_GEN_FIFO_RX_EN_MASK      BIT(17)
#define GQSPI_GEN_FIFO_TX_EN_MASK      BIT(16)
#define GQSPI_GEN_FIFO_BUS_UPPER_MASK  BIT(15)
#define GQSPI_GEN_FIFO_BUS_LOWER_MASK  BIT(14)
#define GQSPI_GEN_FIFO_CS_UPPER_MASK   BIT(13)
#define GQSPI_GEN_FIFO_CS_LOWER_MASK   BIT(12)
#define GQSPI_GEN_FIFO_SPI_MODE_MASK   BIT_MASK(2)
#define GQSPI_GEN_FIFO_SPI_MODE_SHIFT  10
#define GQSPI_GEN_FIFO_SPI_MODE_SINGLE 1
#define GQSPI_GEN_FIFO_SPI_MODE_DUAL   2
#define GQSPI_GEN_FIFO_SPI_MODE_QUAD   3
#define GQSPI_GEN_FIFO_EXPONENT_MASK   BIT(9)
#define GQSPI_GEN_FIFO_DATA_XFER_MASK  BIT(8)
#define GQSPI_GEN_FIFO_IMMED_DATA_MASK BIT_MASK(8)

#define GQSPI_CS_SETUP_CYCLES 10
#define GQSPI_CS_HOLD_CYCLES  10
#define GQSPI_MAX_EXPONENT    28

#define GQSPI_GEN_FIFO_DEPTH 32

#define GQSPI_FIFO_CTRL_RST_RX_FIFO_MASK  BIT(2)
#define GQSPI_FIFO_CTRL_RST_TX_FIFO_MASK  BIT(1)
#define GQSPI_FIFO_CTRL_RST_GEN_FIFO_MASK BIT(0)

/* Bits used in interrupt-related registers */
#define GQSPI_INT_RX_FIFO_EMPTY     BIT(11)
#define GQSPI_INT_GEN_FIFO_FULL     BIT(10)
#define GQSPI_INT_GEN_FIFO_NOT_FULL BIT(9)
#define GQSPI_INT_TX_FIFO_EMPTY     BIT(8)
#define GQSPI_INT_GEN_FIFO_EMPTY    BIT(7)
#define GQSPI_INT_RX_FIFO_FULL      BIT(5)
#define GQSPI_INT_RX_FIFO_NOT_EMPTY BIT(4)
#define GQSPI_INT_TX_FIFO_FULL      BIT(3)
#define GQSPI_INT_TX_FIFO_NOT_FULL  BIT(2)
#define GQSPI_INT_POLL_TIME_EXPIRE  BIT(1)
#define GQSPI_INT_ALL_MASK                                                                         \
	(GQSPI_INT_RX_FIFO_EMPTY | GQSPI_INT_GEN_FIFO_FULL | GQSPI_INT_GEN_FIFO_NOT_FULL |         \
	 GQSPI_INT_TX_FIFO_EMPTY | GQSPI_INT_GEN_FIFO_EMPTY | GQSPI_INT_RX_FIFO_FULL |             \
	 GQSPI_INT_RX_FIFO_NOT_EMPTY | GQSPI_INT_TX_FIFO_FULL | GQSPI_INT_TX_FIFO_NOT_FULL |       \
	 GQSPI_INT_POLL_TIME_EXPIRE)

#define GQSPIDMA_INT_FIFO_OVERFLOW BIT(7)
#define GQSPIDMA_INT_INVALID_APB   BIT(6)
#define GQSPIDMA_INT_THRESH_HIT    BIT(5)
#define GQSPIDMA_INT_TIMEOUT_MEM   BIT(4)
#define GQSPIDMA_INT_TIMEOUT_STRM  BIT(3)
#define GQSPIDMA_INT_AXI_BRESP_ERR BIT(2)
#define GQSPIDMA_INT_DONE          BIT(1)
#define GQSPIDMA_INT_ALL_MASK                                                                      \
	(GQSPIDMA_INT_FIFO_OVERFLOW | GQSPIDMA_INT_INVALID_APB | GQSPIDMA_INT_THRESH_HIT |         \
	 GQSPIDMA_INT_TIMEOUT_MEM | GQSPIDMA_INT_TIMEOUT_STRM | GQSPIDMA_INT_AXI_BRESP_ERR |       \
	 GQSPIDMA_INT_DONE)

/* TODO: Tap delay non-bypass mode not yet supported */
#define GQSPI_MAX_FREQ_LOOPBACK_DISABLE     40000000
#define GQSPI_LPBK_DLY_ADJ_LOOPBACK_DISABLE 0x0
#define GQSPI_DATA_DLY_ADJ_LOOPBACK_DISABLE 0x0
#define GQSPI_MAX_FREQ_LOOPBACK_ENABLE      100000000
#define GQSPI_LPBK_DLY_ADJ_LOOPBACK_ENABLE  0x20
#define GQSPI_DATA_DLY_ADJ_LOOPBACK_ENABLE  0xA0000000

struct xlnx_zynqmp_gqspi_config {
	mm_reg_t base;
	void (*irq_config_func)(const struct device *dev);
	uint32_t ref_clock_freq;
	bool shared_data_bus;
};

struct xlnx_zynqmp_gqspi_data {
	struct spi_context ctx;
	uint32_t spi_cfg;
	struct k_event event;
};

static inline uint32_t xlnx_zynqmp_gqspi_read32(const struct device *dev,
						enum xlnx_zynqmp_gqspi_registers reg)
{
	const struct xlnx_zynqmp_gqspi_config *config = dev->config;

	return sys_read32(config->base + (mm_reg_t)reg);
}

static inline void xlnx_zynqmp_gqspi_write32(const struct device *dev,
					     enum xlnx_zynqmp_gqspi_registers reg, uint32_t value)
{
	const struct xlnx_zynqmp_gqspi_config *config = dev->config;

	sys_write32(value, config->base + (mm_reg_t)reg);
}

static void xlnx_zynqmp_gqspi_cs_control(const struct device *dev, bool on)
{
	const struct xlnx_zynqmp_gqspi_config *config = dev->config;
	struct xlnx_zynqmp_gqspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t genfifo_entry = GQSPI_GEN_FIFO_SPI_MODE_SINGLE << GQSPI_GEN_FIFO_SPI_MODE_SHIFT;

	if (ctx->config->slave == 1 && !config->shared_data_bus) {
		genfifo_entry |= GQSPI_GEN_FIFO_BUS_UPPER_MASK;
	} else {
		genfifo_entry |= GQSPI_GEN_FIFO_BUS_LOWER_MASK;
	}
	if (on) {
		if (ctx->config->slave == 1) {
			genfifo_entry |= GQSPI_GEN_FIFO_CS_UPPER_MASK;
		} else {
			genfifo_entry |= GQSPI_GEN_FIFO_CS_LOWER_MASK;
		}
		genfifo_entry |= GQSPI_CS_SETUP_CYCLES;
	} else {
		if (ctx->config->operation & SPI_HOLD_ON_CS) {
			/* Skip slave select de-assert */
			return;
		}
		genfifo_entry |= GQSPI_CS_HOLD_CYCLES;
	}

	LOG_DBG("CS %s, genfifo_entry: 0x%08x", on ? "assert" : "deassert", genfifo_entry);
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_GEN_FIFO, genfifo_entry);
}

static int xlnx_zynqmp_gqspi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct xlnx_zynqmp_gqspi_config *config = dev->config;
	struct xlnx_zynqmp_gqspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	uint32_t max_frequency = spi_cfg->frequency;
	uint32_t baud_div = 0;
	uint32_t actual_frequency = config->ref_clock_freq / (2 << baud_div);

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Configuration already active */
		return 0;
	}

	if (spi_cfg->operation & (SPI_FRAME_FORMAT_TI | SPI_HALF_DUPLEX | SPI_OP_MODE_SLAVE |
				  SPI_MODE_LOOP | SPI_TRANSFER_LSB | SPI_CS_ACTIVE_HIGH)) {
		LOG_ERR("Unsupported SPI operation mode 0x%x", spi_cfg->operation);
		return -ENOTSUP;
	}

	if ((spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line SPI supported");
		return -ENOTSUP;
	}

	if (spi_cfg->slave >= 2) {
		LOG_ERR("unsupported slave %d", spi_cfg->slave);
		return -ENOTSUP;
	}

	if (word_size != 8) {
		LOG_ERR("unsupported word size %d bits", word_size);
		return -ENOTSUP;
	}

	if (max_frequency > GQSPI_MAX_FREQ_LOOPBACK_ENABLE) {
		max_frequency = GQSPI_MAX_FREQ_LOOPBACK_ENABLE;
	}

	while (actual_frequency > max_frequency && baud_div < GQSPI_CFG_BAUD_RATE_DIV_MASK) {
		baud_div++;
		actual_frequency = config->ref_clock_freq / (2 << baud_div);
	}
	if (actual_frequency > max_frequency) {
		LOG_ERR("unsupported frequency %d", spi_cfg->frequency);
		return -ENOTSUP;
	}
	data->spi_cfg &= ~GQSPI_CFG_BAUD_RATE_DIV_MASK;
	data->spi_cfg |= (baud_div << GQSPI_CFG_BAUD_RATE_DIV_SHIFT);

	if (spi_cfg->operation & SPI_MODE_CPHA) {
		data->spi_cfg |= GQSPI_CFG_CLK_PH_MASK;
	} else {
		data->spi_cfg &= ~GQSPI_CFG_CLK_PH_MASK;
	}

	if (spi_cfg->operation & SPI_MODE_CPOL) {
		data->spi_cfg |= GQSPI_CFG_CLK_POL_MASK;
	} else {
		data->spi_cfg &= ~GQSPI_CFG_CLK_POL_MASK;
	}

	/* Disable controller */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_EN, 0);
	LOG_DBG("GQSPI_CFG: 0x%08x", data->spi_cfg);
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_CFG, data->spi_cfg);

	if (actual_frequency > GQSPI_MAX_FREQ_LOOPBACK_DISABLE) {
		/* Enable loopback */
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_LPBK_DLY_ADJ,
					  GQSPI_LPBK_DLY_ADJ_LOOPBACK_ENABLE);
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_DATA_DLY_ADJ,
					  GQSPI_DATA_DLY_ADJ_LOOPBACK_ENABLE);
	} else {
		/* Disable loopback */
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_LPBK_DLY_ADJ,
					  GQSPI_LPBK_DLY_ADJ_LOOPBACK_DISABLE);
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_DATA_DLY_ADJ,
					  GQSPI_DATA_DLY_ADJ_LOOPBACK_DISABLE);
	}

	xlnx_zynqmp_gqspi_write32(dev, GQSPI_EN, GQSPI_EN_ENABLE_MASK);
	ctx->config = spi_cfg;

	return 0;
}

static bool xlnx_zynqmp_gqspi_service_fifos(const struct device *dev)
{
	struct xlnx_zynqmp_gqspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t isr = xlnx_zynqmp_gqspi_read32(dev, GQSPI_ISR);

	LOG_DBG("Service FIFOs, ISR: 0x%08x", isr);
	/* Note: Each buffer is sent as a separate SPI command,
	 * so do not mix buffers within the same FIFO word.
	 */
	while (spi_context_tx_on(ctx)) {
		uint32_t tx_bytes;
		uint32_t fifo_data = 0;

		if (!spi_context_tx_buf_on(ctx)) {
			/* consume dummy TX buffer */
			spi_context_update_tx(ctx, 1, ctx->tx_len);
			continue;
		}
		if (isr & GQSPI_INT_TX_FIFO_FULL) {
			break;
		}

		tx_bytes = min(4, ctx->tx_len);
		for (size_t i = 0; i < tx_bytes; i++) {
			fifo_data |= ctx->tx_buf[i] << (i * 8);
		}
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_TXD, fifo_data);
		LOG_DBG("TX FIFO data: 0x%08x", fifo_data);
		isr = xlnx_zynqmp_gqspi_read32(dev, GQSPI_ISR);
		spi_context_update_tx(ctx, 1, tx_bytes);
	}

	while (spi_context_rx_on(ctx)) {
		uint32_t rx_bytes;
		uint32_t fifo_data;

		if (!spi_context_rx_buf_on(ctx)) {
			/* consume dummy RX buffer */
			spi_context_update_rx(ctx, 1, ctx->rx_len);
			continue;
		}
		if (isr & GQSPI_INT_RX_FIFO_EMPTY) {
			break;
		}
		rx_bytes = min(4, ctx->rx_len);
		fifo_data = xlnx_zynqmp_gqspi_read32(dev, GQSPI_RXD);

		LOG_DBG("RX FIFO data: 0x%08x", fifo_data);
		for (size_t i = 0; i < rx_bytes; i++) {
			ctx->rx_buf[i] = (fifo_data >> (i * 8)) & 0xFF;
		}
		isr = xlnx_zynqmp_gqspi_read32(dev, GQSPI_ISR);
		spi_context_update_rx(ctx, 1, rx_bytes);
	}
	LOG_DBG("Service FIFOs done, ISR: 0x%08x", isr);

	if (!spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx) &&
	    (isr & GQSPI_INT_GEN_FIFO_EMPTY)) {
		LOG_DBG("Transfer complete");
		spi_context_complete(ctx, dev, 0);
		return true;
	}
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_CFG, data->spi_cfg | GQSPI_CFG_START_GEN_FIFO_MASK);

	return false;
}

static int xlnx_zynqmp_gqspi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, bool async,
					spi_callback_t cb, void *userdata)
{
	const struct xlnx_zynqmp_gqspi_config *config = dev->config;
	struct xlnx_zynqmp_gqspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;
	const size_t num_bufs = max(tx_bufs ? tx_bufs->count : 0, rx_bufs ? rx_bufs->count : 0);

	/* Maximum 30 buffers to avoid filling generic (command) FIFO. */
	if (num_bufs > GQSPI_GEN_FIFO_DEPTH - 2) {
		LOG_ERR("Too many buffers: %zu", num_bufs);
		return -ENOTSUP;
	}

	spi_context_lock(ctx, async, cb, userdata, spi_cfg);

	/* Clear FIFOs */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_FIFO_CTRL,
				  GQSPI_FIFO_CTRL_RST_RX_FIFO_MASK |
					  GQSPI_FIFO_CTRL_RST_TX_FIFO_MASK |
					  GQSPI_FIFO_CTRL_RST_GEN_FIFO_MASK);

	ret = xlnx_zynqmp_gqspi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	xlnx_zynqmp_gqspi_cs_control(dev, true);

	for (size_t buf = 0; buf < num_bufs; buf++) {
		uint32_t genfifo_entry = GQSPI_GEN_FIFO_DATA_XFER_MASK;
		size_t tx_bytes = 0;
		size_t rx_bytes = 0;
		size_t transfer_bytes;

		if (tx_bufs && buf < tx_bufs->count) {
			tx_bytes = tx_bufs->buffers[buf].len;
			if (tx_bytes && tx_bufs->buffers[buf].buf) {
				genfifo_entry |= GQSPI_GEN_FIFO_TX_EN_MASK;
			}
		}
		if (rx_bufs && buf < rx_bufs->count) {
			rx_bytes = rx_bufs->buffers[buf].len;
			if (rx_bytes && rx_bufs->buffers[buf].buf) {
				genfifo_entry |= GQSPI_GEN_FIFO_RX_EN_MASK;
			}
		}
		transfer_bytes = max(tx_bytes, rx_bytes);

		if (ctx->config->slave == 1) {
			genfifo_entry |= GQSPI_GEN_FIFO_CS_UPPER_MASK;
			genfifo_entry |= config->shared_data_bus ? GQSPI_GEN_FIFO_BUS_LOWER_MASK
								 : GQSPI_GEN_FIFO_BUS_UPPER_MASK;
		} else {
			genfifo_entry |=
				GQSPI_GEN_FIFO_CS_LOWER_MASK | GQSPI_GEN_FIFO_BUS_LOWER_MASK;
		}
		genfifo_entry |= GQSPI_GEN_FIFO_SPI_MODE_SINGLE << GQSPI_GEN_FIFO_SPI_MODE_SHIFT;

		if (transfer_bytes < 256) {
			genfifo_entry |= transfer_bytes;
		} else {
			uint32_t exponent = GQSPI_MAX_EXPONENT;
			size_t exponent_len = 1 << exponent;

			while (exponent_len > transfer_bytes) {
				exponent_len >>= 1;
				exponent--;
			}
			if (exponent_len != transfer_bytes) {
				/* Could do this by breaking buffer into multiple transfer requests,
				 * but not implemented yet
				 */
				LOG_ERR("Unsupported buffer size %zu", transfer_bytes);
				ret = -ENOTSUP;
				goto out;
			}
			genfifo_entry |= GQSPI_GEN_FIFO_EXPONENT_MASK | exponent;
		}
		LOG_DBG("Buffer %zu, TX bytes: %zu, RX bytes: %zu, transfer bytes: %zu, "
			"genfifo_entry: 0x%08x",
			buf, tx_bytes, rx_bytes, transfer_bytes, genfifo_entry);
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_GEN_FIFO, genfifo_entry);
	}

	xlnx_zynqmp_gqspi_cs_control(dev, false);

	while (true) {
		bool complete = xlnx_zynqmp_gqspi_service_fifos(dev);
		uint32_t wait_events = 0;

		if (complete || async) {
			break;
		}

		if (spi_context_rx_buf_on(ctx)) {
			wait_events |= GQSPI_INT_RX_FIFO_NOT_EMPTY;
		}
		if (spi_context_tx_buf_on(ctx)) {
			wait_events |= GQSPI_INT_TX_FIFO_NOT_FULL;
		}
		if (!spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx)) {
			wait_events |= GQSPI_INT_GEN_FIFO_EMPTY;
		}
		LOG_DBG("Waiting for events: 0x%08x", wait_events);
		k_event_clear(&data->event, wait_events);
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_IER, wait_events);
		if (xlnx_zynqmp_gqspi_read32(dev, GQSPI_ISR) & wait_events) {
			/* Already have the event */
			continue;
		}
		/**
		 * 20ms should be long enough for 256 byte FIFO at any
		 * reasonable clock speed.
		 */
		if (!k_event_wait(&data->event, wait_events, false,
				  K_MSEC(20 + CONFIG_SPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("Timeout, wait_events 0x%08x, ISR: 0x%08x", wait_events,
				xlnx_zynqmp_gqspi_read32(dev, GQSPI_ISR));
			spi_context_complete(ctx, dev, -ETIMEDOUT);
			break;
		}
	}

	ret = spi_context_wait_for_completion(ctx);
out:
	spi_context_release(ctx, ret);

	return ret;
}

static int xlnx_zynqmp_gqspi_transceive_blocking(const struct device *dev,
						 const struct spi_config *spi_cfg,
						 const struct spi_buf_set *tx_bufs,
						 const struct spi_buf_set *rx_bufs)
{
	return xlnx_zynqmp_gqspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int xlnx_zynqmp_gqspi_transceive_async(const struct device *dev,
					      const struct spi_config *spi_cfg,
					      const struct spi_buf_set *tx_bufs,
					      const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					      void *userdata)
{
	return xlnx_zynqmp_gqspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int xlnx_zynqmp_gqspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct xlnx_zynqmp_gqspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void xlnx_zynqmp_gqspi_isr(const struct device *dev)
{
	struct xlnx_zynqmp_gqspi_data *data = dev->data;
	const uint32_t isr = xlnx_zynqmp_gqspi_read32(dev, GQSPI_ISR);
	const uint32_t masked = isr & ~xlnx_zynqmp_gqspi_read32(dev, GQSPI_IMASK);

	if (masked) {
		LOG_DBG("ISR: 0x%08x, masked: 0x%08x", isr, masked);
		/* Disable any interrupts that were just posted */
		xlnx_zynqmp_gqspi_write32(dev, GQSPI_IDR, masked);

		/**
		 * For async mode, we need to read the RX FIFO and refill the TX FIFO
		 * if needed here.
		 * For sync mode, we do this in the caller's context to avoid doing too much
		 * work in the ISR, so just post the event.
		 */
#ifdef CONFIG_SPI_ASYNC
		struct spi_context *ctx = &data->ctx;

		if (ctx->asynchronous) {
			xlnx_zynqmp_gqspi_service_fifos(dev);
			return;
		}
#endif
		k_event_post(&data->event, masked);
	} else {
		LOG_WRN("unhandled interrupt, isr = 0x%08x", isr);
	}
}

static int xlnx_zynqmp_gqspi_init(const struct device *dev)
{
	const struct xlnx_zynqmp_gqspi_config *config = dev->config;
	struct xlnx_zynqmp_gqspi_data *data = dev->data;

	k_event_init(&data->event);

	/* Ensure that GQSPI (vs. LQSPI) mode is active */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_SEL, GQSPI_SEL_GQSPI_MASK);

	/* Ensure that poll timer interrupt is cleared */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_ISR, GQSPI_INT_POLL_TIME_EXPIRE);

	/* Disable all interrupts*/
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_IDR, GQSPI_INT_ALL_MASK);
	xlnx_zynqmp_gqspi_write32(dev, GQSPIDMA_DST_I_DIS, GQSPIDMA_INT_ALL_MASK);

	/* Disable controller */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_EN, 0);

	xlnx_zynqmp_gqspi_write32(dev, GQSPI_FIFO_CTRL,
				  GQSPI_FIFO_CTRL_RST_RX_FIFO_MASK |
					  GQSPI_FIFO_CTRL_RST_TX_FIFO_MASK |
					  GQSPI_FIFO_CTRL_RST_GEN_FIFO_MASK);

	/* Set TX not full and RX not empty thresholds */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_TX_THRESH, 32);
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_RX_THRESH, 1);

	data->spi_cfg = (GQSPI_CFG_MODE_IO << GQSPI_CFG_MODE_EN_SHIFT) |
			GQSPI_CFG_GEN_FIFO_START_MANUAL_MASK | GQSPI_CFG_WP_HOLD_MASK;

	LOG_DBG("GQSPI_CFG: 0x%08x", data->spi_cfg);
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_CFG, data->spi_cfg);

	/* Enable loopback */
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_LPBK_DLY_ADJ, GQSPI_LPBK_DLY_ADJ_LOOPBACK_ENABLE);
	xlnx_zynqmp_gqspi_write32(dev, GQSPI_DATA_DLY_ADJ, GQSPI_DATA_DLY_ADJ_LOOPBACK_ENABLE);

	xlnx_zynqmp_gqspi_write32(dev, GQSPI_EN, GQSPI_EN_ENABLE_MASK);

	config->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, xlnx_zynqmp_gqspi_driver_api) = {
	.transceive = xlnx_zynqmp_gqspi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = xlnx_zynqmp_gqspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = xlnx_zynqmp_gqspi_release,
};

#define XLNX_ZYNQMP_GQSPI_INIT(n)                                                                  \
	static void xlnx_zynqmp_gqspi_config_func_##n(const struct device *dev);                   \
                                                                                                   \
	static const struct xlnx_zynqmp_gqspi_config xlnx_zynqmp_gqspi_config_##n = {              \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = xlnx_zynqmp_gqspi_config_func_##n,                              \
		.ref_clock_freq = DT_INST_PROP(n, clock_frequency),                                \
		.shared_data_bus = DT_INST_NODE_HAS_PROP(n, shared_data_bus)};                     \
                                                                                                   \
	static struct xlnx_zynqmp_gqspi_data xlnx_zynqmp_gqspi_data_##n = {                        \
		SPI_CONTEXT_INIT_LOCK(xlnx_zynqmp_gqspi_data_##n, ctx),                            \
		SPI_CONTEXT_INIT_SYNC(xlnx_zynqmp_gqspi_data_##n, ctx)};                           \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, &xlnx_zynqmp_gqspi_init, NULL, &xlnx_zynqmp_gqspi_data_##n,   \
				  &xlnx_zynqmp_gqspi_config_##n, POST_KERNEL,                      \
				  CONFIG_SPI_INIT_PRIORITY, &xlnx_zynqmp_gqspi_driver_api);        \
                                                                                                   \
	static void xlnx_zynqmp_gqspi_config_func_##n(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), xlnx_zynqmp_gqspi_isr,      \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(XLNX_ZYNQMP_GQSPI_INIT)
