/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_qspi

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(mss_qspi, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

/*MSS QSPI Register offsets */
#define MSS_QSPI_REG_CONTROL		(0x00)
#define MSS_QSPI_REG_FRAMES		(0x04)
#define MSS_QSPI_REG_IEN		(0x0c)
#define MSS_QSPI_REG_STATUS		(0x10)
#define MSS_QSPI_REG_DIRECT_ACCESS	(0x14)
#define MSS_QSPI_REG_UPPER_ACCESS	(0x18)
#define MSS_QSPI_REG_RX_DATA		(0x40)
#define MSS_QSPI_REG_TX_DATA		(0x44)
#define MSS_QSPI_REG_X4_RX_DATA		(0x48)
#define MSS_QSPI_REG_X4_TX_DATA		(0x4c)
#define MSS_QSPI_REG_FRAMESUP		(0x50)

/* QSPICR bit definitions */
#define MSS_QSPI_CONTROL_ENABLE		BIT(0)
#define MSS_QSPI_CONTROL_MASTER		BIT(1)
#define MSS_QSPI_CONTROL_XIP		BIT(2)
#define MSS_QSPI_CONTROL_XIPADDR	BIT(3)
#define MSS_QSPI_CONTROL_CLKIDLE	BIT(10)
#define MSS_QSPI_CONTROL_SAMPLE_MSK     (3 << 11)
#define MSS_QSPI_CONTROL_MODE0		BIT(13)
#define MSS_QSPI_CONTROL_MODE_EXQUAD	(0x6 << 13)
#define MSS_QSPI_CONTROL_MODE_EXDUAL	(0x2 << 13)
#define MSS_QSPI_CONTROL_MODE12_MSK	(3 << 14)
#define MSS_QSPI_CONTROL_FLAGSX4	BIT(16)
#define MSS_QSPI_CONTROL_CLKRATE_MSK	(0xf << 24)
#define MSS_QSPI_CONTROL_CLKRATE	24

/* QSPIFRAMES bit definitions */
#define MSS_QSPI_FRAMES_TOTALBYTES_MSK	(0xffff << 0)
#define MSS_QSPI_FRAMES_TOTALBYTES_MSK	(0xffff << 0)
#define MSS_QSPI_FRAMES_CMDBYTES_MSK	(0x1ff << 16)
#define MSS_QSPI_FRAMES_CMDBYTES	16
#define MSS_QSPI_FRAMES_QSPI		BIT(25)
#define MSS_QSPI_FRAMES_IDLE_MSK	(0xf << 26)
#define MSS_QSPI_FRAMES_FLAGBYTE	BIT(30)
#define MSS_QSPI_FRAMES_FLAGWORD	BIT(31)

/* QSPIIEN bit definitions */
#define MSS_QSPI_IEN_TXDONE		BIT(0)
#define MSS_QSPI_IEN_RXDONE		BIT(1)
#define MSS_QSPI_IEN_RXAVAILABLE	BIT(2)
#define MSS_QSPI_IEN_TXAVAILABLE	BIT(3)
#define MSS_QSPI_IEN_RXFIFOEMPTY	BIT(4)
#define MSS_QSPI_IEN_TXFIFOFULL		BIT(5)
#define MSS_QSPI_IEN_FLAGSX4		BIT(8)

/* QSPIST bit definitions */
#define MSS_QSPI_STATUS_TXDONE		BIT(0)
#define MSS_QSPI_STATUS_RXDONE		BIT(1)
#define MSS_QSPI_STATUS_RXAVAILABLE	BIT(2)
#define MSS_QSPI_STATUS_TXAVAILABLE	BIT(3)
#define MSS_QSPI_STATUS_RXFIFOEMPTY	BIT(4)
#define MSS_QSPI_STATUS_TXFIFOFULL	BIT(5)
#define MSS_QSPI_STATUS_READY		BIT(7)
#define MSS_QSPI_STATUS_FLAGSX4		BIT(8)

/* QSPIDA bit definitions */
#define MSS_QSPI_DA_EN_SSEL		BIT(0)
#define MSS_QSPI_DA_OP_SSEL		BIT(1)
#define MSS_QSPI_DA_EN_SCLK		BIT(2)
#define MSS_QSPI_DA_OP_SCLK		BIT(3)
#define MSS_QSPI_DA_EN_SDO_MSK		(0xf << 4)
#define MSS_QSPI_DA_OP_SDO_MSK		(0xf << 8)
#define MSS_QSPI_DA_OP_SDATA_MSK	(0xf << 12)
#define MSS_QSPI_DA_IP_SDI_MSK		(0xf << 16)
#define MSS_QSPI_DA_IP_SCLK		BIT(21)
#define MSS_QSPI_DA_IP_SSEL		BIT(22)
#define MSS_QSPI_DA_IDLE		BIT(23)
#define MSS_QSPI_RXDATA_MSK		(0xff << 0)
#define MSS_QSPI_TXDATA_MSK		(0xff << 0)

/* QSPIFRAMESUP bit definitions */
#define MSS_QSPI_FRAMESUP_UP_BYTES_MSK	(0xFFFF << 16)
#define MSS_QSPI_FRAMESUP_LO_BYTES_MSK	(0xFFFF << 0)

/*
 * Private data structure for an SPI slave
 */
struct mss_qspi_config {
	mm_reg_t base;
	void (*irq_config_func)(const struct device *dev);
	int irq;
	uint32_t clock_freq;
};

/* Device run time data */
struct mss_qspi_data {
	struct spi_context ctx;
};

static inline uint32_t mss_qspi_read(const struct mss_qspi_config *cfg,
				     mm_reg_t offset)
{
	return sys_read32(cfg->base + offset);
}

static inline void mss_qspi_write(const struct mss_qspi_config *cfg,
				  uint32_t val, mm_reg_t offset)
{
	sys_write32(val, cfg->base + offset);
}

static void mss_qspi_enable_ints(const struct mss_qspi_config *s)
{
	uint32_t mask = MSS_QSPI_IEN_TXDONE |
		   MSS_QSPI_IEN_RXDONE |
		   MSS_QSPI_IEN_RXAVAILABLE;

	mss_qspi_write(s, mask, MSS_QSPI_REG_IEN);
}

static void mss_qspi_disable_ints(const struct mss_qspi_config *s)
{
	uint32_t mask = 0;

	mss_qspi_write(s, mask, MSS_QSPI_REG_IEN);
}

static inline void mss_qspi_transmit_x8(const struct device *dev, uint32_t len)
{
	const struct mss_qspi_config *s = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t count, skips;

	skips = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
	skips &= ~MSS_QSPI_CONTROL_FLAGSX4;
	mss_qspi_write(s, skips, MSS_QSPI_REG_CONTROL);
	for (count = 0; count < len; ++count) {
		while (mss_qspi_read(s, MSS_QSPI_REG_STATUS) & MSS_QSPI_STATUS_TXFIFOFULL) {
			;
		}
		if (spi_context_tx_buf_on(ctx)) {
			mss_qspi_write(s, ctx->tx_buf[0], MSS_QSPI_REG_TX_DATA);
			spi_context_update_tx(ctx, 1, 1);
		}
	}
}

static inline void mss_qspi_transmit_x32(const struct device *dev, uint32_t len)
{
	const struct mss_qspi_config *s = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t count, ctrl, wdata;

	ctrl = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
	ctrl |= MSS_QSPI_CONTROL_FLAGSX4;
	mss_qspi_write(s, ctrl, MSS_QSPI_REG_CONTROL);
	for (count = 0; count < len / 4; ++count) {
		while (mss_qspi_read(s, MSS_QSPI_REG_STATUS) & MSS_QSPI_STATUS_TXFIFOFULL) {
			;
		}
		if (spi_context_tx_buf_on(ctx)) {
			wdata = UNALIGNED_GET((uint32_t *)(ctx->tx_buf));
			mss_qspi_write(s, wdata, MSS_QSPI_REG_X4_TX_DATA);
			spi_context_update_tx(ctx, 1, 4);
		}
	}
}

static inline void mss_qspi_receive_x32(const struct device *dev, uint32_t len)
{
	const struct mss_qspi_config *s = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t count, ctrl, temp;

	ctrl = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
	ctrl |= MSS_QSPI_CONTROL_FLAGSX4;
	mss_qspi_write(s, ctrl, MSS_QSPI_REG_CONTROL);
	for (count = 0; count < len / 4; ++count) {
		while ((mss_qspi_read(s, MSS_QSPI_REG_STATUS) & MSS_QSPI_STATUS_RXFIFOEMPTY)) {
			;
		}
		if (spi_context_rx_buf_on(ctx)) {
			temp = mss_qspi_read(s, MSS_QSPI_REG_X4_RX_DATA);
			UNALIGNED_PUT(temp, (uint32_t *)ctx->rx_buf);
			spi_context_update_rx(ctx, 1, 4);
		}
	}
}

static inline void mss_qspi_receive_x8(const struct device *dev, uint32_t len)
{
	const struct mss_qspi_config *s = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t rdata, count;

	rdata = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
	rdata &= ~MSS_QSPI_CONTROL_FLAGSX4;
	mss_qspi_write(s, rdata, MSS_QSPI_REG_CONTROL);
	for (count = 0; count < len; ++count) {
		while (mss_qspi_read(s, MSS_QSPI_REG_STATUS) & MSS_QSPI_STATUS_RXFIFOEMPTY) {
			;
		}
		if (spi_context_rx_buf_on(ctx)) {
			rdata =  mss_qspi_read(s, MSS_QSPI_REG_RX_DATA);
			UNALIGNED_PUT(rdata, (uint8_t *)ctx->rx_buf);
			spi_context_update_rx(ctx, 1, 1);
		}
	}
}

static inline void mss_qspi_config_frames(const struct device *dev,
					  uint32_t total_bytes,
					  uint32_t cmd_bytes, bool x8)
{
	const struct mss_qspi_config *s = dev->config;
	uint32_t skips;

	mss_qspi_write(s, (total_bytes & MSS_QSPI_FRAMESUP_UP_BYTES_MSK),
		       MSS_QSPI_REG_FRAMESUP);
	skips = (total_bytes & MSS_QSPI_FRAMESUP_LO_BYTES_MSK);
	if (cmd_bytes) {
		skips |= ((cmd_bytes <<  MSS_QSPI_FRAMES_CMDBYTES) & MSS_QSPI_FRAMES_CMDBYTES_MSK);
	} else {
		skips |= ((total_bytes << MSS_QSPI_FRAMES_CMDBYTES) & MSS_QSPI_FRAMES_CMDBYTES_MSK);
	}
	if (mss_qspi_read(s, MSS_QSPI_REG_CONTROL) & MSS_QSPI_CONTROL_MODE0) {
		skips |= MSS_QSPI_FRAMES_QSPI;
	}

	skips &= ~MSS_QSPI_FRAMES_IDLE_MSK;
	if (x8) {
		skips |= MSS_QSPI_FRAMES_FLAGBYTE;
	} else {
		skips |= MSS_QSPI_FRAMES_FLAGWORD;
	}

	mss_qspi_write(s, skips, MSS_QSPI_REG_FRAMES);
}

static inline void mss_qspi_transmit(const struct device *dev)
{
	const struct mss_qspi_config *s = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t total_byte_cnt, cmd_bytes;

	cmd_bytes = spi_context_longest_current_buf(ctx);
	total_byte_cnt = spi_context_total_tx_len(ctx);

	/*
	 * As per the MSS QSPI IP spec,
	 * The number of command and data bytes are controlled by the frames register
	 * for each SPI sequence. This supports the SPI flash memory read and writes
	 * sequences as below. so configure the cmd and total bytes accordingly.
	 * ---------------------------------------------------------------------
	 * TOTAL BYTES  |  CMD BYTES | What happens                             |
	 * ______________________________________________________________________
	 *              |            |                                          |
	 *     1        |   1        | The SPI core will transmit a single byte |
	 *              |            | and receive data is discarded            |
	 *              |            |                                          |
	 *     1        |   0        | The SPI core will transmit a single byte |
	 *              |            | and return a single byte                 |
	 *              |            |                                          |
	 *     10       |   4        | The SPI core will transmit 4 command     |
	 *              |            | bytes discarding the receive data and    |
	 *              |            | transmits 6 dummy bytes returning the 6  |
	 *              |            | received bytes and return a single byte  |
	 *              |            |                                          |
	 *     10       |   10       | The SPI core will transmit 10 command    |
	 *              |            |                                          |
	 *     10       |    0       | The SPI core will transmit 10 command    |
	 *              |            | bytes and returning 10 received bytes    |
	 * ______________________________________________________________________
	 */
	if (!ctx->rx_buf) {
		if (total_byte_cnt - cmd_bytes) {
			mss_qspi_config_frames(dev, total_byte_cnt, 0, false);
			mss_qspi_transmit_x8(dev, cmd_bytes);
			mss_qspi_transmit_x32(dev, (total_byte_cnt - cmd_bytes));

		} else {
			mss_qspi_config_frames(dev, total_byte_cnt, cmd_bytes, true);
			mss_qspi_transmit_x8(dev, cmd_bytes);
		}
	} else {
		mss_qspi_config_frames(dev, total_byte_cnt, cmd_bytes, true);
		mss_qspi_transmit_x8(dev, cmd_bytes);
	}

	mss_qspi_enable_ints(s);
}

static inline void mss_qspi_receive(const struct device *dev)
{
	const struct mss_qspi_config *s = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t rd_bytes, skips, idx, rdata;

	/*
	 * Point the rx buffer where the actual read data
	 * will be stored
	 */
	spi_context_update_rx(ctx, 1, ctx->rx_len);

	rd_bytes = spi_context_longest_current_buf(ctx);
	if (rd_bytes) {
		if (rd_bytes >= 4) {
			mss_qspi_receive_x32(dev, rd_bytes);
		}

		skips = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
		skips &= ~MSS_QSPI_CONTROL_FLAGSX4;
		mss_qspi_write(s, skips, MSS_QSPI_REG_CONTROL);
		idx = (rd_bytes - (rd_bytes % 4u));
		for (; idx < rd_bytes; ++idx) {
			while (mss_qspi_read(s, MSS_QSPI_REG_STATUS) &
			       MSS_QSPI_STATUS_RXFIFOEMPTY) {
				;
			}
			if (spi_context_rx_buf_on(ctx)) {
				rdata =  mss_qspi_read(s, MSS_QSPI_REG_RX_DATA);
				UNALIGNED_PUT(rdata, (uint8_t *)ctx->rx_buf);
				spi_context_update_rx(ctx, 1, 1);
			}
		}
	}
}

static inline int mss_qspi_clk_gen_set(const struct mss_qspi_config *s,
				       const struct spi_config *spi_cfg)
{
	uint32_t control = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
	uint32_t idx, clkrate, val = 0, speed;

	if (spi_cfg->frequency > s->clock_freq) {
		speed = s->clock_freq / 2;
	}

	for (idx = 1; idx < 16; idx++) {
		clkrate = s->clock_freq / (2 * idx);
		if (clkrate <= spi_cfg->frequency) {
			val = idx;
			break;
		}
	}

	if (val) {
		control = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);
		control &= ~MSS_QSPI_CONTROL_CLKRATE_MSK;
		control |= (val << MSS_QSPI_CONTROL_CLKRATE);
		mss_qspi_write(s, control, MSS_QSPI_REG_CONTROL);
	} else {
		return -1;
	}

	return 0;
}

