/*
 * Copyright (c) 2026 Gabriel Ivo <gabriel.bozi@usp.br>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_l_spi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_nxp_kinetis_l, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#include <fsl_device_registers.h>

#define SPI_TIMEOUT_RETRIES 100000

struct spi_nxp_kinetis_l_config {
	SPI_Type *base;
	const struct pinctrl_dev_config *pin_config;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct spi_nxp_kinetis_l_data {
	struct spi_context context;
};

static int transfer_byte(SPI_Type *base, uint8_t tx)
{
	uint32_t retries = SPI_TIMEOUT_RETRIES;

	while (!(base->S & SPI_S_SPTEF_MASK)) {
		if (--retries == 0) {
			return -ETIMEDOUT;
		}
	}
	base->D = tx;

	retries = SPI_TIMEOUT_RETRIES;

	while (!(base->S & SPI_S_SPRF_MASK)) {
		if (--retries == 0) {
			return -ETIMEDOUT;
		}
	}
	return base->D;
}

static int spi_nxp_kinetis_l_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_nxp_kinetis_l_config *cfg = dev->config;
	SPI_Type *base = cfg->base;
	uint32_t clock_frequency;

	if (clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &clock_frequency) < 0) {
		LOG_ERR("Failed to get SPI clock frequency");
		return -EINVAL;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("word size %d not supported, only 8-bit",
			SPI_WORD_SIZE_GET(config->operation));
		return -ENOTSUP;
	}

	base->C1 &= ~SPI_C1_SPE_MASK;

	base->C1 = SPI_C1_MSTR_MASK;

	if (config->operation & SPI_TRANSFER_LSB) {
		base->C1 |= SPI_C1_LSBFE_MASK;
	}

	if (config->operation & SPI_MODE_CPOL) {
		base->C1 |= SPI_C1_CPOL_MASK;
	}

	if (config->operation & SPI_MODE_CPHA) {
		base->C1 |= SPI_C1_CPHA_MASK;
	}

	uint32_t best_diff = UINT32_MAX;
	uint8_t best_sppr = 0, best_spr = 0;

	for (int spr = 0; spr <= 8; spr++) {
		for (int sppr = 0; sppr <= 7; sppr++) {
			/* baud_rate_divisor = (SPPR + 1) * 2^(SPR + 1) */
			uint32_t divisor = (sppr + 1) * (1U << (spr + 1));
			uint32_t calc_frequency = clock_frequency / divisor;

			if (calc_frequency <= config->frequency) {
				uint32_t diff = config->frequency - calc_frequency;

				if (diff < best_diff) {
					best_diff = diff;
					best_sppr = sppr;
					best_spr = spr;
				}
			}
		}
	}

	base->BR = SPI_BR_SPPR(best_sppr) | SPI_BR_SPR(best_spr);

	base->C1 |= SPI_C1_SPE_MASK;

	return 0;
}

static int spi_nxp_kinetis_l_transceive(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	const struct spi_nxp_kinetis_l_config *cfg = dev->config;
	struct spi_nxp_kinetis_l_data *data = dev->data;
	SPI_Type *base = cfg->base;
	int error = 0;

	spi_context_lock(&data->context, false, NULL, NULL, config);

	if (!spi_context_configured(&data->context, config)) {
		error = spi_nxp_kinetis_l_configure(dev, config);

		if (error < 0) {
			spi_context_release(&data->context, error);
			return error;
		}
	}

	data->context.config = config;

	spi_context_buffers_setup(&data->context, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->context, true);

	while (spi_context_tx_buf_on(&data->context) || spi_context_rx_buf_on(&data->context)) {
		uint8_t tx_val = 0x00;
		uint8_t rx_val;

		if (spi_context_tx_buf_on(&data->context) && data->context.tx_buf != NULL) {
			tx_val = *data->context.tx_buf;
		}

		int ret = transfer_byte(base, tx_val);

		if (ret < 0) {
			error = ret;
			break;
		}
		rx_val = (uint8_t)ret;

		if (spi_context_rx_buf_on(&data->context) && data->context.rx_buf != NULL) {
			*data->context.rx_buf = rx_val;
		}

		spi_context_update_tx(&data->context, 1, 1);
		spi_context_update_rx(&data->context, 1, 1);
	}

	spi_context_cs_control(&data->context, false);

	spi_context_release(&data->context, 0);

	return error;
}

static int spi_nxp_kinetis_l_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_nxp_kinetis_l_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->context);

	return 0;
}

static DEVICE_API(spi, spi_nxp_kinetis_l_api) = {
	.transceive = spi_nxp_kinetis_l_transceive,
	.release = spi_nxp_kinetis_l_release,
};

static int spi_nxp_kinetis_l_init(const struct device *dev)
{
	const struct spi_nxp_kinetis_l_config *dev_config = dev->config;
	struct spi_nxp_kinetis_l_data *data = dev->data;
	int error;

	if (!device_is_ready(dev_config->clock_dev)) {
		return -ENODEV;
	}

	error = clock_control_on(dev_config->clock_dev, dev_config->clock_subsys);
	if (error < 0) {
		return error;
	}

	error = pinctrl_apply_state(dev_config->pin_config, PINCTRL_STATE_DEFAULT);
	if (error < 0) {
		return error;
	}

	error = spi_context_cs_configure_all(&data->context);
	if (error < 0) {
		return error;
	}

	spi_context_unlock_unconditionally(&data->context);

	return 0;
}

#define SPI_NXP_KINETIS_L_INIT(inst)                                                               \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct spi_nxp_kinetis_l_config spi_nxp_kinetis_l_config_##inst = {           \
		.base = (SPI_Type *)DT_INST_REG_ADDR(inst),                                        \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),           \
	};                                                                                         \
	static struct spi_nxp_kinetis_l_data spi_nxp_kinetis_l_data_##inst = {                     \
		SPI_CONTEXT_INIT_LOCK(spi_nxp_kinetis_l_data_##inst, context),                     \
		SPI_CONTEXT_INIT_SYNC(spi_nxp_kinetis_l_data_##inst, context),                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), context)};                      \
	DEVICE_DT_INST_DEFINE(inst, spi_nxp_kinetis_l_init, NULL, &spi_nxp_kinetis_l_data_##inst,  \
			      &spi_nxp_kinetis_l_config_##inst, POST_KERNEL,                       \
			      CONFIG_SPI_INIT_PRIORITY, &spi_nxp_kinetis_l_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_NXP_KINETIS_L_INIT)
