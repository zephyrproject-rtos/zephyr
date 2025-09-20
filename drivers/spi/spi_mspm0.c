/*
 * Copyright (c) 2024 Bang & Olufsen A/S, Denmark
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_spi

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

/* TI DriverLib includes */
#include <driverlib/dl_spi.h>

LOG_MODULE_REGISTER(spi_mspm0, CONFIG_SPI_LOG_LEVEL);

/* This must be included after log module registration */
#include "spi_context.h"

/* Data Frame Size (DFS) */
#define SPI_DFS_8BIT		1
#define SPI_DFS_16BIT		2

/* Range for SPI Serial Clock Rate (SCR) */
#define MSPM0_SPI_SCR_MIN	0
#define MSPM0_SPI_SCR_MAX	1023

/* Delay after enabling power for SPI module */
#define POWER_STARTUP_DELAY	16

#define SPI_DT_CLK_DIV(inst)									\
	DT_INST_PROP(inst, clk_div)

#define SPI_DT_CLK_DIV_ENUM(inst)								\
	_CONCAT(DL_SPI_CLOCK_DIVIDE_RATIO_, SPI_DT_CLK_DIV(inst))

#define SPI_MODE(operation)									\
	(operation & BIT(0) ? DL_SPI_MODE_PERIPHERAL : DL_SPI_MODE_CONTROLLER)

#define BIT_ORDER_MODE(operation)								\
	(operation & BIT(4) ? DL_SPI_BIT_ORDER_LSB_FIRST : DL_SPI_BIT_ORDER_MSB_FIRST)

/* MSPM0 DSS field expects word size - 1 */
#define DATA_SIZE_MODE(operation)								\
	(SPI_WORD_SIZE_GET(operation) - 1)

#define POLARITY_MODE(operation)								\
	(SPI_MODE_GET(operation) & SPI_MODE_CPOL ? SPI_CTL0_SPO_HIGH : SPI_CTL0_SPO_LOW)

#define PHASE_MODE(operation)									\
	(SPI_MODE_GET(operation) & SPI_MODE_CPHA ? SPI_CTL0_SPH_SECOND : SPI_CTL0_SPH_FIRST)

#define DUPLEX_MODE(operation)									\
	(operation & BIT(11) ? SPI_CTL0_FRF_MOTOROLA_3WIRE : SPI_CTL0_FRF_MOTOROLA_4WIRE)

/* TI uses fixed frame format; Motorola combines duplex, polarity, phase */
#define FRAME_FORMAT_MODE(operation)								\
	(operation & SPI_FRAME_FORMAT_TI							\
		? SPI_CTL0_FRF_TI_SYNC								\
		: DUPLEX_MODE(operation) | POLARITY_MODE(operation) | PHASE_MODE(operation))

/* Computes the minimum number of bytes per frame using word_size. Utilizes ceil
 * division, to ensure sufficient byte allocation for non-multiples of 8 bits
 */
#define BYTES_PER_FRAME(word_size) (((word_size) + 7) / 8)

struct spi_mspm0_config {
	SPI_Regs *base;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pinctrl;

	const DL_SPI_ClockConfig clock_config;
};

struct spi_mspm0_data {
	struct spi_context ctx;
};

static int spi_mspm0_configure(const struct device *dev, const struct spi_config *spi_cfg,
			       uint8_t dfs)
{
	const struct spi_mspm0_config *const cfg = dev->config;
	struct spi_mspm0_data *const data = dev->data;
	uint16_t clock_scr;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	/* Only master mode is supported */
	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		return -ENOTSUP;
	}

	if (spi_cfg->frequency > (cfg->clock_frequency / 2)) {
		return -EINVAL;
	}

	/* See DL_SPI_setBitRateSerialClockDivider for details */
	clock_scr = (cfg->clock_frequency / (2 * spi_cfg->frequency)) - 1;
	if (!IN_RANGE(clock_scr, MSPM0_SPI_SCR_MIN, MSPM0_SPI_SCR_MAX)) {
		return -EINVAL;
	}

	const DL_SPI_Config dl_cfg = {
		.parity = DL_SPI_PARITY_NONE,			/* Currently unused in zephyr */
		.chipSelectPin = DL_SPI_CHIP_SELECT_NONE,	/* spi_context controls the CS */
		.mode = SPI_MODE(spi_cfg->operation),
		.bitOrder = BIT_ORDER_MODE(spi_cfg->operation),
		.dataSize = DATA_SIZE_MODE(spi_cfg->operation),
		.frameFormat = FRAME_FORMAT_MODE(spi_cfg->operation),
	};

	/* Peripheral should be disabled before applying a new configuration */
	DL_SPI_disable(cfg->base);

	DL_SPI_init(cfg->base, (DL_SPI_Config *)&dl_cfg);
	DL_SPI_setBitRateSerialClockDivider(cfg->base, (uint32_t)clock_scr);

	if (dfs > SPI_DFS_16BIT) {
		DL_SPI_enablePacking(cfg->base);
	} else {
		DL_SPI_disablePacking(cfg->base);
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		DL_SPI_enableLoopbackMode(cfg->base);
	} else {
		DL_SPI_disableLoopbackMode(cfg->base);
	}

	DL_SPI_enable(cfg->base);

	/* Cache SPI config for reuse, required by spi_context owner */
	data->ctx.config = spi_cfg;

	return 0;
}

