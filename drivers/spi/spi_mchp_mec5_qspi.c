/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_qspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mec5, CONFIG_SPI_LOG_LEVEL);

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <zephyr/irq.h>

#include "spi_context.h"

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_ecia_api.h>
#include <mec_qspi_api.h>

/* microseconds for busy wait and total wait interval */
#define MEC5_QSPI_WAIT_INTERVAL	8
#define MEC5_QSPI_WAIT_COUNT 64
#define MEC5_QSPI_WAIT_FULL_FIFO 1024

/* Device constant configuration parameters */
struct mec5_qspi_config {
	struct qspi_regs *regs;
	int clock_freq;
	uint32_t cs1_freq;
	uint32_t cs_timing;
	uint8_t irqn;
	uint8_t irq_pri;
	uint8_t chip_sel;
	uint8_t width;	/* 0(half) 1(single), 2(dual), 4(quad) */
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_MEC5_QSPI_INTERRUPT
	void (*irq_config_func)(void);
#endif
};

/* Device run time data */
struct mec5_qspi_data {
	struct spi_context ctx;
	struct spi_config curr_cfg;
	uint32_t byte_time_ns;
	volatile uint32_t qstatus;
};

static int spi_feature_support(const struct spi_config *config)
{
	/* NOTE: bit(11) is Half-duplex */
	if (config->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE
				 | SPI_MODE_LOOP | BIT(11))) {
		LOG_ERR("Driver does not support LSB first, slave, loop back, or half-duplex");
		return -ENOTSUP;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size != 8 not supported");
		return -ENOTSUP;
	}

	return 0;
}

static enum mec_qspi_io lines_to_io(uint32_t opw)
{
	enum mec_qspi_io iom = MEC_QSPI_IO_FULL_DUPLEX;

#if defined(CONFIG_SPI_EXTENDED_MODES)
	switch (opw & SPI_LINES_MASK) {
	case SPI_LINES_DUAL:
		iom = MEC_QSPI_IO_DUAL;
		break;
	case SPI_LINES_QUAD:
		iom = MEC_QSPI_IO_QUAD;
		break;
	default:
		break;
	}
#endif
	return iom;
}

