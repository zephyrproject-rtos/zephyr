/*
 * Copyright (c) 2026 Simon Maurer <mail@maurer.systems>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_zynq_qspi

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(zynq_qspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define MAX_BAUD_RATE_DIV 256

enum xlnx_zynq_qspi_registers {
	REG_CR = 0x00,    /* Configuration Register */
	REG_SR = 0x04,    /* Interrupt Status Register */
	REG_IER = 0x08,   /* Interrupt Enable Register */
	REG_IDR = 0x0C,   /* Interrupt Disable Register */
	REG_IMR = 0x10,   /* Interrupt Mask Register */
	REG_ER = 0x14,    /* SPI Enable Register */
	REG_DR = 0x18,    /* Delay Register */
	REG_TXD4 = 0x1C,  /* Transmit Data Register. Keyhole addresses for the Transmit data FIFO */
	REG_RXD = 0x20,   /* Receive Data Register */
	REG_SICR = 0x24,  /* Slave Idle Count Register */
	REG_TXTHR = 0x28, /* TX_FIFO Threshold Register */
	REG_RXTHR = 0x2C, /* RX_FIFO Threshold Register */
	REG_GPIO = 0x30,  /* General Purpose Inputs and Outputs Register */
	REG_LPBK = 0x38,  /* Loopback Master Clock DelayAdjustment Register */
	REG_TXD1 = 0x80,  /* Transmit Data Register. Keyhole addresses for the Transmit data FIFO */
	REG_TXD2 = 0x84,  /* Transmit Data Register. Keyhole addresses for the Transmit data FIFO */
	REG_TXD3 = 0x88,  /* Transmit Data Register. Keyhole addresses for the Transmit data FIFO */
	REG_LQSPI_CR = 0xA0, /* Configuration Register for the Linear Quad-SPI Controller */
	REG_LQSPI_SR = 0xA4, /* Status Register for the Linear Quad-SPI Controller */
};

/* Mode select
 * 1: The QSPI is in master mode
 * 0: RESERVED
 */
#define CR_MSTREN BIT(0)

/* Clock polarity outside QSPI word
 * 1: The QSPI clock is quiescent high
 * 0: The QSPI clock is quiescent low
 */
#define CR_CPOL BIT(1)

/* Clock phase
 * 1: the QSPI clock is inactive outside the word
 * 0: the QSPI clock is active outside the word
 */
#define CR_CPHA BIT(2)

/* Master mode baud rate divisor
 * 000: divide by 2. This is the only baud rate setting that can be used if the loopback clock is
 * enabled (USE_LPBK). This setting also works in non-loopback mode. 001: divide by 4 010: divide by
 * 8 011: divide by 16 100: divide by 32 101: divide by 64 110: divide by 128 111: divide by 256
 */
#define CR_BAUD_RATE_OFST     3
#define CR_BAUD_RATE_DIV_4    BIT(3)
#define CR_BAUD_RATE_DIV_8    BIT(4)
#define CR_BAUD_RATE_DIV_16   (BIT(3) | BIT(4))
#define CR_BAUD_RATE_DIV_32   BIT(5)
#define CR_BAUD_RATE_DIV_64   (BIT(3) | BIT(5))
#define CR_BAUD_RATE_DIV_128  (BIT(4) | BIT(5))
#define CR_BAUD_RATE_DIV_256  (BIT(3) | BIT(4) | BIT(5))
#define CR_BAUD_RATE_DIV_MASK CR_BAUD_RATE_DIV_256

/* FIFO width
 * Must be set to 2'b11 (32bits). All other settings are not supported.
 */
#define CR_FIFO_WIDTH_32 (BIT(6) | BIT(7))

/* Peripheral chip select line, directly drive n_ss_out if Manual_C is set */
#define CR_PCS BIT(10)

/* Manual CS
 * 1: manual CS mode
 * 0: auto mode
 */
#define CR_SSFORCE BIT(14)

/* Manual Start Enable
 * 1: enables manual start
 * 0: auto mode
 */
#define CR_MANSTRTEN BIT(15)

/* Manual Start Command
 * 1: start transmission of data
 * 0: don't care
 */
#define CR_MANSTRT BIT(16)

/* If set, Holdb and WPn pins are actively driven by the qspi controller
 * in 1-bit and 2-bit modes
 */
#define CR_PINS_PULLUP BIT(19)

/* 0 for little endian format */
#define CR_ENDIAN BIT(26)

/* Flash memory interface mode control:
 * 0: legacy SPI mode
 * 1: Flash memory interface mode
 */