static inline int mss_qspi_hw_mode_set(const struct mss_qspi_config *s,
				       uint16_t mode)
{
	uint32_t ctrl = mss_qspi_read(s, MSS_QSPI_REG_CONTROL);

	if ((mode & SPI_MODE_CPHA) && (mode & SPI_MODE_CPOL)) {
		/* mode 3 */
		ctrl |= MSS_QSPI_CONTROL_CLKIDLE;
	} else if (!(mode & SPI_MODE_CPHA) && !(mode & SPI_MODE_CPOL)) {
		/* mode 0 */
		ctrl &= ~MSS_QSPI_CONTROL_CLKIDLE;
	} else {
		return -1;
	}

	if ((mode & SPI_LINES_QUAD)) {
		/* Quad mode */
		ctrl &= ~(MSS_QSPI_CONTROL_MODE0);
		ctrl |= (MSS_QSPI_CONTROL_MODE_EXQUAD);
	} else if ((mode & SPI_LINES_DUAL)) {
		/* Dual mode */
		ctrl &= ~(MSS_QSPI_CONTROL_MODE0);
		ctrl |= (MSS_QSPI_CONTROL_MODE_EXDUAL);
	} else {
		/* Normal mode */
		ctrl &= ~(MSS_QSPI_CONTROL_MODE0);
	}

	mss_qspi_write(s, ctrl, MSS_QSPI_REG_CONTROL);

	return 0;
}

