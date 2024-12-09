/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "../spi_context.h"

#if CONFIG_NXP_LP_FLEXCOMM
#include <zephyr/drivers/mfd/nxp_lp_flexcomm.h>
#endif

#include <fsl_lpspi.h>

/* If any hardware revisions change this, make it into a DT property.
 * DONT'T make #ifdefs here by platform.
 */
#define LPSPI_CHIP_SELECT_COUNT   4
#define LPSPI_MIN_FRAME_SIZE_BITS 8

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct spi_mcux_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct spi_mcux_data *)(_dev)->data)

/* flag for SDK API for master transfers */
#define LPSPI_MASTER_XFER_CFG_FLAGS(slave)                                                         \
	kLPSPI_MasterPcsContinuous | (slave << LPSPI_MASTER_PCS_SHIFT)

struct spi_mcux_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t pcs_sck_delay;
	uint32_t sck_pcs_delay;
	uint32_t transfer_delay;
	const struct pinctrl_dev_config *pincfg;
	lpspi_pin_config_t data_pin_config;
};

#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
#include <zephyr/drivers/dma.h>

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel; /* stores the channel for dma */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
};
#endif /* CONFIG_SPI_MCUX_LPSPI_DMA */

struct spi_mcux_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	const struct device *dev;
	lpspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif
#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
	volatile uint32_t status_flags;
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
	/* dummy value used for transferring NOP when tx buf is null */
	uint32_t dummy_buffer;
#endif
};


static int spi_nxp_init_common(const struct device *dev)
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

static int spi_mcux_configure(const struct device *dev, const struct spi_config *spi_cfg)
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

	LPSPI_MasterInit(base, &master_config, clock_freq);
	LPSPI_SetDummyData(base, 0);

	if (IS_ENABLED(CONFIG_DEBUG)) {
		base->CR |= LPSPI_CR_DBGEN_MASK;
	}

	return 0;
}

static int spi_mcux_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_transfer_next_packet(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_context *ctx = &data->ctx;
	size_t max_chunk = spi_context_max_continuous_chunk(ctx);
	lpspi_transfer_t transfer;
	status_t status;

	if (max_chunk == 0) {
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, 0);
		return 0;
	}

	data->transfer_len = max_chunk;

	transfer.configFlags = LPSPI_MASTER_XFER_CFG_FLAGS(ctx->config->slave);
	transfer.txData = (ctx->tx_len == 0 ? NULL : ctx->tx_buf);
	transfer.rxData = (ctx->rx_len == 0 ? NULL : ctx->rx_buf);
	transfer.dataSize = max_chunk;

	status = LPSPI_MasterTransferNonBlocking(base, &data->handle, &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start on %s: %d", dev->name, status);
		return status == kStatus_LPSPI_Busy ? -EBUSY : -EINVAL;
	}

	return 0;
}

static void spi_mcux_master_callback(LPSPI_Type *base, lpspi_master_handle_t *handle,
				     status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

/* Argument to MCUX SDK IRQ handler */
#define LPSPI_IRQ_HANDLE_ARG COND_CODE_1(CONFIG_NXP_LP_FLEXCOMM, (LPSPI_GetInstance(base)), (base))

static void lpspi_isr(const struct device *dev);

#if defined(CONFIG_NXP_LP_FLEXCOMM)
#define SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                                 \
	nxp_lp_flexcomm_setirqhandler(DEVICE_DT_GET(DT_INST_PARENT(n)), DEVICE_DT_INST_GET(n),     \
				      LP_FLEXCOMM_PERIPH_LPSPI, lpspi_isr);
#else
#define SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                                 \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), lpspi_isr,                       \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQN(n));
#endif

#define SPI_MCUX_LPSPI_CONFIG_INIT(n)	\
	static const struct spi_mcux_config spi_mcux_config_##n = {                                \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.irq_config_func = spi_mcux_config_func_##n,                                       \
		.pcs_sck_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, pcs_sck_delay),                 \
					  DT_INST_PROP(n, pcs_sck_delay)),                         \
		.sck_pcs_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, sck_pcs_delay),                 \
					  DT_INST_PROP(n, sck_pcs_delay)),                         \
		.transfer_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, transfer_delay),               \
					   DT_INST_PROP(n, transfer_delay)),                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.data_pin_config = DT_INST_ENUM_IDX(n, data_pin_config),                           \
	};                                                                                         \

#define SPI_NXP_LPSPI_COMMON_INIT(n)								   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
												   \
	static void spi_mcux_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                         \
	}                                                                                          \

#define SPI_NXP_LPSPI_COMMON_DATA_INIT(n)							   \
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)
