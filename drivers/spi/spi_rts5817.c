/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5817_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_rts5817);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/cache.h>

#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>

#include "spi_rts5817_m_ctrl_reg.h"
#include "spi_rts5817_dw_reg.h"
#include "spi_rts5817.h"
#include "spi_context.h"

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

static void dw_reg_write(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t value)
{
	sys_write32(value, cfg->dw_regs + offset);
}

static uint32_t dw_reg_read(const struct spi_rts5817_config *cfg, uint32_t offset)
{
	return sys_read32(cfg->dw_regs + offset);
}

static void dw_reg_set_bits(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t bits)
{
	sys_set_bits(cfg->dw_regs + offset, bits);
}

static void dw_reg_clear_bits(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t bits)
{
	sys_clear_bits(cfg->dw_regs + offset, bits);
}

static void ctrl_reg_write(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t value)
{
	sys_write32(value, cfg->ctrl_regs + offset);
}

static uint32_t ctrl_reg_read(const struct spi_rts5817_config *cfg, uint32_t offset)
{
	return sys_read32(cfg->ctrl_regs + offset);
}

static void ctrl_reg_set_bits(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t bits)
{
	sys_set_bits(cfg->ctrl_regs + offset, bits);
}

static void ctrl_reg_clear_bits(const struct spi_rts5817_config *cfg, uint32_t offset,
				uint32_t bits)
{
	sys_clear_bits(cfg->ctrl_regs + offset, bits);
}

static inline bool spi_rts5817_is_slave(struct spi_rts5817_data *data)
{
	return (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(&data->ctx));
}

static inline bool spi_rts5817_transfer_ongoing(struct spi_rts5817_data *data)
{
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
}

static void completed(const struct device *dev, int err)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (err) {
		goto out;
	}

	if (spi_rts5817_transfer_ongoing(data)) {
		return;
	}

out:
	if (!spi_rts5817_is_slave(data)) {
		/* disable cs ser */
		dw_reg_clear_bits(cfg, R_SSI_SER, SER_MASK);
		/* if cs is defined as gpio, disable gpio cs */
		if (spi_cs_is_gpio(ctx->config)) {
			spi_context_cs_control(&data->ctx, false);
		}

		/* Disable interrupts */
		ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE, MST_DONE_INT_ENABLE_MASK);
	} else {
		/* TODO: spi slave support */
	}

	dw_reg_clear_bits(cfg, R_SSI_SSIENR, SSI_EN_MASK);

	LOG_DBG("SPI transaction finished %s error", err ? "with" : "without");

	spi_context_complete(&data->ctx, dev, err);
}

