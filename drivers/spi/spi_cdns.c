/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_spi_r1p6
#include "spi_cdns.h"
#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
LOG_MODULE_REGISTER(cdns_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

struct cdns_spi_config {
	mm_reg_t base;
	void (*irq_config_func)(const struct device *dev);
	uint16_t num_ss_bits;
	uint16_t is_decoded_cs;
	uint32_t input_clk;
};

struct cdns_spi_data {
	size_t xfer_cnt;
	uint32_t spi_frequency;
	struct spi_context ctx;
	uint16_t slave;
};

static inline uint32_t cdns_spi_read32(const struct device *dev,
				       mm_reg_t offset)
{
	const struct cdns_spi_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void cdns_spi_write32(const struct device *dev,
				    uint32_t value, mm_reg_t offset)
{
	const struct cdns_spi_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static bool cdns_spi_transfer_ongoing(struct cdns_spi_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void cdns_spi_xfer_abort(const struct device *dev)
{
	/* Disable the spi device. */
	cdns_spi_write32(dev, ~CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);

	/* Clear the RX FIFO and drop any data. */
	while (cdns_spi_read32(dev, CDNS_SPI_SR_OFFSET) & CDNS_SPI_IXR_RXNEMPTY_MASK)
		cdns_spi_read32(dev, CDNS_SPI_RXD_OFFSET);

	/* Clear the mode fail bit. */
	cdns_spi_write32(dev, CDNS_SPI_SR_OFFSET,
			 CDNS_SPI_IXR_MODF_MASK);
}

static void cdns_spi_cs_cntrl(const struct device *dev,
			      bool on)
{
	struct cdns_spi_data *data = dev->data;
	const struct cdns_spi_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t config_reg;

	if (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(ctx)) {
		/* Skip slave select assert/de-assert in slave mode. */
		return;
	}

	config_reg = cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET);
	if (on) {
		if (config->is_decoded_cs)
			data->slave = ctx->config->slave << CDNS_SPI_CR_SSCTRL_SHIFT;
		else
			data->slave = ((~(1U << ctx->config->slave)) & CDNS_SPI_CR_SSCTRL_MAXIMUM) <<
				       CDNS_SPI_CR_SSCTRL_SHIFT;

		config_reg &= (uint32_t)(~CDNS_SPI_CR_SSCTRL_MASK);
		config_reg |= data->slave;
	} else {
		config_reg |= (uint32_t)(CDNS_SPI_CR_SSCTRL_MASK);
	}

	cdns_spi_write32(dev, config_reg, CDNS_SPI_CR_OFFSET);
}

static void cdns_spi_config_clock_mode(const struct device *dev,
				       const struct spi_config *spi_cfg)
{
	uint32_t ctrl_reg = 0, new_ctrl_reg = 0;

	new_ctrl_reg = cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET);
	ctrl_reg = new_ctrl_reg;

	new_ctrl_reg &= ~(CDNS_SPI_CR_CPHA | CDNS_SPI_CR_CPOL);
	if (spi_cfg->operation & SPI_MODE_CPOL) {
		new_ctrl_reg |= CDNS_SPI_CR_CPOL;
	}

	if (spi_cfg->operation & SPI_MODE_CPHA) {
		new_ctrl_reg |= CDNS_SPI_CR_CPHA;
	}

	if (new_ctrl_reg != ctrl_reg) {
		cdns_spi_write32(dev, ~CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);
		cdns_spi_write32(dev, new_ctrl_reg, CDNS_SPI_CR_OFFSET);
		cdns_spi_write32(dev, CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);
	}
}

static void cdns_spi_setup_transfer(const struct device *dev,
				    const struct spi_config *spi_cfg)
{
	struct cdns_spi_data *data = dev->data;
	uint32_t ctrl_reg, baud_rate_val, frequency;

	frequency = data->spi_frequency;
	if (data->spi_frequency != spi_cfg->frequency) {
		ctrl_reg = cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET);

		/* The first valid value is 1. */
		baud_rate_val = CDNS_SPI_BAUD_DIV_MIN;
		while ((baud_rate_val < CDNS_SPI_BAUD_DIV_MAX) &&
		       (frequency / (2 << baud_rate_val)) > spi_cfg->frequency)
			baud_rate_val++;

		ctrl_reg &= ~CDNS_SPI_CR_BAUD_DIV;
		ctrl_reg |= baud_rate_val << CDNS_SPI_BAUD_DIV_SHIFT;

		data->spi_frequency = spi_cfg->frequency;
		cdns_spi_write32(dev, ctrl_reg, CDNS_SPI_CR_OFFSET);
	}
}

static int cdns_spi_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct cdns_spi_config *config = dev->config;
	struct cdns_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t spicr = 0;

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	spicr = cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET);
	if (!(spi_cfg->operation & SPI_OP_MODE_SLAVE)) {
		spicr |= CDNS_SPI_CR_DEFAULT;
	}

	if (config->is_decoded_cs)
		spicr |= CDNS_SPI_CR_PERI_SEL;

	cdns_spi_write32(dev, spicr, CDNS_SPI_CR_OFFSET);

	data->spi_frequency = config->input_clk;

	/* Configure the clock phase and clock polarity. */
	cdns_spi_config_clock_mode(dev, spi_cfg);

	ctx->config = spi_cfg;

	return 0;
}

