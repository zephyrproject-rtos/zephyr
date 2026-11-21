/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * G1 QSPI controller (PIC32CX SG41/SG60/SG61, SAM D5x/E5x) in SPI mode
 * (QSPI_CTRLB.MODE = 0): a single-line SPI controller with one hardware chip
 * select (CS), used for example by the jedec,spi-nor flash driver.
 */

#define DT_DRV_COMPAT microchip_qspi_g1_spi

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <mchp_dt_helper.h>

LOG_MODULE_REGISTER(spi_mchp_qspi_g1, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

/* QSPI_BAUD.BAUD divides CLK_QSPI_AHB by 1 to 256 */
#define QSPI_G1_DIV_MAX 256U

#ifdef CONFIG_SPI_MCHP_QSPI_G1_DMA
/* DMAC_BTCNT is 16 bits wide and the driver uses byte beats */
#define QSPI_G1_DMA_MAX_LEN UINT16_MAX

/*
 * When receiving, both channels are triggered by RXC: the RX channel reads the
 * byte just received and the TX channel writes the next one. The RX channel has
 * the higher priority level, so it is served first.
 */
#define QSPI_G1_DMA_RX_PRIORITY (DMAC_LVL_NUM - 1U)
#define QSPI_G1_DMA_TX_PRIORITY 0U

#endif

#define QSPI_G1_DMA_CHANNEL_NONE 0xFFU

/* DMAC channels from the dmas property; dev is NULL without dmas */
struct spi_mchp_qspi_g1_dma {
	const struct device *dev;
	uint8_t tx_channel;
	uint8_t tx_trigsrc;
	uint8_t rx_channel;
	uint8_t rx_trigsrc;
};

struct spi_mchp_qspi_g1_config {
	qspi_registers_t *regs;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_apb_sys;
	clock_control_subsys_t mclk_ahb_sys;
	const struct pinctrl_dev_config *pcfg;
	struct spi_mchp_qspi_g1_dma dma;
};

struct spi_mchp_qspi_g1_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_MCHP_QSPI_G1_DMA
	struct k_sem dma_done;
	int dma_status;
	uint32_t sck_hz;
#endif
};

static int spi_mchp_qspi_g1_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	qspi_registers_t *regs = cfg->regs;
	uint32_t clk_hz;
	uint32_t divisor;
	uint32_t ctrlb;
	uint32_t baud;
	int ret;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if ((config->operation & (SPI_HALF_DUPLEX | SPI_TRANSFER_LSB)) != 0U) {
		LOG_ERR("Half-duplex and LSB first are not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8U) {
		LOG_ERR("Only 8-bit words are supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE)) {
		LOG_ERR("Only single line transfers are supported");
		return -ENOTSUP;
	}

	if (!spi_cs_is_gpio(config) && (config->peripheral != 0U)) {
		LOG_ERR("Only chip select 0 (CS) is available");
		return -EINVAL;
	}

	if (config->frequency == 0U) {
		LOG_ERR("Invalid frequency");
		return -EINVAL;
	}

	/* The baud rate generator runs from CLK_QSPI_AHB */
	ret = clock_control_get_rate(cfg->clock_dev, cfg->mclk_ahb_sys, &clk_hz);
	if (ret < 0) {
		return ret;
	}

	/* Use the requested or the next lower possible frequency */
	divisor = DIV_ROUND_UP(clk_hz, config->frequency);
	divisor = CLAMP(divisor, 1U, QSPI_G1_DIV_MAX);
	baud = QSPI_BAUD_BAUD(divisor - 1U);

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		baud |= QSPI_BAUD_CPOL_Msk;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		baud |= QSPI_BAUD_CPHA_Msk;
	}

	/* SPI mode, 8-bit words, CS held asserted until QSPI_CTRLA.LASTXFER */
	ctrlb = QSPI_CTRLB_CSMODE_LASTXFER | QSPI_CTRLB_DATALEN_8BITS;

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		ctrlb |= QSPI_CTRLB_LOOPEN_Msk;
	}

	/* QSPI_CTRLA is write-only: writing 0 clears ENABLE */
	regs->QSPI_CTRLA = 0U;
	while ((regs->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) != 0U) {
	}

	regs->QSPI_CTRLB = ctrlb;
	regs->QSPI_BAUD = baud;

	regs->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk;
	while ((regs->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) == 0U) {
	}

	LOG_DBG("SCK %u Hz", clk_hz / divisor);

