/*
 * Copyright (c) 2026 Bang & Olufsen A/S, Denmark
 * Copyright (c) 2026 Linumiz
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
	DEVICE_MMIO_ROM;
	const struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
	DL_SPI_CLOCK clock_sel;
};

struct spi_mspm0_data {
	DEVICE_MMIO_RAM;
	struct spi_context spi_ctx;
};

static inline SPI_Regs *spi_mspm0_regs(const struct device *dev)
{
	return (SPI_Regs *)DEVICE_MMIO_GET(dev);
}

static int spi_mspm0_configure(const struct device *dev, const struct spi_config *spi_cfg,
			       uint8_t dfs)
{
	const struct spi_mspm0_config *const config = dev->config;
	struct spi_mspm0_data *const data = dev->data;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	SPI_Regs *regs = spi_mspm0_regs(dev);
	uint32_t clock_rate;
	uint16_t clock_scr;
	int ret;
	DL_SPI_Config dl_spi_cfg;

	if (spi_context_configured(&data->spi_ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	/* Only master mode is supported */
	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(clk_dev,
				     (clock_control_subsys_t)(uintptr_t)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	if (spi_cfg->frequency > (clock_rate / 2)) {
		return -EINVAL;
	}

	/* Ceil to prevent frequency overshoot
	 * Refer DL_SPI_setBitRateSerialClockDivider for details
	 */
	clock_scr = DIV_ROUND_UP(clock_rate, 2 * spi_cfg->frequency) - 1;
	if (!IN_RANGE(clock_scr, MSPM0_SPI_SCR_MIN, MSPM0_SPI_SCR_MAX)) {
		return -EINVAL;
	}

	/* Peripheral should be disabled before applying a new configuration */
	DL_SPI_disable(regs);

	dl_spi_cfg.parity = DL_SPI_PARITY_NONE;			/* Currently unused in zephyr */
	dl_spi_cfg.chipSelectPin = DL_SPI_CHIP_SELECT_NONE;	/* spi_context controls the CS */
	dl_spi_cfg.mode = SPI_MODE(spi_cfg->operation);
	dl_spi_cfg.bitOrder = BIT_ORDER_MODE(spi_cfg->operation);
	dl_spi_cfg.dataSize = DATA_SIZE_MODE(spi_cfg->operation);
	dl_spi_cfg.frameFormat = FRAME_FORMAT_MODE(spi_cfg->operation);

	DL_SPI_init(regs, (DL_SPI_Config *)&dl_spi_cfg);
	DL_SPI_setBitRateSerialClockDivider(regs, (uint32_t)clock_scr);

	if (dfs > SPI_DFS_16BIT) {
		DL_SPI_enablePacking(regs);
	} else {
		DL_SPI_disablePacking(regs);
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		DL_SPI_enableLoopbackMode(regs);
	} else {
		DL_SPI_disableLoopbackMode(regs);
	}

	DL_SPI_enable(regs);

	/* Cache SPI config for reuse, required by spi_context owner */
	data->spi_ctx.config = spi_cfg;

	return 0;
}

static void spi_mspm0_frame_tx(const struct device *dev, uint8_t dfs)
{
	struct spi_mspm0_data *data = dev->data;
	SPI_Regs *regs = spi_mspm0_regs(dev);
	uint32_t tx_frame = 0;

	if (spi_context_tx_buf_on(&data->spi_ctx)) {
		if (dfs == SPI_DFS_8BIT) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->spi_ctx.tx_buf));
		} else if (dfs == SPI_DFS_16BIT) {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->spi_ctx.tx_buf));
		} else {
			tx_frame = UNALIGNED_GET((uint32_t *)(data->spi_ctx.tx_buf));
		}
	}
	DL_SPI_transmitDataCheck32(regs, tx_frame);

	while (DL_SPI_isBusy(regs)) {
		/* Wait for tx frame to be sent */
	}

	spi_context_update_tx(&data->spi_ctx, dfs, 1);
}

