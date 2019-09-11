/*
 * Copyright (c) 2019 Western Digital Corporation or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_oc_simple);

#include <sys_io.h>
#include <spi.h>

#include "spi_context.h"
#include "spi_oc_simple.h"

/* Bit 5:4 == ESPR, Bit 1:0 == SPR */
u8_t DIVIDERS[] = { 0x00,       /*   2  */
		    0x01,       /*   4  */
		    0x10,       /*   8  */
		    0x02,       /*  16  */
		    0x03,       /*  32  */
		    0x11,       /*  64  */
		    0x12,       /* 128  */
		    0x13,       /* 256  */
		    0x20,       /* 512  */
		    0x21,       /* 1024 */
		    0x22,       /* 2048 */
		    0x23 };     /* 4096 */

static int spi_oc_simple_configure(const struct spi_oc_simple_cfg *info,
				struct spi_oc_simple_data *spi,
				const struct spi_config *config)
{
	u8_t spcr = 0U;
	int i;

	if (spi_context_configured(&spi->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	/* Simple SPI only supports master mode */
	if (spi_context_is_slave(&spi->ctx)) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_MODE_LOOP | SPI_TRANSFER_LSB |
				 SPI_LINES_DUAL | SPI_LINES_QUAD)) {
		LOG_ERR("Unsupported configuration");
		return -EINVAL;
	}

	/* SPI mode */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		spcr |= SPI_OC_SIMPLE_SPCR_CPOL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		spcr |= SPI_OC_SIMPLE_SPCR_CPHA;
	}

	/* Set clock divider */
	for (i = 0; i < 12; i++)
		if ((config->frequency << (i + 1)) >
		    CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
			break;
		}

	sys_write8((DIVIDERS[i] >> 4) & 0x3, SPI_OC_SIMPLE_SPER(info));
	spcr |= (DIVIDERS[i] & 0x3);

	/* Configure and Enable SPI controller */
	sys_write8(spcr | SPI_OC_SIMPLE_SPCR_SPE, SPI_OC_SIMPLE_SPCR(info));

	spi->ctx.config = config;

	if (config->cs) {
		spi_context_cs_configure(&spi->ctx);
	}

	return 0;
}

int spi_oc_simple_transceive(struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
	const struct spi_oc_simple_cfg *info = dev->config->config_info;
	struct spi_oc_simple_data *spi = SPI_OC_SIMPLE_DATA(dev);
	struct spi_context *ctx = &spi->ctx;

	u8_t rx_byte;
	size_t i;
	size_t cur_xfer_len;
	int rc;

	/* Lock the SPI Context */
	spi_context_lock(ctx, false, NULL);

	spi_oc_simple_configure(info, spi, config);

	/* Set chip select */
	if (config->cs) {
		spi_context_cs_control(&spi->ctx, true);
	} else {
		sys_write8(1 << config->slave, SPI_OC_SIMPLE_SPSS(info));
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
		cur_xfer_len = spi_context_longest_current_buf(ctx);

		for (i = 0; i < cur_xfer_len; i++) {

			/* Write byte */
			if (spi_context_tx_buf_on(ctx)) {
				sys_write8(*ctx->tx_buf,
					   SPI_OC_SIMPLE_SPDR(info));
				spi_context_update_tx(ctx, 1, 1);
			} else {
				sys_write8(0, SPI_OC_SIMPLE_SPDR(info));
			}

			/* Wait for rx FIFO empty flag to clear */
			while (sys_read8(SPI_OC_SIMPLE_SPSR(info)) & 0x1) {
			}

			/* Get received byte */
			rx_byte = sys_read8(SPI_OC_SIMPLE_SPDR(info));

			/* Store received byte if rx buffer is on */
			if (spi_context_rx_on(ctx)) {
				*ctx->rx_buf = rx_byte;
				spi_context_update_rx(ctx, 1, 1);
			}
		}
	}

	/* Clear chip-select */
	if (config->cs) {
		spi_context_cs_control(&spi->ctx, false);
	} else {
		sys_write8(0 << config->slave, SPI_OC_SIMPLE_SPSS(info));
	}

	spi_context_complete(ctx, 0);
	rc = spi_context_wait_for_completion(ctx);

	spi_context_release(ctx, rc);

	return rc;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_oc_simple_transceive_async(struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

int spi_oc_simple_release(struct device *dev, const struct spi_config *config)
{
	spi_context_unlock_unconditionally(&SPI_OC_SIMPLE_DATA(dev)->ctx);
	return 0;
}

static struct spi_driver_api spi_oc_simple_api = {
	.transceive = spi_oc_simple_transceive,
	.release = spi_oc_simple_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_oc_simple_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
};

int spi_oc_simple_init(struct device *dev)
{
	const struct spi_oc_simple_cfg *info = dev->config->config_info;

	/* Clear chip selects */
	sys_write8(0, SPI_OC_SIMPLE_SPSS(info));

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&SPI_OC_SIMPLE_DATA(dev)->ctx);

	/* Initial clock stucks high, so add this workaround */
	sys_write8(SPI_OC_SIMPLE_SPCR_SPE, SPI_OC_SIMPLE_SPCR(info));
	sys_write8(0, SPI_OC_SIMPLE_SPDR(info));
	while (sys_read8(SPI_OC_SIMPLE_SPSR(info)) & 0x1) {
	}

	sys_read8(SPI_OC_SIMPLE_SPDR(info));

	return 0;
}

#ifdef DT_INST_0_OPENCORES_SPI_SIMPLE
static struct spi_oc_simple_cfg spi_oc_simple_cfg_0 = {
	.base = DT_INST_0_OPENCORES_SPI_SIMPLE_CONTROL_BASE_ADDRESS,
};

static struct spi_oc_simple_data spi_oc_simple_data_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_oc_simple_data_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_oc_simple_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_oc_simple_0,
		    DT_INST_0_OPENCORES_SPI_SIMPLE_LABEL,
		    spi_oc_simple_init,
		    &spi_oc_simple_data_0,
		    &spi_oc_simple_cfg_0,
		    POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY,
		    &spi_oc_simple_api);
#endif

#ifdef DT_INST_1_OPENCORES_SPI_SIMPLE
static struct spi_oc_simple_cfg spi_oc_simple_cfg_1 = {
	.base = DT_INST_1_OPENCORES_SPI_SIMPLE_CONTROL_BASE_ADDRESS,
};

static struct spi_oc_simple_data spi_oc_simple_data_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_oc_simple_data_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_oc_simple_data_1, ctx),
};

DEVICE_AND_API_INIT(spi_oc_simple_1,
		    DT_INST_1_OPENCORES_SPI_SIMPLE_LABEL,
		    spi_oc_simple_init,
		    &spi_oc_simple_data_1,
		    &spi_oc_simple_cfg_1,
		    POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY,
		    &spi_oc_simple_api);
#endif