static int mss_qspi_release(const struct device *dev,
			    const struct spi_config *config)
{
	struct mss_qspi_data *data = dev->data;
	const struct mss_qspi_config *cfg = dev->config;
	uint32_t control = mss_qspi_read(cfg, MSS_QSPI_REG_CONTROL);

	mss_qspi_disable_ints(cfg);

	control &= ~MSS_QSPI_CONTROL_ENABLE;
	mss_qspi_write(cfg, control, MSS_QSPI_REG_CONTROL);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void mss_qspi_interrupt(const struct device *dev)
{
	const struct mss_qspi_config *cfg = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	int intfield = mss_qspi_read(cfg, MSS_QSPI_REG_STATUS);
	int ienfield = mss_qspi_read(cfg, MSS_QSPI_REG_IEN);

	if ((intfield & ienfield) == 0) {
		return;
	}

	if (intfield & MSS_QSPI_IEN_TXDONE) {
		mss_qspi_write(cfg, MSS_QSPI_IEN_TXDONE, MSS_QSPI_REG_STATUS);
	}

	if (intfield & MSS_QSPI_IEN_RXAVAILABLE) {
		mss_qspi_write(cfg, MSS_QSPI_IEN_RXAVAILABLE, MSS_QSPI_REG_STATUS);
		mss_qspi_receive(dev);
	}

	if ((intfield & MSS_QSPI_IEN_RXDONE))  {
		mss_qspi_write(cfg, MSS_QSPI_IEN_RXDONE, MSS_QSPI_REG_STATUS);
		spi_context_complete(ctx, dev, 0);
	}

	if (intfield & MSS_QSPI_IEN_TXAVAILABLE) {
		mss_qspi_write(cfg, MSS_QSPI_IEN_TXAVAILABLE, MSS_QSPI_REG_STATUS);
	}

	if (intfield & MSS_QSPI_IEN_RXFIFOEMPTY) {
		mss_qspi_write(cfg, MSS_QSPI_IEN_RXFIFOEMPTY, MSS_QSPI_REG_STATUS);
	}

	if (intfield & MSS_QSPI_IEN_TXFIFOFULL) {
		mss_qspi_write(cfg, MSS_QSPI_IEN_TXFIFOFULL, MSS_QSPI_REG_STATUS);
	}
}

static int mss_qspi_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct mss_qspi_config *cfg = dev->config;

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode is not supported\n\r");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loop back mode is not supported\n\r");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & (SPI_TRANSFER_LSB) ||
	    ((IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
		 (spi_cfg->operation & (SPI_LINES_DUAL |
			SPI_LINES_QUAD |
		       SPI_LINES_OCTAL))))) {
		LOG_ERR("Unsupported configuration\n\r");
		return -ENOTSUP;
	}

	if (mss_qspi_clk_gen_set(cfg, spi_cfg)) {
		LOG_ERR("can't set clk divider\n");
		return -EINVAL;
	}

	return 0;
}