static void spi_mspm0_frame_rx(const struct device *dev, uint8_t dfs)
{
	struct spi_mspm0_data *data = dev->data;
	SPI_Regs *regs = spi_mspm0_regs(dev);
	uint32_t rx_val = 0;

	DL_SPI_receiveDataCheck32(regs, &rx_val);

	if (dfs == SPI_DFS_8BIT) {
		UNALIGNED_PUT((uint8_t)rx_val, (uint8_t *)data->spi_ctx.rx_buf);
	} else if (dfs == SPI_DFS_16BIT) {
		UNALIGNED_PUT((uint16_t)rx_val, (uint16_t *)data->spi_ctx.rx_buf);
	} else {
		UNALIGNED_PUT(rx_val, (uint32_t *)data->spi_ctx.rx_buf);
	}

	spi_context_update_rx(&data->spi_ctx, dfs, 1);
}

static void spi_mspm0_start_transfer(const struct device *dev, uint8_t dfs)
{
	struct spi_mspm0_data *data = dev->data;

	spi_context_cs_control(&data->spi_ctx, true);

	while (spi_context_tx_on(&data->spi_ctx) || spi_context_rx_on(&data->spi_ctx)) {
		spi_mspm0_frame_tx(dev, dfs);
		spi_mspm0_frame_rx(dev, dfs);
	}

	spi_context_cs_control(&data->spi_ctx, false);
	spi_context_complete(&data->spi_ctx, dev, 0);
}

static int spi_mspm0_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_mspm0_data *data = dev->data;
	int ret;
	uint8_t dfs, word_size;

	if (!tx_bufs && !rx_bufs) {
		return -EINVAL;
	}

	if (spi_cfg->frequency == 0) {
		return -EINVAL;
	}

	spi_context_lock(&data->spi_ctx, false, NULL, NULL, spi_cfg);

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (!IN_RANGE(word_size, 4, 16)) {
		spi_context_release(&data->spi_ctx, -ENOTSUP);
		return -ENOTSUP;
	}

	dfs = BYTES_PER_FRAME(word_size);

	ret = spi_mspm0_configure(dev, spi_cfg, dfs);
	if (ret != 0) {
		spi_context_release(&data->spi_ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(&data->spi_ctx, tx_bufs, rx_bufs, dfs);
	spi_mspm0_start_transfer(dev, dfs);

	ret = spi_context_wait_for_completion(&data->spi_ctx);
	spi_context_release(&data->spi_ctx, ret);

	return ret;
}

static int spi_mspm0_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mspm0_data *data = dev->data;
	SPI_Regs *regs = spi_mspm0_regs(dev);

	if (!spi_context_configured(&data->spi_ctx, spi_cfg)) {
		return -EINVAL;
	}

	if (DL_SPI_isBusy(regs)) {
		return -EBUSY;
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
	const struct spi_mspm0_config *config = dev->config;
	struct spi_mspm0_data *data = dev->data;
	SPI_Regs *regs;
	int ret;
	DL_SPI_ClockConfig clk_cfg;

	regs = spi_mspm0_regs(dev);

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	DL_SPI_enablePower(regs);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	ret = spi_context_cs_configure_all(&data->spi_ctx);
	if (ret < 0) {
		return ret;
	}

	clk_cfg.clockSel = config->clock_sel;
	clk_cfg.divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1;

	DL_SPI_setClockConfig(regs, (DL_SPI_ClockConfig *)&clk_cfg);
	DL_SPI_enable(regs);

	spi_context_unlock_unconditionally(&data->spi_ctx);

	return ret;
}

#define MSPM0_SPI_INIT(inst)									\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static const struct mspm0_sys_clock mspm0_spi_sys_clock##inst =				\
		MSPM0_CLOCK_SUBSYS_FN(inst);							\
												\
	static const struct spi_mspm0_config spi_mspm0_config_##inst = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),					\
		.clock_subsys = &mspm0_spi_sys_clock##inst,					\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.clock_sel = MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(inst, clk)),	\
	};											\
												\
	static struct spi_mspm0_data spi_mspm0_data_##inst = {					\
		SPI_CONTEXT_INIT_LOCK(spi_mspm0_data_##inst, spi_ctx),				\
		SPI_CONTEXT_INIT_SYNC(spi_mspm0_data_##inst, spi_ctx),				\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), spi_ctx)			\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, spi_mspm0_init, NULL, &spi_mspm0_data_##inst,		\
			      &spi_mspm0_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,	\
			      &spi_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_SPI_INIT)
