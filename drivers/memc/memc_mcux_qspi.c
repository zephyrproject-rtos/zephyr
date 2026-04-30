/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_qspi

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>
#include "memc_mcux_qspi.h"
#if defined(FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG) && FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG
#include "fsl_qspi_soc.h"
#endif

LOG_MODULE_REGISTER(memc_mcux_qspi, CONFIG_MEMC_LOG_LEVEL);

struct memc_mcux_qspi_config {
	const struct pinctrl_dev_config *pincfg;
	qspi_config_t qspi_config;
#if (!defined(FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG)) || \
	(!FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG)
#if (!defined(FSL_FEATURE_QSPI_HAS_NO_SOCCR_REG)) || !FSL_FEATURE_QSPI_HAS_NO_SOCCR_REG
	qspi_dqs_config_t dqs_config;
	bool dqs_valid;
#endif
#endif
};

struct memc_mcux_qspi_data {
	QuadSPI_Type *base;
	uint32_t amba_address;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

int memc_qspi_set_device_config(const struct device *dev,
				const qspi_flash_config_t *flash_config,
				const uint32_t *lut_array, size_t lut_count)
{
	struct memc_mcux_qspi_data *data = dev->data;
	QuadSPI_Type *base = data->base;
	qspi_flash_config_t config;

	memcpy(&config, flash_config, sizeof(qspi_flash_config_t));
	memcpy(config.lookuptable, lut_array, lut_count * sizeof(uint32_t));

	QSPI_SetFlashConfig(base, &config);

	return 0;
}

uint32_t memc_mcux_qspi_get_ahb_address(const struct device *dev)
{
	return ((const struct memc_mcux_qspi_data *)dev->data)->amba_address;
}

static void qspi_wait_for_idle(QuadSPI_Type *base)
{
	while (QSPI_GetStatusFlags(base) & (uint32_t)kQSPI_Busy) {
	}
}

static void qspi_execute_command(QuadSPI_Type *base,
				 uint32_t device_address, uint8_t seq_index)
{
	QSPI_SetIPCommandAddress(base, device_address);
	QSPI_ExecuteIPCommand(base, seq_index);
	qspi_wait_for_idle(base);
}

static void qspi_read_data(QuadSPI_Type *base,
			   const struct memc_mcux_qspi_transfer *transfer)
{
	uint32_t leftSize = transfer->data_size;
	uint32_t rxFifoSize = 4U * FSL_FEATURE_QSPI_RXFIFO_DEPTH;
	uint8_t *dst = transfer->data;

	for (uint32_t i = 0U; leftSize != 0U; i++) {
		uint32_t transSize = (leftSize > rxFifoSize) ? rxFifoSize : leftSize;
		uint32_t tmp[FSL_FEATURE_QSPI_RXFIFO_DEPTH];

		QSPI_ClearFifo(base, kQSPI_RxFifo);
		QSPI_SetIPCommandSize(base, transSize);
		QSPI_SetIPCommandAddress(base, transfer->device_address + rxFifoSize * i);
		QSPI_ExecuteIPCommand(base, transfer->seq_index);
		qspi_wait_for_idle(base);

		/* FIFO is 32-bit wide; read into aligned temp buffer then
		 * copy exact byte count to avoid overflowing the user buffer.
		 */
		QSPI_ReadBlocking(base, tmp, (transSize + 3U) & ~3U);
		memcpy(dst, tmp, transSize);
		dst += transSize;
		leftSize -= transSize;
	}
}

static void qspi_write_data(QuadSPI_Type *base,
			    const struct memc_mcux_qspi_transfer *transfer)
{
	/*
	 * The QSPI IP TX path requires a minimum transfer of 16 bytes (4 words).
	 * Pad to 4-byte alignment and enforce the 16-byte minimum; extra bytes
	 * are filled with 0xFF (NOR flash erase value — safe to program).
	 */
	uint32_t dataSize = transfer->data_size;
	uint32_t alignedSize = MAX((dataSize + 3U) & ~3U, 16U);
	uint32_t txFifoSize = 4U * FSL_FEATURE_QSPI_TXFIFO_DEPTH;
	const uint8_t *byteSrc = transfer->data;
	uint32_t totalWords = alignedSize / 4U;
	uint32_t preFillWords = MIN(totalWords, txFifoSize / 4U);

	QSPI_ClearFifo(base, kQSPI_TxFifo);

	/* Pre-fill FIFO before issuing IP command to prevent underrun. */
	for (uint32_t i = 0U; i < preFillWords; i++) {
		uint32_t word = 0xFFFFFFFFU;
		uint32_t byteOffset = i * 4U;

		if (byteOffset < dataSize) {
			memcpy(&word, byteSrc + byteOffset,
			       MIN(4U, dataSize - byteOffset));
		}
		while (QSPI_GetStatusFlags(base) & (uint32_t)kQSPI_TxBufferFull) {
		}
		base->TBDR = word;
	}

	/* Set command size, then address immediately before execute to avoid SFAR reset. */
	QSPI_SetIPCommandSize(base, alignedSize);
	QSPI_SetIPCommandAddress(base, transfer->device_address);
	QSPI_ExecuteIPCommand(base, transfer->seq_index);

	/* Write remaining words while command executes (only for writes > FIFO depth). */
	for (uint32_t i = preFillWords; i < totalWords; i++) {
		uint32_t word = 0xFFFFFFFFU;
		uint32_t byteOffset = i * 4U;

		if (byteOffset < dataSize) {
			memcpy(&word, byteSrc + byteOffset,
			       MIN(4U, dataSize - byteOffset));
		}
		while (QSPI_GetStatusFlags(base) & (uint32_t)kQSPI_TxBufferFull) {
		}
		base->TBDR = word;
	}

	qspi_wait_for_idle(base);
}

int memc_mcux_qspi_transfer(const struct device *dev, struct memc_mcux_qspi_transfer *transfer)
{
	QuadSPI_Type *base = ((struct memc_mcux_qspi_data *)dev->data)->base;

	if (transfer->data_size == 0U) {
		qspi_execute_command(base, transfer->device_address, transfer->seq_index);
	} else if (transfer->is_read) {
		qspi_read_data(base, transfer);
	} else {
		qspi_write_data(base, transfer);
	}

	return 0;
}

static int memc_mcux_qspi_init(const struct device *dev)
{
	const struct memc_mcux_qspi_config *memc_qspi_config = dev->config;
	const struct pinctrl_dev_config *pincfg = memc_qspi_config->pincfg;
	qspi_config_t config = memc_qspi_config->qspi_config;
	struct memc_mcux_qspi_data *data = dev->data;
	QuadSPI_Type *base = data->base;
	uint32_t clock_rate;
	int ret;

	ret = pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}
	ret = clock_control_get_rate(data->clock_dev, data->clock_subsys, &clock_rate);
	if (ret < 0) {
		LOG_ERR("Failed to get root clock rate: %d", ret);
		return ret;
	}
	LOG_DBG("%s clock_rate=%u", __func__, clock_rate);

