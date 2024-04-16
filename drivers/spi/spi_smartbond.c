/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_smartbond);

#include "spi_context.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>

#include <DA1469xAB.h>

#define DIVN_CLK	32000000	/* divN_clk 32MHz */
#define SCLK_FREQ_2MHZ	(DIVN_CLK / 14) /* 2.285714MHz*/
#define SCLK_FREQ_4MHZ	(DIVN_CLK / 8)	/* 4MHz */
#define SCLK_FREQ_8MHZ	(DIVN_CLK / 4)	/* 8MHz */
#define SCLK_FREQ_16MHZ (DIVN_CLK / 2)	/* 16MHz */

struct spi_smartbond_cfg {
	SPI_Type *regs;
	int periph_clock_config;
	const struct pinctrl_dev_config *pcfg;
};

struct spi_smartbond_data {
	struct spi_context ctx;
	uint8_t dfs;
};

static inline void spi_smartbond_enable(const struct spi_smartbond_cfg *cfg, bool enable)
{
	if (enable) {
		cfg->regs->SPI_CTRL_REG |= SPI_SPI_CTRL_REG_SPI_ON_Msk;
		cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_RST_Msk;
	} else {
		cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_ON_Msk;
		cfg->regs->SPI_CTRL_REG |= SPI_SPI_CTRL_REG_SPI_RST_Msk;
	}
}

static inline bool spi_smartbond_isenabled(const struct spi_smartbond_cfg *cfg)
{
	return (!!(cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_ON_Msk)) &&
	       (!(cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_RST_Msk));
}

static inline int spi_smartbond_set_speed(const struct spi_smartbond_cfg *cfg,
					  const uint32_t frequency)
{
	if (frequency < SCLK_FREQ_2MHZ) {
		LOG_ERR("Frequency is lower than minimal SCLK %d", SCLK_FREQ_2MHZ);
		return -ENOTSUP;
	} else if (frequency < SCLK_FREQ_4MHZ) {
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk) |
			3UL << SPI_SPI_CTRL_REG_SPI_CLK_Pos;
	} else if (frequency < SCLK_FREQ_8MHZ) {
		cfg->regs->SPI_CTRL_REG = (cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk);
	} else if (frequency < SCLK_FREQ_16MHZ) {
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk) |
			1UL << SPI_SPI_CTRL_REG_SPI_CLK_Pos;
	} else {
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk) |
			2UL << SPI_SPI_CTRL_REG_SPI_CLK_Pos;
	}
	return 0;
}

static inline int spi_smartbond_set_word_size(const struct spi_smartbond_cfg *cfg,
					      struct spi_smartbond_data *data,
					      const uint32_t operation)
{
	switch (SPI_WORD_SIZE_GET(operation)) {
	case 8:
		data->dfs = 1;
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_WORD_Msk);
		break;
	case 16:
		data->dfs = 2;
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_WORD_Msk) |
			(1UL << SPI_SPI_CTRL_REG_SPI_WORD_Pos);
		break;
	case 32:
		data->dfs = 4;
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_WORD_Msk) |
			(2UL << SPI_SPI_CTRL_REG_SPI_WORD_Pos);
		break;
	default:
		LOG_ERR("Word size not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int spi_smartbond_configure(const struct spi_smartbond_cfg *cfg,
				   struct spi_smartbond_data *data,
				   const struct spi_config *spi_cfg)
{
	int rc;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		return 0;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not yet supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	if (spi_smartbond_isenabled(cfg)) {
		spi_smartbond_enable(cfg, false);
	}

	rc = spi_smartbond_set_speed(cfg, spi_cfg->frequency);
	if (rc) {
		return rc;
	}

	cfg->regs->SPI_CTRL_REG =
		(spi_cfg->operation & SPI_MODE_CPOL)
			? (cfg->regs->SPI_CTRL_REG | SPI_SPI_CTRL_REG_SPI_POL_Msk)
			: (cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_POL_Msk);

	cfg->regs->SPI_CTRL_REG =
		(spi_cfg->operation & SPI_MODE_CPHA)
			? (cfg->regs->SPI_CTRL_REG | SPI_SPI_CTRL_REG_SPI_PHA_Msk)
			: (cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_PHA_Msk);

	rc = spi_smartbond_set_word_size(cfg, data, spi_cfg->operation);
	if (rc) {
		return rc;
	}

	cfg->regs->SPI_CTRL_REG &= ~(SPI_SPI_CTRL_REG_SPI_FIFO_MODE_Msk);

	spi_smartbond_enable(cfg, true);

	cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_MINT_Msk;

	data->ctx.config = spi_cfg;

	return 0;
}