static int spi_rts5817_configure(const struct device *dev, const struct spi_config *config)
{

	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	uint32_t ctrlr0 = 0U;

	LOG_DBG("%p (prev %p)", config, data->ctx.config);

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	/* Verify if requested op mode is relevant to this controller */
	if (config->operation & SPI_OP_MODE_SLAVE) {
		if (!(cfg->serial_target)) {
			LOG_ERR("Slave mode not supported");
			return -ENOTSUP;
		}
	} else {
		if (cfg->serial_target) {
			LOG_ERR("Master mode not supported");
			return -ENOTSUP;
		}
	}

	if ((config->operation & SPI_TRANSFER_LSB) ||
	    (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	     (config->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD | SPI_LINES_OCTAL)))) {
		LOG_ERR("Unsupported configuration");
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size of %u not allowed", SPI_WORD_SIZE_GET(config->operation));
		return -ENOTSUP;
	}

	/* Word size */
	ctrlr0 |= SPI_WORD_SIZE_GET(config->operation) - 1;

	/* Determine how many bytes are required per-frame */
	data->dfs = SPI_WS_TO_DFS(SPI_WORD_SIZE_GET(config->operation));

	/* SPI mode */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		ctrlr0 |= SCPOL_MASK;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		ctrlr0 |= SCPH_MASK;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		ctrlr0 |= SRL_MASK;
	}

	/* Installing the configuration */
	dw_reg_write(cfg, R_SSI_CTRLR0, ctrlr0);

	LOG_DBG("CTRLR0:%x", dw_reg_read(cfg, R_SSI_CTRLR0));

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = config;

	if (!spi_rts5817_is_slave(data)) {
		/* Baud rate and Slave select, for master only */
		dw_reg_write(cfg, R_SSI_BAUDR,
			     SPI_RTS_CLK_DIVIDER(data->clock_frequency, config->frequency));

		/* config txftlr & rxftlr */
		dw_reg_write(cfg, R_SSI_TXFTLR, 0x1);
		dw_reg_write(cfg, R_SSI_RXFTLR, 0x1);

		/* config rx sample delay */
		if ((config->frequency >= 60000000) &&
		    (cfg->pcfg->states->pins->power_source == IO_POWER_1V8)) {
			dw_reg_write(cfg, R_SSI_RX_SAMPLE_DLY, 0x2);
		} else {
			dw_reg_write(cfg, R_SSI_RX_SAMPLE_DLY, 0x1);
		}

		ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_CONTROL, MST_SCK_INTERVAL_EN_MASK);
	} else {
		/* TODO: spi slave support */
	}

	if (spi_rts5817_is_slave(data)) {
		LOG_DBG("Installed slave config %p:"
			" ws/dfs %u/%u, mode %u/%u/%u",
			config, SPI_WORD_SIZE_GET(config->operation), data->dfs,
			(SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0,
			(SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0,
			(SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) ? 1 : 0);
	} else {
		LOG_DBG("Installed master config %p: freq %uHz (div = %u),"
			" ws/dfs %u/%u, mode %u/%u/%u, slave %u",
			config, config->frequency,
			SPI_RTS_CLK_DIVIDER(data->clock_frequency, config->frequency),
			SPI_WORD_SIZE_GET(config->operation), data->dfs,
			(SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0,
			(SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0,
			(SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) ? 1 : 0, config->slave);
	}

	return 0;
}

static int spi_rts5817_transceive_once(const struct device *dev, struct spi_rts5817_data *data)
{
	const struct spi_rts5817_config *cfg = dev->config;
	size_t dma_len = 0;
	uint8_t *tx_buf = NULL;

	if (!spi_rts5817_transfer_ongoing(data)) {
		spi_context_complete(&data->ctx, dev, 0);
		return 0;
	}

	if (data->ctx.tx_len == 0) {
		dma_len = data->ctx.rx_len;
	} else if (data->ctx.rx_len == 0) {
		dma_len = data->ctx.tx_len;
	} else {
		dma_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
	}

	if (dma_len > UINT16_MAX) {
		return -EINVAL;
	}

	data->aligned_buffer = k_aligned_alloc(32, dma_len);
	if (data->aligned_buffer == NULL) {
		return -ENOMEM;
	}

	memset(data->aligned_buffer, 0, dma_len);

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx_buf = (uint8_t *)data->ctx.tx_buf;
		sys_cache_data_flush_range(tx_buf, dma_len);
	} else {
		tx_buf = data->aligned_buffer;
	}

	sys_cache_data_flush_and_invd_range(data->aligned_buffer, dma_len);

	if (spi_rts5817_is_slave(data)) {
		/* TODO: spi slave support */
	} else {
		/* config address & length */
		ctrl_reg_write(cfg, R_MST_SPI_SSI_WR_ADDR, (uint32_t)data->aligned_buffer);
		ctrl_reg_write(cfg, R_MST_SPI_SSI_RD_ADDR, (uint32_t)tx_buf);
		ctrl_reg_write(cfg, R_MST_SPI_SSI_DATA_LEN, dma_len);

		data->xfer_len = dma_len;

		/* Start transfer */
		ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_START_CTRL, MST_SSI_START_MASK);

		if (cfg->interrupt_enable) {
			return 0;
		}

		if (!WAIT_FOR((ctrl_reg_read(cfg, R_MST_SPI_SSI_START_CTRL) & MST_SSI_START_MASK) ==
				      0,
			      RTS5817_SPI_TIME_OUT_MS * 1000, k_yield())) {
			ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_START_CTRL, MST_SSI_START_MASK);
			k_free(data->aligned_buffer);
			return -ETIMEDOUT;
		}

		if (spi_context_rx_buf_on(&data->ctx)) {
			memcpy(data->ctx.rx_buf, data->aligned_buffer, dma_len);
		}

		k_free(data->aligned_buffer);

		spi_context_update_rx(&data->ctx, data->dfs, dma_len);
		spi_context_update_tx(&data->ctx, data->dfs, dma_len);
	}

	return 0;
}

