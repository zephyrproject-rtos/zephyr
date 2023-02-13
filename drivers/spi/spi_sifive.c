/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_spi0

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sifive);

#include "spi_sifive.h"

#include <soc.h>
#include <stdbool.h>

/* Helper Functions */

static ALWAYS_INLINE
void sys_set_mask(mem_addr_t addr, uint32_t mask, uint32_t value)
{
	uint32_t temp = sys_read32(addr);

	temp &= ~(mask);
	temp |= value;

	sys_write32(temp, addr);
}

static int spi_config(const struct device *dev, uint32_t frequency,
	       uint16_t operation)
{
	uint32_t div;
	uint32_t fmt_len;

	if (operation & SPI_HALF_DUPLEX) {
		return -ENOTSUP;
	}

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

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
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

static ALWAYS_INLINE bool spi_sifive_send_available(const struct device *dev)
{
	return !(sys_read32(SPI_REG(dev, REG_TXDATA)) & SF_TXDATA_FULL);
}

static ALWAYS_INLINE
void spi_sifive_send(const struct device *dev, uint8_t frame)
{
	sys_write32((uint32_t) frame, SPI_REG(dev, REG_TXDATA));
}

static ALWAYS_INLINE
bool spi_sifive_recv(const struct device *dev, uint8_t *val)
{
	uint32_t reg = sys_read32(SPI_REG(dev, REG_RXDATA));

	if (reg & SF_RXDATA_EMPTY) {
		return false;
	}
	*val = (uint8_t) reg;
	return true;
}

static void spi_sifive_xfer(const struct device *dev, const bool hw_cs_control)
{
	struct spi_context *ctx = &SPI_DATA(dev)->ctx;
	uint8_t txd, rxd;
	int queued_frames = 0;

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx) || queued_frames > 0) {
		bool send = false;

		/* As long as frames remain to be sent, attempt to queue them on Tx FIFO. If
		 * the FIFO is full then another attempt will be made next pass. If Rx length
		 * > Tx length then queue dummy Tx in order to read the requested Rx data.
		 */
		if (spi_context_tx_buf_on(ctx)) {
			send = true;
			txd = *ctx->tx_buf;
		} else if (queued_frames == 0) {  /* Implies spi_context_rx_on(). */
			send = true;
			txd = 0U;
		}

		if (send && spi_sifive_send_available(dev)) {
			spi_sifive_send(dev, txd);
			queued_frames++;
			spi_context_update_tx(ctx, 1, 1);
		}

		if (queued_frames > 0 && spi_sifive_recv(dev, &rxd)) {
			if (spi_context_rx_buf_on(ctx)) {
				*ctx->rx_buf = rxd;
			}
			queued_frames--;
			spi_context_update_rx(ctx, 1, 1);
		}
	}

	/* Deassert the CS line */
	if (!hw_cs_control) {
		spi_context_cs_control(&SPI_DATA(dev)->ctx, false);
	} else {
		sys_write32(SF_CSMODE_OFF, SPI_REG(dev, REG_CSMODE));
	}

	spi_context_complete(ctx, dev, 0);
}

/* API Functions */

static int spi_sifive_init(const struct device *dev)
{
	int err;
#ifdef CONFIG_PINCTRL
	struct spi_sifive_cfg *cfg = (struct spi_sifive_cfg *)dev->config;
#endif
	/* Disable SPI Flash mode */
	sys_clear_bit(SPI_REG(dev, REG_FCTRL), SF_FCTRL_EN);

	err = spi_context_cs_configure_all(&SPI_DATA(dev)->ctx);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_PINCTRL
	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}
#endif

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);
	return 0;
}

static int spi_sifive_transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
	int rc = 0;
	bool hw_cs_control = false;

	/* Lock the SPI Context */
	spi_context_lock(&SPI_DATA(dev)->ctx, false, NULL, NULL, config);

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

static int spi_sifive_release(const struct device *dev,
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
	PINCTRL_DT_INST_DEFINE(n); \
	static struct spi_sifive_data spi_sifive_data_##n = { \
		SPI_CONTEXT_INIT_LOCK(spi_sifive_data_##n, ctx), \
		SPI_CONTEXT_INIT_SYNC(spi_sifive_data_##n, ctx), \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	}; \
	static struct spi_sifive_cfg spi_sifive_cfg_##n = { \
		.base = DT_INST_REG_ADDR_BY_NAME(n, control), \
		.f_sys = SIFIVE_PERIPHERAL_CLOCK_FREQUENCY, \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
	}; \
	DEVICE_DT_INST_DEFINE(n, \
			spi_sifive_init, \
			NULL, \
			&spi_sifive_data_##n, \
			&spi_sifive_cfg_##n, \
			POST_KERNEL, \
			CONFIG_SPI_INIT_PRIORITY, \
			&spi_sifive_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
