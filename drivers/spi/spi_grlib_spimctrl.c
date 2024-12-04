/*
 * Copyright (c) 2023 Frontgrade Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gaisler_spimctrl

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_spimctrl);
#include "spi_context.h"


struct spimctrl_regs {
	uint32_t conf;
	uint32_t ctrl;
	uint32_t stat;
	uint32_t rx;
	uint32_t tx;
};

#define CONF_READCMD    0x0000007f
#define CTRL_RST        0x00000010
#define CTRL_CSN        0x00000008
#define CTRL_EAS        0x00000004
#define CTRL_IEN        0x00000002
#define CTRL_USRC       0x00000001
#define STAT_INIT       0x00000004
#define STAT_BUSY       0x00000002
#define STAT_DONE       0x00000001

#define SPI_DATA(dev) ((struct data *) ((dev)->data))

struct cfg {
	volatile struct spimctrl_regs *regs;
	int interrupt;
};

struct data {
	struct spi_context ctx;
};

static int spi_config(struct spi_context *ctx, const struct spi_config *config)
{
	if (config->slave != 0) {
		LOG_ERR("More slaves than supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size must be 8");
		return -ENOTSUP;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		LOG_ERR("Only supports CPOL=CPHA=0");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback not supported");
		return -ENOTSUP;
	}

	ctx->config = config;

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs)
{
	const struct cfg *const cfg = dev->config;
	volatile struct spimctrl_regs *const regs = cfg->regs;
	struct spi_context *ctx = &SPI_DATA(dev)->ctx;
	uint8_t txval;
	int rc;

	spi_context_lock(ctx, false, NULL, NULL, config);

	rc = spi_config(ctx, config);
	if (rc) {
		LOG_ERR("%s: config", __func__);
		spi_context_release(ctx, rc);
		return rc;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	regs->ctrl |= (CTRL_USRC | CTRL_IEN);
	regs->ctrl &= ~CTRL_CSN;

	if (spi_context_tx_buf_on(ctx)) {
		txval = *ctx->tx_buf;
		spi_context_update_tx(ctx, 1, 1);
	} else {
		txval = 0;
	}
	/* This will eventually trig the interrupt */
	regs->tx = txval;

	rc = spi_context_wait_for_completion(ctx);

	regs->ctrl |= CTRL_CSN;
	regs->ctrl &= ~CTRL_USRC;
	spi_context_release(ctx, rc);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int transceive_async(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs,
			    const struct spi_buf_set *rx_bufs,
			    struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int release(const struct device *dev, const struct spi_config *config)
{
	spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);
	return 0;
}

static void spim_isr(struct device *dev)
{
	const struct cfg *const cfg = dev->config;
	volatile struct spimctrl_regs *const regs = cfg->regs;
	struct spi_context *ctx = &SPI_DATA(dev)->ctx;
	uint8_t rx_byte;
	uint8_t val;

	if ((regs->stat & STAT_DONE) == 0) {
		return;
	}

	regs->stat = STAT_DONE;

	/* Always read register and maybe write mem. */
	rx_byte = regs->rx;
	if (spi_context_rx_on(ctx)) {
		*ctx->rx_buf = rx_byte;
		spi_context_update_rx(ctx, 1, 1);
	}

	if (spi_context_tx_buf_on(ctx) == false && spi_context_rx_buf_on(ctx) == false) {
		regs->ctrl &= ~CTRL_IEN;
		spi_context_complete(ctx, dev, 0);
		return;
	}

	val = 0;
	if (spi_context_tx_buf_on(ctx)) {
		val = *ctx->tx_buf;
		spi_context_update_tx(ctx, 1, 1);
	}
	regs->tx = val;
}

static int init(const struct device *dev)
{
	const struct cfg *const cfg = dev->config;
	volatile struct spimctrl_regs *const regs = cfg->regs;

	regs->ctrl = CTRL_CSN;
	while (regs->stat & STAT_BUSY) {
		;
	}
	regs->stat = STAT_DONE;

	irq_connect_dynamic(
		cfg->interrupt,
		0,
		(void (*)(const void *)) spim_isr,
		dev,
		0
	);
	irq_enable(cfg->interrupt);

	spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);

	return 0;
}

static DEVICE_API(spi, api) = {
	.transceive             = transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async       = transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release                = release,
};

#define SPI_INIT(n)	                                                \
	static const struct cfg cfg_##n = {                             \
		.regs           = (struct spimctrl_regs *)              \
				  DT_INST_REG_ADDR(n),                  \
		.interrupt      = DT_INST_IRQN(n),                      \
	};                                                              \
	static struct data data_##n = {                                 \
		SPI_CONTEXT_INIT_LOCK(data_##n, ctx),                   \
		SPI_CONTEXT_INIT_SYNC(data_##n, ctx),                   \
	};                                                              \
	DEVICE_DT_INST_DEFINE(n,                                        \
			init,                                           \
			NULL,                                           \
			&data_##n,                                      \
			&cfg_##n,                                       \
			POST_KERNEL,                                    \
			CONFIG_SPI_INIT_PRIORITY,                       \
			&api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
