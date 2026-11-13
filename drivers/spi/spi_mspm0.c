/*
 * Copyright (c) 2026 Bang & Olufsen A/S, Denmark
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_spi

#include <errno.h>
#include <soc.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(spi_mspm0, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* SPI registers */
#define MSPM0_SPI_PWREN  0x800
#define MSPM0_SPI_CLKDIV 0x1000
#define MSPM0_SPI_CLKSEL 0x1004
#define MSPM0_SPI_CTL0   0x1100
#define MSPM0_SPI_CTL1   0x1104
#define MSPM0_SPI_CLKCTL 0x1108
#define MSPM0_SPI_STAT   0x1110
#define MSPM0_SPI_RXDATA 0x1130
#define MSPM0_SPI_TXDATA 0x1140

/* Power enable*/
#define MSPM0_SPI_PWREN_KEY         (0x26U << 24)
#define MSPM0_SPI_PWREN_ENABLE_MASK BIT(0)

/* Control register-0 */
#define MSPM0_SPI_CTL0_DSS_MASK GENMASK(4, 0)
#define MSPM0_SPI_CTL0_FRF_MASK GENMASK(6, 5)
#define MSPM0_SPI_CTL0_SPO_MASK BIT(8)
#define MSPM0_SPI_CTL0_SPH_MASK BIT(9)

/* Frame format selection */
#define MSPM0_SPI_CTL0_FRF_MOTOROLA_4WIRE FIELD_PREP(MSPM0_SPI_CTL0_FRF_MASK, 1)
#define MSPM0_SPI_CTL0_FRF_TI             FIELD_PREP(MSPM0_SPI_CTL0_FRF_MASK, 2)

/* Control register-1 */
#define MSPM0_SPI_CTL1_ENABLE_MASK BIT(0)
#define MSPM0_SPI_CTL1_LBM_MASK    BIT(1)
#define MSPM0_SPI_CTL1_CP_MASK     BIT(2)
#define MSPM0_SPI_CTL1_MSB_MASK    BIT(4)

/* Clock control */
#define MSPM0_SPI_CLKCTL_SCR_MASK GENMASK(9, 0)
#define MSPM0_SPI_SCR_MIN         0
#define MSPM0_SPI_SCR_MAX         1023

/* Status register*/
#define MSPM0_SPI_STAT_TNF_MASK  BIT(1) /* TX FIFO not full */
#define MSPM0_SPI_STAT_RFE_MASK  BIT(2) /* RX FIFO empty */
#define MSPM0_SPI_STAT_BUSY_MASK BIT(4) /* Transfer status */

#define MSPM0_SPI_CLKDIV_RATIO   0U
#define MSPM0_SPI_MAX_FIFO_DEPTH 4U
#define MSPM0_SPI_CTL1_ENABLE    FIELD_PREP(MSPM0_SPI_CTL1_ENABLE_MASK, 1)
#define MSPM0_SPI_CTL1_DISABLE   FIELD_PREP(MSPM0_SPI_CTL1_ENABLE_MASK, 0)
#define MSPM0_SPI_DSS_MIN        4
#define MSPM0_SPI_DSS_MAX        16

/* Computes the minimum number of bytes per word using word_size. */
#define BYTES_PER_WORD(word_size) (((word_size) + 7) / 8)

/* Helper macros */
#define SPI_UPDATE_REG(reg_addr, value, mask)                                                      \
	{                                                                                          \
		uint32_t tmp = sys_read32(reg_addr);                                               \
		tmp = (tmp & ~(mask)) | ((value) & (mask));                                        \
		sys_write32(tmp, reg_addr);                                                        \
	}

struct spi_mspm0_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct mspm0_sys_clock clk_subsys;
};

struct spi_mspm0_data {
	struct spi_context spi_ctx;
	uint8_t bytes_per_word;
	size_t words_in_fifo;
	size_t words_left;
};

static void spi_mspm0_tx_fill(uint32_t base, struct spi_mspm0_data *data)
{
	struct spi_context *ctx = &data->spi_ctx;
	uint32_t tx_data;

	while (data->words_left && (sys_read32(base + MSPM0_SPI_STAT) & MSPM0_SPI_STAT_TNF_MASK) &&
	       (data->words_in_fifo < MSPM0_SPI_MAX_FIFO_DEPTH)) {

		tx_data = 0;
		if (spi_context_tx_buf_on(ctx)) {
			memcpy(&tx_data, ctx->tx_buf, data->bytes_per_word);
		}
		sys_write32(tx_data, base + MSPM0_SPI_TXDATA);
		spi_context_update_tx(ctx, data->bytes_per_word, 1);
		data->words_in_fifo++;
		data->words_left--;
	}
}

