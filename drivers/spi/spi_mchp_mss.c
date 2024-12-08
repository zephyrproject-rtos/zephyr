/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_spi

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(mss_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* MSS SPI Register offsets */
#define MSS_SPI_REG_CONTROL     (0x00)
#define MSS_SPI_REG_TXRXDF_SIZE (0x04)
#define MSS_SPI_REG_STATUS      (0x08)
#define MSS_SPI_REG_INT_CLEAR   (0x0c)
#define MSS_SPI_REG_RX_DATA     (0x10)
#define MSS_SPI_REG_TX_DATA     (0x14)
#define MSS_SPI_REG_CLK_GEN     (0x18)
#define MSS_SPI_REG_SS          (0x1c)
#define MSS_SPI_REG_MIS         (0x20)
#define MSS_SPI_REG_RIS         (0x24)
#define MSS_SPI_REG_CONTROL2    (0x28)
#define MSS_SPI_REG_COMMAND     (0x2c)
#define MSS_SPI_REG_PKTSIZE     (0x30)
#define MSS_SPI_REG_CMD_SIZE    (0x34)
#define MSS_SPI_REG_HWSTATUS    (0x38)
#define MSS_SPI_REG_FRAMESUP    (0x50)

/* SPICR bit definitions */
#define MSS_SPI_CONTROL_ENABLE       BIT(0)
#define MSS_SPI_CONTROL_MASTER       BIT(1)
#define MSS_SPI_CONTROL_PROTO_MSK    BIT(2)
#define MSS_SPI_CONTROL_PROTO_MOTO   (0 << 2)
#define MSS_SPI_CONTROL_RX_DATA_INT  BIT(4)
#define MSS_SPI_CONTROL_TX_DATA_INT  BIT(5)
#define MSS_SPI_CONTROL_RX_OVER_INT  BIT(6)
#define MSS_SPI_CONTROL_TX_UNDER_INT BIT(7)
#define MSS_SPI_CONTROL_CNT_MSK      (0xffff << 8)
#define MSS_SPI_CONTROL_CNT_SHF      (8)
#define MSS_SPI_CONTROL_SPO          BIT(24)
#define MSS_SPI_CONTROL_SPH          BIT(25)
#define MSS_SPI_CONTROL_SPS          BIT(26)
#define MSS_SPI_CONTROL_FRAMEURUN    BIT(27)
#define MSS_SPI_CONTROL_CLKMODE      BIT(28)
#define MSS_SPI_CONTROL_BIGFIFO      BIT(29)
#define MSS_SPI_CONTROL_OENOFF       BIT(30)
#define MSS_SPI_CONTROL_RESET        BIT(31)

/* SPIFRAMESIZE bit definitions */
#define MSS_SPI_FRAMESIZE_DEFAULT (8)

/* SPISS bit definitions */
#define MSS_SPI_SSEL_MASK (0xff)
#define MSS_SPI_DIRECT    (0x100)
#define MSS_SPI_SSELOUT   (0x200)
#define MSS_SPI_MIN_SLAVE (0)
#define MSS_SPI_MAX_SLAVE (7)

/* SPIST bit definitions */
#define MSS_SPI_STATUS_ACTIVE                 BIT(14)
#define MSS_SPI_STATUS_SSEL                   BIT(13)
#define MSS_SPI_STATUS_FRAMESTART             BIT(12)
#define MSS_SPI_STATUS_TXFIFO_EMPTY_NEXT_READ BIT(11)
#define MSS_SPI_STATUS_TXFIFO_EMPTY           BIT(10)
#define MSS_SPI_STATUS_TXFIFO_FULL_NEXT_WRITE BIT(9)
#define MSS_SPI_STATUS_TXFIFO_FULL            BIT(8)
#define MSS_SPI_STATUS_RXFIFO_EMPTY_NEXT_READ BIT(7)
#define MSS_SPI_STATUS_RXFIFO_EMPTY           BIT(6)
#define MSS_SPI_STATUS_RXFIFO_FULL_NEXT_WRITE BIT(5)
#define MSS_SPI_STATUS_RXFIFO_FULL            BIT(4)
#define MSS_SPI_STATUS_TX_UNDERRUN            BIT(3)
#define MSS_SPI_STATUS_RX_OVERFLOW            BIT(2)
#define MSS_SPI_STATUS_RXDAT_RCED             BIT(1)
#define MSS_SPI_STATUS_TXDAT_SENT             BIT(0)

/* SPIINT register defines */
#define MSS_SPI_INT_TXDONE       BIT(0)
#define MSS_SPI_INT_RXRDY        BIT(1)
#define MSS_SPI_INT_RX_CH_OVRFLW BIT(2)
#define MSS_SPI_INT_TX_CH_UNDRUN BIT(3)
#define MSS_SPI_INT_CMD          BIT(4)
#define MSS_SPI_INT_SSEND        BIT(5)

/* SPICOMMAND bit definitions */
#define MSS_SPI_COMMAND_FIFO_MASK (0xC)

/* SPIFRAMESUP bit definitions */
#define MSS_SPI_FRAMESUP_UP_BYTES_MSK (0xFFFF << 16)
#define MSS_SPI_FRAMESUP_LO_BYTES_MSK (0xFFFF << 0)

struct mss_spi_config {
	mm_reg_t base;
	uint8_t clk_gen;
	int clock_freq;
};

struct mss_spi_transfer {
	uint32_t rx_len;
	uint32_t control;
};

struct mss_spi_data {
	struct spi_context ctx;
	struct mss_spi_transfer xfer;
};

static inline uint32_t mss_spi_read(const struct mss_spi_config *cfg, mm_reg_t offset)
{
	return sys_read32(cfg->base + offset);
}

static inline void mss_spi_write(const struct mss_spi_config *cfg, mm_reg_t offset, uint32_t val)
{
	sys_write32(val, cfg->base + offset);
}

static inline void mss_spi_hw_tfsz_set(const struct mss_spi_config *cfg, int len)
{
	uint32_t control;

	mss_spi_write(cfg, MSS_SPI_REG_FRAMESUP, (len & MSS_SPI_FRAMESUP_UP_BYTES_MSK));
	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);
	control &= ~MSS_SPI_CONTROL_CNT_MSK;
	control |= ((len & MSS_SPI_FRAMESUP_LO_BYTES_MSK) << MSS_SPI_CONTROL_CNT_SHF);
	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);
}