#ifdef CONFIG_SPI_MCHP_QSPI_G1_DMA
	data->sck_hz = clk_hz / divisor;
#endif

	data->ctx.config = config;

	return 0;
}

/*
 * Shift len bytes, one at a time: the next byte is written only after the
 * previous one is received, so the single receive data register cannot
 * overrun, whatever the interrupt latency.
 */
static void spi_mchp_qspi_g1_shift(qspi_registers_t *regs, const uint8_t *tx_buf, uint8_t *rx_buf,
				   size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		uint8_t rx;

		regs->QSPI_TXDATA = (tx_buf != NULL) ? tx_buf[i] : 0U;

		while ((regs->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) == 0U) {
		}

		rx = (uint8_t)regs->QSPI_RXDATA;
		if (rx_buf != NULL) {
			rx_buf[i] = rx;
		}
	}
}

#ifdef CONFIG_SPI_MCHP_QSPI_G1_DMA
static void spi_mchp_qspi_g1_dma_done(const struct device *dma_dev, void *arg, uint32_t channel,
				      int status)
{
	struct spi_mchp_qspi_g1_data *data = arg;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status == DMA_STATUS_BLOCK) {
		return;
	}

	data->dma_status = (status < 0) ? status : 0;
	k_sem_give(&data->dma_done);
}

/*
 * Shift len bytes with DMA.
 *
 * Sending only (rx_buf NULL): one TX channel on DRE keeps TXDATA full. Nothing
 * reads RXDATA, so it overruns; that is harmless and cleared at the end.
 *
 * Receiving: the CPU writes the first byte, then the RX and TX channels are
 * both triggered by RXC: the RX channel reads the byte just received, the TX
 * channel writes the next one. A byte is only sent after the previous one is
 * received, so RXDATA cannot overrun however late the DMAC is served (with TX
 * on DRE, the RX channel had one byte time, and missed it under bus load).
 */