static void spi_mspm0_rx_drain(uint32_t base, struct spi_mspm0_data *data)
{
	struct spi_context *ctx = &data->spi_ctx;
	uint32_t rx_data;

	while (data->words_in_fifo &&
	       (!(sys_read32(base + MSPM0_SPI_STAT) & MSPM0_SPI_STAT_RFE_MASK))) {

		rx_data = sys_read32(base + MSPM0_SPI_RXDATA);
		if (spi_context_rx_buf_on(ctx)) {
			memcpy(ctx->rx_buf, &rx_data, data->bytes_per_word);
		}
		spi_context_update_rx(ctx, data->bytes_per_word, 1);
		data->words_in_fifo--;
	}
}

static void spi_mspm0_finish(const struct device *dev, int status)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;

	while (sys_read32(cfg->base + MSPM0_SPI_STAT) & MSPM0_SPI_STAT_BUSY_MASK) {
		/* Wait until transfer is completed */
	}

	spi_context_cs_control(&data->spi_ctx, false);
	spi_context_complete(&data->spi_ctx, dev, status);
}

static void spi_mspm0_pretransfer_clear(uint32_t base, struct spi_mspm0_data *data)
{

	while (!(sys_read32(base + MSPM0_SPI_STAT) & MSPM0_SPI_STAT_RFE_MASK)) {
		(void)sys_read32(base + MSPM0_SPI_RXDATA); /* drain leftover RX words */
	}

	data->words_in_fifo = 0;
}

static int spi_mspm0_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;
	uint32_t ctl0 = 0;
	uint32_t ctl1 = MSPM0_SPI_CTL1_CP_MASK; /* controller mode */
	uint32_t clock_rate;
	uint32_t scr = 0;
	int ret;

	if (spi_context_configured(&data->spi_ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER) {
		return -ENOTSUP; /* Only controller mode is supported */
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		return -ENOTSUP;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		return -ENOTSUP; /* Does not support half-duplex mode */
	}

	if (!IN_RANGE(SPI_WORD_SIZE_GET(config->operation), MSPM0_SPI_DSS_MIN, MSPM0_SPI_DSS_MAX)) {
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->clk_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	if (config->frequency == 0 || config->frequency > clock_rate / 2) {
		return -EINVAL;
	}

	scr = DIV_ROUND_UP(clock_rate, 2 * config->frequency) - 1;

	if (!IN_RANGE(scr, MSPM0_SPI_SCR_MIN, MSPM0_SPI_SCR_MAX)) {
		return -EINVAL;
	}

	if (config->operation & SPI_FRAME_FORMAT_TI) {
		ctl0 |= MSPM0_SPI_CTL0_FRF_TI;
	} else { /* CPOL/CPHA only supported in Motorola frame format */
		ctl0 |= MSPM0_SPI_CTL0_FRF_MOTOROLA_4WIRE;
		ctl0 |= (config->operation & SPI_MODE_CPOL) ? MSPM0_SPI_CTL0_SPO_MASK : 0;
		ctl0 |= (config->operation & SPI_MODE_CPHA) ? MSPM0_SPI_CTL0_SPH_MASK : 0;
	}

	data->bytes_per_word = BYTES_PER_WORD(SPI_WORD_SIZE_GET(config->operation));

	ctl0 |= FIELD_PREP(MSPM0_SPI_CTL0_DSS_MASK, (SPI_WORD_SIZE_GET(config->operation) - 1));
	ctl1 |= (config->operation & SPI_TRANSFER_LSB) ? 0 : MSPM0_SPI_CTL1_MSB_MASK;
	ctl1 |= (config->operation & SPI_MODE_LOOP) ? MSPM0_SPI_CTL1_LBM_MASK : 0;

	SPI_UPDATE_REG(cfg->base + MSPM0_SPI_CTL1, MSPM0_SPI_CTL1_DISABLE,
		       MSPM0_SPI_CTL1_ENABLE_MASK)

	SPI_UPDATE_REG(cfg->base + MSPM0_SPI_CTL0, ctl0,
		       MSPM0_SPI_CTL0_DSS_MASK | MSPM0_SPI_CTL0_FRF_MASK | MSPM0_SPI_CTL0_SPO_MASK |
			       MSPM0_SPI_CTL0_SPH_MASK)

	SPI_UPDATE_REG(cfg->base + MSPM0_SPI_CTL1, ctl1,
		       MSPM0_SPI_CTL1_LBM_MASK | MSPM0_SPI_CTL1_CP_MASK | MSPM0_SPI_CTL1_MSB_MASK)

	SPI_UPDATE_REG(cfg->base + MSPM0_SPI_CLKCTL, FIELD_PREP(MSPM0_SPI_CLKCTL_SCR_MASK, scr),
		       MSPM0_SPI_CLKCTL_SCR_MASK)

	SPI_UPDATE_REG(cfg->base + MSPM0_SPI_CTL1, MSPM0_SPI_CTL1_ENABLE,
		       MSPM0_SPI_CTL1_ENABLE_MASK)
	data->spi_ctx.config = config;

	return 0;
}

static void spi_mspm0_transceive_sync(const struct device *dev)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;

	while (data->words_in_fifo || data->words_left) {
		spi_mspm0_rx_drain(cfg->base, data);
		spi_mspm0_tx_fill(cfg->base, data);
	}

	spi_mspm0_finish(dev, 0U);
}

