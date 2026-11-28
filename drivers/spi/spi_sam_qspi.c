/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * QSPI controller of the SAM E70/S70/V70/V71 and PIC32CZ CA70/MC70 in SPI
 * mode (QSPI_MR.SMM = 0): a single-line SPI controller with one hardware chip
 * select (QCS), used for example by the jedec,spi-nor flash driver.
 */

#define DT_DRV_COMPAT atmel_sam_qspi

#include <errno.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_sam_qspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

/*
 * The Atmel headers name the QSPI register block Qspi; the Microchip headers
 * (PIC32CZ CA70) use qspi_registers_t. Both define the _Msk names used here.
 */
#ifdef QSPI_REGS
typedef qspi_registers_t sam_qspi_regs_t;
#else
typedef Qspi sam_qspi_regs_t;
#endif

/* QSPI_SCR.SCBR divides the peripheral clock by 1 to 256 */
#define SAM_QSPI_DIV_MAX 256U

#ifdef CONFIG_SPI_SAM_QSPI_DMA
/* XDMAC_CUBC.UBLEN is 24 bits wide and the driver uses byte transfers */
#define SAM_QSPI_DMA_MAX_LEN 0xFFFFFFU

/*
 * The DMA writes the receive buffer behind the data cache: only whole cache
 * lines of it go through DMA, so that invalidating them cannot discard data
 * the CPU writes next to the buffer.
 */
#if defined(CONFIG_DCACHE) && defined(CONFIG_CACHE_MANAGEMENT)
#define SAM_QSPI_DMA_RX_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define SAM_QSPI_DMA_RX_ALIGN 1U
#endif
#endif /* CONFIG_SPI_SAM_QSPI_DMA */

/* XDMAC channels and peripheral IDs from the dmas property; dev is NULL without dmas */
struct spi_sam_qspi_dma {
	const struct device *dev;
	uint32_t tx_channel;
	uint32_t tx_perid;
	uint32_t rx_channel;
	uint32_t rx_perid;
};

struct spi_sam_qspi_config {
	sam_qspi_regs_t *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	struct spi_sam_qspi_dma dma;
};

struct spi_sam_qspi_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_SAM_QSPI_DMA
	struct k_sem dma_done;
	int dma_status;
	uint32_t sck_hz;
#endif
};

static int spi_sam_qspi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	sam_qspi_regs_t *regs = cfg->regs;
	uint32_t mck_hz;
	uint32_t divisor;
	uint32_t mr;
	uint32_t scr = 0U;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if ((config->operation & (SPI_HALF_DUPLEX | SPI_TRANSFER_LSB | SPI_MODE_LOOP)) != 0U) {
		LOG_ERR("Half-duplex, LSB first and loopback are not supported");
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
		LOG_ERR("Only chip select 0 (QCS) is available");
		return -EINVAL;
	}

#ifdef SOC_ATMEL_SAM_MCK_FREQ_HZ
	mck_hz = SOC_ATMEL_SAM_MCK_FREQ_HZ;
#else
	/* SoCs without a fixed MCK rate (PIC32CZ CA70): ask the PMC. */
	int ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
					 (clock_control_subsys_t)&cfg->clock_cfg, &mck_hz);

	if (ret < 0) {
		return ret;
	}
#endif

	/* Use the requested or the next lower possible frequency */
	divisor = DIV_ROUND_UP(mck_hz, config->frequency);
	divisor = CLAMP(divisor, 1U, SAM_QSPI_DIV_MAX);
	scr |= QSPI_SCR_SCBR(divisor - 1U);

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		scr |= QSPI_SCR_CPOL_Msk;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		scr |= QSPI_SCR_CPHA_Msk;
	}

	/* SPI mode, 8-bit words, QCS held low until QSPI_CR.LASTXFER */
	mr = QSPI_MR_CSMODE_LASTXFER | QSPI_MR_NBBITS(0U);

	regs->QSPI_CR = QSPI_CR_QSPIDIS_Msk;
	while ((regs->QSPI_SR & QSPI_SR_QSPIENS_Msk) != 0U) {
	}

	regs->QSPI_MR = mr;
	regs->QSPI_SCR = scr;

	regs->QSPI_CR = QSPI_CR_QSPIEN_Msk;
	while ((regs->QSPI_SR & QSPI_SR_QSPIENS_Msk) == 0U) {
	}

	LOG_DBG("QSCK %u Hz", mck_hz / divisor);

