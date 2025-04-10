/*
 * Copyright 2018, 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_lpspi_common, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"
#include <fsl_lpspi.h>

#if defined(LPSPI_RSTS) || defined(LPSPI_CLOCKS)
static LPSPI_Type *const lpspi_bases[] = LPSPI_BASE_PTRS;
#endif

#ifdef LPSPI_RSTS
static const reset_ip_name_t lpspi_resets[] = LPSPI_RSTS;

static inline reset_ip_name_t lpspi_get_reset(LPSPI_Type *const base)
{
	reset_ip_name_t rst = -1; /* invalid initial value */

	ARRAY_FOR_EACH(lpspi_bases, idx) {
		if (lpspi_bases[idx] == base) {
			rst = lpspi_resets[idx];
			break;
		}
	}

	__ASSERT_NO_MSG(rst != -1);
	return rst;

}
#endif

#ifdef LPSPI_CLOCKS
static const clock_ip_name_t lpspi_clocks[] = LPSPI_CLOCKS;

static inline clock_ip_name_t lpspi_get_clock(LPSPI_Type *const base)
{
	clock_ip_name_t clk = -1; /* invalid initial value */

	ARRAY_FOR_EACH(lpspi_bases, idx) {
		if (lpspi_bases[idx] == base) {
			clk = lpspi_clocks[idx];
			break;
		}
	}

	__ASSERT_NO_MSG(clk != -1);
	return clk;
}
#endif

void lpspi_wait_tx_fifo_empty(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	while (LPSPI_GetTxFifoCount(base) != 0) {
	}
}

int spi_lpspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct lpspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static inline int lpspi_validate_xfer_args(const struct spi_config *spi_cfg)
{
	uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	uint32_t pcs = spi_cfg->slave;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		/* the IP DOES support half duplex, need to implement driver support */
		LOG_WRN("Half-duplex not supported");
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
		LOG_WRN("Word size %d not allowed", word_size);
		return -EINVAL;
	}

	if (pcs > LPSPI_CHIP_SELECT_COUNT - 1) {
		LOG_WRN("Peripheral %d select exceeds max %d", pcs, LPSPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	return 0;
}

int spi_mcux_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool already_configured = spi_context_configured(ctx, spi_cfg);
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	lpspi_master_config_t master_config;
	uint32_t clock_freq;
	int ret;

	/* fast path to avoid reconfigure */
	/* TODO: S32K3 errata ERR050456 requiring module reset before every transfer,
	 * investigate alternative workaround so we don't have this latency for S32.
	 */
	if (already_configured && !IS_ENABLED(CONFIG_SOC_FAMILY_NXP_S32)) {
		return 0;
	}

	ret = lpspi_validate_xfer_args(spi_cfg);
	if (ret) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq);
	if (ret) {
		return ret;
	}

	if (already_configured) {
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

		/* this is workaround for ERR050456 */
		base->CR |= LPSPI_CR_RST_MASK;
		base->CR |= LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK;
		base->CR = 0x00U;
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
	master_config.whichPcs = spi_cfg->slave + kLPSPI_Pcs0;
	master_config.pcsActiveHighOrLow = (spi_cfg->operation & SPI_CS_ACTIVE_HIGH)
				    ? kLPSPI_PcsActiveHigh : kLPSPI_PcsActiveLow;
	master_config.pinCfg = config->data_pin_config;
	master_config.dataOutConfig = config->tristate_output ? kLpspiDataOutTristate :
								kLpspiDataOutRetained;

	LPSPI_MasterInit(base, &master_config, clock_freq);
	LPSPI_SetDummyData(base, 0);

	if (IS_ENABLED(CONFIG_DEBUG)) {
		base->CR |= LPSPI_CR_DBGEN_MASK;
	}

	return 0;
}

static void lpspi_module_system_init(LPSPI_Type *base)
{
#ifdef LPSPI_CLOCKS
	CLOCK_EnableClock(lpspi_get_clock(base));
#endif

#ifdef LPSPI_RSTS
	RESET_ReleasePeripheralReset(lpspi_get_reset(base));
#endif
}

int spi_nxp_init_common(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	int err = 0;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	data->dev = dev;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	lpspi_module_system_init(base);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	LPSPI_Reset(base);

	config->irq_config_func(dev);

	return err;
}
