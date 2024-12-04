/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_spi_2_00_a

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(xlnx_quadspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* AXI Quad SPI v3.2 register offsets (See Xilinx PG153 for details) */
#define SRR_OFFSET             0x40
#define SPICR_OFFSET           0x60
#define SPISR_OFFSET           0x64
#define SPI_DTR_OFFSET         0x68
#define SPI_DRR_OFFSET         0x6c
#define SPISSR_OFFSET          0x70
#define SPI_TX_FIFO_OCR_OFFSET 0x74
#define SPI_RX_FIFO_OCR_OFFSET 0x78
#define DGIER_OFFSET           0x1c
#define IPISR_OFFSET           0x20
#define IPIER_OFFSET           0x28

/* SRR bit definitions */
#define SRR_SOFTRESET_MAGIC 0xa

/* SPICR bit definitions */
#define SPICR_LOOP            BIT(0)
#define SPICR_SPE             BIT(1)
#define SPICR_MASTER          BIT(2)
#define SPICR_CPOL            BIT(3)
#define SPICR_CPHA            BIT(4)
#define SPICR_TX_FIFO_RESET   BIT(5)
#define SPICR_RX_FIFO_RESET   BIT(6)
#define SPICR_MANUAL_SS       BIT(7)
#define SPICR_MASTER_XFER_INH BIT(8)
#define SPICR_LSB_FIRST       BIT(9)

/* SPISR bit definitions */
#define SPISR_RX_EMPTY          BIT(0)
#define SPISR_RX_FULL           BIT(1)
#define SPISR_TX_EMPTY          BIT(2)
#define SPISR_TX_FULL           BIT(3)
#define SPISR_MODF              BIT(4)
#define SPISR_SLAVE_MODE_SELECT BIT(5)
#define SPISR_CPOL_CPHA_ERROR   BIT(6)
#define SPISR_SLAVE_MODE_ERROR  BIT(7)
#define SPISR_MSB_ERROR         BIT(8)
#define SPISR_LOOPBACK_ERROR    BIT(9)
#define SPISR_COMMAND_ERROR     BIT(10)

#define SPISR_ERROR_MASK (SPISR_COMMAND_ERROR |		\
			  SPISR_LOOPBACK_ERROR |	\
			  SPISR_MSB_ERROR |		\
			  SPISR_SLAVE_MODE_ERROR |	\
			  SPISR_CPOL_CPHA_ERROR)

/* DGIER bit definitions */
#define DGIER_GIE BIT(31)

/* IPISR and IPIER bit definitions */
#define IPIXR_MODF               BIT(0)
#define IPIXR_SLAVE_MODF         BIT(1)
#define IPIXR_DTR_EMPTY          BIT(2)
#define IPIXR_DTR_UNDERRUN       BIT(3)
#define IPIXR_DRR_FULL           BIT(4)
#define IPIXR_DRR_OVERRUN        BIT(5)
#define IPIXR_TX_FIFO_HALF_EMPTY BIT(6)
#define IPIXR_SLAVE_MODE_SELECT  BIT(7)
#define IPIXR_DDR_NOT_EMPTY      BIT(8)
#define IPIXR_CPOL_CPHA_ERROR    BIT(9)
#define IPIXR_SLAVE_MODE_ERROR   BIT(10)
#define IPIXR_MSB_ERROR          BIT(11)
#define IPIXR_LOOPBACK_ERROR     BIT(12)
#define IPIXR_COMMAND_ERROR      BIT(13)

struct xlnx_quadspi_config {
	mm_reg_t base;
	void (*irq_config_func)(const struct device *dev);
	uint8_t num_ss_bits;
	uint8_t num_xfer_bytes;
	uint16_t fifo_size;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(xlnx_startup_block)
	bool startup_block;
#endif
};

struct xlnx_quadspi_data {
	struct spi_context ctx;
	struct k_event dtr_empty;
};