#ifdef CONFIG_SPI_SAM_QSPI_DMA
	data->sck_hz = mck_hz / divisor;
#endif

	data->ctx.config = config;

	return 0;
}

/*
 * Shift len bytes, one at a time: the next byte is written only after the
 * previous one is received, so the single receive holding register cannot
 * overrun, whatever the interrupt latency.
 */
static void spi_sam_qspi_shift(sam_qspi_regs_t *regs, const uint8_t *tx_buf, uint8_t *rx_buf,
			       size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		uint8_t rx;

		regs->QSPI_TDR = (tx_buf != NULL) ? tx_buf[i] : 0U;

		while ((regs->QSPI_SR & QSPI_SR_RDRF_Msk) == 0U) {
		}

		rx = (uint8_t)regs->QSPI_RDR;
		if (rx_buf != NULL) {
			rx_buf[i] = rx;
		}
	}
}

#ifdef CONFIG_SPI_SAM_QSPI_DMA
static void spi_sam_qspi_dma_done(const struct device *dma_dev, void *arg, uint32_t channel,
				  int status)
{
	struct spi_sam_qspi_data *data = arg;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	/* The XDMAC reports the channel error flags as a positive status */
	data->dma_status = (status != 0) ? -EIO : 0;
	k_sem_give(&data->dma_done);
}

/*
 * Shift len bytes with the XDMAC.
 *
 * Sending only (rx_buf NULL): one TX channel, triggered by TDRE, keeps QSPI_TDR
 * full. Nothing reads QSPI_RDR, so it overruns; that is harmless and cleared at
 * the end. The received bytes are dropped.
 *
 * Receiving: the CPU writes the first byte, then the RX and TX channels are
 * both triggered by RDRF: the RX channel reads the byte just received into
 * rx_buf and the TX channel writes the next one. A byte is only sent after the
 * previous one is received, so QSPI_RDR cannot overrun however late the XDMAC
 * is served (with TX on TDRE the RX channel had one byte time, 160 ns at
 * 50 MHz, and missed it under bus load: QSPI_RDR overrun, a lost byte, and a
 * transfer that never completed). The RX channel is started first, so the
 * transfer is complete once its last byte is received. rx_buf, if set, must
 * cover whole data cache lines and len must be at least 2.
 */