#define CR_IFMODE BIT(31)

#define CR_INIT_MASK                                                                               \
	(CR_MSTREN | CR_CPOL | CR_CPHA | CR_FIFO_WIDTH_32 | CR_BAUD_RATE_DIV_256 | CR_SSFORCE |    \
	 CR_MANSTRTEN | CR_MANSTRT | CR_IFMODE)
#define CR_INIT (CR_MSTREN | CR_FIFO_WIDTH_32 | CR_BAUD_RATE_DIV_16 | CR_SSFORCE | CR_IFMODE)

/* Receive Overflow interrupt, write one to this bit location to clear.
 * 1: overflow occurred
 * 0: no overflow occurred
 * Write 1 to this bit location to clear
 */
#define IXR_RXOVERFLOW BIT(0)

/* TX FIFO not full (current FIFO status)
 * 1: FIFO has less than THRESHOLD entries
 * 0: FIFO has more than or equal toTHRESHOLD entries
 */
#define IXR_TXNFULL BIT(2)

/* TX FIFO full (current FIFO status)
 * 1: FIFO is full
 * 0: FIFO is not full
 */
#define IXR_TXFULL BIT(3)

/* RX FIFO not empty (current FIFO status)
 * 1: FIFO has more than or equal to THRESHOLD entries
 * 0: FIFO has less than RX THRESHOLD entries
 */
#define IXR_RXNEMPTY BIT(4)

/* RX FIFO full (current FIFO status)
 * 1: FIFO is full
 * 0: FIFO is not full
 */
#define IXR_RXFULL BIT(5)

/* TX FIFO underflow, write one to this bit location to clear.
 * 1: underflow is detected
 * 0: no underflow has been detected
 * Write 1 to this bit location to clear
 */
#define IXR_TXUNDERFLOW BIT(6)

#define IXR_ALL                                                                                    \
	(IXR_RXOVERFLOW | IXR_TXNFULL | IXR_TXFULL | IXR_RXNEMPTY | IXR_RXFULL | IXR_TXUNDERFLOW)

/* SPI_Enable
 * 1: enable the SPI
 * 0: disable the SPI
 */
#define ER_ENABLE BIT(0)

#define FIFO_DEPTH   63
#define RX_THRESHOLD 32
#define TX_THRESHOLD 1

struct zynq_qspi_config {
	DEVICE_MMIO_ROM;
	uint32_t clk_freq;
	void (*irq_config_func)(const struct device *dev);
};

struct zynq_qspi_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	struct {
		/* Number of 32-bit words remaining to be written to the TX FIFO. */
		unsigned int pending;

		/* Number of 32-bit words currently residing in the hardware FIFO. */
		unsigned int queued;

		/**
		 * Remainder bytes (0-3) if the total transfer size is not 4-byte aligned.
		 * These must be written using the controller's manual/special registers.
		 */
		unsigned int remainder;

		unsigned int rx_overflow;
		unsigned int tx_underflow;

	} xfer;
};

static inline uint32_t zynq_qspi_read32(const struct device *dev, enum xlnx_zynq_qspi_registers reg)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	return sys_read32(base + (mm_reg_t)reg);
}

static inline void zynq_qspi_write32(const struct device *dev, enum xlnx_zynq_qspi_registers reg,
				     uint32_t value)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	sys_write32(value, base + (mm_reg_t)reg);
}

static int zynq_qspi_calc_freq_div(const struct device *dev, uint32_t frequency)
{
	const struct zynq_qspi_config *config = dev->config;
	uint32_t divider = 0;

	while ((divider <= 7) && (config->clk_freq / (1 << (divider + 1))) > frequency) {
		divider++;
	}

	if (config->clk_freq / (1 << (divider + 1)) > frequency) {
		LOG_ERR("requested baud rate %u can't be reached with ref freq %u", frequency,
			config->clk_freq);
		return -ENOTSUP;
	}

	LOG_DBG("frequency requested=%u input frequency=%u CR_BAUD_RATE_DIV=0x%x actual "
		"frequency=%u",
		frequency, config->clk_freq, divider, config->clk_freq / (1 << (divider + 1)));

	return divider;
}

static void zynq_qspi_cs_control(const struct device *dev, bool on)
{
	uint32_t cr = zynq_qspi_read32(dev, REG_CR);

	if (on) {
		cr &= ~CR_PCS;
	} else {
		cr |= CR_PCS;
	}

	zynq_qspi_write32(dev, REG_CR, cr);
}

