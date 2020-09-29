/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_spi0

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_sifive);

#include "spi_sifive.h"

#include <stdbool.h>

/* Helper Functions */

static inline void sys_set_mask(mem_addr_t addr, uint32_t mask, uint32_t value)
{
	uint32_t temp = sys_read32(addr);

	temp &= ~(mask);
	temp |= value;

	sys_write32(temp, addr);
}

int spi_config(const struct device *dev, uint32_t frequency,
	       uint16_t operation)
{
	uint32_t div;
	uint32_t fmt_len;

	if (SPI_OP_MODE_GET(operation) != SPI_OP_MODE_MASTER) {
		return -ENOTSUP;
	}

	if (operation & SPI_MODE_LOOP) {
		return -ENOTSUP;
	}

	/* Set the SPI frequency */
	div = (SPI_CFG(dev)->f_sys / (frequency * 2U)) - 1;
	sys_write32((SF_SCKDIV_DIV_MASK & div), SPI_REG(dev, REG_SCKDIV));

	/* Set the polarity */
	if (operation & SPI_MODE_CPOL) {
		/* If CPOL is set, then SCK idles at logical 1 */
		sys_set_bit(SPI_REG(dev, REG_SCKMODE), SF_SCKMODE_POL);
	} else {
		/* SCK idles at logical 0 */
		sys_clear_bit(SPI_REG(dev, REG_SCKMODE), SF_SCKMODE_POL);
	}

	/* Set the phase */
	if (operation & SPI_MODE_CPHA) {
		/*
		 * If CPHA is set, then data is sampled
		 * on the trailing SCK edge
		 */
		sys_set_bit(SPI_REG(dev, REG_SCKMODE), SF_SCKMODE_PHA);
	} else {
		/* Data is sampled on the leading SCK edge */
		sys_clear_bit(SPI_REG(dev, REG_SCKMODE), SF_SCKMODE_PHA);
	}

	/* Get the frame length */
	fmt_len = SPI_WORD_SIZE_GET(operation);
	if (fmt_len > SF_FMT_LEN_MASK) {
		return -ENOTSUP;
	}

	/* Set the frame length */
	fmt_len = fmt_len << SF_FMT_LEN;
	fmt_len &= SF_FMT_LEN_MASK;
	sys_set_mask(SPI_REG(dev, REG_FMT), SF_FMT_LEN_MASK, fmt_len);

	if ((operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		return -ENOTSUP;
	}
	/* Set single line operation */
	sys_set_mask(SPI_REG(dev, REG_FMT),
		SF_FMT_PROTO_MASK,
		SF_FMT_PROTO_SINGLE);

	/* Set the endianness */
	if (operation & SPI_TRANSFER_LSB) {
		sys_set_bit(SPI_REG(dev, REG_FMT), SF_FMT_ENDIAN);
	} else {
		sys_clear_bit(SPI_REG(dev, REG_FMT), SF_FMT_ENDIAN);
	}

	return 0;
}

void spi_sifive_send(const struct device *dev, uint16_t frame)
{
	while (sys_read32(SPI_REG(dev, REG_TXDATA)) & SF_TXDATA_FULL) {
	}

	sys_write32((uint32_t) frame, SPI_REG(dev, REG_TXDATA));
}

uint16_t spi_sifive_recv(const struct device *dev)
{
	uint32_t val;

	while ((val = sys_read32(SPI_REG(dev, REG_RXDATA))) & SF_RXDATA_EMPTY) {
	}

	return (uint16_t) val;
}

void spi_sifive_xfer(const struct device *dev, const bool hw_cs_control)
{
	struct spi_context *ctx = &SPI_DATA(dev)->ctx;

	uint32_t send_len = spi_context_longest_current_buf(ctx);

	for (uint32_t i = 0; i < send_len; i++) {

		/* Send a frame */
		if (i < ctx->tx_len) {
			spi_sifive_send(dev, (uint16_t) (ctx->tx_buf)[i]);
		} else {
			/* Send dummy bytes */
			spi_sifive_send(dev, 0);
		}

		/* Receive a frame */
		if (i < ctx->rx_len) {
			ctx->rx_buf[i] = (uint8_t) spi_sifive_recv(dev);
		} else {
			/* Discard returned value */
			spi_sifive_recv(dev);
		}
	}

	/* Deassert the CS line */
	if (!hw_cs_control) {
		spi_context_cs_control(&SPI_DATA(dev)->ctx, false);
	} else {
		sys_write32(SF_CSMODE_OFF, SPI_REG(dev, REG_CSMODE));
	}

	spi_context_complete(ctx, 0);
}

/* API Functions */

int spi_sifive_init(const struct device *dev)
{
	/* Disable SPI Flash mode */
	sys_clear_bit(SPI_REG(dev, REG_FCTRL), SF_FCTRL_EN);

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);
	return 0;
}

