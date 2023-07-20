/*
 * Copyright (c) 2022 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_andes_atcspi200.h"

#include <zephyr/irq.h>

#define DT_DRV_COMPAT andestech_atcspi200

typedef void (*atcspi200_cfg_func_t)(void);

struct spi_atcspi200_data {
	struct spi_context ctx;
	uint32_t tx_fifo_size;
	uint32_t rx_fifo_size;
	int tx_cnt;
	size_t chunk_len;
	bool busy;
};

struct spi_atcspi200_cfg {
	atcspi200_cfg_func_t cfg_func;
	uint32_t base;
	uint32_t irq_num;
	uint32_t f_sys;
	bool xip;
};

/* API Functions */
static int spi_config(const struct device *dev,
		      const struct spi_config *config)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	uint32_t sclk_div, data_len;

	/* Set the divisor for SPI interface sclk */
	sclk_div = (cfg->f_sys / (config->frequency << 1)) - 1;
	sys_clear_bits(SPI_TIMIN(cfg->base), TIMIN_SCLK_DIV_MSK);
	sys_set_bits(SPI_TIMIN(cfg->base), sclk_div);

	/* Set Master mode */
	sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_SLVMODE_MSK);

	/* Disable data merge mode */
	sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_DATA_MERGE_MSK);

	/* Set data length */
	data_len = SPI_WORD_SIZE_GET(config->operation) - 1;
	sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_DATA_LEN_MSK);
	sys_set_bits(SPI_TFMAT(cfg->base), (data_len << TFMAT_DATA_LEN_OFFSET));

	/* Set SPI frame format */
	if (config->operation & SPI_MODE_CPHA) {
		sys_set_bits(SPI_TFMAT(cfg->base), TFMAT_CPHA_MSK);
	} else {
		sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_CPHA_MSK);
	}

	if (config->operation & SPI_MODE_CPOL) {
		sys_set_bits(SPI_TFMAT(cfg->base), TFMAT_CPOL_MSK);
	} else {
		sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_CPOL_MSK);
	}

	/* Set SPI bit order */
	if (config->operation & SPI_TRANSFER_LSB) {
		sys_set_bits(SPI_TFMAT(cfg->base), TFMAT_LSB_MSK);
	} else {
		sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_LSB_MSK);
	}

	/* Set TX/RX FIFO threshold */
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_TX_THRES_MSK);
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_RX_THRES_MSK);

	sys_set_bits(SPI_CTRL(cfg->base), TX_FIFO_THRESHOLD << CTRL_TX_THRES_OFFSET);
	sys_set_bits(SPI_CTRL(cfg->base), RX_FIFO_THRESHOLD << CTRL_RX_THRES_OFFSET);

	return 0;
}

static int spi_transfer(const struct device *dev)
{
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t data_len, tctrl, int_msk;

	if (data->chunk_len != 0) {
		data_len = data->chunk_len - 1;
	} else {
		data_len = 0;
	}

	if (len > MAX_TRANSFER_CNT) {
		return -EINVAL;
	}

	data->tx_cnt = 0;

	if (!spi_context_rx_on(ctx)) {
		tctrl = (TRNS_MODE_WRITE_ONLY << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_WR_TCNT_OFFSET);
		int_msk = IEN_TX_FIFO_MSK | IEN_END_MSK;
	} else if (!spi_context_tx_on(ctx)) {
		tctrl = (TRNS_MODE_READ_ONLY << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_RD_TCNT_OFFSET);
		int_msk = IEN_RX_FIFO_MSK | IEN_END_MSK;
	} else {
		tctrl = (TRNS_MODE_WRITE_READ << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_WR_TCNT_OFFSET) |
			(data_len << TCTRL_RD_TCNT_OFFSET);
		int_msk = IEN_TX_FIFO_MSK |
			  IEN_RX_FIFO_MSK |
			  IEN_END_MSK;
	}

	sys_write32(tctrl, SPI_TCTRL(cfg->base));

	/* Enable TX/RX FIFO interrupts */
	sys_write32(int_msk, SPI_INTEN(cfg->base));

	/* Start transferring */
	sys_write32(0, SPI_CMD(cfg->base));

	return 0;
}


static int configure(const struct device *dev,
		     const struct spi_config *config)
{
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &(data->ctx);

	if (spi_context_configured(ctx, config)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported on %s",
			    dev->name);
		return -EINVAL;
	}

	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	ctx->config = config;

	/* SPI configuration */
	spi_config(dev, config);

	return 0;
}

static int transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs,
			  bool asynchronous,
			  spi_callback_t cb,
			  void *userdata)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int error, dfs;
	size_t chunk_len;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);
	error = configure(dev, config);
	if (error == 0) {
		data->busy = true;

		dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;
		spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, dfs);
		spi_context_cs_control(ctx, true);

		transfer_next_chunk(dev);
		sys_set_bits(SPI_CTRL(cfg->base), CTRL_TX_FIFO_RST_MSK);
		sys_set_bits(SPI_CTRL(cfg->base), CTRL_RX_FIFO_RST_MSK);

		if (!spi_context_rx_on(ctx)) {
			chunk_len = spi_context_total_tx_len(ctx);
		} else if (!spi_context_tx_on(ctx)) {
			chunk_len = spi_context_total_rx_len(ctx);
		} else {
			size_t rx_len = spi_context_total_rx_len(ctx);
			size_t tx_len = spi_context_total_tx_len(ctx);

			chunk_len = MIN(rx_len, tx_len);
		}

		data->chunk_len = chunk_len;

		error = spi_transfer(dev);
		if (error != 0) {
			return error;
		}

		error = spi_context_wait_for_completion(ctx);
		spi_context_cs_control(ctx, false);
	}

	spi_context_release(ctx, error);

	return error;
}

