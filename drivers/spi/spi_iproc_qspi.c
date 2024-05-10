/*
 * Copyright (c) 2020, Broadcom
 * Copyright (c) 2024, Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_iproc_qspi

#include <errno.h>
#include <stdint.h>

#include "spi_iproc_qspi.h"

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
LOG_MODULE_REGISTER(spi_iproc_qspi);

#include "spi_context.h"

#define DEV_CFG(dev)       ((struct iproc_qspi_config *)(dev)->config)
#define DEV_DATA(dev)      ((struct iproc_qspi_data *)(dev)->data)
#define DEV_BSPI_BASE(dev) ((mem_addr_t)(DEV_CFG(dev))->bspi_base)
#define DEV_MSPI_BASE(dev) ((mem_addr_t)(DEV_CFG(dev))->mspi_base)

struct iproc_qspi_config {
	mem_addr_t bspi_base;
	mem_addr_t mspi_base;
};

struct iproc_qspi_data {
	struct spi_context ctx;
	size_t transfer_len;
};

static void bspi_flush_prefetch_buffers(const struct device *dev)
{
	/* Flush BSPI pre-fetch buffers - Needs a rising edge on the register */
	sys_clear_bit(DEV_BSPI_BASE(dev) + BSPI_B0_CTRL, BSPI_B0_CTRL_B0_FLUSH_SHIFT);
	sys_set_bit(DEV_BSPI_BASE(dev) + BSPI_B0_CTRL, BSPI_B0_CTRL_B0_FLUSH_SHIFT);

	sys_clear_bit(DEV_BSPI_BASE(dev) + BSPI_B1_CTRL, BSPI_B1_CTRL_B1_FLUSH_SHIFT);
	sys_set_bit(DEV_BSPI_BASE(dev) + BSPI_B1_CTRL, BSPI_B1_CTRL_B1_FLUSH_SHIFT);
}

static void mspi_acquire_bus(const struct device *dev)
{
	unsigned long start = 0;
	uint32_t bspi_mast_n_boot =
		sys_read32(DEV_BSPI_BASE(dev) + BSPI_MAST_N_BOOT_CTRL) & BIT(MAST_N_BOOT_SHIFT);

	/* If BSPI is busy, wait for BSPI to complete the transaction */
	if ((bspi_mast_n_boot) == 0) {
		/* Poll on bspi busy status bit */
		while (start < QSPI_WAIT_TIMEOUT_US) {
			uint32_t bspi_busy =
				sys_read32(DEV_BSPI_BASE(dev) + BSPI_BUSY_STATUS) & BIT(BUSY_SHIFT);

			if ((bspi_busy) == 0) {
				/* Claim the bus by setting mast_n_boot bit */
				sys_set_bit(DEV_BSPI_BASE(dev) + BSPI_MAST_N_BOOT_CTRL,
					    MAST_N_BOOT_SHIFT);
				k_usleep(1);
				break;
			}
			k_usleep(1);
			start++;
		}
	}
}

static void mspi_release_bus(const struct device *dev)
{
	/* Flush pre-fetch buffers */
	bspi_flush_prefetch_buffers(dev);

	/* Release the bus by clearing mast_n_boot bit */
	sys_write32(0x0, DEV_BSPI_BASE(dev) + BSPI_MAST_N_BOOT_CTRL);
}