static inline void mss_spi_enable_controller(const struct mss_spi_config *cfg)
{
	uint32_t control;

	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);
	control |= MSS_SPI_CONTROL_ENABLE;
	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);
}

static inline void mss_spi_disable_controller(const struct mss_spi_config *cfg)
{
	uint32_t control;

	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);
	control &= ~MSS_SPI_CONTROL_ENABLE;
	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);
}

static void mss_spi_enable_ints(const struct mss_spi_config *cfg)
{
	uint32_t control;
	uint32_t mask = MSS_SPI_CONTROL_RX_DATA_INT | MSS_SPI_CONTROL_TX_DATA_INT |
			MSS_SPI_CONTROL_RX_OVER_INT | MSS_SPI_CONTROL_TX_UNDER_INT;

	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);
	control |= mask;
	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);
}

static void mss_spi_disable_ints(const struct mss_spi_config *cfg)
{
	uint32_t control;
	uint32_t mask = MSS_SPI_CONTROL_RX_DATA_INT | MSS_SPI_CONTROL_TX_DATA_INT |
			MSS_SPI_CONTROL_RX_OVER_INT | MSS_SPI_CONTROL_TX_UNDER_INT;

	mask = ~mask;
	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);
	control &= ~mask;
	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);
}

static inline void mss_spi_readwr_fifo(const struct device *dev)
{
	const struct mss_spi_config *cfg = dev->config;
	struct mss_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	struct mss_spi_transfer *xfer = &data->xfer;
	uint32_t rx_raw = 0, rd_byte_size, tr_len;
	uint32_t data8, transfer_idx = 0;
	int count;

	tr_len = spi_context_longest_current_buf(ctx);
	count = spi_context_total_tx_len(ctx);
	if (ctx->rx_buf) {
		rd_byte_size = count - tr_len;
	} else {
		rd_byte_size = 0;
	}
	mss_spi_hw_tfsz_set(cfg, count);

	mss_spi_enable_ints(cfg);
	spi_context_update_rx(ctx, 1, xfer->rx_len);
	while (transfer_idx < count) {
		if (!(mss_spi_read(cfg, MSS_SPI_REG_STATUS) & MSS_SPI_STATUS_RXFIFO_EMPTY)) {
			rx_raw = mss_spi_read(cfg, MSS_SPI_REG_RX_DATA);
			if (transfer_idx >= tr_len) {
				if (spi_context_rx_buf_on(ctx)) {
					UNALIGNED_PUT(rx_raw, (uint8_t *)ctx->rx_buf);
					spi_context_update_rx(ctx, 1, 1);
				}
			}
			++transfer_idx;
		}

		if (!(mss_spi_read(cfg, MSS_SPI_REG_STATUS) & MSS_SPI_STATUS_TXFIFO_FULL)) {
			if (spi_context_tx_buf_on(ctx)) {
				data8 = ctx->tx_buf[0];
				mss_spi_write(cfg, MSS_SPI_REG_TX_DATA, data8);
				spi_context_update_tx(ctx, 1, 1);
			} else {
				mss_spi_write(cfg, MSS_SPI_REG_TX_DATA, 0x0);
			}
		}
	}
}