static int spi_sam_qspi_dma_shift(const struct device *dev, const uint8_t *tx_buf, uint8_t *rx_buf,
				  size_t len)
{
	/* In SRAM: a fixed source in flash stalls the XDMAC, see above */
	static uint8_t tx_dummy;
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	const struct spi_sam_qspi_dma *dma = &cfg->dma;
	sam_qspi_regs_t *regs = cfg->regs;
	const bool rx = (rx_buf != NULL);
	/* When receiving, the CPU writes the first byte */
	const size_t tx_len = rx ? len - 1U : len;
	const uint8_t *tx_src = (tx_buf != NULL) ? (rx ? &tx_buf[1] : tx_buf) : &tx_dummy;
	struct dma_block_config rx_blk = {
		.block_size = len,
		.source_address = (uint32_t)&regs->QSPI_RDR,
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.dest_address = (uint32_t)rx_buf,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
	};
	struct dma_block_config tx_blk = {
		.block_size = tx_len,
		.source_address = (uint32_t)tx_src,
		.source_addr_adj =
			(tx_buf != NULL) ? DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE,
		.dest_address = (uint32_t)&regs->QSPI_TDR,
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
	};
	struct dma_config rx_cfg = {
		.dma_slot = dma->rx_perid,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.complete_callback_en = true,
		.head_block = &rx_blk,
		.user_data = data,
		.dma_callback = spi_sam_qspi_dma_done,
	};
	struct dma_config tx_cfg = {
		/* When receiving, send the next byte when the previous one is received */
		.dma_slot = rx ? dma->rx_perid : dma->tx_perid,
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.complete_callback_en = (rx_buf == NULL),
		.head_block = &tx_blk,
		.user_data = data,
		.dma_callback = (rx_buf == NULL) ? spi_sam_qspi_dma_done : NULL,
	};
	/* Twice the time on the wire, plus a margin for scheduling */
	k_timeout_t timeout =
		K_MSEC(DIV_ROUND_UP((uint64_t)len * 8U * 2U * MSEC_PER_SEC, data->sck_hz) + 10U);
	int ret;

	__ASSERT_NO_MSG(!rx || len >= 2U);

	if (tx_buf != NULL) {
		(void)sys_cache_data_flush_range((void *)tx_buf, len);
	}

	if (rx_buf != NULL) {
		/* No dirty line may be written back over the received data */
		(void)sys_cache_data_invd_range(rx_buf, len);
	}

	k_sem_reset(&data->dma_done);

	/* Clear a stale overrun flag (cleared on read) */
	(void)regs->QSPI_SR;

	if (rx_buf != NULL) {
		ret = dma_config(dma->dev, dma->rx_channel, &rx_cfg);
		if (ret < 0) {
			return ret;
		}
	}

	ret = dma_config(dma->dev, dma->tx_channel, &tx_cfg);
	if (ret < 0) {
		return ret;
	}

	if (rx_buf != NULL) {
		ret = dma_start(dma->dev, dma->rx_channel);
		if (ret < 0) {
			return ret;
		}
	}

	ret = dma_start(dma->dev, dma->tx_channel);
	if (ret < 0) {
		if (rx_buf != NULL) {
			(void)dma_stop(dma->dev, dma->rx_channel);
		}
		return ret;
	}

	if (rx) {
		/* Both channels wait for RDRF: this byte starts the chain */
		regs->QSPI_TDR = (tx_buf != NULL) ? tx_buf[0] : 0U;
	}

	ret = k_sem_take(&data->dma_done, timeout);
	if (ret < 0) {
		LOG_ERR("DMA transfer timed out");
		ret = -ETIMEDOUT;
	} else {
		ret = data->dma_status;
	}

	(void)dma_stop(dma->dev, dma->tx_channel);

	if (rx_buf == NULL) {
		/* The last bytes are still in the shift and holding registers */
		while ((regs->QSPI_SR & QSPI_SR_TXEMPTY_Msk) == 0U) {
		}

		/* Drop the last received byte and the overrun flag */
		(void)regs->QSPI_RDR;
		(void)regs->QSPI_SR;

		return ret;
	}

	(void)dma_stop(dma->dev, dma->rx_channel);

	/* Drop lines the CPU may have fetched speculatively meanwhile */
	(void)sys_cache_data_invd_range(rx_buf, len);

	/* A late RX DMA lets QSPI_RDR overrun and drops a byte */
	if ((regs->QSPI_SR & QSPI_SR_OVRES_Msk) != 0U) {
		LOG_ERR("QSPI_RDR overrun during DMA transfer");
		ret = -EIO;
	}

	return ret;
}

/*
 * Shift len bytes, through DMA for the part of at least
 * CONFIG_SPI_SAM_QSPI_DMA_THRESHOLD bytes that covers whole cache lines of
 * rx_buf, by the CPU for the rest. Returns the number of bytes shifted.
 */
static int spi_sam_qspi_xfer(const struct device *dev, const uint8_t *tx_buf, uint8_t *rx_buf,
			     size_t len)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	size_t head = 0U;
	size_t body = len;
	int ret;

	if (rx_buf != NULL) {
		head = MIN(len, (size_t)(ROUND_UP((uintptr_t)rx_buf, SAM_QSPI_DMA_RX_ALIGN) -
					 (uintptr_t)rx_buf));
		body = ROUND_DOWN(len - head, SAM_QSPI_DMA_RX_ALIGN);
	}

	body = MIN(body, ROUND_DOWN(SAM_QSPI_DMA_MAX_LEN, SAM_QSPI_DMA_RX_ALIGN));

	if ((cfg->dma.dev == NULL) || (body < MAX((size_t)CONFIG_SPI_SAM_QSPI_DMA_THRESHOLD, 2U))) {
		spi_sam_qspi_shift(cfg->regs, tx_buf, rx_buf, len);
		return (int)len;
	}

	/* Bytes before the first whole line of rx_buf */
	if (head > 0U) {
		spi_sam_qspi_shift(cfg->regs, tx_buf, rx_buf, head);
		return (int)head;
	}

	ret = spi_sam_qspi_dma_shift(dev, tx_buf, rx_buf, body);

	return (ret < 0) ? ret : (int)body;
}
#endif /* CONFIG_SPI_SAM_QSPI_DMA */