static int spi_mspm0_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;
	struct spi_context *ctx = &data->spi_ctx;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return -EINVAL;
	}

	spi_context_lock(ctx, false, NULL, NULL, config);
	ret = spi_mspm0_configure(dev, config);
	if (ret < 0) {
		goto exit;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->bytes_per_word);
	data->words_left = MAX(spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx)) /
			   data->bytes_per_word;

	if (!data->words_left) {
		ret = 0; /* zero-length buffer handling */
		goto exit;
	}

	spi_mspm0_pretransfer_clear(cfg->base, data);
	spi_context_cs_control(ctx, true);
	spi_mspm0_transceive_sync(dev);
	ret = spi_context_wait_for_completion(ctx);
exit:
	spi_context_release(ctx, ret);
	return ret;
}

static int spi_mspm0_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_mspm0_data *data = dev->data;

	if (!spi_context_configured(&data->spi_ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->spi_ctx);

	return 0;
}

static DEVICE_API(spi, spi_mspm0_api) = {
	.transceive = spi_mspm0_transceive,
	.release = spi_mspm0_release,
};

static int spi_mspm0_init(const struct device *dev)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->clk_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	sys_write32(MSPM0_SPI_PWREN_KEY | MSPM0_SPI_PWREN_ENABLE_MASK, cfg->base + MSPM0_SPI_PWREN);
	msp_delay_peripheral_startup();

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->spi_ctx);
	if (ret < 0) {
		return ret;
	}

	/* Select the functional clock source given by devicetree with no division */
	sys_write32(MSPM0_SPI_CLKDIV_RATIO, cfg->base + MSPM0_SPI_CLKDIV);
	sys_write32(MSPM0_CLOCK_PERIPH_REG_MASK(cfg->clk_subsys.clk), cfg->base + MSPM0_SPI_CLKSEL);
	spi_context_unlock_unconditionally(&data->spi_ctx);

	return 0;
}

#define MSPM0_SPI_INIT(inst)                                                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct spi_mspm0_config spi_mspm0_cfg_##inst = {                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.base = (uint32_t)DT_INST_REG_ADDR(inst),                                          \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.clk_subsys = MSPM0_CLOCK_SUBSYS_FN(inst),                                         \
	};                                                                                         \
                                                                                                   \
	static struct spi_mspm0_data spi_mspm0_data_##inst = {                                     \
		SPI_CONTEXT_INIT_LOCK(spi_mspm0_data_##inst, spi_ctx),                             \
		SPI_CONTEXT_INIT_SYNC(spi_mspm0_data_##inst, spi_ctx),                             \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), spi_ctx)};                      \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(inst, spi_mspm0_init, NULL, &spi_mspm0_data_##inst,              \
				  &spi_mspm0_cfg_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,    \
				  &spi_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_SPI_INIT)