static void spi_mspm0_frame_tx(const struct device *dev, uint8_t dfs)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;

	/* Transmit dummy frame when no TX data is provided */
	uint32_t tx_frame = 0;

	if (spi_context_tx_buf_on(&data->ctx)) {
		if (dfs == SPI_DFS_8BIT) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		} else if (dfs == SPI_DFS_16BIT) {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
		} else {
			tx_frame = UNALIGNED_GET((uint32_t *)(data->ctx.tx_buf));
		}
	}
	DL_SPI_transmitDataCheck32(cfg->base, tx_frame);

	while (DL_SPI_isBusy(cfg->base)) {
		/* Wait for tx fifo to be sent */
	}

	spi_context_update_tx(&data->ctx, dfs, 1);
}

static void spi_mspm0_frame_rx(const struct device *dev, uint8_t dfs)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;
	uint32_t rx_val = 0;

	DL_SPI_receiveDataCheck32(cfg->base, &rx_val);

	if (!spi_context_rx_buf_on(&data->ctx)) {
		return;
	}

	if (dfs == SPI_DFS_8BIT) {
		UNALIGNED_PUT((uint8_t)rx_val, (uint8_t *)data->ctx.rx_buf);
	} else if (dfs == SPI_DFS_16BIT) {
		UNALIGNED_PUT((uint16_t)rx_val, (uint16_t *)data->ctx.rx_buf);
	} else {
		UNALIGNED_PUT(rx_val, (uint32_t *)data->ctx.rx_buf);
	}

	spi_context_update_rx(&data->ctx, dfs, 1);
}

static void spi_mspm0_start_transfer(const struct device *dev, const struct spi_config *spi_cfg,
				     uint8_t dfs)
{
	struct spi_mspm0_data *data = dev->data;

	spi_context_cs_control(&data->ctx, true);

	while (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx)) {
		spi_mspm0_frame_tx(dev, dfs);
		spi_mspm0_frame_rx(dev, dfs);
	}

	spi_context_cs_control(&data->ctx, false);
	spi_context_complete(&data->ctx, dev, 0);
}

static int spi_mspm0_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_mspm0_data *data = dev->data;
	int ret;
	uint8_t dfs;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);

	dfs = BYTES_PER_FRAME(SPI_WORD_SIZE_GET(spi_cfg->operation));

	ret = spi_mspm0_configure(dev, spi_cfg, dfs);
	if (ret != 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);

	spi_mspm0_start_transfer(dev, spi_cfg, dfs);

	ret = spi_context_wait_for_completion(&data->ctx);
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_mspm0_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	if (DL_SPI_isBusy(cfg->base)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static const struct spi_driver_api spi_mspm0_api = {
	.transceive = spi_mspm0_transceive,
	.release    = spi_mspm0_release,
};

static int spi_mspm0_init(const struct device *dev)
{
	const struct spi_mspm0_config *cfg = dev->config;
	struct spi_mspm0_data *data = dev->data;
	int ret;

	DL_SPI_enablePower(cfg->base);
	delay_cycles(POWER_STARTUP_DELAY);

	ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	DL_SPI_setClockConfig(cfg->base, (DL_SPI_ClockConfig *)&cfg->clock_config);
	DL_SPI_enable(cfg->base);

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

#define MSPM0_SPI_INIT(inst)									\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static struct spi_mspm0_config spi_mspm0_##inst##_cfg = {				\
		.base = (SPI_Regs *)DT_INST_REG_ADDR(inst),					\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.clock_config = {.clockSel =							\
				  MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(inst, clk)),  \
				 .divideRatio = SPI_DT_CLK_DIV_ENUM(inst)},			\
		.clock_frequency = DT_PROP(DT_INST_CLOCKS_CTLR(inst), clock_frequency),		\
	};											\
												\
	static struct spi_mspm0_data spi_mspm0_##inst##_data = {				\
		SPI_CONTEXT_INIT_LOCK(spi_mspm0_##inst##_data, ctx),				\
		SPI_CONTEXT_INIT_SYNC(spi_mspm0_##inst##_data, ctx),				\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};			\
												\
	DEVICE_DT_INST_DEFINE(inst, spi_mspm0_init, NULL, &spi_mspm0_##inst##_data,		\
			      &spi_mspm0_##inst##_cfg, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,	\
			      &spi_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_SPI_INIT)