static int spi_mchp_qspi_g1_dma_shift(const struct device *dev, const uint8_t *tx_buf,
				      uint8_t *rx_buf, size_t len)
{
	static const uint8_t tx_dummy;
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	const struct spi_mchp_qspi_g1_dma *dma = &cfg->dma;
	qspi_registers_t *regs = cfg->regs;
	const bool rx = (rx_buf != NULL);
	/* When receiving, the CPU writes the first byte */
	const size_t tx_len = rx ? len - 1U : len;
	const uint8_t *tx_src = (tx_buf != NULL) ? (rx ? &tx_buf[1] : tx_buf) : &tx_dummy;
	struct dma_block_config rx_blk = {
		.block_size = len,
		.source_address = (uint32_t)&regs->QSPI_RXDATA,
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.dest_address = (uint32_t)rx_buf,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
	};
	struct dma_block_config tx_blk = {
		.block_size = tx_len,
		.source_address = (uint32_t)tx_src,
		.source_addr_adj =
			(tx_buf != NULL) ? DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE,
		.dest_address = (uint32_t)&regs->QSPI_TXDATA,
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
	};
	struct dma_config rx_cfg = {
		.dma_slot = dma->rx_trigsrc,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.channel_priority = QSPI_G1_DMA_RX_PRIORITY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.block_count = 1U,
		.head_block = &rx_blk,
		.user_data = data,
		.dma_callback = spi_mchp_qspi_g1_dma_done,
	};
	struct dma_config tx_cfg = {
		.dma_slot = rx ? dma->rx_trigsrc : dma->tx_trigsrc,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.channel_priority = QSPI_G1_DMA_TX_PRIORITY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.block_count = 1U,
		.head_block = &tx_blk,
		/* Without RX, the TX channel signals the end of the transfer */
		.user_data = rx ? NULL : data,
		.dma_callback = rx ? NULL : spi_mchp_qspi_g1_dma_done,
	};
	/* Four times the time on the wire, plus a margin for scheduling */
	k_timeout_t timeout =
		K_MSEC(DIV_ROUND_UP((uint64_t)len * 8U * 4U * MSEC_PER_SEC, data->sck_hz) + 10U);
	int ret;

	k_sem_reset(&data->dma_done);

	if (rx) {
		ret = dma_config(dma->dev, dma->rx_channel, &rx_cfg);
		if (ret < 0) {
			return ret;
		}

		ret = dma_start(dma->dev, dma->rx_channel);
		if (ret < 0) {
			return ret;
		}
	}

	if (tx_len > 0U) {
		ret = dma_config(dma->dev, dma->tx_channel, &tx_cfg);
		if (ret == 0) {
			ret = dma_start(dma->dev, dma->tx_channel);
		}
		if (ret < 0) {
			if (rx) {
				(void)dma_stop(dma->dev, dma->rx_channel);
			}
			return ret;
		}
	}

	if (rx) {
		/* The first byte starts the RXC-paced transfer */
		regs->QSPI_TXDATA = (tx_buf != NULL) ? tx_buf[0] : 0U;
	}

	ret = k_sem_take(&data->dma_done, timeout);
	if (ret < 0) {
		LOG_ERR("DMA transfer timed out");
		ret = -ETIMEDOUT;
	} else {
		ret = data->dma_status;
	}

	if (tx_len > 0U) {
		(void)dma_stop(dma->dev, dma->tx_channel);
	}

	if (rx) {
		(void)dma_stop(dma->dev, dma->rx_channel);

		if ((regs->QSPI_INTFLAG & QSPI_INTFLAG_ERROR_Msk) != 0U) {
			regs->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;
			LOG_ERR("RXDATA overrun during DMA transfer");
			ret = -EIO;
		}
	} else {
		/* The DMA is done once the last byte is in TXDATA: wait for it to be sent */
		if ((ret == 0) &&
		    !WAIT_FOR((regs->QSPI_INTFLAG & QSPI_INTFLAG_TXC_Msk) != 0U, 1000, NULL)) {
			LOG_ERR("TXC timed out");
			ret = -ETIMEDOUT;
		}

		/* Nothing read the received bytes: drop them and the overrun flag */
		while ((regs->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) != 0U) {
			(void)regs->QSPI_RXDATA;
		}
		regs->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;
	}

	return ret;
}

static bool spi_mchp_qspi_g1_use_dma(const struct device *dev, size_t len)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;

	return (cfg->dma.dev != NULL) && (len >= CONFIG_SPI_MCHP_QSPI_G1_DMA_THRESHOLD);
}
#endif /* CONFIG_SPI_MCHP_QSPI_G1_DMA */

static void spi_mchp_qspi_g1_release_cs(qspi_registers_t *regs)
{
	/*
	 * QSPI_CTRLA is write-only, so ENABLE must be written with LASTXFER.
	 * The shift register is idle here, so CS is deasserted at once.
	 */
	regs->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk | QSPI_CTRLA_LASTXFER_Msk;
	while ((regs->QSPI_STATUS & QSPI_STATUS_CSSTATUS_Msk) == 0U) {
	}
}