static inline void zynq_qspi_read_rxd(const struct device *dev)
{
	struct zynq_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	uint32_t rxd = zynq_qspi_read32(dev, REG_RXD);

	for (unsigned int i = 0; i < sizeof(uint32_t); i++) {
		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = rxd & 0xff;
		}

		spi_context_update_rx(ctx, 1, 1);
		rxd >>= 8;
	}
}

static inline void zynq_qspi_write_txd(const struct device *dev, unsigned int size)
{
	static const enum xlnx_zynq_qspi_registers reg_lut[] = {
		REG_TXD1,
		REG_TXD2,
		REG_TXD3,
		REG_TXD4,
	};

	struct zynq_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	__ASSERT((size > 0) && (size <= 4), "invalid value for TXD size");

	unsigned int txd = 0;

	for (unsigned int i = 0; i < size; i++) {
		if (spi_context_tx_buf_on(ctx)) {
			txd |= ((uint32_t)*ctx->tx_buf) << (i * 8);
		}

		spi_context_update_tx(ctx, 1, 1);
	}

	unsigned int reg_idx = size - 1;

	mm_reg_t reg = reg_lut[reg_idx & 0x3];

	zynq_qspi_write32(dev, reg, txd);
}

static int zynq_qspi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct zynq_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t freq_div;
	uint32_t cr;
	int ret;

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Configuration already active, just enable SPI IOs */
		return 0;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("slave mode support not enabled");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("unsupported CS polarity active high");
		return -ENOTSUP;
	}

	ret = zynq_qspi_calc_freq_div(dev, spi_cfg->frequency);

	if (ret < 0) {
		return ret;
	}

	freq_div = (uint32_t)ret << CR_BAUD_RATE_OFST;

	cr = zynq_qspi_read32(dev, REG_CR);

	cr &= ~(CR_CPOL | CR_CPHA | CR_BAUD_RATE_DIV_256);

	cr |= freq_div & CR_BAUD_RATE_DIV_MASK;

	if (spi_cfg->operation & SPI_MODE_CPHA) {
		cr |= CR_CPHA;
	}

	if (spi_cfg->operation & SPI_MODE_CPOL) {
		cr |= CR_CPOL;
	}

	zynq_qspi_write32(dev, REG_CR, cr);

	ctx->config = spi_cfg;

	return 0;
}

static void zynq_qspi_flush_rx_fifo(const struct device *dev)
{
	for (int i = 0; i < FIFO_DEPTH; i++) {
		zynq_qspi_read32(dev, REG_RXD);
	}
}