static inline uint32_t xlnx_quadspi_read32(const struct device *dev,
					   mm_reg_t offset)
{
	const struct xlnx_quadspi_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void xlnx_quadspi_write32(const struct device *dev,
					uint32_t value,
					mm_reg_t offset)
{
	const struct xlnx_quadspi_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static void xlnx_quadspi_cs_control(const struct device *dev, bool on)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t spissr = BIT_MASK(config->num_ss_bits);

	if (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(ctx)) {
		/* Skip slave select assert/de-assert in slave mode */
		return;
	}

	if (on) {
		/* SPISSR is one-hot, active-low */
		spissr &= ~BIT(ctx->config->slave);
	} else if (ctx->config->operation & SPI_HOLD_ON_CS) {
		/* Skip slave select de-assert */
		return;
	}

	xlnx_quadspi_write32(dev, spissr, SPISSR_OFFSET);
	spi_context_cs_control(ctx, on);
}

static int xlnx_quadspi_configure(const struct device *dev,
				  const struct spi_config *spi_cfg)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t word_size;
	uint32_t spicr;
	uint32_t spisr;

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Configuration already active, just enable SPI IOs */
		spicr = xlnx_quadspi_read32(dev, SPICR_OFFSET);
		spicr |= SPICR_SPE;
		xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->slave >= config->num_ss_bits) {
		LOG_ERR("unsupported slave %d, num_ss_bits %d",
			spi_cfg->slave, config->num_ss_bits);
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("unsupported CS polarity active high");
		return -ENOTSUP;
	}

	if (!IS_ENABLED(CONFIG_SPI_SLAVE) && \
	    (spi_cfg->operation & SPI_OP_MODE_SLAVE)) {
		LOG_ERR("slave mode support not enabled");
		return -ENOTSUP;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size != (config->num_xfer_bytes * 8)) {
		LOG_ERR("unsupported word size %d bits, num_xfer_bytes %d",
			word_size, config->num_xfer_bytes);
		return -ENOTSUP;
	}

	/* Reset FIFOs, SPI IOs enabled */
	spicr = SPICR_TX_FIFO_RESET | SPICR_RX_FIFO_RESET | SPICR_SPE;

	/* Master mode, inhibit master transmit, manual slave select */
	if (!IS_ENABLED(CONFIG_SPI_SLAVE) ||
	    (spi_cfg->operation & SPI_OP_MODE_SLAVE) == 0U) {
		spicr |= SPICR_MASTER | SPICR_MASTER_XFER_INH | SPICR_MANUAL_SS;
	}

	if (spi_cfg->operation & SPI_MODE_CPOL) {
		spicr |= SPICR_CPOL;
	}

	if (spi_cfg->operation & SPI_MODE_CPHA) {
		spicr |= SPICR_CPHA;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		spicr |= SPICR_LOOP;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		spicr |= SPICR_LSB_FIRST;
	}

	/*
	 * Write configuration and verify it is compliant with the IP core
	 * configuration. Tri-state SPI IOs on error.
	 */
	xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
	spisr = xlnx_quadspi_read32(dev, SPISR_OFFSET);
	if (spisr & SPISR_ERROR_MASK) {
		LOG_ERR("unsupported configuration, spisr = 0x%08x", spisr);
		xlnx_quadspi_write32(dev, SPICR_MASTER_XFER_INH, SPICR_OFFSET);
		ctx->config = NULL;
		return -ENOTSUP;
	}

	ctx->config = spi_cfg;

	return 0;
}