static int cdns_spi_write_fifo(const struct device *dev)
{
	struct cdns_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t txr_cnt = 0U, tx_data, config_reg;
	size_t xfer_len;
#ifndef CONFIG_CDNS_SPI_INTR
	uint32_t status_reg;
	uint32_t check_transfer;
#endif

	/* Can only see as far as the current rx buffer. */
	xfer_len = ctx->tx_len > ctx->rx_len ? ctx->tx_len : ctx->rx_len;

	/* Write TX data */
	while ((xfer_len > (uint32_t)0) &&
	       ((uint32_t)txr_cnt < (uint32_t)CDNS_SPI_FIFO_DEPTH)) {
		if (spi_context_tx_buf_on(ctx)) {
			tx_data = UNALIGNED_GET((uint32_t *)(ctx->tx_buf));
		} else {
			/* No TX buffer. Use dummy TX data. */
			tx_data = 0U;
		}

		cdns_spi_write32(dev, tx_data, CDNS_SPI_TXD_OFFSET);
		spi_context_update_tx(ctx, 1, 1);
		xfer_len--;
		++txr_cnt;
	}

	data->xfer_cnt = txr_cnt;

#ifdef CONFIG_CDNS_SPI_INTR
	cdns_spi_write32(dev, CDNS_SPI_IXR_DFLT_MASK, CDNS_SPI_IER_OFFSET);
#endif

	/*
	 * If master mode and manual start mode, issue manual start
	 * command to start the transfer.
	 */
	if (((cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET) & (uint32_t)CDNS_SPI_CR_MANSTRTEN)) &&
	    ((cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET) & CDNS_SPI_CR_MSTREN))) {
		config_reg = cdns_spi_read32(dev, CDNS_SPI_CR_OFFSET);
		config_reg |= CDNS_SPI_CR_MANTXSTRT;
		cdns_spi_write32(dev, config_reg, CDNS_SPI_CR_OFFSET);
	}

#ifndef CONFIG_CDNS_SPI_INTR
	/* Wait for the transfer to finish by polling Tx fifo status. */
	check_transfer = 0U;
	while (check_transfer == 0U) {
		status_reg = cdns_spi_read32(dev, CDNS_SPI_SR_OFFSET);
		if ((status_reg & CDNS_SPI_IXR_MODF_MASK) != (uint32_t)0U) {
			/* Deassert the CS line. */
			cdns_spi_cs_cntrl(dev, false);

			/* Abort target transfer. */
			cdns_spi_xfer_abort(dev);
			spi_context_complete(ctx, dev, -ENOTSUP);
			return -EIO;
		}

		check_transfer = (status_reg &
				  CDNS_SPI_IXR_TXOW_MASK);
	}
#endif
	return 0;
}

static void cdns_spi_read_fifo(const struct device *dev)
{
	struct cdns_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t xfer_cnt = data->xfer_cnt;
	uint8_t rx_data;

	/* Read RX data. */
	while (xfer_cnt) {
		rx_data = (uint8_t)cdns_spi_read32(dev, CDNS_SPI_RXD_OFFSET);

		if (spi_context_rx_buf_on(ctx)) {
			UNALIGNED_PUT(rx_data, (uint8_t *)ctx->rx_buf);
		}

		spi_context_update_rx(ctx, 1, 1);
		--xfer_cnt;
	}

}

