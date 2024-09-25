/*
 * Copyright (c) 2024, STRIM, ALC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_flexio_spi

#include <errno.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_flexio_spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/misc/nxp_flexio/nxp_flexio.h>

LOG_MODULE_REGISTER(spi_mcux_flexio_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"


struct spi_mcux_flexio_config {
	FLEXIO_SPI_Type *flexio_spi;
	const struct device *flexio_dev;
	const struct pinctrl_dev_config *pincfg;
	const struct nxp_flexio_child *child;
};

struct spi_mcux_flexio_data {
	const struct device *dev;
	flexio_spi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
	uint8_t transfer_flags;
};


static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_flexio_config *config = dev->config;
	struct spi_mcux_flexio_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	flexio_spi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return;
	}

	transfer.flags = kFLEXIO_SPI_csContinuous | data->transfer_flags;

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		transfer.txData = NULL;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = NULL;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len == ctx->rx_len) {
		/* rx and tx are the same length */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len > ctx->rx_len) {
		/* Break up the tx into multiple transfers so we don't have to
		 * rx into a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
	}

	data->transfer_len = transfer.dataSize;

	status = FLEXIO_SPI_MasterTransferNonBlocking(config->flexio_spi, &data->handle,
						 &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start");
	}
}

static int spi_mcux_flexio_isr(void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	const struct spi_mcux_flexio_config *config = dev->config;
	struct spi_mcux_flexio_data *data = dev->data;

	FLEXIO_SPI_MasterTransferHandleIRQ(config->flexio_spi, &data->handle);

	return 0;
}

static void spi_mcux_master_transfer_callback(FLEXIO_SPI_Type *flexio_spi,
	flexio_spi_master_handle_t *handle, status_t status, void *userData)
{
	struct spi_mcux_flexio_data *data = userData;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

static void spi_flexio_master_init(FLEXIO_SPI_Type *base, flexio_spi_master_config_t *masterConfig,
	uint8_t pol, uint32_t srcClock_Hz)
{
	assert(base != NULL);
	assert(masterConfig != NULL);

	flexio_shifter_config_t shifterConfig;
	flexio_timer_config_t timerConfig;
	uint32_t ctrlReg  = 0;
	uint16_t timerDiv = 0;
	uint16_t timerCmp = 0;

	/* Clear the shifterConfig & timerConfig struct. */
	(void)memset(&shifterConfig, 0, sizeof(shifterConfig));
	(void)memset(&timerConfig, 0, sizeof(timerConfig));

	/* Configure FLEXIO SPI Master */
	ctrlReg = base->flexioBase->CTRL;
	ctrlReg &= ~(FLEXIO_CTRL_DOZEN_MASK | FLEXIO_CTRL_DBGE_MASK |
			FLEXIO_CTRL_FASTACC_MASK | FLEXIO_CTRL_FLEXEN_MASK);
	ctrlReg |= (FLEXIO_CTRL_DBGE(masterConfig->enableInDebug) |
		FLEXIO_CTRL_FASTACC(masterConfig->enableFastAccess) |
		FLEXIO_CTRL_FLEXEN(masterConfig->enableMaster));
	if (!masterConfig->enableInDoze) {
		ctrlReg |= FLEXIO_CTRL_DOZEN_MASK;
	}

	base->flexioBase->CTRL = ctrlReg;

	/* Do hardware configuration. */
	/* 1. Configure the shifter 0 for tx. */
	shifterConfig.timerSelect = base->timerIndex[0];
	shifterConfig.pinConfig   = kFLEXIO_PinConfigOutput;
	shifterConfig.pinSelect   = base->SDOPinIndex;
	shifterConfig.pinPolarity = kFLEXIO_PinActiveHigh;
	shifterConfig.shifterMode = kFLEXIO_ShifterModeTransmit;
	shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
	if (masterConfig->phase == kFLEXIO_SPI_ClockPhaseFirstEdge) {
		shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnNegitive;
		shifterConfig.shifterStop   = kFLEXIO_ShifterStopBitDisable;
		shifterConfig.shifterStart  = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable;
	} else {
		shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive;
		shifterConfig.shifterStop   = kFLEXIO_ShifterStopBitLow;
		shifterConfig.shifterStart  = kFLEXIO_ShifterStartBitDisabledLoadDataOnShift;
	}

	FLEXIO_SetShifterConfig(base->flexioBase, base->shifterIndex[0], &shifterConfig);

	/* 2. Configure the shifter 1 for rx. */
	shifterConfig.timerSelect  = base->timerIndex[0];
	shifterConfig.pinConfig    = kFLEXIO_PinConfigOutputDisabled;
	shifterConfig.pinSelect    = base->SDIPinIndex;
	shifterConfig.pinPolarity  = kFLEXIO_PinActiveHigh;
	shifterConfig.shifterMode  = kFLEXIO_ShifterModeReceive;
	shifterConfig.inputSource  = kFLEXIO_ShifterInputFromPin;
	shifterConfig.shifterStop  = kFLEXIO_ShifterStopBitDisable;
	shifterConfig.shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable;
	if (masterConfig->phase == kFLEXIO_SPI_ClockPhaseFirstEdge) {
		shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive;
	} else {
		shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnNegitive;
	}

	FLEXIO_SetShifterConfig(base->flexioBase, base->shifterIndex[1], &shifterConfig);

	/*3. Configure the timer 0 for SCK. */
	timerConfig.triggerSelect   = FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(base->shifterIndex[0]);
	timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
	timerConfig.triggerSource   = kFLEXIO_TimerTriggerSourceInternal;
	timerConfig.pinConfig       = kFLEXIO_PinConfigOutput;
	timerConfig.pinSelect       = base->SCKPinIndex;
	timerConfig.pinPolarity     = pol ? kFLEXIO_PinActiveLow : kFLEXIO_PinActiveHigh;
	timerConfig.timerMode       = kFLEXIO_TimerModeDual8BitBaudBit;
	timerConfig.timerOutput     = kFLEXIO_TimerOutputZeroNotAffectedByReset;
	timerConfig.timerDecrement  = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
	timerConfig.timerReset      = kFLEXIO_TimerResetNever;
	timerConfig.timerDisable    = kFLEXIO_TimerDisableOnTimerCompare;
	timerConfig.timerEnable     = kFLEXIO_TimerEnableOnTriggerHigh;
	timerConfig.timerStop       = kFLEXIO_TimerStopBitEnableOnTimerDisable;
	timerConfig.timerStart      = kFLEXIO_TimerStartBitEnabled;
	/* Low 8-bits are used to configure baudrate. */
	timerDiv = (uint16_t)(srcClock_Hz / masterConfig->baudRate_Bps);
	timerDiv = timerDiv / 2U - 1U;
	/* High 8-bits are used to configure shift clock edges(transfer width). */
	timerCmp = ((uint16_t)masterConfig->dataMode * 2U - 1U) << 8U;
	timerCmp |= timerDiv;

	timerConfig.timerCompare = timerCmp;

	FLEXIO_SetTimerConfig(base->flexioBase, base->timerIndex[0], &timerConfig);
}

