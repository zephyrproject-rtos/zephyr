/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_ambiq);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <stdlib.h>
#include <errno.h>
#include "spi_context.h"
#include <am_mcu_apollo.h>

#define PWRCTRL_MAX_WAIT_US 5

typedef int (*ambiq_spi_pwr_func_t)(void);

struct spi_ambiq_config {
	uint32_t base;
	int size;
	uint32_t clock_freq;
	const struct pinctrl_dev_config *pcfg;
	ambiq_spi_pwr_func_t pwr_func;
};

struct spi_ambiq_data {
	struct spi_context ctx;
	am_hal_iom_config_t iom_cfg;
	void *IOMHandle;
};

#define SPI_BASE      (((const struct spi_ambiq_config *)(dev)->config)->base)
#define REG_STAT      0x248
#define IDLE_STAT     0x4
#define SPI_STAT(dev) (SPI_BASE + REG_STAT)
#define SPI_WORD_SIZE 8

static int spi_config(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	const struct spi_ambiq_config *cfg = dev->config;
	struct spi_context *ctx = &(data->ctx);

	data->iom_cfg.eInterfaceMode = AM_HAL_IOM_SPI_MODE;

	int ret = 0;

	if (spi_context_configured(ctx, config)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_CPOL) {
		if (config->operation & SPI_MODE_CPHA) {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_3;
		} else {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_2;
		}
	} else {
		if (config->operation & SPI_MODE_CPHA) {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_1;
		} else {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_0;
		}
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}
	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode not supported");
		return -ENOTSUP;
	}

	if (cfg->clock_freq > AM_HAL_IOM_MAX_FREQ) {
		LOG_ERR("Clock frequency too high");
		return -ENOTSUP;
	}

	data->iom_cfg.ui32ClockFreq = cfg->clock_freq;
	ctx->config = config;

	/* Disable IOM instance as it cannot be configured when enabled*/
	ret = am_hal_iom_disable(data->IOMHandle);

	ret = am_hal_iom_configure(data->IOMHandle, &data->iom_cfg);

	ret = am_hal_iom_enable(data->IOMHandle);

	return ret;
}

static int spi_ambiq_xfer(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	am_hal_iom_transfer_t trans = {0};

	if (ctx->tx_len) {
		trans.ui64Instr = *ctx->tx_buf;
		trans.ui32InstrLen = 1;
		spi_context_update_tx(ctx, 1, 1);

		if (ctx->rx_buf != NULL) {
			if (ctx->tx_len > 0) {
				/* The instruction length can only be 0~5. */
				if (ctx->tx_len > 4) {
					spi_context_complete(ctx, dev, 0);
					return -ENOTSUP;
				}

				/* Put the remaining TX data in instruction. */
				trans.ui32InstrLen += ctx->tx_len;
				for (int i = 0; i < trans.ui32InstrLen - 1; i++) {
					trans.ui64Instr = (trans.ui64Instr << 8) | (*ctx->tx_buf);
					spi_context_update_tx(ctx, 1, 1);
				}
			}

			/* Set RX direction and hold CS to continue to receive data. */
			trans.eDirection = AM_HAL_IOM_RX;
			trans.bContinue = true;
			trans.pui32RxBuffer = (uint32_t *)ctx->rx_buf;
			trans.ui32NumBytes = ctx->rx_len;
			ret = am_hal_iom_blocking_transfer(data->IOMHandle, &trans);
		} else if (ctx->tx_buf != NULL) {
			/* Set TX direction to send data and release CS after transmission. */
			trans.eDirection = AM_HAL_IOM_TX;
			trans.bContinue = false;
			trans.ui32NumBytes = ctx->tx_len;
			trans.pui32TxBuffer = (uint32_t *)ctx->tx_buf;
			ret = am_hal_iom_blocking_transfer(data->IOMHandle, &trans);
		}
	} else {
		/* Set RX direction to receive data and release CS after transmission. */
		trans.ui64Instr = 0;
		trans.ui32InstrLen = 0;
		trans.eDirection = AM_HAL_IOM_RX;
		trans.bContinue = false;
		trans.pui32RxBuffer = (uint32_t *)ctx->rx_buf;
		trans.ui32NumBytes = ctx->rx_len;
		ret = am_hal_iom_blocking_transfer(data->IOMHandle, &trans);
	}

	spi_context_complete(ctx, dev, 0);

	return ret;
}

static int spi_ambiq_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_ambiq_data *data = dev->data;
	int ret;

	ret = spi_config(dev, config);

	if (ret) {
		return ret;
	}

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	ret = spi_ambiq_xfer(dev, config);

	return ret;
}

static int spi_ambiq_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;

	if (!sys_read32(SPI_STAT(dev))) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct spi_driver_api spi_ambiq_driver_api = {
	.transceive = spi_ambiq_transceive,
	.release = spi_ambiq_release,
};

static int spi_ambiq_init(const struct device *dev)
{
	struct spi_ambiq_data *data = dev->data;
	const struct spi_ambiq_config *cfg = dev->config;
	int ret;

	ret = am_hal_iom_initialize((cfg->base - REG_IOM_BASEADDR) / cfg->size, &data->IOMHandle);

	ret = cfg->pwr_func();

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	return ret;
}

#define AMBIQ_SPI_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_spi_##n(void)                                                      \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static struct spi_ambiq_data spi_ambiq_data##n = {                                         \
		SPI_CONTEXT_INIT_LOCK(spi_ambiq_data##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_ambiq_data##n, ctx)};                                    \
	static const struct spi_ambiq_config spi_ambiq_config##n = {                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.pwr_func = pwr_on_ambiq_spi_##n};                                                 \
	DEVICE_DT_INST_DEFINE(n, spi_ambiq_init, NULL, &spi_ambiq_data##n, &spi_ambiq_config##n,   \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_SPI_INIT)