int spi_atcspi200_transceive(const struct device *dev,
			const struct spi_config *config,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
int spi_atcspi200_transceive_async(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

int spi_atcspi200_release(const struct device *dev,
			  const struct spi_config *config)
{

	struct spi_atcspi200_data * const data = dev->data;

	if (data->busy) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

int spi_atcspi200_init(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	int err = 0;

	/* we should not configure the device we are running on */
	if (cfg->xip) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	/* Get the TX/RX FIFO size of this device */
	data->tx_fifo_size = TX_FIFO_SIZE(cfg->base);
	data->rx_fifo_size = RX_FIFO_SIZE(cfg->base);

	cfg->cfg_func();

	irq_enable(cfg->irq_num);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	return 0;
}

static const struct spi_driver_api spi_atcspi200_api = {
	.transceive = spi_atcspi200_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_atcspi200_transceive_async,
#endif
	.release = spi_atcspi200_release
};

static void spi_atcspi200_irq_handler(void *arg)
{
	const struct device * const dev = (const struct device *) arg;
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t rx_data, cur_tx_fifo_num, cur_rx_fifo_num;
	uint32_t i, dfs, intr_status, spi_status;
	uint32_t tx_num = 0, tx_data = 0;
	int error = 0;

	intr_status = sys_read32(SPI_INTST(cfg->base));
	dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;

	if ((intr_status & INTST_TX_FIFO_INT_MSK) &&
	    !(intr_status & INTST_END_INT_MSK)) {

		spi_status = sys_read32(SPI_STAT(cfg->base));
		cur_tx_fifo_num = GET_TX_NUM(cfg->base);

		tx_num = data->tx_fifo_size - cur_tx_fifo_num;

		for (i = tx_num; i > 0; i--) {

			if (data->tx_cnt >= data->chunk_len) {
				/* Have already sent a chunk of data, so stop
				 * sending data!
				 */
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_TX_FIFO_MSK);
				break;
			}

			if (spi_context_tx_buf_on(ctx)) {

				switch (dfs) {
				case 1:
					tx_data = *ctx->tx_buf;
					break;
				case 2:
					tx_data = *(uint16_t *)ctx->tx_buf;
					break;
				}

			} else if (spi_context_tx_on(ctx)) {
				tx_data = 0;
			} else {
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_TX_FIFO_MSK);
				break;
			}

			sys_write32(tx_data, SPI_DATA(cfg->base));

			spi_context_update_tx(ctx, dfs, 1);

			data->tx_cnt++;
		}
		sys_write32(INTST_TX_FIFO_INT_MSK, SPI_INTST(cfg->base));

	}

	if (intr_status & INTST_RX_FIFO_INT_MSK) {
		cur_rx_fifo_num = GET_RX_NUM(cfg->base);

		for (i = cur_rx_fifo_num; i > 0; i--) {

			rx_data = sys_read32(SPI_DATA(cfg->base));

			if (spi_context_rx_buf_on(ctx)) {

				switch (dfs) {
				case 1:
					*ctx->rx_buf = rx_data;
					break;
				case 2:
					*(uint16_t *)ctx->rx_buf = rx_data;
					break;
				}

			} else if (!spi_context_rx_on(ctx)) {
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_RX_FIFO_MSK);
			}

			spi_context_update_rx(ctx, dfs, 1);
		}
		sys_write32(INTST_RX_FIFO_INT_MSK, SPI_INTST(cfg->base));
	}

	if (intr_status & INTST_END_INT_MSK) {

		/* Clear end interrupt */
		sys_write32(INTST_END_INT_MSK, SPI_INTST(cfg->base));

		/* Disable all SPI interrupts */
		sys_write32(0, SPI_INTEN(cfg->base));

		data->busy = false;

		spi_context_complete(ctx, dev, error);

	}
}

#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)
#define SPI_BUSY_INIT .busy = false,

#if (CONFIG_XIP)
#define SPI_ROM_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_BUS(DT_CHOSEN(zephyr_flash)))
#else
#define SPI_ROM_CFG_XIP(node_id) false
#endif

#define SPI_INIT(n)							\
	static struct spi_atcspi200_data spi_atcspi200_dev_data_##n = {\
		.self_dev = DEVICE_DT_INST_GET(n),			\
		SPI_CONTEXT_INIT_LOCK(spi_atcspi200_dev_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_atcspi200_dev_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
		SPI_BUSY_INIT						\
		SPI_DMA_CHANNEL(n, rx, RX, PERIPHERAL, MEMORY)		\
		SPI_DMA_CHANNEL(n, tx, TX, MEMORY, PERIPHERAL)		\
	};								\
	static void spi_atcspi200_cfg_##n(void);			\
	static struct spi_atcspi200_cfg spi_atcspi200_dev_cfg_##n = {	\
		.cfg_func = spi_atcspi200_cfg_##n,			\
		.base = DT_INST_REG_ADDR(n),				\
		.irq_num = DT_INST_IRQN(n),				\
		.f_sys = DT_INST_PROP(n, clock_frequency),		\
		.xip = SPI_ROM_CFG_XIP(DT_DRV_INST(n)),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
		spi_atcspi200_init,					\
		NULL,							\
		&spi_atcspi200_dev_data_##n,				\
		&spi_atcspi200_dev_cfg_##n,				\
		POST_KERNEL,						\
		CONFIG_SPI_INIT_PRIORITY,				\
		&spi_atcspi200_api);					\
									\
	static void spi_atcspi200_cfg_##n(void)				\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),		\
				spi_atcspi200_irq_handler,		\
				DEVICE_DT_INST_GET(n),			\
				0);					\
	};

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