static int zynq_qspi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs, bool async, spi_callback_t cb,
				void *userdata)
{
	struct zynq_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, async, cb, userdata, spi_cfg);

	ret = zynq_qspi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	zynq_qspi_cs_control(dev, true);

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	data->xfer.tx_underflow = 0;
	data->xfer.rx_overflow = 0;

	size_t tx_len = spi_context_total_tx_len(ctx);
	size_t rx_len = spi_context_total_rx_len(ctx);

	size_t xfer_len = MAX(tx_len, rx_len);

	unsigned int pending = xfer_len / 4;
	unsigned int remainder = xfer_len - 4 * pending;

	unsigned int fifo_cnt = MIN(FIFO_DEPTH, pending);

	LOG_DBG("tx_len=%u rx_len=%u pending=%u remainder=%u", tx_len, rx_len, pending, remainder);

	if (fifo_cnt == 0) {
		data->xfer.pending = 0;
		data->xfer.remainder = 0;
		data->xfer.queued = 1;

		zynq_qspi_write_txd(dev, remainder);
	} else {
		data->xfer.pending = pending - fifo_cnt;
		data->xfer.remainder = remainder;
		data->xfer.queued = fifo_cnt;

		while (fifo_cnt--) {
			zynq_qspi_write_txd(dev, 4);
		}
	}

	zynq_qspi_write32(dev, REG_IER, IXR_TXNFULL | IXR_RXNEMPTY);

	ret = spi_context_wait_for_completion(ctx);

	if (data->xfer.rx_overflow) {
		LOG_ERR("Rx Overflow occurred");
	}

	if (data->xfer.tx_underflow) {
		LOG_ERR("Tx Underflow occurred");
	}

	zynq_qspi_cs_control(dev, false);

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int zynq_qspi_transceive_blocking(const struct device *dev, const struct spi_config *spi_cfg,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs)
{
	return zynq_qspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int zynq_qspi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				      void *userdata)
{
	return zynq_qspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int zynq_qspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	ARG_UNUSED(spi_cfg);

	struct zynq_qspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void zynq_qspi_isr(const struct device *dev)
{
	struct zynq_qspi_data *data = dev->data;

	uint32_t sr = zynq_qspi_read32(dev, REG_SR);

	/* Clear the asserted interrupts */
	zynq_qspi_write32(dev, REG_SR, sr);

	if (sr & IXR_TXUNDERFLOW) {
		data->xfer.tx_underflow++;
	}

	if (sr & IXR_RXOVERFLOW) {
		data->xfer.rx_overflow++;
	}

	if ((sr & IXR_TXNFULL) || (sr & IXR_RXNEMPTY)) {
		bool empty = !!(sr & IXR_TXNFULL);

		/* Read the available 32-bit words from the RX FIFO up to the threshold */
		unsigned int n = MIN(RX_THRESHOLD, data->xfer.queued);

		data->xfer.queued -= n;

		while (n--) {
			zynq_qspi_read_rxd(dev);
		}

		if (data->xfer.pending > 0) {
			/* Case 1: Keep filling the Tx FIFO with 32-bit words
			 * Fill up to the RX_THRESHOLD. Because the transfer was initiated with
			 * a full TX queue (indicated by remaining pending 32-bit words), we are
			 * guaranteed to have space for at least RX_THRESHOLD 32-bit words.
			 */
			n = MIN(RX_THRESHOLD, data->xfer.pending);

			data->xfer.queued += n;
			data->xfer.pending -= n;

			while (n--) {
				zynq_qspi_write_txd(dev, 4);
			}
		} else {
			if (data->xfer.queued == 0 && empty) {
				if (data->xfer.remainder > 0) {
					/*
					 * Case 2: No full 32-bit words left.
					 * Write the remaining bytes (less than 4 bytes)
					 * once the TX FIFO has completely emptied.
					 */
					zynq_qspi_write_txd(dev, data->xfer.remainder);
					data->xfer.remainder = 0;
					data->xfer.queued = 1;
				} else {
					/*
					 * Case 3: Transfer complete.
					 * Disable interrupts and notify the stack
					 */
					zynq_qspi_write32(dev, REG_IDR, IXR_TXNFULL | IXR_RXNEMPTY);
					spi_context_complete(&data->ctx, dev, 0);
				}
			}
		}
	}
}

static int zynq_qspi_init(const struct device *dev)
{
	struct zynq_qspi_data *data = dev->data;
	const struct zynq_qspi_config *config = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* disable the chip */
	zynq_qspi_write32(dev, REG_ER, 0);

	/* disable all interrupts */
	zynq_qspi_write32(dev, REG_IDR, IXR_ALL);

	/* disable linear quad SPI */
	zynq_qspi_write32(dev, REG_LQSPI_CR, 0);

	/* flush rx fifo */
	zynq_qspi_flush_rx_fifo(dev);

	/* clear all interrupts */
	zynq_qspi_write32(dev, REG_SR, IXR_ALL);

	/* default configuration */
	uint32_t cr = zynq_qspi_read32(dev, REG_CR);

	cr &= ~CR_INIT_MASK;
	cr |= CR_INIT;

	zynq_qspi_write32(dev, REG_CR, cr);

	/* set FIFO tresholds */
	zynq_qspi_write32(dev, REG_RXTHR, RX_THRESHOLD);
	zynq_qspi_write32(dev, REG_TXTHR, TX_THRESHOLD);

	zynq_qspi_write32(dev, REG_ER, ER_ENABLE);

	config->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, zynq_qspi_driver_api) = {
	.transceive = zynq_qspi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = zynq_qspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = zynq_qspi_release,
};

#define XLNX_ZYNQ_QSPI_INIT(n)                                                                     \
	static void zynq_qspi_config_func_##n(const struct device *dev);                           \
                                                                                                   \
	static const struct zynq_qspi_config zynq_qspi_config_##n = {                              \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clk_freq = DT_INST_PROP(n, clock_frequency),                                      \
		.irq_config_func = zynq_qspi_config_func_##n};                                     \
                                                                                                   \
	static struct zynq_qspi_data zynq_qspi_data_##n = {                                        \
		SPI_CONTEXT_INIT_LOCK(zynq_qspi_data_##n, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(zynq_qspi_data_##n, ctx),                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &zynq_qspi_init, NULL, &zynq_qspi_data_##n,                       \
			      &zynq_qspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
			      &zynq_qspi_driver_api);                                              \
                                                                                                   \
	static void zynq_qspi_config_func_##n(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), zynq_qspi_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(XLNX_ZYNQ_QSPI_INIT)