static int mspi_xfer(const struct device *dev, uint8_t *tx, uint8_t *rx, uint32_t len, bool end)
{
	struct iproc_qspi_data *data = DEV_DATA(dev);
	struct spi_context *ctx = &data->ctx;
	uint32_t cdram_val;
	uint32_t bytes = len;
	uint32_t txram_addr, rxram_addr, cdram_addr;

	/* Set PCS val */
	cdram_val = BIT(ctx->config->slave) ^ MSPI_CDRAM_PCS_MASK;
	cdram_val |= BIT(MSPI_CDRAM_CONT_SHIFT);

	while (bytes != 0) {
		/* Determine how many bytes to process this time */
		uint32_t chunk = MIN(bytes, NUM_CDRAM);

		/* Fill CDRAMs and TXRAMS*/
		/* Program 1st data byte in TXRAM00(offset 0x0) & set CDRAM00
		 * Program 2nd data byte in TXRAM02(offset 0x8) and set CDRAM01
		 * Similarly Continue the above steps for all other data bytes
		 */
		for (uint32_t i = 0; i < chunk; i++) {
			/* calculate txram_addr to fill data byte to be sent */
			txram_addr = DEV_MSPI_BASE(dev) + MSPI_TXRAM +
				     (i * (MSPI_TXRAM02_OFFSET - MSPI_TXRAM00_OFFSET));
			sys_write32(tx ? tx[i] : 0xff, txram_addr);

			/* calculate cdram addr */
			cdram_addr = DEV_MSPI_BASE(dev) + MSPI_CDRAM +
				     (i * (MSPI_CDRAM01_OFFSET - MSPI_CDRAM00_OFFSET));

			sys_write32(cdram_val, cdram_addr);
		}

		/* If this is the last chunk then clear
		 * the cont bit for the last write
		 */
		if (end && (bytes == chunk)) {
			cdram_addr = DEV_MSPI_BASE(dev) + MSPI_CDRAM +
				     ((chunk - 1) * (MSPI_CDRAM01_OFFSET - MSPI_CDRAM00_OFFSET));
			sys_write32((cdram_val & (~BIT(MSPI_CDRAM_CONT_SHIFT))), cdram_addr);
		}

		/* Setup queue pointers */
		sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_NEWQP);
		sys_write32(chunk - 1, DEV_MSPI_BASE(dev) + MSPI_ENDQP);

		/* Kick off */
		sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_STATUS);
		sys_write32(0xc0, DEV_MSPI_BASE(dev) + MSPI_SPCR2);

		uint32_t start = 0;
		/* Wait for SPIF - (SPI finished) */
		while (start < QSPI_WAIT_TIMEOUT_US) {
			if (sys_read32(DEV_MSPI_BASE(dev) + MSPI_STATUS) & MSPI_STATUS_SPIF_MASK) {
				break;
			}
			k_usleep(1);
			start++;
		}

		if (start >= QSPI_WAIT_TIMEOUT_US) {
			return -ETIMEDOUT;
		}

		/* Read data out */
		if (rx != NULL) {
			for (uint32_t i = 0,
				      index = MSPI_RXRAM01_OFFSET +
					      (i * (MSPI_RXRAM02_OFFSET - MSPI_RXRAM00_OFFSET));
			     i < chunk; i++) {
				rxram_addr = DEV_MSPI_BASE(dev) + MSPI_RXRAM + index;
				rx[i] = sys_read32(rxram_addr);
			}
		}

		/* Update remaining bytes and tx,rx pointers */
		bytes -= chunk;

		/* Advance pointers */
		if (tx != NULL) {
			tx += chunk;
		}
		if (rx != NULL) {
			rx += chunk;
		}
	}

	return 0;
}

static inline int mspi_tx_bytes(const struct device *dev, uint8_t *tx, uint32_t len, bool end)
{
	return mspi_xfer(dev, tx, NULL, len, end);
}

static inline int mspi_rx_bytes(const struct device *dev, uint8_t *rx, uint32_t len, bool end)
{
	return mspi_xfer(dev, NULL, rx, len, end);
}

static int mspi_transfer(const struct device *dev, uint8_t *tx, uint32_t tx_len, uint8_t *rx,
			 uint32_t rx_len)
{
	int ret;

	/* Check params */
	if ((tx == NULL) || (tx_len == 0)) {
		return -EINVAL;
	}

	/* Acquire the bus for MSPI transfer */
	mspi_acquire_bus(dev);

	/* Set the write lock bit */
	sys_write32(1, DEV_MSPI_BASE(dev) + MSPI_WRITE_LOCK);

	/* Send command and optionally address + data */
	ret = mspi_tx_bytes(dev, tx, tx_len, rx_len == 0);
	if (ret != 0) {
		goto done;
	}

	if ((rx != NULL) && (rx_len != 0)) {
		/* Read data */
		ret = mspi_rx_bytes(dev, rx, rx_len, true);
		if (ret != 0) {
			goto done;
		}
	}

done:
	/* Clear the write lock bit */
	sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_WRITE_LOCK);

	/* Release the bus to BSPI */
	mspi_release_bus(dev);

	return ret;
}

/*
 * 256 - max data sz
 *   4 - cmd + addr bytes
 */
#define MAX_TX_BUF_LEN (256 + 4)

