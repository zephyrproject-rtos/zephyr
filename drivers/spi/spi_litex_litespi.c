/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_spi_litespi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_litex_litespi);

#include <zephyr/sys/byteorder.h>
#include "spi_litex_common.h"

#define SPI_LITEX_ANY_HAS_IRQ DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)

#define SPIFLASH_CORE_MASTER_PHYCONFIG_LEN_OFFSET   0x0
#define SPIFLASH_CORE_MASTER_PHYCONFIG_WIDTH_OFFSET 0x1
#define SPIFLASH_CORE_MASTER_PHYCONFIG_MASK_OFFSET  0x2

#define SPIFLASH_CORE_MASTER_STATUS_TX_READY_OFFSET 0x0
#define SPIFLASH_CORE_MASTER_STATUS_RX_READY_OFFSET 0x1

#define SPI_MAX_WORD_SIZE 32
#define SPI_MAX_CS_SIZE   4

struct spi_litex_dev_config {
	uint32_t core_master_cs_addr;
	uint32_t core_master_phyconfig_addr;
	uint32_t core_master_rxtx_addr;
	uint32_t core_master_rxtx_size;
	uint32_t core_master_status_addr;
	uint32_t phy_clk_divisor_addr;
	bool phy_clk_divisor_exists;
#if SPI_LITEX_ANY_HAS_IRQ
	bool has_irq;
	uint32_t core_master_ev_pending_addr;
	uint32_t core_master_ev_enable_addr;
#endif
};

struct spi_litex_data {
	struct spi_context ctx;
	uint8_t dfs; /* dfs in bytes: 1,2 or 4 */
#if SPI_LITEX_ANY_HAS_IRQ
	struct k_sem sem_rx_ready;
#endif /* SPI_LITEX_ANY_HAS_IRQ */
};


static int spi_litex_set_frequency(const struct device *dev, const struct spi_config *config)
{
	const struct spi_litex_dev_config *dev_config = dev->config;

	if (!dev_config->phy_clk_divisor_exists) {
		/* In the LiteX Simulator the phy_clk_divisor doesn't exists, thats why we check. */
		LOG_WRN("No phy_clk_divisor found, can't change frequency");
		return 0;
	}

	uint32_t divisor = DIV_ROUND_UP(sys_clock_hw_cycles_per_sec(), (2 * config->frequency)) - 1;

	litex_write32(divisor, dev_config->phy_clk_divisor_addr);
	return 0;
}

/* Helper Functions */
static int spi_config(const struct device *dev, const struct spi_config *config)
{
	struct spi_litex_data *dev_data = dev->data;

	if (config->slave != 0) {
		if (config->slave >= SPI_MAX_CS_SIZE) {
			LOG_ERR("More slaves than supported");
			return -ENOTSUP;
		}
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) > SPI_MAX_WORD_SIZE) {
		LOG_ERR("Word size must be <= %d, is %d", SPI_MAX_WORD_SIZE,
			SPI_WORD_SIZE_GET(config->operation));
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

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
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
		LOG_ERR("Loopback mode not supported");
		return -ENOTSUP;
	}

	dev_data->dfs = get_dfs_value(config);

	spi_litex_set_frequency(dev, config);

	return 0;
}

static void spiflash_len_mask_width_write(uint32_t len, uint32_t width, uint32_t mask,
					  uint32_t addr)
{
	uint32_t tmp = len & BIT_MASK(8);
	uint32_t word = tmp << (SPIFLASH_CORE_MASTER_PHYCONFIG_LEN_OFFSET * 8);

	tmp = width & BIT_MASK(8);
	word |= tmp << (SPIFLASH_CORE_MASTER_PHYCONFIG_WIDTH_OFFSET * 8);
	tmp = mask & BIT_MASK(8);
	word |= tmp << (SPIFLASH_CORE_MASTER_PHYCONFIG_MASK_OFFSET * 8);
	litex_write32(word, addr);
}

static void spi_litex_wait_for_rx_ready(const struct device *dev)
{
	const struct spi_litex_dev_config *dev_config = dev->config;

#if SPI_LITEX_ANY_HAS_IRQ
	struct spi_litex_data *data = dev->data;

	if (dev_config->has_irq) {
		/* Wait for the RX ready event */
		k_sem_take(&data->sem_rx_ready, K_FOREVER);
		return;
	}
#endif /* SPI_LITEX_ANY_HAS_IRQ */

	while (!(litex_read8(dev_config->core_master_status_addr) &
		 BIT(SPIFLASH_CORE_MASTER_STATUS_RX_READY_OFFSET))) {
		;
	}
}