static int spi_mcux_flexio_configure(const struct device *dev,
	const struct spi_config *spi_cfg)
{
	const struct spi_mcux_flexio_config *config = dev->config;
	struct spi_mcux_flexio_data *data = dev->data;

	flexio_spi_master_config_t master_config;
	uint32_t clock_freq;
	uint32_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Mode Slave not supported");
		return -ENOTSUP;
	}

	FLEXIO_SPI_MasterGetDefaultConfig(&master_config);

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if ((word_size != 8) && (word_size != 16) && (word_size != 32)) {
		LOG_ERR("Word size %d must be 8, 16 or 32", word_size);
		return -EINVAL;
	}
	master_config.dataMode = word_size;

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		if (word_size == 8) {
			data->transfer_flags = kFLEXIO_SPI_8bitLsb;
		} else if (word_size == 16) {
			data->transfer_flags = kFLEXIO_SPI_16bitLsb;
		} else {
			data->transfer_flags = kFLEXIO_SPI_32bitLsb;
		}
	} else {
		if (word_size == 8) {
			data->transfer_flags = kFLEXIO_SPI_8bitMsb;
		} else if (word_size == 16) {
			data->transfer_flags = kFLEXIO_SPI_16bitMsb;
		} else {
			data->transfer_flags = kFLEXIO_SPI_32bitMsb;
		}
	}

	if (nxp_flexio_get_rate(config->flexio_dev, &clock_freq)) {
		return -EINVAL;
	}

	master_config.phase =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kFLEXIO_SPI_ClockPhaseSecondEdge
		: kFLEXIO_SPI_ClockPhaseFirstEdge;

	master_config.baudRate_Bps = spi_cfg->frequency;
	spi_flexio_master_init(config->flexio_spi, &master_config,
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL), clock_freq);

	FLEXIO_SPI_MasterTransferCreateHandle(config->flexio_spi, &data->handle,
					spi_mcux_master_transfer_callback,
					data);
	/* No SetDummyData() for FlexIO_SPI */

	data->ctx.config = spi_cfg;

	return 0;
}


