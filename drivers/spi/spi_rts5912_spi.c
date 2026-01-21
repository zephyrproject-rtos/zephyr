/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_spi

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rts5912_clock.h>
#include "reg/reg_spi.h"

#define RTS5912_SPI_TIMEOUT_ROUND              100
#define RTS5912_SPI_TX_FIFO_LIMIT              128
#define RTS5912_SPI_ADDR_NUM                   0x07
#define RTS5912_SPI_FREQUENCY_DEFAULT          22 /* 3.84MHz */
#define RTS5912_SPI_FREQUENCY_REGISTER_MAXIMUM 0xFFFFFFFF
#define RTS5912_SPI_FREQUENCY_BUS_MAXIMUM      (50000000ul)
#define RTS5912_SPI_FREQUENCY_BUS_MINIMUM      (15000ul)
#define RTS5912_SPI_ADDR_ONLY_MODE             0
#define RTS5912_SPI_ADDR_AND_DATA_MODE         2

LOG_MODULE_REGISTER(spi_rts5912_spi, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

struct spi_rts5912_config {
	volatile struct spi_reg *const spi_reg_base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
};

struct spi_rts5912_data {
	struct spi_context ctx;
	size_t transfer_len;
	size_t receive_len;
	uint32_t spi_input_clock_rate;
};

static int spi_rts5912_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_rts5912_config *spi_config = dev->config;
	struct spi_rts5912_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	volatile struct spi_reg *const spi = spi_config->spi_reg_base;

	if (spi_cfg->slave > 1) {
		LOG_ERR("Slave %d is greater than 1", spi_cfg->slave);
		return -EINVAL;
	}

	LOG_DBG("chip select: %d, operation: 0x%x", spi_cfg->slave, spi_cfg->operation);

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("Unsupported SPI slave mode");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		LOG_ERR("Unsupported loopback mode");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		LOG_ERR("Unsupported cpha mode");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		LOG_ERR("Unsupported cpol mode");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	if (ctx->rx_len > 0) {
		LOG_ERR("Can't support Pure RX");
		return -EINVAL;
	}

	if (spi_cfg->frequency < RTS5912_SPI_FREQUENCY_BUS_MINIMUM) {
		LOG_ERR("Can't support frequency %d", spi_cfg->frequency);
		return -EINVAL;
	}

	ctx->config = spi_cfg;

	spi->CTRL |= RTS5912_SPI_CTRL_RST_MASK;
	spi->CTRL &= ~RTS5912_SPI_CTRL_MODE_MASK;
	spi->CTRL |= RTS5912_SPI_CTRL_TRANSEL_MASK;

	spi->CMDL = 0x00;                 /* cmd mode setting */
	spi->CMDN = RTS5912_SPI_ADDR_NUM; /* 7+1bit = 1Byte CMD */
	spi->ADDR = 0x0;
	spi->ADDRN = RTS5912_SPI_ADDR_NUM;
	if (spi_cfg->frequency < RTS5912_SPI_FREQUENCY_BUS_MAXIMUM) {
		spi->CKDV = ((data->spi_input_clock_rate / 2) / spi_cfg->frequency) - 1;
	} else {
		spi->CKDV = 0;
	}
	spi->CTRL |= RTS5912_SPI_CTRL_RST_MASK;

	return 0;
}

static inline bool spi_rts5912_transfer_done(struct spi_context *ctx)
{
	return !spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx);
}

static void spi_rts5912_complete(const struct device *dev, const int status)
{
	struct spi_rts5912_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_complete(ctx, dev, status);
	if (spi_cs_is_gpio(ctx->config)) {
		spi_context_cs_control(ctx, false);
	}
	pm_device_busy_clear(dev);
}

