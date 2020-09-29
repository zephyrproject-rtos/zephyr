/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_spi_2_00_a

#include <device.h>
#include <drivers/spi.h>
#include <sys/sys_io.h>
#include <logging/log.h>
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
};

struct xlnx_quadspi_data {
	struct spi_context ctx;
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

	if (!IS_ENABLED(CONFIG_SPI_SLAVE) || !spi_context_is_slave(ctx)) {
		spi_context_cs_configure(ctx);
	}

	return 0;
}

static void xlnx_quadspi_start_tx(const struct device *dev)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t xfer_len;
	uint32_t spicr = 0U;
	uint32_t spisr;
	uint32_t dtr = 0U;

	if (!spi_context_tx_on(ctx) && !spi_context_rx_on(ctx)) {
		/* All done, de-assert slave select */
		xlnx_quadspi_cs_control(dev, false);

		if ((ctx->config->operation & SPI_HOLD_ON_CS) == 0U) {
			/* Tri-state SPI IOs */
			spicr = xlnx_quadspi_read32(dev, SPICR_OFFSET);
			spicr &= ~(SPICR_SPE);
			xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
		}

		spi_context_complete(ctx, 0);
		return;
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
			};
		} else {
			/* No TX buffer. Use dummy TX data */
			dtr = 0U;
		}

		xlnx_quadspi_write32(dev, dtr, SPI_DTR_OFFSET);
		spi_context_update_tx(ctx, config->num_xfer_bytes, 1);

		spisr = xlnx_quadspi_read32(dev, SPISR_OFFSET);
		if (spisr & SPISR_TX_FULL) {
			break;
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

		spi_context_complete(ctx, -ENOTSUP);
	}

	if (!IS_ENABLED(CONFIG_SPI_SLAVE) || !spi_context_is_slave(ctx)) {
		/* Uninhibit master transaction */
		spicr &= ~(SPICR_MASTER_XFER_INH);
		xlnx_quadspi_write32(dev, spicr, SPICR_OFFSET);
	}
}

static int xlnx_quadspi_transceive(const struct device *dev,
				   const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs,
				   bool async, struct k_poll_signal *signal)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, async, signal);

	ret = xlnx_quadspi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs,
				  config->num_xfer_bytes);

	xlnx_quadspi_cs_control(dev, true);

	xlnx_quadspi_start_tx(dev);

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
				       NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int xlnx_quadspi_transceive_async(const struct device *dev,
					 const struct spi_config *spi_cfg,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs,
					 struct k_poll_signal *signal)
{
	return xlnx_quadspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true,
				       signal);
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
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t temp;
	uint32_t drr;

	/* Acknowledge interrupt */
	temp = xlnx_quadspi_read32(dev, IPISR_OFFSET);
	xlnx_quadspi_write32(dev, temp, IPISR_OFFSET);

	if (temp & IPIXR_DTR_EMPTY) {
		temp = xlnx_quadspi_read32(dev, SPISR_OFFSET);

		/* Read RX data */
		while (!(temp & SPISR_RX_EMPTY)) {
			drr = xlnx_quadspi_read32(dev, SPI_DRR_OFFSET);

			if (spi_context_rx_buf_on(ctx)) {
				switch (config->num_xfer_bytes) {
				case 1:
					UNALIGNED_PUT(drr,
						      (uint8_t *)ctx->rx_buf);
					break;
				case 2:
					UNALIGNED_PUT(drr,
						      (uint16_t *)ctx->rx_buf);
					break;
				case 4:
					UNALIGNED_PUT(drr,
						      (uint32_t *)ctx->rx_buf);
					break;
				default:
					__ASSERT(0,
						 "unsupported num_xfer_bytes");
				}
			}

			spi_context_update_rx(ctx, config->num_xfer_bytes, 1);

			temp = xlnx_quadspi_read32(dev, SPISR_OFFSET);
		}

		/* Start next TX */
		xlnx_quadspi_start_tx(dev);
	} else {
		LOG_WRN("unhandled interrupt, ipisr = 0x%08x", temp);
	}
}

static int xlnx_quadspi_init(const struct device *dev)
{
	const struct xlnx_quadspi_config *config = dev->config;
	struct xlnx_quadspi_data *data = dev->data;

	/* Reset controller */
	xlnx_quadspi_write32(dev, SRR_SOFTRESET_MAGIC, SRR_OFFSET);

	config->irq_config_func(dev);

	/* Enable DTR Empty interrupt */
	xlnx_quadspi_write32(dev, IPIXR_DTR_EMPTY, IPIER_OFFSET);
	xlnx_quadspi_write32(dev, DGIER_GIE, DGIER_OFFSET);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api xlnx_quadspi_driver_api = {
	.transceive = xlnx_quadspi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = xlnx_quadspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = xlnx_quadspi_release,
};

#define XLNX_QUADSPI_INIT(n)						\
	static void xlnx_quadspi_config_func_##n(const struct device *dev);	\
									\
	static const struct xlnx_quadspi_config xlnx_quadspi_config_##n = { \
		.base = DT_INST_REG_ADDR(n),				\
		.irq_config_func = xlnx_quadspi_config_func_##n,	\
		.num_ss_bits = DT_INST_PROP(n, xlnx_num_ss_bits),	\
		.num_xfer_bytes =					\
			DT_INST_PROP(n, xlnx_num_transfer_bits) / 8,	\
	};								\
									\
	static struct xlnx_quadspi_data xlnx_quadspi_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(xlnx_quadspi_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(xlnx_quadspi_data_##n, ctx),	\
	};								\
									\
	DEVICE_AND_API_INIT(xlnx_quadspi_##n, DT_INST_LABEL(n),		\
			    &xlnx_quadspi_init, &xlnx_quadspi_data_##n,	\
			    &xlnx_quadspi_config_##n, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &xlnx_quadspi_driver_api);			\
									\
	static void xlnx_quadspi_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    xlnx_quadspi_isr,				\
			    DEVICE_GET(xlnx_quadspi_##n), 0);		\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(XLNX_QUADSPI_INIT)