int spi_sifive_transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
	int rc = 0;
	bool hw_cs_control = false;

	/* Lock the SPI Context */
	spi_context_lock(&SPI_DATA(dev)->ctx, false, NULL);

	/* Configure the SPI bus */
	SPI_DATA(dev)->ctx.config = config;

	/*
	 * If the chip select configuration is not present, we'll ask the
	 * SPI peripheral itself to control the CS line
	 */
	if (config->cs == NULL) {
		hw_cs_control = true;
	}

	if (!hw_cs_control) {
		/*
		 * If the user has requested manual GPIO control, ask the
		 * context for control and disable HW control
		 */
		spi_context_cs_configure(&SPI_DATA(dev)->ctx);
		sys_write32(SF_CSMODE_OFF, SPI_REG(dev, REG_CSMODE));
	} else {
		/*
		 * Tell the hardware to control the requested CS pin.
		 * NOTE:
		 *	For the SPI peripheral, the pin number is not the
		 *	GPIO pin, but the index into the list of available
		 *	CS lines for the SPI peripheral.
		 */
		sys_write32(config->slave, SPI_REG(dev, REG_CSID));
		sys_write32(SF_CSMODE_OFF, SPI_REG(dev, REG_CSMODE));
	}

	rc = spi_config(dev, config->frequency, config->operation);
	if (rc < 0) {
		spi_context_release(&SPI_DATA(dev)->ctx, rc);
		return rc;
	}

	spi_context_buffers_setup(&SPI_DATA(dev)->ctx, tx_bufs, rx_bufs, 1);

	/* Assert the CS line */
	if (!hw_cs_control) {
		spi_context_cs_control(&SPI_DATA(dev)->ctx, true);
	} else {
		sys_write32(SF_CSMODE_HOLD, SPI_REG(dev, REG_CSMODE));
	}

	/* Perform transfer */
	spi_sifive_xfer(dev, hw_cs_control);

	rc = spi_context_wait_for_completion(&SPI_DATA(dev)->ctx);

	spi_context_release(&SPI_DATA(dev)->ctx, rc);

	return rc;
}

int spi_sifive_release(const struct device *dev,
		       const struct spi_config *config)
{
	spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);
	return 0;
}

/* Device Instantiation */

static struct spi_driver_api spi_sifive_api = {
	.transceive = spi_sifive_transceive,
	.release = spi_sifive_release,
};

#define SPI_INIT(n)	\
	static struct spi_sifive_data spi_sifive_data_##n = { \
		SPI_CONTEXT_INIT_LOCK(spi_sifive_data_##n, ctx), \
		SPI_CONTEXT_INIT_SYNC(spi_sifive_data_##n, ctx), \
	}; \
	static struct spi_sifive_cfg spi_sifive_cfg_##n = { \
		.base = DT_INST_REG_ADDR_BY_NAME(n, control), \
		.f_sys = DT_INST_PROP(n, clock_frequency), \
	}; \
	DEVICE_AND_API_INIT(spi_##n, \
			DT_INST_LABEL(n), \
			spi_sifive_init, \
			&spi_sifive_data_##n, \
			&spi_sifive_cfg_##n, \
			POST_KERNEL, \
			CONFIG_SPI_INIT_PRIORITY, \
			&spi_sifive_api)

#ifndef CONFIG_SIFIVE_SPI_0_ROM
#if DT_INST_NODE_HAS_PROP(0, label)

SPI_INIT(0);

#endif /* DT_INST_NODE_HAS_PROP(0, label) */
#endif /* !CONFIG_SIFIVE_SPI_0_ROM */

#if DT_INST_NODE_HAS_PROP(1, label)

SPI_INIT(1);

#endif /* DT_INST_NODE_HAS_PROP(1, label) */

#if DT_INST_NODE_HAS_PROP(2, label)

SPI_INIT(2);

#endif /* DT_INST_NODE_HAS_PROP(2, label) */