static int mss_qspi_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs,
			       bool async,
			       spi_callback_t cb,
			       void *userdata)
{
	const struct mss_qspi_config *config = dev->config;
	struct mss_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	spi_context_lock(ctx, async, cb, userdata, spi_cfg);
	ret = mss_qspi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	mss_qspi_hw_mode_set(config, spi_cfg->operation);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs,
				  1);
	mss_qspi_transmit(dev);
	ret = spi_context_wait_for_completion(ctx);
out:
	spi_context_release(ctx, ret);
	mss_qspi_disable_ints(config);

	return ret;
}

static int mss_qspi_transceive_blocking(const struct device *dev,
					const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	return mss_qspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false,
				   NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int mss_qspi_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return mss_qspi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true,
				   cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int mss_qspi_init(const struct device *dev)
{
	const struct mss_qspi_config *cfg = dev->config;
	struct mss_qspi_data *data = dev->data;
	unsigned int ret = 0;
	uint32_t control = 0;

	cfg->irq_config_func(dev);

	control &= ~(MSS_QSPI_CONTROL_SAMPLE_MSK);
	control &= ~(MSS_QSPI_CONTROL_MODE0);
	control |= (MSS_QSPI_CONTROL_CLKRATE_MSK);
	control &=  ~(MSS_QSPI_CONTROL_XIP);
	control |=  (MSS_QSPI_CONTROL_CLKIDLE | MSS_QSPI_CONTROL_ENABLE);
	mss_qspi_write(cfg, control, MSS_QSPI_REG_CONTROL);
	mss_qspi_disable_ints(cfg);
	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

static const struct spi_driver_api mss_qspi_driver_api = {
	.transceive = mss_qspi_transceive_blocking,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = mss_qspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = mss_qspi_release,
};

#define MSS_QSPI_INIT(n)						\
	static void mss_qspi_config_func_##n(const struct device *dev);	\
									\
	static const struct mss_qspi_config mss_qspi_config_##n = { \
		.base = DT_INST_REG_ADDR(n),				\
		.irq_config_func = mss_qspi_config_func_##n,	\
		.clock_freq = DT_INST_PROP(n, clock_frequency),	\
	};								\
									\
	static struct mss_qspi_data mss_qspi_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(mss_qspi_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(mss_qspi_data_##n, ctx),	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &mss_qspi_init,			\
			    NULL,					\
			    &mss_qspi_data_##n,			\
			    &mss_qspi_config_##n, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mss_qspi_driver_api);			\
									\
	static void mss_qspi_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    mss_qspi_interrupt,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(MSS_QSPI_INIT)