static inline void rts5912_spi_tx(const struct device *dev)
{
	const struct spi_rts5912_config *spi_config = dev->config;
	struct spi_rts5912_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	volatile struct spi_reg *const spi = spi_config->spi_reg_base;
	uint32_t timeout_cnt = 0;

	if (ctx->tx_len == 1) {
		spi->TRSF = ((spi->TRSF & ~RTS5912_SPI_TRSF_MODE_MASK) |
			     (RTS5912_SPI_ADDR_ONLY_MODE & RTS5912_SPI_TRSF_MODE_MASK));
	} else {
		spi->TRSF = ((spi->TRSF & ~RTS5912_SPI_TRSF_MODE_MASK) |
			     (RTS5912_SPI_ADDR_AND_DATA_MODE & RTS5912_SPI_TRSF_MODE_MASK));
	}

	spi->CTRL |= RTS5912_SPI_CTRL_RST_MASK;
	spi->LEN = ctx->tx_len - 1;
	spi->CMDL = ctx->tx_buf[0];

	for (int i = 1; i < ctx->tx_len; i++) {
		spi->TX = ctx->tx_buf[i];
	}
	spi->TRSF |= RTS5912_SPI_TRSF_START_MASK;
	while ((!(spi->TRSF & RTS5912_SPI_TRSF_END_MASK)) &&
	       (timeout_cnt < RTS5912_SPI_TIMEOUT_ROUND)) {
		k_msleep(10);
		timeout_cnt++;
	}
	spi->CTRL |= RTS5912_SPI_CTRL_RST_MASK;
	timeout_cnt = 0;
	while ((spi->CTRL & RTS5912_SPI_CTRL_RST_MASK) &&
	       (timeout_cnt < RTS5912_SPI_TIMEOUT_ROUND)) {
		k_msleep(10);
		timeout_cnt++;
	}
}

static int rts5912_spi_xfer(const struct device *dev)
{
	struct spi_rts5912_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (spi_cs_is_gpio(ctx->config)) {
		spi_context_cs_control(ctx, true);
	}

	if (spi_context_longest_current_buf(ctx) > RTS5912_SPI_TX_FIFO_LIMIT) {
		return -EINVAL;
	}

	rts5912_spi_tx(dev);

	return 0;
}

static int rts5912_spi_transceive(const struct device *dev, const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	struct spi_rts5912_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_rts5912_configure(dev, config);
	if (ret) {
		spi_context_release(ctx, ret);
		return ret;
	}

	pm_device_busy_set(dev);

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	do {
		ret = rts5912_spi_xfer(dev);

		if (!ret) {
			spi_context_update_tx(ctx, 1U, ctx->tx_len);
		}
	} while (!ret && !spi_rts5912_transfer_done(ctx));

	if (spi_rts5912_transfer_done(ctx)) {
		spi_rts5912_complete(dev, 0);
	}

	spi_context_release(ctx, ret);

	return ret;
}

static int rts5912_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_rts5912_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_rts5912_spi_init(const struct device *dev)
{
	const struct spi_rts5912_config *cfg = dev->config;
	struct spi_rts5912_data *data = dev->data;
	uint32_t pll_clock_rate = 0;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to set default pinctrl");
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret) {
		return ret;
	}

	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->sccon_cfg,
				     &pll_clock_rate);
	if (ret) {
		return ret;
	}

	data->spi_input_clock_rate = pll_clock_rate;
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_rts5912_driver_api) = {
	.transceive = rts5912_spi_transceive,
	.release = rts5912_spi_release,
};

#define SPI_rts5912_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct spi_rts5912_config spi_rts5912_cfg_##n = {                             \
		.spi_reg_base = (volatile struct spi_reg *const)DT_INST_REG_ADDR(n),               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.sccon_cfg = {                                                                     \
			.clk_grp = DT_INST_CLOCKS_CELL(n, clk_grp),                                \
			.clk_idx = DT_INST_CLOCKS_CELL(n, clk_idx),                                \
		}};                                                                                \
                                                                                                   \
	static struct spi_rts5912_data spi_rts5912_data_##n = {                                    \
		SPI_CONTEXT_INIT_LOCK(spi_rts5912_data_##n, ctx),                                  \
		SPI_CONTEXT_INIT_SYNC(spi_rts5912_data_##n, ctx),                                  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &spi_rts5912_spi_init, NULL, &spi_rts5912_data_##n,               \
			      &spi_rts5912_cfg_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_rts5912_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_rts5912_INIT)