static int mec5_qspi_configure(const struct device *dev,
			       const struct spi_config *config)
{
	const struct mec5_qspi_config *cfg = dev->config;
	struct mec5_qspi_data *qdata = dev->data;
	struct spi_config *curr_cfg = &qdata->curr_cfg;
	enum mec_qspi_io iom = MEC_QSPI_IO_FULL_DUPLEX;
	enum mec_qspi_signal_mode sgm = MEC_SPI_SIGNAL_MODE_0;
	int ret;

	if (!config) {
		return -EINVAL;
	}

	if (config->frequency != curr_cfg->frequency) {
		ret = mec_qspi_set_freq(cfg->regs, config->frequency);
		if (ret != MEC_RET_OK) {
			return -EINVAL;
		}
		curr_cfg->frequency = config->frequency;
		mec_qspi_byte_time_ns(cfg->regs, &qdata->byte_time_ns);
	}

	if (config->operation == curr_cfg->operation) {
		qdata->ctx.config = config;
		return 0;
	}

	ret = spi_feature_support(config);
	if (ret) {
		return ret;
	}

	iom = lines_to_io(config->operation);

	ret = mec_qspi_io(cfg->regs, iom);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	if ((config->operation & (SPI_MODE_CPOL | SPI_MODE_CPHA))
	    == (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		sgm = MEC_SPI_SIGNAL_MODE_3;
	}

	ret = mec_qspi_spi_signal_mode(cfg->regs, sgm);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	qdata->ctx.config = config;

	return 0;
}

static int mec5_qspi_xfr_sync(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	const struct mec5_qspi_config *cfg = dev->config;
	struct mec5_qspi_data *qdata = dev->data;
	struct spi_context *ctx = &qdata->ctx;
#ifdef CONFIG_SPI_MEC5_QSPI_INTERRUPT
	uint32_t xfr_flags = BIT(MEC_QSPI_XFR_FLAG_START_POS) | BIT(MEC_QSPI_XFR_FLAG_IEN_POS);
#else
	uint32_t xfr_flags = BIT(MEC_QSPI_XFR_FLAG_START_POS);
#endif
	size_t total_rx = 0, total_tx = 0;
	int ret = 0;

	if (!config) {
		return -EINVAL;
	}

#if defined(CONFIG_SPI_EXTENDED_MODES)
	if (((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) && tx_bufs && rx_bufs) {
		LOG_ERR("Dual/Quad are half-duplex: one direction(buffer set) only");
		return -EINVAL;
	}
#endif

	spi_context_lock(&qdata->ctx, false, NULL, NULL, config);

	ret = mec5_qspi_configure(dev, config);
	if (ret != 0) {
		spi_context_release(ctx, ret);
		return ret;
	}

	spi_context_cs_control(ctx, true);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	total_tx = spi_context_total_tx_len(ctx);
	total_rx = spi_context_total_rx_len(ctx);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t chunk_len = spi_context_max_continuous_chunk(ctx);

		if (total_tx) {
			total_tx -= chunk_len;
		}
		if (total_rx) {
			total_rx -= chunk_len;
		}

		if (!total_tx && !total_rx && !(config->operation & SPI_HOLD_ON_CS)) {
			xfr_flags |= BIT(MEC_QSPI_XFR_FLAG_CLOSE_POS);
		}

		ret = mec_qspi_ldma(cfg->regs, (const uint8_t *)ctx->tx_buf,
						ctx->rx_buf, chunk_len, xfr_flags);
		if (ret != MEC_RET_OK) {
			mec_qspi_force_stop(cfg->regs);
			spi_context_unlock_unconditionally(ctx);
			LOG_ERR("QSPI sync HAL error (%d)", ret);
			return -EIO;
		}

#ifdef CONFIG_SPI_MEC5_QSPI_INTERRUPT
		ret = spi_context_wait_for_completion(&qdata->ctx);
#else
		ret = mec_qspi_done(cfg->regs);
		while (ret == MEC_RET_ERR_BUSY) {
			ret = mec_qspi_done(cfg->regs);
		}

		ret = (ret == MEC_RET_OK) ? 0 : -EIO;
#endif
		if (ret) {
			spi_context_unlock_unconditionally(ctx);
			LOG_ERR("QSPI sync HAL done error (%d)", ret);
			return ret;
		}

		spi_context_update_tx(ctx, 1, chunk_len);
		spi_context_update_rx(ctx, 1, chunk_len);
	}

	if (!(config->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}

	/* gives lock semaphore in struct spi_context
	 * This will allow other threads to use this controller
	 * even if chip select is still asserted via SPI_HOLD_ON_CS!
	 */
	spi_context_release(ctx, 0);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int mec5_qspi_xfr_async(const struct device *dev,
			       const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs,
			       spi_callback_t cb,
			       void *userdata)
{
	return -ENOTSUP;
}
#endif

static int mec5_qspi_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct mec5_qspi_data *qdata = dev->data;
	const struct mec5_qspi_config *cfg = dev->config;
	int ret = mec_qspi_force_stop(cfg->regs);

	/* increments lock semphare in ctx up to initial limit */
	spi_context_unlock_unconditionally(&qdata->ctx);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_SPI_MEC5_QSPI_INTERRUPT
static void mec5_qspi_isr(const struct device *dev)
{
	struct mec5_qspi_data *qdata = dev->data;
	const struct mec5_qspi_config *cfg = dev->config;
	struct qspi_regs *regs = cfg->regs;
	uint32_t hwsts = 0u;
	int status = 0;

	hwsts = mec_qspi_hw_status(regs);
	qdata->qstatus = hwsts;
	status = mec_qspi_done(regs);

	mec_qspi_intr_ctrl(regs, 0);
	mec_qspi_hw_status_clr(regs, hwsts);

	if (status == MEC_RET_OK) {
		status = 0;
	} else {
		status = -EIO;
	}

	spi_context_complete(&qdata->ctx, dev, status);
}
#endif /* CONFIG_SPI_MEC5_QSPI_INTERRUPT */

/*
 * Called for each QSPI controller by the kernel during driver load phase
 * specified in the device initialization structure below.
 * Initialize QSPI controller.
 * Initialize SPI context.
 * QSPI will be fully configured and enabled when the transceive API
 * is called.
 */
static int mec5_qspi_init(const struct device *dev)
{
	const struct mec5_qspi_config *cfg = dev->config;
	struct mec5_qspi_data *qdata = dev->data;
	enum mec_qspi_cs cs = MEC_QSPI_CS_0;
	enum mec_qspi_io iom = MEC_QSPI_IO_FULL_DUPLEX;
	enum mec_qspi_signal_mode spi_mode = MEC_SPI_SIGNAL_MODE_0;
	int ret;

	if (cfg->chip_sel) {
		cs = MEC_QSPI_CS_1;
	}

	ret = mec_qspi_init(cfg->regs, (uint32_t)cfg->clock_freq, spi_mode, iom, cs);
	if (ret != MEC_RET_OK) {
		LOG_ERR("QSPI init error (%d)", ret);
		return -EINVAL;
	}

	qdata->curr_cfg.frequency = cfg->clock_freq;
	qdata->curr_cfg.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	mec_qspi_byte_time_ns(cfg->regs, &qdata->byte_time_ns);

	if (cfg->cs_timing) {
		ret =  mec_qspi_cs_timing(cfg->regs, cfg->cs_timing);
		if (ret != MEC_RET_OK) {
			return -EINVAL;
		}
	}

	if (cfg->cs1_freq) {
		ret =  mec_qspi_cs1_freq(cfg->regs, cfg->cs1_freq);
		if (ret != MEC_RET_OK) {
			return -EINVAL;
		}
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
	}

#ifdef CONFIG_SPI_MEC5_QSPI_INTERRUPT
	if (cfg->irq_config_func) {
		cfg->irq_config_func();
	}
#endif

	spi_context_unlock_unconditionally(&qdata->ctx);

	return ret;
}

static const struct spi_driver_api mec5_qspi_driver_api = {
	.transceive = mec5_qspi_xfr_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = mec5_qspi_xfr_async,
#endif
	.release = mec5_qspi_release,
};

#define MEC5_QSPI_CS_TIMING_VAL(a, b, c, d) \
	(((a) & 0xFu) | (((b) & 0xFu) << 8) \
	 | (((c) & 0xFu) << 16) | (((d) & 0xFu) << 24))

#define MEC5_QSPI_TAPS_ADJ_VAL(a, b) (((a) & 0xffu) | (((b) & 0xffu) << 8))

#define MEC5_QSPI_CS_TIMING(i) MEC5_QSPI_CS_TIMING_VAL(			\
				DT_INST_PROP_OR(i, dcsckon, 6),		\
				DT_INST_PROP_OR(i, dckcsoff, 4),	\
				DT_INST_PROP_OR(i, dldh, 6),		\
				DT_INST_PROP_OR(i, dcsda, 6))

#ifdef CONFIG_SPI_MEC5_QSPI_INTERRUPT
#define MEC5_QSPI_IRQ_HANDLER_FUNC(id)					\
	.irq_config_func = mec5_qspi_irq_config_##id,

#define MEC5_QSPI_IRQ_HANDLER_CFG(id)					\
static void mec5_qspi_irq_config_##id(void)				\
{									\
	IRQ_CONNECT(DT_INST_IRQN(id),					\
		    DT_INST_IRQ(id, priority),				\
		    mec5_qspi_isr,					\
		    DEVICE_DT_INST_GET(id), 0);				\
	irq_enable(DT_INST_IRQN(id));					\
}
#else
#define MEC5_QSPI_IRQ_HANDLER_FUNC(id)
#define MEC5_QSPI_IRQ_HANDLER_CFG(id)
#endif

/* The instance number, i is not related to block ID's rather the
 * order the DT tools process all DT files in a build.
 */
#define MEC5_QSPI_DEVICE(i)							\
	PINCTRL_DT_INST_DEFINE(i);						\
	MEC5_QSPI_IRQ_HANDLER_CFG(i)						\
										\
	static struct mec5_qspi_data mec5_qspi_data_##i = {			\
		SPI_CONTEXT_INIT_LOCK(mec5_qspi_data_##i, ctx),			\
		SPI_CONTEXT_INIT_SYNC(mec5_qspi_data_##i, ctx),			\
	};									\
	static const struct mec5_qspi_config mec5_qspi_config_##i = {		\
		.regs = (struct qspi_regs *)DT_INST_REG_ADDR(i),		\
		.clock_freq = DT_INST_PROP_OR(i, clock_frequency, MHZ(12)),	\
		.cs1_freq = DT_INST_PROP_OR(i, cs1 - freq, 0),			\
		.cs_timing = MEC5_QSPI_CS_TIMING(i),				\
		.irqn = DT_INST_IRQN(i),					\
		.irq_pri = DT_INST_IRQ(i, priority),				\
		.chip_sel = DT_INST_PROP_OR(i, chip_select, 0),			\
		.width = DT_INST_PROP_OR(0, lines, 1),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),			\
		MEC5_QSPI_IRQ_HANDLER_FUNC(i)					\
	};									\
	DEVICE_DT_INST_DEFINE(i, &mec5_qspi_init, NULL,				\
		&mec5_qspi_data_##i, &mec5_qspi_config_##i,			\
		POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,				\
		&mec5_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MEC5_QSPI_DEVICE)
