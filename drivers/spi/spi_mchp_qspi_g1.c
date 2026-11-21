/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * G1 QSPI controller (PIC32CX SG41/SG60/SG61, SAM D5x/E5x) in SPI mode
 * (QSPI_CTRLB.MODE = 0): a single-line SPI controller with one hardware chip
 * select (CS), used for example by the jedec,spi-nor flash driver.
 */

#define DT_DRV_COMPAT microchip_qspi_g1_spi

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_mchp_qspi_g1, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

/* QSPI_BAUD.BAUD divides CLK_QSPI_AHB by 1 to 256 */
#define QSPI_G1_DIV_MAX 256U

struct spi_mchp_qspi_g1_config {
	qspi_registers_t *regs;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_apb_sys;
	clock_control_subsys_t mclk_ahb_sys;
	const struct pinctrl_dev_config *pcfg;
};

struct spi_mchp_qspi_g1_data {
	struct spi_context ctx;
};

static int spi_mchp_qspi_g1_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	qspi_registers_t *regs = cfg->regs;
	uint32_t clk_hz;
	uint32_t divisor;
	uint32_t ctrlb;
	uint32_t baud;
	int ret;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if ((config->operation & (SPI_HALF_DUPLEX | SPI_TRANSFER_LSB)) != 0U) {
		LOG_ERR("Half-duplex and LSB first are not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8U) {
		LOG_ERR("Only 8-bit words are supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE)) {
		LOG_ERR("Only single line transfers are supported");
		return -ENOTSUP;
	}

	if (!spi_cs_is_gpio(config) && (config->peripheral != 0U)) {
		LOG_ERR("Only chip select 0 (CS) is available");
		return -EINVAL;
	}

	if (config->frequency == 0U) {
		LOG_ERR("Invalid frequency");
		return -EINVAL;
	}

	/* The baud rate generator runs from CLK_QSPI_AHB */
	ret = clock_control_get_rate(cfg->clock_dev, cfg->mclk_ahb_sys, &clk_hz);
	if (ret < 0) {
		return ret;
	}

	/* Use the requested or the next lower possible frequency */
	divisor = DIV_ROUND_UP(clk_hz, config->frequency);
	divisor = CLAMP(divisor, 1U, QSPI_G1_DIV_MAX);
	baud = QSPI_BAUD_BAUD(divisor - 1U);

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		baud |= QSPI_BAUD_CPOL_Msk;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		baud |= QSPI_BAUD_CPHA_Msk;
	}

	/* SPI mode, 8-bit words, CS held asserted until QSPI_CTRLA.LASTXFER */
	ctrlb = QSPI_CTRLB_CSMODE_LASTXFER | QSPI_CTRLB_DATALEN_8BITS;

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		ctrlb |= QSPI_CTRLB_LOOPEN_Msk;
	}

	/* QSPI_CTRLA is write-only: writing 0 clears ENABLE */
	regs->QSPI_CTRLA = 0U;
	while ((regs->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) != 0U) {
	}

	regs->QSPI_CTRLB = ctrlb;
	regs->QSPI_BAUD = baud;

	regs->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk;
	while ((regs->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) == 0U) {
	}

	LOG_DBG("SCK %u Hz", clk_hz / divisor);

	data->ctx.config = config;

	return 0;
}

/*
 * Shift len bytes, one at a time: the next byte is written only after the
 * previous one is received, so the single receive data register cannot
 * overrun, whatever the interrupt latency.
 */
static void spi_mchp_qspi_g1_shift(qspi_registers_t *regs, const uint8_t *tx_buf, uint8_t *rx_buf,
				   size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		uint8_t rx;

		regs->QSPI_TXDATA = (tx_buf != NULL) ? tx_buf[i] : 0U;

		while ((regs->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) == 0U) {
		}

		rx = (uint8_t)regs->QSPI_RXDATA;
		if (rx_buf != NULL) {
			rx_buf[i] = rx;
		}
	}
}

static void spi_mchp_qspi_g1_release_cs(qspi_registers_t *regs)
{
	/*
	 * QSPI_CTRLA is write-only, so ENABLE must be written with LASTXFER.
	 * The shift register is idle here, so CS is deasserted at once.
	 */
	regs->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk | QSPI_CTRLA_LASTXFER_Msk;
	while ((regs->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) == 0U) {
	}
}

static int spi_mchp_qspi_g1_transceive(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	qspi_registers_t *regs = cfg->regs;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_mchp_qspi_g1_configure(dev, config);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Drop stale received data and clear the overrun flag */
	while ((regs->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) != 0U) {
		(void)regs->QSPI_RXDATA;
	}
	regs->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;

	spi_context_cs_control(ctx, true);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t len = spi_context_max_continuous_chunk(ctx);

		spi_mchp_qspi_g1_shift(regs, spi_context_tx_buf_on(ctx) ? ctx->tx_buf : NULL,
				       spi_context_rx_buf_on(ctx) ? ctx->rx_buf : NULL, len);
		spi_context_update_tx(ctx, 1, len);
		spi_context_update_rx(ctx, 1, len);
	}

	if (!spi_cs_is_gpio(config) && ((config->operation & SPI_HOLD_ON_CS) == 0U)) {
		spi_mchp_qspi_g1_release_cs(regs);
	}

	spi_context_cs_control(ctx, false);

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_mchp_qspi_g1_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;

	if (!spi_cs_is_gpio(config) && spi_context_configured(&data->ctx, config)) {
		spi_mchp_qspi_g1_release_cs(cfg->regs);
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mchp_qspi_g1_init(const struct device *dev)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	int ret;

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_apb_sys);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_ahb_sys);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* The reset leaves the controller disabled, in SPI mode */
	cfg->regs->QSPI_CTRLA = QSPI_CTRLA_SWRST_Msk;

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_mchp_qspi_g1_driver_api) = {
	.transceive = spi_mchp_qspi_g1_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mchp_qspi_g1_release,
};

#define SPI_MCHP_QSPI_G1_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct spi_mchp_qspi_g1_config spi_mchp_qspi_g1_config_##n = {                \
		.regs = (qspi_registers_t *)DT_INST_REG_ADDR(n),                                   \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_apb_sys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_apb,   \
										    subsystem),    \
		.mclk_ahb_sys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_ahb,   \
										    subsystem),    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	static struct spi_mchp_qspi_g1_data spi_mchp_qspi_g1_data_##n = {                          \
		SPI_CONTEXT_INIT_LOCK(spi_mchp_qspi_g1_data_##n, ctx),                             \
		SPI_CONTEXT_INIT_SYNC(spi_mchp_qspi_g1_data_##n, ctx),                             \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mchp_qspi_g1_init, NULL, &spi_mchp_qspi_g1_data_##n,      \
				  &spi_mchp_qspi_g1_config_##n, POST_KERNEL,                       \
				  CONFIG_SPI_INIT_PRIORITY, &spi_mchp_qspi_g1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_MCHP_QSPI_G1_INIT)