static int spi_rts5817_transceive_next(const struct spi_rts5817_config *cfg,
				       struct spi_rts5817_data *data)
{
	size_t dma_len = 0;
	uint8_t *tx_buf = NULL;

	if (spi_context_rx_buf_on(&data->ctx)) {
		memcpy(data->ctx.rx_buf, data->aligned_buffer, data->xfer_len);
	}

	k_free(data->aligned_buffer);

	spi_context_update_rx(&data->ctx, data->dfs, data->xfer_len);
	spi_context_update_tx(&data->ctx, data->dfs, data->xfer_len);

	if (!spi_rts5817_transfer_ongoing(data)) {
		return 0;
	}

	if (data->ctx.tx_len == 0) {
		dma_len = data->ctx.rx_len;
	} else if (data->ctx.rx_len == 0) {
		dma_len = data->ctx.tx_len;
	} else {
		dma_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
	}

	if (dma_len > UINT16_MAX) {
		return -EINVAL;
	}

	data->aligned_buffer = k_aligned_alloc(32, dma_len);
	if (data->aligned_buffer == NULL) {
		return -ENOMEM;
	}

	memset(data->aligned_buffer, 0, dma_len);

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx_buf = (uint8_t *)data->ctx.tx_buf;
		sys_cache_data_flush_range(tx_buf, dma_len);
	} else {
		tx_buf = data->aligned_buffer;
	}

	sys_cache_data_flush_and_invd_range(data->aligned_buffer, dma_len);

	if (spi_rts5817_is_slave(data)) {
		/* TODO: spi slave support */
	} else {
		/* config address & length */
		ctrl_reg_write(cfg, R_MST_SPI_SSI_WR_ADDR, (uint32_t)data->aligned_buffer);
		ctrl_reg_write(cfg, R_MST_SPI_SSI_RD_ADDR, (uint32_t)tx_buf);
		ctrl_reg_write(cfg, R_MST_SPI_SSI_DATA_LEN, dma_len);

		data->xfer_len = dma_len;

		/* Start transfer */
		ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_START_CTRL, MST_SSI_START_MASK);
	}

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	int err = 0;

	if (!config) {
		return -EINVAL;
	}

	if (!tx_bufs && !rx_bufs) {
		return -ECANCELED;
	}

	if (asynchronous && (!cfg->interrupt_enable)) {
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	/* Configure */
	err = spi_rts5817_configure(dev, config);
	if (err) {
		goto out;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	/* Use interrupt */
	if (cfg->interrupt_enable) {
		if (!spi_rts5817_is_slave(data)) {
			/* enable ssi ser */
			dw_reg_set_bits(cfg, R_SSI_SER, SER_MASK);
			/* if cs is defined as gpio, enable gpio cs */
			if (spi_cs_is_gpio(config)) {
				spi_context_cs_control(&data->ctx, true);
			}

			/* Clear interrupt flag*/
			ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_STATUS, MST_DONE_INT_MASK);
			/* Enable interrupt */
			ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE, MST_DONE_INT_ENABLE_MASK);
		} else {
			/* TODO: spi slave support */
		}

		dw_reg_set_bits(cfg, R_SSI_SSIENR, SSI_EN_MASK);

		err = spi_rts5817_transceive_once(dev, data);
		if (err) {
			dw_reg_clear_bits(cfg, R_SSI_SSIENR, SSI_EN_MASK);
			ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE,
					    MST_DONE_INT_ENABLE_MASK);
			goto out;
		}

		err = spi_context_wait_for_completion(&data->ctx);

#ifdef CONFIG_SPI_SLAVE
		if (spi_context_is_slave(&spi->ctx) && !err) {
			err = spi->ctx.recv_frames;
		}
#endif /* CONFIG_SPI_SLAVE */
	} else {
		if (!spi_rts5817_is_slave(data)) {
			/* enable ssi ser */
			dw_reg_set_bits(cfg, R_SSI_SER, SER_MASK);
			/* if cs is defined as gpio, enable gpio cs */
			if (spi_cs_is_gpio(config)) {
				spi_context_cs_control(&data->ctx, true);
			}
		}

		dw_reg_set_bits(cfg, R_SSI_SSIENR, SSI_EN_MASK);

		do {
			err = spi_rts5817_transceive_once(dev, data);
		} while (!err && spi_rts5817_transfer_ongoing(data));

		dw_reg_clear_bits(cfg, R_SSI_SSIENR, SSI_EN_MASK);

		if (!spi_rts5817_is_slave(data)) {
			/* disable ssi ser */
			dw_reg_clear_bits(cfg, R_SSI_SER, SER_MASK);
			/* if cs is defined as gpio, disable gpio cs */
			if (spi_cs_is_gpio(config)) {
				spi_context_cs_control(&data->ctx, false);
			}
		}
	}