static void spi_sam_qspi_release_cs(sam_qspi_regs_t *regs)
{
	regs->QSPI_CR = QSPI_CR_LASTXFER_Msk;
	while ((regs->QSPI_SR & QSPI_SR_TXEMPTY_Msk) == 0U) {
	}
}

static int spi_sam_qspi_transceive(const struct device *dev, const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	sam_qspi_regs_t *regs = cfg->regs;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_sam_qspi_configure(dev, config);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Drop stale received data and clear the overrun flag */
	while ((regs->QSPI_SR & QSPI_SR_RDRF_Msk) != 0U) {
		(void)regs->QSPI_RDR;
	}

	spi_context_cs_control(ctx, true);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t len = spi_context_max_continuous_chunk(ctx);
		const uint8_t *tx_buf = spi_context_tx_buf_on(ctx) ? ctx->tx_buf : NULL;
		uint8_t *rx_buf = spi_context_rx_buf_on(ctx) ? ctx->rx_buf : NULL;

#ifdef CONFIG_SPI_SAM_QSPI_DMA
		ret = spi_sam_qspi_xfer(dev, tx_buf, rx_buf, len);
		if (ret < 0) {
			break;
		}

		len = (size_t)ret;
		ret = 0;
#else
		spi_sam_qspi_shift(regs, tx_buf, rx_buf, len);
#endif
		spi_context_update_tx(ctx, 1, len);
		spi_context_update_rx(ctx, 1, len);
	}

	if (!spi_cs_is_gpio(config) && ((config->operation & SPI_HOLD_ON_CS) == 0U)) {
		spi_sam_qspi_release_cs(regs);
	}

	spi_context_cs_control(ctx, false);

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_sam_qspi_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;

	if (!spi_cs_is_gpio(config)) {
		spi_sam_qspi_release_cs(cfg->regs);
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_sam_qspi_init(const struct device *dev)
{
	const struct spi_sam_qspi_config *cfg = dev->config;
	struct spi_sam_qspi_data *data = dev->data;
	int ret;

	ret = clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* The reset leaves the controller disabled, in SPI mode */
	cfg->regs->QSPI_CR = QSPI_CR_SWRST_Msk;

#ifdef CONFIG_SPI_SAM_QSPI_DMA
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

static DEVICE_API(spi, spi_sam_qspi_driver_api) = {
	.transceive = spi_sam_qspi_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_sam_qspi_release,
};

/* Without CONFIG_SPI_SAM_QSPI_DMA, the XDMAC is not referenced even if dmas is set */
#define SPI_SAM_QSPI_DMA_CFG(n)                                                                    \
	COND_CODE_1(UTIL_AND(IS_ENABLED(CONFIG_SPI_SAM_QSPI_DMA), DT_INST_NODE_HAS_PROP(n, dmas)), \
		    ({                                                                             \
			    .dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),                \
			    .tx_channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),               \
			    .tx_perid = DT_INST_DMAS_CELL_BY_NAME(n, tx, perid),                   \
			    .rx_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),               \
			    .rx_perid = DT_INST_DMAS_CELL_BY_NAME(n, rx, perid),                   \
		    }),                                                                            \
		    ({.dev = NULL}))

#define SPI_SAM_QSPI_INIT(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct spi_sam_qspi_config spi_sam_qspi_config_##n = {                        \
		.regs = (sam_qspi_regs_t *)DT_INST_REG_ADDR_BY_NAME(n, qspi),                      \
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.dma = SPI_SAM_QSPI_DMA_CFG(n),                                                    \
	};                                                                                         \
                                                                                                   \
	static struct spi_sam_qspi_data spi_sam_qspi_data_##n = {                                  \
		SPI_CONTEXT_INIT_LOCK(spi_sam_qspi_data_##n, ctx),                                 \
		SPI_CONTEXT_INIT_SYNC(spi_sam_qspi_data_##n, ctx),                                 \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_sam_qspi_init, NULL, &spi_sam_qspi_data_##n,              \
				  &spi_sam_qspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, \
				  &spi_sam_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SAM_QSPI_INIT)