static bool xlnx_quadspi_start_tx(const struct device *dev)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t xfer_len;
	uint32_t spicr = 0U;
	uint32_t spisr;
	uint32_t dtr = 0U;
	uint32_t fifo_avail_words = config->fifo_size ? config->fifo_size : 1;
	bool complete = false;

	if (!spi_context_tx_on(ctx) && !spi_context_rx_on(ctx)) {
		/* All done, de-assert slave select */
		xlnx_quadspi_cs_control(dev, false);

		if ((ctx->config->operation & SPI_HOLD_ON_CS) == 0U) {
			/* Tri-state SPI IOs */
			spicr = xlnx_quadspi_read32(dev, SPICR_OFFSET);
			spicr &= ~(SPICR_SPE);
			xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
		}

		spi_context_complete(ctx, dev, 0);
		complete = true;
		return complete;
	}

	if (!IS_ENABLED(CONFIG_SPI_SLAVE) || !spi_context_is_slave(ctx)) {
		/* Inhibit master transaction while writing TX data */
		spicr = xlnx_quadspi_read32(dev, SPICR_OFFSET);
		spicr |= SPICR_MASTER_XFER_INH;
		xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
	}

	/* We can only see as far as the current rx buffer */
	xfer_len = spi_context_longest_current_buf(ctx);

	/* Write TX data */
	while (xfer_len--) {
		if (spi_context_tx_buf_on(ctx)) {
			switch (config->num_xfer_bytes) {
			case 1:
				dtr = UNALIGNED_GET((uint8_t *)(ctx->tx_buf));
				break;
			case 2:
				dtr = UNALIGNED_GET((uint16_t *)(ctx->tx_buf));
				break;
			case 4:
				dtr = UNALIGNED_GET((uint32_t *)(ctx->tx_buf));
				break;
			default:
				__ASSERT(0, "unsupported num_xfer_bytes");
			}
		} else {
			/* No TX buffer. Use dummy TX data */
			dtr = 0U;
		}

		xlnx_quadspi_write32(dev, dtr, SPI_DTR_OFFSET);
		spi_context_update_tx(ctx, config->num_xfer_bytes, 1);

		if (--fifo_avail_words == 0) {
			spisr = xlnx_quadspi_read32(dev, SPISR_OFFSET);
			if (spisr & SPISR_TX_FULL) {
				break;
			}
			if (!config->fifo_size) {
				fifo_avail_words = 1;
			} else if (spisr & SPISR_TX_EMPTY) {
				fifo_avail_words = config->fifo_size;
			} else {
				fifo_avail_words = config->fifo_size -
					 xlnx_quadspi_read32(dev, SPI_TX_FIFO_OCR_OFFSET) - 1;
			}
		}
	}

	spisr = xlnx_quadspi_read32(dev, SPISR_OFFSET);
	if (spisr & SPISR_COMMAND_ERROR) {
		/* Command not supported by memory type configured in IP core */
		LOG_ERR("unsupported command");
		xlnx_quadspi_cs_control(dev, false);

		spicr = xlnx_quadspi_read32(dev, SPICR_OFFSET);
		if ((ctx->config->operation & SPI_HOLD_ON_CS) == 0U) {
			/* Tri-state SPI IOs */
			spicr &= ~(SPICR_SPE);
		}
		xlnx_quadspi_write32(dev, spicr | SPICR_TX_FIFO_RESET,
				     SPICR_OFFSET);

		spi_context_complete(ctx, dev, -ENOTSUP);
		complete = true;
	}

	if (!IS_ENABLED(CONFIG_SPI_SLAVE) || !spi_context_is_slave(ctx)) {
		/* Uninhibit master transaction */
		spicr &= ~(SPICR_MASTER_XFER_INH);
		xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
	}
	return complete;
}

static void xlnx_quadspi_read_fifo(const struct device *dev)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t spisr = xlnx_quadspi_read32(dev, SPISR_OFFSET);
	/* RX FIFO occupancy register only exists if FIFO is implemented */
	uint32_t rx_fifo_words = config->fifo_size ?
		xlnx_quadspi_read32(dev, SPI_RX_FIFO_OCR_OFFSET) + 1 : 1;

	/* Read RX data */
	while (!(spisr & SPISR_RX_EMPTY)) {
		uint32_t drr = xlnx_quadspi_read32(dev, SPI_DRR_OFFSET);

		if (spi_context_rx_buf_on(ctx)) {
			switch (config->num_xfer_bytes) {
			case 1:
				UNALIGNED_PUT(drr, (uint8_t *)ctx->rx_buf);
				break;
			case 2:
				UNALIGNED_PUT(drr, (uint16_t *)ctx->rx_buf);
				break;
			case 4:
				UNALIGNED_PUT(drr, (uint32_t *)ctx->rx_buf);
				break;
			default:
				__ASSERT(0, "unsupported num_xfer_bytes");
			}
		}

		spi_context_update_rx(ctx, config->num_xfer_bytes, 1);

		if (--rx_fifo_words == 0) {
			spisr = xlnx_quadspi_read32(dev, SPISR_OFFSET);
			rx_fifo_words = config->fifo_size ?
				xlnx_quadspi_read32(dev, SPI_RX_FIFO_OCR_OFFSET) + 1 : 1;
		}
	}
}

