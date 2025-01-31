/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi_common, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

int spi_mcux_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

int spi_mcux_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	lpspi_master_config_t master_config;
	uint32_t clock_freq;
	int ret;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		/* the IP DOES support half duplex, need to implement driver support */
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (word_size < 8 || (word_size % 32 == 1)) {
		/* Zephyr word size == hardware FRAME size (not word size)
		 * Max frame size: 4096 bits
		 *   (zephyr field is 6 bit wide for max 64 bit size, no need to check)
		 * Min frame size: 8 bits.
		 * Minimum hardware word size is 2. Since this driver is intended to work
		 * for 32 bit platforms, and 64 bits is max size, then only 33 and 1 are invalid.
		 */
		LOG_ERR("Word size %d not allowed", word_size);
		return -EINVAL;
	}

	if (spi_cfg->slave > LPSPI_CHIP_SELECT_COUNT) {
		LOG_ERR("Peripheral %d select exceeds max %d", spi_cfg->slave,
			LPSPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq);
	if (ret) {
		return ret;
	}

	if (data->ctx.config != NULL) {
		/* Setting the baud rate in LPSPI_MasterInit requires module to be disabled. Only
		 * disable if already configured, otherwise the clock is not enabled and the
		 * CR register cannot be written.
		 */
		LPSPI_Enable(base, false);
		while ((base->CR & LPSPI_CR_MEN_MASK) != 0U) {
			/* Wait until LPSPI is disabled. Datasheet:
			 * After writing 0, MEN (Module Enable) remains set until the LPSPI has
			 * completed the current transfer and is idle.
			 */
		}
	}

	data->ctx.config = spi_cfg;

	LPSPI_MasterGetDefaultConfig(&master_config);

	master_config.bitsPerFrame = word_size;
	master_config.cpol = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
				     ? kLPSPI_ClockPolarityActiveLow
				     : kLPSPI_ClockPolarityActiveHigh;
	master_config.cpha = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
				     ? kLPSPI_ClockPhaseSecondEdge
				     : kLPSPI_ClockPhaseFirstEdge;
	master_config.direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB) ? kLPSPI_LsbFirst : kLPSPI_MsbFirst;
	master_config.baudRate = spi_cfg->frequency;
	master_config.pcsToSckDelayInNanoSec = config->pcs_sck_delay;
	master_config.lastSckToPcsDelayInNanoSec = config->sck_pcs_delay;
	master_config.betweenTransferDelayInNanoSec = config->transfer_delay;
	master_config.pinCfg = config->data_pin_config;
	master_config.dataOutConfig = config->output_config ? kLpspiDataOutTristate :
							      kLpspiDataOutRetained;

	LPSPI_MasterInit(base, &master_config, clock_freq);
	LPSPI_SetDummyData(base, 0);

	if (IS_ENABLED(CONFIG_DEBUG)) {
		base->CR |= LPSPI_CR_DBGEN_MASK;
	}

	return 0;
}

int spi_nxp_init_common(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	data->dev = dev;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	config->irq_config_func(dev);

	return err;
}