static int iproc_qspi_xfer(const struct device *dev)
{
	struct iproc_qspi_data *data = DEV_DATA(dev);
	struct spi_context *ctx = &data->ctx;
	uint8_t tx[MAX_TX_BUF_LEN];
	uint32_t tx_len, rx_len;
	uint8_t *rx;
	int ret = 0;

	/*
	 * In the buffer series in spi ctx,
	 * both tx and rx buf ptrs point to same buffers
	 * 1st buffer contain cmd and optionally addr
	 * 2nd buffer contain ptr to data and len
	 * To get the data pointer, update tx/rx with len
	 */
	tx_len = ctx->tx_len;
	memcpy(tx, ctx->tx_buf, ctx->tx_len);
	if (spi_context_rx_buf_on(&data->ctx) == 0) {
		/* Write operation, only tx */
		rx = NULL;
		rx_len = 0;
		/* Addr & data follows write cmd byte */
		if (tx_len != 1) {
			/* update tx and get nxt tx data pointer to send */
			spi_context_update_tx(&data->ctx, 1, data->ctx.tx_len);
			memcpy(tx + tx_len, ctx->tx_buf, ctx->tx_len);

			tx_len += ctx->tx_len;
		}

		ret = mspi_transfer(dev, tx, tx_len, rx, rx_len);
		spi_context_update_tx(ctx, 1, ctx->tx_len);
	} else {
		/* update tx rx ctx buf ptrs and get nxt data buf pointers */
		spi_context_update_tx(ctx, 1, ctx->tx_len);
		spi_context_update_rx(&data->ctx, 1, data->ctx.rx_len);
		rx = ctx->rx_buf;
		rx_len = ctx->rx_len;

		ret = mspi_transfer(dev, tx, tx_len, rx, rx_len);
		/* update tx also to indicate end of xfer */
		spi_context_update_tx(ctx, 1, ctx->tx_len);
		spi_context_update_rx(ctx, 1, rx_len);
	}

	return ret;
}

static bool iproc_qspi_transfer_ongoing(struct iproc_qspi_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int iproc_qspi_setup(const struct device *dev, const struct spi_config *spi_cfg)
{
	uint32_t val = 0x0;

	/* MSPI: Basic hardware initialization */
	sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_SPCR1_LSB);
	sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_SPCR1_MSB);
	sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_NEWQP);
	sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_ENDQP);
	sys_write32(0, DEV_MSPI_BASE(dev) + MSPI_SPCR2);

	/* MSPI: SCK configuration */
	sys_write32(SPBR_MIN, DEV_MSPI_BASE(dev) + MSPI_SPCR0_LSB);

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		val |= BIT(MSPI_SPCR0_MSB_CPOL_SHIFT);
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		val |= BIT(MSPI_SPCR0_MSB_CPHA_SHIFT);
	}

	val |= (SPI_WORD_SIZE_GET(spi_cfg->operation) << MSPI_SPCR0_MSB_BITS_SHIFT);

	val |= BIT(MSPI_SPCR0_MSB_MSTR_SHIFT);

	/* MSPI: Mode configuration */
	sys_write32(val, DEV_MSPI_BASE(dev) + MSPI_SPCR0_MSB);

	return 0;
}

static int iproc_qspi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct iproc_qspi_data *data = DEV_DATA(dev);

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		return -ENOTSUP;
	}

	iproc_qspi_setup(dev, spi_cfg);

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = spi_cfg;

	LOG_DBG("Installed config %p, slave %u", spi_cfg, spi_cfg->slave);

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	int ret;
	struct iproc_qspi_data *data = DEV_DATA(dev);

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = iproc_qspi_configure(dev, spi_cfg);
	if (ret) {
		goto release;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	do {
		ret = iproc_qspi_xfer(dev);
	} while (!ret && iproc_qspi_transfer_ongoing(data));

	spi_context_cs_control(&data->ctx, false);

release:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int iproc_qspi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int iproc_qspi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				       void *userdata)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int iproc_qspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct iproc_qspi_data *data = DEV_DATA(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int iproc_qspi_init(const struct device *dev)
{
	int err;
	struct iproc_qspi_data *data = DEV_DATA(dev);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	/* NOTE: Device initialization is done in configure function */
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api iproc_qspi_driver_api = {
	.transceive = iproc_qspi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = iproc_qspi_transceive_async,
#endif
	.release = iproc_qspi_release,
};

#define IPROC_QSPI_INIT(n)                                                                         \
	static const struct iproc_qspi_config config_##n = {                                       \
		.bspi_base = DT_INST_REG_ADDR_BY_NAME(n, bspi_regs),                               \
		.mspi_base = DT_INST_REG_ADDR_BY_NAME(n, mspi_regs),                               \
	};                                                                                         \
                                                                                                   \
	static struct iproc_qspi_data data_##n = {                                                 \
		SPI_CONTEXT_INIT_LOCK(data_##n, ctx), SPI_CONTEXT_INIT_SYNC(data_##n, ctx),        \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &iproc_qspi_init, NULL, &data_##n, &config_##n, POST_KERNEL,      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &iproc_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IPROC_QSPI_INIT)