static int xlnx_quadspi_transceive(const struct device *dev,
				   const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs,
				   bool async,
				   spi_callback_t cb,
				   void *userdata)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, async, cb, userdata, spi_cfg);

	ret = xlnx_quadspi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs,
				  config->num_xfer_bytes);

	xlnx_quadspi_cs_control(dev, true);

	while (true) {
		k_event_clear(&data->dtr_empty, 1);
		bool complete = xlnx_quadspi_start_tx(dev);

		if (complete || async) {
			break;
		}

		/**
		 * 20ms should be long enough for 256 byte FIFO at any
		 * reasonable clock speed.
		 */
		if (!k_event_wait(&data->dtr_empty, 1, false,
				  K_MSEC(20 + CONFIG_SPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			/* Timeout */
			LOG_ERR("DTR empty timeout");
			spi_context_complete(ctx, dev, -ETIMEDOUT);
			break;
		}
		xlnx_quadspi_read_fifo(dev);
	}

	ret = spi_context_wait_for_completion(ctx);
out:
	spi_context_release(ctx, ret);

	return ret;
}

static int xlnx_quadspi_transceive_blocking(const struct device *dev,
					    const struct spi_config *spi_cfg,
					    const struct spi_buf_set *tx_bufs,
					    const struct spi_buf_set *rx_bufs)
{
	return xlnx_quadspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false,
				       NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int xlnx_quadspi_transceive_async(const struct device *dev,
					 const struct spi_config *spi_cfg,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs,
					 spi_callback_t cb,
					 void *userdata)
{
	return xlnx_quadspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true,
				       cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int xlnx_quadspi_release(const struct device *dev,
				const struct spi_config *spi_cfg)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	uint32_t spicr;

	/* Force slave select de-assert */
	xlnx_quadspi_write32(dev, BIT_MASK(config->num_ss_bits), SPISSR_OFFSET);

	/* Tri-state SPI IOs */
	spicr = xlnx_quadspi_read32(dev, SPICR_OFFSET);
	spicr &= ~(SPICR_SPE);
	xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void xlnx_quadspi_isr(const struct device *dev)
{
	struct xlnx_quadspi_data *data = dev->data;
	uint32_t ipisr;

	/* Acknowledge interrupt */
	ipisr = xlnx_quadspi_read32(dev, IPISR_OFFSET);
	xlnx_quadspi_write32(dev, ipisr, IPISR_OFFSET);

	if (ipisr & IPIXR_DTR_EMPTY) {
		/**
		 * For async mode, we need to read the RX FIFO and refill the TX FIFO
		 * if needed here.
		 * For sync mode, we do this in the caller's context to avoid doing too much
		 * work in the ISR, so just post the event.
		 */
#ifdef CONFIG_SPI_ASYNC
		struct spi_context *ctx = &data->ctx;

		if (ctx->asynchronous) {
			xlnx_quadspi_read_fifo(dev);
			xlnx_quadspi_start_tx(dev);
			return;
		}
#endif
		k_event_post(&data->dtr_empty, 1);
	} else {
		LOG_WRN("unhandled interrupt, ipisr = 0x%08x", ipisr);
	}
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(xlnx_startup_block)
static int xlnx_quadspi_startup_block_workaround(const struct device *dev)
{
	const struct xlnx_quadspi_config *config = dev->config;
	uint32_t spissr = BIT_MASK(config->num_ss_bits);
	uint32_t spicr;

	/**
	 * See https://support.xilinx.com/s/article/52626?language=en_US
	 * Up to 3 clock cycles must be issued before the output clock signal
	 * is passed to the output CCLK pin from the SPI core.
	 * Use JEDEC READ ID as dummy command to chip select 0.
	 */
	spissr &= ~BIT(0);
	xlnx_quadspi_write32(dev, spissr, SPISSR_OFFSET);

	xlnx_quadspi_write32(dev, 0x9F, SPI_DTR_OFFSET);
	xlnx_quadspi_write32(dev, 0, SPI_DTR_OFFSET);
	xlnx_quadspi_write32(dev, 0, SPI_DTR_OFFSET);

	spicr = SPICR_MANUAL_SS | SPICR_MASTER | SPICR_SPE;
	xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);

	for (int i = 0;
		 i < 10 && (xlnx_quadspi_read32(dev, SPISR_OFFSET) & SPISR_TX_EMPTY) == 0; i++) {
		k_msleep(1);
	}
	if ((xlnx_quadspi_read32(dev, SPISR_OFFSET) & SPISR_TX_EMPTY) == 0) {
		LOG_ERR("timeout waiting for TX_EMPTY");
		return -EIO;
	}
	spicr |= SPICR_MASTER_XFER_INH;
	xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);

	while ((xlnx_quadspi_read32(dev, SPISR_OFFSET) & SPISR_RX_EMPTY) == 0) {
		xlnx_quadspi_read32(dev, SPI_DRR_OFFSET);
	}

	spissr = BIT_MASK(config->num_ss_bits);
	xlnx_quadspi_write32(dev, spissr, SPISSR_OFFSET);

	/* Reset controller to clean up */
	xlnx_quadspi_write32(dev, SRR_SOFTRESET_MAGIC, SRR_OFFSET);

	return 0;
}
#endif

static int xlnx_quadspi_init(const struct device *dev)
{
	int err;
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;

	k_event_init(&data->dtr_empty);

	/* Reset controller */
	xlnx_quadspi_write32(dev, SRR_SOFTRESET_MAGIC, SRR_OFFSET);

	config->irq_config_func(dev);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(xlnx_startup_block)
	if (config->startup_block) {
		err = xlnx_quadspi_startup_block_workaround(dev);
		if (err < 0) {
			return err;
		}
	}
#endif

	/* Enable DTR Empty interrupt */
	xlnx_quadspi_write32(dev, IPIXR_DTR_EMPTY, IPIER_OFFSET);
	xlnx_quadspi_write32(dev, DGIER_GIE, DGIER_OFFSET);

	return 0;
}

static DEVICE_API(spi, xlnx_quadspi_driver_api) = {
	.transceive = xlnx_quadspi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = xlnx_quadspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = xlnx_quadspi_release,
};
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(xlnx_startup_block)
#define STARTUP_BLOCK_INIT(n) .startup_block = DT_INST_PROP(n, xlnx_startup_block),
#else
#define STARTUP_BLOCK_INIT(n)
#endif

#define XLNX_QUADSPI_INIT(n)						\
	static void xlnx_quadspi_config_func_##n(const struct device *dev);	\
									\
	static const struct xlnx_quadspi_config xlnx_quadspi_config_##n = { \
		.base = DT_INST_REG_ADDR(n),				\
		.irq_config_func = xlnx_quadspi_config_func_##n,	\
		.num_ss_bits = DT_INST_PROP(n, xlnx_num_ss_bits),	\
		.num_xfer_bytes =					\
			DT_INST_PROP(n, xlnx_num_transfer_bits) / 8,	\
		.fifo_size = DT_INST_PROP_OR(n, fifo_size, 0),		\
		STARTUP_BLOCK_INIT(n)					\
	};								\
									\
	static struct xlnx_quadspi_data xlnx_quadspi_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(xlnx_quadspi_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(xlnx_quadspi_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &xlnx_quadspi_init,			\
			    NULL,					\
			    &xlnx_quadspi_data_##n,			\
			    &xlnx_quadspi_config_##n, POST_KERNEL,	\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &xlnx_quadspi_driver_api);			\
									\
	static void xlnx_quadspi_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    xlnx_quadspi_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(XLNX_QUADSPI_INIT)