static inline int mss_spi_select_slave(const struct mss_spi_config *cfg, int cs)
{
	uint32_t slave;
	uint32_t reg = mss_spi_read(cfg, MSS_SPI_REG_SS);

	slave = (cs >= MSS_SPI_MIN_SLAVE && cs <= MSS_SPI_MAX_SLAVE) ? (1 << cs) : 0;
	reg &= ~MSS_SPI_SSEL_MASK;
	reg |= slave;

	mss_spi_write(cfg, MSS_SPI_REG_SS, reg);

	return 0;
}

static inline void mss_spi_activate_cs(struct mss_spi_config *cfg)
{
	uint32_t reg = mss_spi_read(cfg, MSS_SPI_REG_SS);

	reg |= MSS_SPI_SSELOUT;
	mss_spi_write(cfg, MSS_SPI_REG_SS, reg);
}

static inline void mss_spi_deactivate_cs(const struct mss_spi_config *cfg)
{
	uint32_t reg = mss_spi_read(cfg, MSS_SPI_REG_SS);

	reg &= ~MSS_SPI_SSELOUT;
	mss_spi_write(cfg, MSS_SPI_REG_SS, reg);
}

static inline int mss_spi_clk_gen_set(const struct mss_spi_config *cfg,
				      const struct spi_config *spi_cfg)
{
	uint32_t idx, clkrate, val = 0, speed;

	if (spi_cfg->frequency > cfg->clock_freq) {
		speed = cfg->clock_freq / 2;
	}

	for (idx = 1; idx < 16; idx++) {
		clkrate = cfg->clock_freq / (2 * idx);
		if (clkrate <= spi_cfg->frequency) {
			val = idx;
			break;
		}
	}

	mss_spi_write(cfg, MSS_SPI_REG_CLK_GEN, val);

	return 0;
}

static inline int mss_spi_hw_mode_set(const struct mss_spi_config *cfg, unsigned int mode)
{
	uint32_t control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);

	/* set the mode */
	if (mode & SPI_MODE_CPHA) {
		control |= MSS_SPI_CONTROL_SPH;
	} else {
		control &= ~MSS_SPI_CONTROL_SPH;
	}

	if (mode & SPI_MODE_CPOL) {
		control |= MSS_SPI_CONTROL_SPO;
	} else {
		control &= ~MSS_SPI_CONTROL_SPO;
	}

	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);

	return 0;
}

static void mss_spi_interrupt(const struct device *dev)
{
	const struct mss_spi_config *cfg = dev->config;
	struct mss_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int intfield = mss_spi_read(cfg, MSS_SPI_REG_MIS) & 0xf;

	if (intfield == 0) {
		return;
	}

	mss_spi_write(cfg, MSS_SPI_REG_INT_CLEAR, intfield);
	spi_context_complete(ctx, dev, 0);
}

static int mss_spi_release(const struct device *dev, const struct spi_config *config)
{
	const struct mss_spi_config *cfg = dev->config;
	struct mss_spi_data *data = dev->data;

	mss_spi_disable_ints(cfg);

	/* release kernel resources */
	spi_context_unlock_unconditionally(&data->ctx);
	mss_spi_disable_controller(cfg);

	return 0;
}