static int spi_smartbond_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t bitmask;
	int rc;

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);
	rc = spi_smartbond_configure(cfg, data, spi_cfg);
	if (rc == 0) {
		spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);
		spi_context_cs_control(ctx, true);

		bitmask = ~((~0UL) << SPI_WORD_SIZE_GET(data->ctx.config->operation));
		while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
			if (spi_context_tx_buf_on(ctx)) {
				cfg->regs->SPI_RX_TX_REG = (*(uint32_t *)ctx->tx_buf) & bitmask;
				spi_context_update_tx(ctx, data->dfs, 1);
			} else {
				cfg->regs->SPI_RX_TX_REG = 0UL;
			}

			while (!(cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_INT_BIT_Msk)) {
			};
			if (spi_context_rx_buf_on(ctx)) {
				(*(uint32_t *)ctx->rx_buf) = cfg->regs->SPI_RX_TX_REG & bitmask;
				spi_context_update_rx(ctx, data->dfs, 1);
			} else {
				(void)cfg->regs->SPI_RX_TX_REG;
			}
			cfg->regs->SPI_CLEAR_INT_REG = 1UL;
		}
	}
	spi_context_cs_control(ctx, false);
	spi_context_release(&data->ctx, rc);

	return rc;
}
#ifdef CONFIG_SPI_ASYNC
static int spi_smartbond_transceive_async(const struct device *dev,
					  const struct spi_config *spi_cfg,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					  void *userdata)
{
	return -ENOTSUP;
}
#endif

static int spi_smartbond_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, spi_cfg)) {
		LOG_ERR("SPI configuration was not the last one to be used");
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static const struct spi_driver_api spi_smartbond_driver_api = {
	.transceive = spi_smartbond_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_smartbond_transceive_async,
#endif
	.release = spi_smartbond_release,
};

static int spi_smartbond_init(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;
	int rc;

	CRG_COM->RESET_CLK_COM_REG = cfg->periph_clock_config << 1;
	CRG_COM->SET_CLK_COM_REG = cfg->periph_clock_config;

	rc = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("Failed to configure SPI pins");
		return rc;
	}

	rc = spi_context_cs_configure_all(&data->ctx);
	if (rc < 0) {
		LOG_ERR("Failed to configure CS pins: %d", rc);
		return rc;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SPI_SMARTBOND_DEVICE(id)                                                                   \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static const struct spi_smartbond_cfg spi_smartbond_##id##_cfg = {                         \
		.regs = (SPI_Type *)DT_INST_REG_ADDR(id),                                          \
		.periph_clock_config = DT_INST_PROP(id, periph_clock_config),                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                        \
	};                                                                                         \
	static struct spi_smartbond_data spi_smartbond_##id##_data = {                             \
		SPI_CONTEXT_INIT_LOCK(spi_smartbond_##id##_data, ctx),                             \
		SPI_CONTEXT_INIT_SYNC(spi_smartbond_##id##_data, ctx),                             \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)};                            \
	DEVICE_DT_INST_DEFINE(id, spi_smartbond_init, NULL, &spi_smartbond_##id##_data,            \
			      &spi_smartbond_##id##_cfg, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,    \
			      &spi_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SMARTBOND_DEVICE)