static int transceive(const struct device *dev,
			const struct spi_config *spi_cfg,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs,
			bool asynchronous,
			spi_callback_t cb,
			void *userdata)
{
	const struct spi_mcux_flexio_config *config = dev->config;
	struct spi_mcux_flexio_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	nxp_flexio_lock(config->flexio_dev);
	ret = spi_mcux_flexio_configure(dev, spi_cfg);
	nxp_flexio_unlock(config->flexio_dev);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	nxp_flexio_lock(config->flexio_dev);
	nxp_flexio_irq_disable(config->flexio_dev);

	spi_mcux_transfer_next_packet(dev);

	nxp_flexio_irq_enable(config->flexio_dev);
	nxp_flexio_unlock(config->flexio_dev);

	ret = spi_context_wait_for_completion(&data->ctx);
out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_mcux_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mcux_release(const struct device *dev,
				const struct spi_config *spi_cfg)
{
	struct spi_mcux_flexio_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_init(const struct device *dev)
{
	const struct spi_mcux_flexio_config *config = dev->config;
	struct spi_mcux_flexio_data *data = dev->data;
	int err;

	err = nxp_flexio_child_attach(config->flexio_dev, config->child);
	if (err < 0) {
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	data->dev = dev;

	/* TODO: DMA */

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_mcux_driver_api = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
	.release = spi_mcux_release,
};

#define SPI_MCUX_FLEXIO_SPI_INIT(n)					\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static FLEXIO_SPI_Type flexio_spi_##n = {			\
		.flexioBase = (FLEXIO_Type *)DT_REG_ADDR(DT_INST_PARENT(n)), \
		.SDOPinIndex = DT_INST_PROP(n, sdo_pin),		\
		.SDIPinIndex = DT_INST_PROP(n, sdi_pin),		\
		.SCKPinIndex = DT_INST_PROP(n, sck_pin),		\
	};								\
									\
	static const struct nxp_flexio_child nxp_flexio_spi_child_##n = { \
		.isr = spi_mcux_flexio_isr,				\
		.user_data = (void *)DEVICE_DT_INST_GET(n),		\
		.res = {						\
			.shifter_index = flexio_spi_##n.shifterIndex,	\
			.shifter_count = ARRAY_SIZE(flexio_spi_##n.shifterIndex), \
			.timer_index = flexio_spi_##n.timerIndex,	\
			.timer_count = ARRAY_SIZE(flexio_spi_##n.timerIndex) \
		}							\
	};								\
									\
	static const struct spi_mcux_flexio_config spi_mcux_flexio_config_##n = { \
		.flexio_spi = &flexio_spi_##n,				\
		.flexio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.child = &nxp_flexio_spi_child_##n,			\
	};								\
									\
	static struct spi_mcux_flexio_data spi_mcux_flexio_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_flexio_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_flexio_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &spi_mcux_init, NULL,			\
				&spi_mcux_flexio_data_##n,		\
				&spi_mcux_flexio_config_##n, POST_KERNEL, \
				CONFIG_SPI_INIT_PRIORITY,		\
				&spi_mcux_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_FLEXIO_SPI_INIT)