static int mss_spi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct mss_spi_config *cfg = dev->config;
	struct mss_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	struct mss_spi_transfer *xfer = &data->xfer;
	uint32_t control;

	if (spi_cfg->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE | SPI_MODE_LOOP)) {
		LOG_WRN("not supported operation\n\r");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != MSS_SPI_FRAMESIZE_DEFAULT) {
		return -ENOTSUP;
	}

	ctx->config = spi_cfg;
	mss_spi_select_slave(cfg, spi_cfg->slave);
	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);

	/*
	 * Fill up the default values
	 * Slave select behaviour set
	 * Fifo depth greater than 4 frames
	 * Methodology to calculate SPI Clock:
	 *	0:	SPICLK = 1 / (2 CLK_GEN + 1) , CLK_GEN is from 0 to 15
	 *	1:	SPICLK = 1 / (2 * (CLK_GEN + 1)) , CLK_GEN is from 0 to 255
	 */

	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, xfer->control);

	if (mss_spi_clk_gen_set(cfg, spi_cfg)) {
		LOG_ERR("can't set clk divider\n");
		return -EINVAL;
	}

	mss_spi_hw_mode_set(cfg, spi_cfg->operation);
	mss_spi_write(cfg, MSS_SPI_REG_TXRXDF_SIZE, MSS_SPI_FRAMESIZE_DEFAULT);
	mss_spi_enable_controller(cfg);
	mss_spi_write(cfg, MSS_SPI_REG_COMMAND, MSS_SPI_COMMAND_FIFO_MASK);

	return 0;
}

static int mss_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			      bool async, spi_callback_t cb, void *userdata)
{

	const struct mss_spi_config *config = dev->config;
	struct mss_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	struct mss_spi_transfer *xfer = &data->xfer;
	int ret = 0;

	spi_context_lock(ctx, async, cb, userdata, spi_cfg);

	ret = mss_spi_configure(dev, spi_cfg);
	if (ret) {
		LOG_ERR("Fail to configure\n\r");
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	xfer->rx_len = ctx->rx_len;
	mss_spi_readwr_fifo(dev);
	ret = spi_context_wait_for_completion(ctx);
out:
	spi_context_release(ctx, ret);
	mss_spi_disable_ints(config);
	mss_spi_disable_controller(config);

	return ret;
}

static int mss_spi_transceive_blocking(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{

	return mss_spi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int mss_spi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				    void *userdata)
{
	return mss_spi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int mss_spi_init(const struct device *dev)
{
	const struct mss_spi_config *cfg = dev->config;
	struct mss_spi_data *data = dev->data;
	struct mss_spi_transfer *xfer = &data->xfer;
	int ret = 0;
	uint32_t control = 0;

	/* Remove SPI from Reset  */
	control = mss_spi_read(cfg, MSS_SPI_REG_CONTROL);
	control &= ~MSS_SPI_CONTROL_RESET;
	mss_spi_write(cfg, MSS_SPI_REG_CONTROL, control);

	/* Set master mode */
	mss_spi_disable_controller(cfg);
	xfer->control = (MSS_SPI_CONTROL_SPS | MSS_SPI_CONTROL_BIGFIFO | MSS_SPI_CONTROL_MASTER |
			 MSS_SPI_CONTROL_CLKMODE);

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

#define MICROCHIP_SPI_PM_OPS (NULL)

static DEVICE_API(spi, mss_spi_driver_api) = {
	.transceive = mss_spi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = mss_spi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = mss_spi_release,
};

#define MSS_SPI_INIT(n)                                                                            \
	static int mss_spi_init_##n(const struct device *dev)                                      \
	{                                                                                          \
		mss_spi_init(dev);                                                                 \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mss_spi_interrupt,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct mss_spi_config mss_spi_config_##n = {                                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
	};                                                                                         \
                                                                                                   \
	static struct mss_spi_data mss_spi_data_##n = {                                            \
		SPI_CONTEXT_INIT_LOCK(mss_spi_data_##n, ctx),                                      \
		SPI_CONTEXT_INIT_SYNC(mss_spi_data_##n, ctx),                                      \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, mss_spi_init_##n, NULL, &mss_spi_data_##n,                    \
				  &mss_spi_config_##n, POST_KERNEL,                                \
				  CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mss_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSS_SPI_INIT)