out:
	spi_context_release(&data->ctx, err);

	return err;
}

static int spi_rts5817_transceive(const struct device *dev, const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	LOG_DBG("%p, %p, %p", dev, tx_bufs, rx_bufs);

	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_rts5817_transceive_async(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					void *userdata)
{
	LOG_DBG("%p, %p, %p, %p, %p", dev, tx_bufs, rx_bufs, cb, userdata);

	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_rts5817_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_rts5817_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

void spi_rts5817_isr(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	uint32_t int_status = 0;
	int err = 0;

	int_status = ctrl_reg_read(cfg, R_MST_SPI_SSI_IRQ_STATUS);

	LOG_DBG("SPI %p int_status 0x%x", dev, int_status);

	if (int_status & MST_DONE_INT_MASK) {
		ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_STATUS, MST_DONE_INT_MASK);
		err = spi_rts5817_transceive_next(cfg, data);
	} else {
		err = -EIO;
	}

	completed(dev, err);
}

static int spi_rts5817_clock_enable(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	int err = 0;

	if (device_is_ready(cfg->clock_dev)) {
		clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &data->clock_frequency);
	} else {
		err = -ENODEV;
	}

	return err;
}

static DEVICE_API(spi, rts5817_spi_api) = {
	.transceive = spi_rts5817_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_rts5817_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_rts5817_release,
};

int spi_rts5817_init(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	int err = 0;

#ifdef CONFIG_PINCTRL
	pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
#endif

	err = spi_rts5817_clock_enable(dev);
	if (err < 0) {
		return err;
	}

	/* Masking interrupt and making sure controller is disabled */
	if (cfg->serial_target) {
		/* TODO: spi slave support */
	} else {
		ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE, SSI_MST_INT_MASK);
		ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_STATUS, SSI_MST_INT_MASK);
	}
	dw_reg_write(cfg, R_SSI_SSIENR, 0x0);
	dw_reg_write(cfg, R_SSI_IMR, 0x0);

	cfg->irq_config_func();

	LOG_DBG("RTS5817 SPI driver initialized on device: %p", dev);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SPI_RTS5817_IRQ_HANDLER(inst)                                                              \
	static void spi_rts5817_irq_config_##inst(void)                                            \
	{                                                                                          \
		IF_ENABLED(DT_INST_PROP(inst, interrupt_enable),                         \
			   (IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), \
					spi_rts5817_isr, DEVICE_DT_INST_GET(inst), 0);   \
			    irq_enable(DT_INST_IRQN(inst));))         \
	}

#define SPI_RTS5817_INIT(inst)                                                                     \
	IF_ENABLED(CONFIG_PINCTRL,               \
		  (PINCTRL_DT_INST_DEFINE(inst);))                                                 \
	SPI_RTS5817_IRQ_HANDLER(inst);                                                             \
	static struct spi_rts5817_data spi_rts5817_data_##inst = {                                 \
		SPI_CONTEXT_INIT_LOCK(spi_rts5817_data_##inst, ctx),                               \
		SPI_CONTEXT_INIT_SYNC(spi_rts5817_data_##inst, ctx),                               \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};                          \
	static const struct spi_rts5817_config spi_rts5817_config_##inst = {                       \
		.ctrl_regs = DT_INST_REG_ADDR_BY_NAME(inst, ctrl),                                 \
		.dw_regs = DT_INST_REG_ADDR_BY_NAME(inst, dw),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.irq_config_func = spi_rts5817_irq_config_##inst,                                  \
		.serial_target = DT_INST_PROP(inst, serial_target),                                \
		.interrupt_enable = DT_INST_PROP(inst, interrupt_enable),                          \
		IF_ENABLED(CONFIG_PINCTRL,                                                         \
			  (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),))};         \
	DEVICE_DT_INST_DEFINE(inst, spi_rts5817_init, NULL, &spi_rts5817_data_##inst,              \
			      &spi_rts5817_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,   \
			      &rts5817_spi_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_RTS5817_INIT)