static int spi_mchp_qspi_g1_transceive(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	qspi_registers_t *regs = cfg->regs;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_mchp_qspi_g1_configure(dev, config);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Drop stale received data and clear the overrun flag */
	while ((regs->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) != 0U) {
		(void)regs->QSPI_RXDATA;
	}
	regs->QSPI_INTFLAG = QSPI_INTFLAG_ERROR_Msk;

	spi_context_cs_control(ctx, true);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t len = spi_context_max_continuous_chunk(ctx);
		const uint8_t *tx_buf = spi_context_tx_buf_on(ctx) ? ctx->tx_buf : NULL;
		uint8_t *rx_buf = spi_context_rx_buf_on(ctx) ? ctx->rx_buf : NULL;

#ifdef CONFIG_SPI_MCHP_QSPI_G1_DMA
		if (spi_mchp_qspi_g1_use_dma(dev, len)) {
			len = MIN(len, QSPI_G1_DMA_MAX_LEN);
			ret = spi_mchp_qspi_g1_dma_shift(dev, tx_buf, rx_buf, len);
			if (ret < 0) {
				break;
			}
		} else {
			spi_mchp_qspi_g1_shift(regs, tx_buf, rx_buf, len);
		}
#else
		spi_mchp_qspi_g1_shift(regs, tx_buf, rx_buf, len);
#endif
		spi_context_update_tx(ctx, 1, len);
		spi_context_update_rx(ctx, 1, len);
	}

	if (!spi_cs_is_gpio(config) && ((config->operation & SPI_HOLD_ON_CS) == 0U)) {
		spi_mchp_qspi_g1_release_cs(regs);
	}

	spi_context_cs_control(ctx, false);

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_mchp_qspi_g1_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;

	if (!spi_cs_is_gpio(config) && spi_context_configured(&data->ctx, config)) {
		spi_mchp_qspi_g1_release_cs(cfg->regs);
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mchp_qspi_g1_init(const struct device *dev)
{
	const struct spi_mchp_qspi_g1_config *cfg = dev->config;
	struct spi_mchp_qspi_g1_data *data = dev->data;
	int ret;

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_apb_sys);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_ahb_sys);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* The reset leaves the controller disabled, in SPI mode */
	cfg->regs->QSPI_CTRLA = QSPI_CTRLA_SWRST_Msk;

#ifdef CONFIG_SPI_MCHP_QSPI_G1_DMA
	k_sem_init(&data->dma_done, 0, 1);

	if ((cfg->dma.dev != NULL) && !device_is_ready(cfg->dma.dev)) {
		return -ENODEV;
	}
#endif

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_mchp_qspi_g1_driver_api) = {
	.transceive = spi_mchp_qspi_g1_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mchp_qspi_g1_release,
};

/* Without CONFIG_SPI_MCHP_QSPI_G1_DMA, the DMAC is not referenced even if dmas is set */
#define QSPI_G1_DMA_CFG(n)                                                                         \
	COND_CODE_1(UTIL_AND(IS_ENABLED(CONFIG_SPI_MCHP_QSPI_G1_DMA),                              \
			     DT_INST_NODE_HAS_PROP(n, dmas)),                                      \
		    ({                                                                             \
			    .dev = DEVICE_DT_GET(MCHP_DT_INST_DMA_CTLR(n, tx)),                    \
			    .tx_channel = MCHP_DT_INST_DMA_CHANNEL(n, tx),                         \
			    .tx_trigsrc = MCHP_DT_INST_DMA_TRIGSRC(n, tx),                         \
			    .rx_channel = MCHP_DT_INST_DMA_CHANNEL(n, rx),                         \
			    .rx_trigsrc = MCHP_DT_INST_DMA_TRIGSRC(n, rx),                         \
		    }),                                                                            \
		    ({                                                                             \
			    .dev = NULL,                                                           \
			    .tx_channel = QSPI_G1_DMA_CHANNEL_NONE,                                \
			    .rx_channel = QSPI_G1_DMA_CHANNEL_NONE,                                \
		    }))

#define SPI_MCHP_QSPI_G1_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct spi_mchp_qspi_g1_config spi_mchp_qspi_g1_config_##n = {                \
		.regs = (qspi_registers_t *)DT_INST_REG_ADDR(n),                                   \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_apb_sys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_apb,   \
										    subsystem),    \
		.mclk_ahb_sys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_ahb,   \
										    subsystem),    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.dma = QSPI_G1_DMA_CFG(n),                                                         \
	};                                                                                         \
                                                                                                   \
	static struct spi_mchp_qspi_g1_data spi_mchp_qspi_g1_data_##n = {                          \
		SPI_CONTEXT_INIT_LOCK(spi_mchp_qspi_g1_data_##n, ctx),                             \
		SPI_CONTEXT_INIT_SYNC(spi_mchp_qspi_g1_data_##n, ctx),                             \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mchp_qspi_g1_init, NULL, &spi_mchp_qspi_g1_data_##n,      \
				  &spi_mchp_qspi_g1_config_##n, POST_KERNEL,                       \
				  CONFIG_SPI_INIT_PRIORITY, &spi_mchp_qspi_g1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_MCHP_QSPI_G1_INIT)