static int spi_litex_xfer(const struct device *dev, const struct spi_config *config)
{
	const struct spi_litex_dev_config *dev_config = dev->config;
	struct spi_litex_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t txd, rxd;
	int ret = 0;

	uint8_t len = data->dfs; /* SPI Xfer length*/
	uint8_t old_len = len;   /* old SPI Xfer length*/
	uint8_t width = BIT(0);  /* SPI Xfer width*/
	uint8_t mask = BIT(0);   /* SPI Xfer mask*/

	spiflash_len_mask_width_write(len * 8, width, mask, dev_config->core_master_phyconfig_addr);

	litex_write32(BIT(config->slave), dev_config->core_master_cs_addr);

	/* Flush RX buffer */
	while ((litex_read8(dev_config->core_master_status_addr) &
		BIT(SPIFLASH_CORE_MASTER_STATUS_RX_READY_OFFSET))) {
		rxd = litex_read32(dev_config->core_master_rxtx_addr);
		LOG_DBG("flushed rxd: 0x%x", rxd);
	}

#if SPI_LITEX_ANY_HAS_IRQ
	if (dev_config->has_irq) {
		litex_write8(BIT(0), dev_config->core_master_ev_enable_addr);
		litex_write8(BIT(0), dev_config->core_master_ev_pending_addr);
		k_sem_reset(&data->sem_rx_ready);
	}
#endif /* SPI_LITEX_ANY_HAS_IRQ */

	do {
		len = MIN(spi_context_max_continuous_chunk(ctx), dev_config->core_master_rxtx_size);
		if (len != old_len) {
			spiflash_len_mask_width_write(len * 8, width, mask,
						      dev_config->core_master_phyconfig_addr);
			old_len = len;
		}

		if (spi_context_tx_buf_on(ctx)) {
			litex_spi_tx_put(len, &txd, ctx->tx_buf);
		} else {
			txd = 0U;
		}

		while (!(litex_read8(dev_config->core_master_status_addr) &
			 BIT(SPIFLASH_CORE_MASTER_STATUS_TX_READY_OFFSET))) {
			;
		}

		LOG_DBG("txd: 0x%x", txd);
		litex_write32(txd, dev_config->core_master_rxtx_addr);

		spi_context_update_tx(ctx, data->dfs, len / data->dfs);

		spi_litex_wait_for_rx_ready(dev);

		rxd = litex_read32(dev_config->core_master_rxtx_addr);
		LOG_DBG("rxd: 0x%x", rxd);

		if (spi_context_rx_buf_on(ctx)) {
			litex_spi_rx_put(len, &rxd, ctx->rx_buf);
		}

		spi_context_update_rx(ctx, data->dfs, len / data->dfs);

	} while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx));

	litex_write32(0, dev_config->core_master_cs_addr);

#if SPI_LITEX_ANY_HAS_IRQ
	if (dev_config->has_irq) {
		/* Wait for the RX ready event */
		litex_write8(0, dev_config->core_master_ev_enable_addr);
	}
#endif /* SPI_LITEX_ANY_HAS_IRQ */
	spi_context_complete(ctx, dev, 0);

	return ret;
}

static int spi_litex_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_litex_data *data = dev->data;

	int ret = spi_config(dev, config);

	if (ret) {
		return ret;
	}

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	ret = spi_litex_xfer(dev, config);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_litex_transceive_async(const struct device *dev, const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_litex_release(const struct device *dev, const struct spi_config *config)
{

	return 0;
}

#if SPI_LITEX_ANY_HAS_IRQ
static void spi_litex_irq_handler(const struct device *dev)
{
	struct spi_litex_data *data = dev->data;
	const struct spi_litex_dev_config *dev_config = dev->config;

	if (litex_read8(dev_config->core_master_ev_pending_addr) & BIT(0)) {
		k_sem_give(&data->sem_rx_ready);

		/* ack reader irq */
		litex_write8(BIT(0), dev_config->core_master_ev_pending_addr);
	}
}
#endif /* SPI_LITEX_ANY_HAS_IRQ */

/* Device Instantiation */
static DEVICE_API(spi, spi_litex_api) = {
	.transceive = spi_litex_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_litex_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_litex_release,
};

#define SPI_LITEX_IRQ(n)									   \
	BUILD_ASSERT(DT_INST_REG_HAS_NAME(n, core_master_ev_pending) &&				   \
	DT_INST_REG_HAS_NAME(n, core_master_ev_enable), "registers for interrupts missing");	   \
												   \
	static int spi_litex_irq_config##n(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_litex_irq_handler,      \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
												   \
		return 0;                                                                          \
	};

#define SPI_LITEX_IRQ_DATA(n)									   \
	.sem_rx_ready = Z_SEM_INITIALIZER(spi_litex_data_##n.sem_rx_ready, 0, 1),

#define SPI_LITEX_IRQ_CONFIG(n)									   \
	.has_irq = DT_INST_IRQ_HAS_IDX(n, 0),							   \
	.core_master_ev_pending_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, core_master_ev_pending, 0),  \
	.core_master_ev_enable_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, core_master_ev_enable, 0),

#define SPI_INIT(n)										   \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (SPI_LITEX_IRQ(n)))				   \
												   \
	static struct spi_litex_data spi_litex_data_##n = {					   \
		SPI_CONTEXT_INIT_LOCK(spi_litex_data_##n, ctx),					   \
		SPI_CONTEXT_INIT_SYNC(spi_litex_data_##n, ctx),					   \
		IF_ENABLED(SPI_LITEX_ANY_HAS_IRQ, (SPI_LITEX_IRQ_DATA(n)))			   \
	};											   \
												   \
	static struct spi_litex_dev_config spi_litex_cfg_##n = {                                   \
		.core_master_cs_addr = DT_INST_REG_ADDR_BY_NAME(n, core_master_cs),                \
		.core_master_phyconfig_addr = DT_INST_REG_ADDR_BY_NAME(n, core_master_phyconfig),  \
		.core_master_rxtx_addr = DT_INST_REG_ADDR_BY_NAME(n, core_master_rxtx),            \
		.core_master_rxtx_size = DT_INST_REG_SIZE_BY_NAME(n, core_master_rxtx),            \
		.core_master_status_addr = DT_INST_REG_ADDR_BY_NAME(n, core_master_status),        \
		.phy_clk_divisor_exists = DT_INST_REG_HAS_NAME(n, phy_clk_divisor),                \
		.phy_clk_divisor_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, phy_clk_divisor, 0),        \
		IF_ENABLED(SPI_LITEX_ANY_HAS_IRQ, (SPI_LITEX_IRQ_CONFIG(n)))			   \
	};                                                                                         \
												   \
	SPI_DEVICE_DT_INST_DEFINE(n,								   \
		COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (spi_litex_irq_config##n), (NULL)), NULL,   \
		&spi_litex_data_##n, &spi_litex_cfg_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,	   \
		&spi_litex_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