static int cdns_spi_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs,
			       bool asynchronous,
			       spi_callback_t cb,
			       void *userdata)
{
	struct cdns_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	/* Lock the SPI Context. */
	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	ret = cdns_spi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	if (!IS_ENABLED(CONFIG_SPI_SLAVE) || !spi_context_is_slave(ctx)) {
		cdns_spi_setup_transfer(dev, spi_cfg);
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Enable the spi device. */
	cdns_spi_write32(dev, CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);

	/* Assert the CS line. */
	cdns_spi_cs_cntrl(dev, true);

#ifdef CONFIG_CDNS_SPI_INTR
	ret = cdns_spi_write_fifo(dev);
	if (ret) {
		goto out;
	}

	ret = spi_context_wait_for_completion(ctx);
#else
	do {
		ret = cdns_spi_write_fifo(dev);
		if (ret) {
			break;
		}
		cdns_spi_read_fifo(dev);
	} while (cdns_spi_transfer_ongoing(data));
#endif
	/* Deassert the CS line. */
	cdns_spi_cs_cntrl(dev, false);

	/* Disable the spi device. */
	cdns_spi_write32(dev, ~CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);
	spi_context_complete(ctx, dev, 0);

	ret = spi_context_wait_for_completion(ctx);
out:
	spi_context_release(ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int cdns_spi_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int cdns_spi_transceive_blocking(const struct device *dev,
					const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	return cdns_spi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false,
			NULL, NULL);
}

static int cdns_spi_release(const struct device *dev,
			    const struct spi_config *spi_cfg)
{
	const struct cdns_spi_config *config = dev->config;
	struct cdns_spi_data *data = dev->data;

	/* Force slave select de-assert. */
	cdns_spi_write32(dev, BIT_MASK(config->num_ss_bits), CDNS_SPI_CR_OFFSET);

	cdns_spi_write32(dev, ~CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void cdns_spi_isr(const struct device *dev)
{
#ifdef CONFIG_CDNS_SPI_INTR
	struct cdns_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int xstatus = 0;
	uint32_t isr;

	isr = cdns_spi_read32(dev, CDNS_SPI_SR_OFFSET);
	cdns_spi_write32(dev, isr, CDNS_SPI_SR_OFFSET);
	cdns_spi_write32(dev, CDNS_SPI_IXR_TXOW_MASK, CDNS_SPI_IDR_OFFSET);

	if (isr & CDNS_SPI_IXR_MODF_MASK) {
		/*
		 * Indicate that transfer is completed, the SPI subsystem will
		 * identify the error as the remaining bytes to be
		 * transferred is non-zero.
		 */
		cdns_spi_cs_cntrl(dev, false);
		cdns_spi_xfer_abort(dev);

		xstatus = -EIO;
		spi_context_complete(ctx, dev, xstatus);
		return;
	}

	if (isr & CDNS_SPI_IXR_TXOW_MASK) {
		cdns_spi_read_fifo(dev);

		if (!cdns_spi_transfer_ongoing(data)) {
			cdns_spi_write32(dev, CDNS_SPI_IXR_DFLT_MASK, CDNS_SPI_IDR_OFFSET);
			cdns_spi_cs_cntrl(dev, false);
			spi_context_complete(ctx, dev, 0);
		} else {
			cdns_spi_write_fifo(dev);
		}
	}

	/* Check for overflow and underflow errors. */
	if (((isr & CDNS_SPI_IXR_RXOVR_MASK) != 0U) ||
	    ((isr & CDNS_SPI_IXR_TXUF_MASK) != 0U)) {
		/*
		 * The Slave select lines are being manually controlled.
		 * Disable them because the transfer is complete.
		 */
		cdns_spi_cs_cntrl(dev, false);

		xstatus = -EIO;
		spi_context_complete(ctx, dev, xstatus);
	}
#endif
}

static int cdns_spi_init(const struct device *dev)
{
	uint32_t err;
	const struct cdns_spi_config *config = dev->config;
	struct cdns_spi_data *data = dev->data;

	cdns_spi_write32(dev, ~CDNS_SPI_ER_ENABLE, CDNS_SPI_ER_OFFSET);

	/* Clear the RX FIFO. */
	while (cdns_spi_read32(dev, CDNS_SPI_SR_OFFSET) & CDNS_SPI_IXR_RXNEMPTY_MASK) {
		cdns_spi_read32(dev, CDNS_SPI_RXD_OFFSET);
	}

	cdns_spi_write32(dev, CDNS_SPI_IXR_MODF_MASK, CDNS_SPI_SR_OFFSET);

	cdns_spi_write32(dev, CDNS_SPI_CR_RESET_STATE, CDNS_SPI_CR_OFFSET);
	cdns_spi_write32(dev, CDNS_SPI_IXR_ALL, CDNS_SPI_CR_OFFSET);
	config->irq_config_func(dev);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api cdns_spi_driver_api = {
	.transceive = cdns_spi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = cdns_spi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = cdns_spi_release,
};

#define CDNS_SPI_INIT(n)							\
	static void cdns_spi_config_func_##n(const struct device *dev);		\
										\
	static const struct cdns_spi_config cdns_spi_config_##n = {		\
		.base = DT_INST_REG_ADDR(n),					\
		.irq_config_func = cdns_spi_config_func_##n,			\
		.input_clk = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency), \
		.num_ss_bits = DT_INST_PROP(n, cdns_num_ss_bits),		\
		.is_decoded_cs = DT_INST_PROP(n, is_decoded_cs),		\
	};									\
										\
	static struct cdns_spi_data cdns_spi_data_##n = {			\
		SPI_CONTEXT_INIT_LOCK(cdns_spi_data_##n, ctx),			\
		SPI_CONTEXT_INIT_SYNC(cdns_spi_data_##n, ctx),			\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &cdns_spi_init,				\
			NULL,							\
			&cdns_spi_data_##n,					\
			&cdns_spi_config_##n, POST_KERNEL,			\
			CONFIG_SPI_INIT_PRIORITY,				\
			&cdns_spi_driver_api);					\
										\
	static void cdns_spi_config_func_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			cdns_spi_isr,						\
			DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(CDNS_SPI_INIT)