	QSPI_Init(base, &config, clock_rate);

#if defined(CONFIG_SOC_SERIES_MCXE24X)
	/* MCXE24X: configure SOC clock divider and input buffer enable. */
	{
		qspi_soc_config_t soc_config = {0};

		soc_config.inputBufEnable = true;
		soc_config.divEnable      = true;
		soc_config.internalClk    = kQSPI_PllDiv1Clock;
		soc_config.clkMode        = kQSPI_SysClock;
		/* SFIF_CLK = SPLLDIV1_CLK / clkDiv = 160MHz / 8 = 20MHz */
		soc_config.clkDiv         = 8;

		QSPI_Enable(base, false);
		QSPI_SocConfigure(base, &soc_config);
		QSPI_Enable(base, true);
	}
#endif /* CONFIG_SOC_SERIES_MCXE24X */
#if (!defined(FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG)) || \
	(!FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG)
#if (!defined(FSL_FEATURE_QSPI_HAS_NO_SOCCR_REG)) || !FSL_FEATURE_QSPI_HAS_NO_SOCCR_REG
	if (memc_qspi_config->dqs_valid) {
		QSPI_SetDqsConfig(base, (qspi_dqs_config_t *)&memc_qspi_config->dqs_config);
	}
#endif
#endif /* !FSL_FEATURE_QSPI_HAS_SOC_SPECIFIC_CONFIG */

	return 0;
}

#define MCUX_QSPI_INIT(n)	\
	PINCTRL_DT_INST_DEFINE(n);	\
	static const struct memc_mcux_qspi_config memc_mcux_qspi_config_##n = {	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),	\
		.qspi_config = {	\
			.txWatermark = DT_INST_PROP_OR(n, tx_watermark, 8U),	\
			.rxWatermark = DT_INST_PROP_OR(n, rx_watermark, 8U),	\
			.AHBbufferSize = {0, 0, 0, 256},	\
			.AHBbufferMaster = {0xE, 0xE, 0xE, 0},	\
			.enableAHBbuffer3AllMaster = true,	\
			.enableQspi = true,	\
		},	\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, dqs_config), (	\
			.dqs_config = {	\
				.portADelayTapNum = DT_INST_PROP_BY_IDX(n, dqs_config, 0),	\
				.shift = DT_INST_PROP_BY_IDX(n, dqs_config, 1),	\
				.rxSampleClock = DT_INST_PROP_BY_IDX(n, dqs_config, 2),	\
				.enableDQSClkInverse = DT_INST_PROP_BY_IDX(n, dqs_config, 3),	\
			},	\
			.dqs_valid = true,	\
		))	\
	};	\
	static struct memc_mcux_qspi_data memc_mcux_qspi_data_##n = {	\
		.base = (QuadSPI_Type *)DT_INST_REG_ADDR(n),	\
		.amba_address = DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
	};	\
	DEVICE_DT_INST_DEFINE(n, &memc_mcux_qspi_init, NULL,	\
		&memc_mcux_qspi_data_##n, &memc_mcux_qspi_config_##n,	\
		POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MCUX_QSPI_INIT)
