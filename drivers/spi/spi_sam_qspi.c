/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * QSPI controller of the SAM E70/S70/V70/V71 and PIC32CZ CA70/MC70 in SPI
 * mode (QSPI_MR.SMM = 0): a single-line SPI controller with one hardware chip
 * select (QCS), used for example by the jedec,spi-nor flash driver.
 */

#define DT_DRV_COMPAT atmel_sam_qspi

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_sam_qspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

/*
 * The Atmel headers name the QSPI register block Qspi; the Microchip headers
 * (PIC32CZ CA70) use qspi_registers_t. Both define the _Msk names used here.
 */
#ifdef QSPI_REGS
typedef qspi_registers_t sam_qspi_regs_t;
#else
typedef Qspi sam_qspi_regs_t;
#endif

/* QSPI_SCR.SCBR divides the peripheral clock by 1 to 256 */
#define SAM_QSPI_DIV_MAX 256U

struct spi_sam_qspi_config {
	sam_qspi_regs_t *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
};

struct spi_sam_qspi_data {
	struct spi_context ctx;
};

static int spi_sam_qspi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	sam_qspi_regs_t *regs = cfg->regs;
	uint32_t mck_hz;
	uint32_t divisor;
	uint32_t mr;
	uint32_t scr = 0U;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if ((config->operation & (SPI_HALF_DUPLEX | SPI_TRANSFER_LSB | SPI_MODE_LOOP)) != 0U) {
		LOG_ERR("Half-duplex, LSB first and loopback are not supported");
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
		LOG_ERR("Only chip select 0 (QCS) is available");
		return -EINVAL;
	}

#ifdef SOC_ATMEL_SAM_MCK_FREQ_HZ
	mck_hz = SOC_ATMEL_SAM_MCK_FREQ_HZ;
#else
	/* SoCs without a fixed MCK rate (PIC32CZ CA70): ask the PMC. */
	int ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
					 (clock_control_subsys_t)&cfg->clock_cfg, &mck_hz);

	if (ret < 0) {
		return ret;
	}
#endif

	/* Use the requested or the next lower possible frequency */
	divisor = DIV_ROUND_UP(mck_hz, config->frequency);
	divisor = CLAMP(divisor, 1U, SAM_QSPI_DIV_MAX);
	scr |= QSPI_SCR_SCBR(divisor - 1U);

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		scr |= QSPI_SCR_CPOL_Msk;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		scr |= QSPI_SCR_CPHA_Msk;
	}

	/* SPI mode, 8-bit words, QCS held low until QSPI_CR.LASTXFER */
	mr = QSPI_MR_CSMODE_LASTXFER | QSPI_MR_NBBITS(0U);

	regs->QSPI_CR = QSPI_CR_QSPIDIS_Msk;
	while ((regs->QSPI_SR & QSPI_SR_QSPIENS_Msk) != 0U) {
	}

	regs->QSPI_MR = mr;
	regs->QSPI_SCR = scr;

	regs->QSPI_CR = QSPI_CR_QSPIEN_Msk;
	while ((regs->QSPI_SR & QSPI_SR_QSPIENS_Msk) == 0U) {
	}

	LOG_DBG("QSCK %u Hz", mck_hz / divisor);

	data->ctx.config = config;

	return 0;
}

/*
 * Shift len bytes, one at a time: the next byte is written only after the
 * previous one is received, so the single receive holding register cannot
 * overrun, whatever the interrupt latency.
 */
static void spi_sam_qspi_shift(sam_qspi_regs_t *regs, const uint8_t *tx_buf, uint8_t *rx_buf,
			       size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		uint8_t rx;

		regs->QSPI_TDR = (tx_buf != NULL) ? tx_buf[i] : 0U;

		while ((regs->QSPI_SR & QSPI_SR_RDRF_Msk) == 0U) {
		}

		rx = (uint8_t)regs->QSPI_RDR;
		if (rx_buf != NULL) {
			rx_buf[i] = rx;
		}
	}
}

static void spi_sam_qspi_release_cs(sam_qspi_regs_t *regs)
{
	regs->QSPI_CR = QSPI_CR_LASTXFER_Msk;
	while ((regs->QSPI_SR & QSPI_SR_TXEMPTY_Msk) == 0U) {
	}
}

static int spi_sam_qspi_transceive(const struct device *dev, const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	sam_qspi_regs_t *regs = cfg->regs;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_sam_qspi_configure(dev, config);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Drop stale received data and clear the overrun flag */
	while ((regs->QSPI_SR & QSPI_SR_RDRF_Msk) != 0U) {
		(void)regs->QSPI_RDR;
	}

	spi_context_cs_control(ctx, true);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t len = spi_context_max_continuous_chunk(ctx);

		spi_sam_qspi_shift(regs, spi_context_tx_buf_on(ctx) ? ctx->tx_buf : NULL,
				   spi_context_rx_buf_on(ctx) ? ctx->rx_buf : NULL, len);
		spi_context_update_tx(ctx, 1, len);
		spi_context_update_rx(ctx, 1, len);
	}

	if (!spi_cs_is_gpio(config) && ((config->operation & SPI_HOLD_ON_CS) == 0U)) {
		spi_sam_qspi_release_cs(regs);
	}

	spi_context_cs_control(ctx, false);

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_sam_qspi_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;

	if (!spi_cs_is_gpio(config)) {
		spi_sam_qspi_release_cs(cfg->regs);
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_sam_qspi_init(const struct device *dev)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	int ret;

	ret = clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* The reset leaves the controller disabled, in SPI mode */
	cfg->regs->QSPI_CR = QSPI_CR_SWRST_Msk;

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_sam_qspi_driver_api) = {
	.transceive = spi_sam_qspi_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_sam_qspi_release,
};

#define SPI_SAM_QSPI_INIT(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct spi_sam_qspi_config spi_sam_qspi_config_##n = {                        \
		.regs = (sam_qspi_regs_t *)DT_INST_REG_ADDR_BY_NAME(n, qspi),                      \
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	static struct spi_sam_qspi_data spi_sam_qspi_data_##n = {                                  \
		SPI_CONTEXT_INIT_LOCK(spi_sam_qspi_data_##n, ctx),                                 \
		SPI_CONTEXT_INIT_SYNC(spi_sam_qspi_data_##n, ctx),                                 \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_sam_qspi_init, NULL, &spi_sam_qspi_data_##n,              \
				  &spi_sam_qspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, \
				  &spi_sam_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM_QSPI_INIT)
