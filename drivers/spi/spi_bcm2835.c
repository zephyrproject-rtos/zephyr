/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Broadcom BCM2835 / BCM2710 / BCM2837 SPI0 ("main") controller.
 *
 * Polled (no IRQ, no DMA) implementation of the Zephyr SPI device API,
 * for the 40-pin-header SPI0 bus on the Raspberry Pi family. The
 * auxiliary SPI1 / SPI2 blocks have a different register layout and
 * are not handled here.
 *
 * A polled implementation keeps the driver simple, and for short
 * display-class transfers a tight byte-pump keeps the FIFO full
 * without IRQ overhead -- Linux's spi-bcm2835.c likewise runs short
 * transfers in a polling path. IRQ / DMA can be layered on later
 * behind the same transceive() entry point.
 *
 * Silicon notes:
 *   - Chip select is always GPIO-backed (cs-gpios); the controller's
 *     native CE0/CE1 lines are not used. Native CE ties CS assertion
 *     to FIFO activity, which is awkward for peripherals that hold CS
 *     across a D/C-pin toggle (e.g. SPI displays). GPIO CS is also
 *     the conventional Zephyr pattern.
 *   - SCLK = core_clock / CDIV, with CDIV even. The 2012 datasheet
 *     says "power of 2", but BCM2835-and-later silicon accepts any
 *     even value. CDIV == 0 encodes 65536.
 *   - MSB-first, 8-bit words only. LSB-first, slave mode, and multi-
 *     line (dual / quad / octal) modes are rejected.
 *
 * Reference: Linux drivers/spi/spi-bcm2835.c; BCM2835 ARM Peripherals
 * datasheet ch. 10 (SPI).
 */

#define DT_DRV_COMPAT brcm_bcm2835_spi

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(spi_bcm2835, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* Register offsets (BCM2835 ARM Peripherals datasheet ch. 10). */
#define SPI_CS   0x00 /* control and status */
#define SPI_FIFO 0x04 /* TX / RX data FIFO port */
#define SPI_CLK  0x08 /* clock divider (CDIV) */
#define SPI_DLEN 0x0c /* data length -- DMA mode only */
#define SPI_LTOH 0x10 /* LoSSI output hold delay */
#define SPI_DC   0x14 /* DMA DREQ controls */

/* SPI_CS bits. */
#define CS_CS       (BIT(0) | BIT(1)) /* native chip-select select */
#define CS_CPHA     BIT(2)
#define CS_CPOL     BIT(3)
#define CS_CLEAR_TX BIT(4)
#define CS_CLEAR_RX BIT(5)
#define CS_CSPOL    BIT(6)
#define CS_TA       BIT(7)  /* transfer active */
#define CS_DONE     BIT(16) /* transfer done */
#define CS_RXD      BIT(17) /* RX FIFO contains data */
#define CS_TXD      BIT(18) /* TX FIFO has space */
#define CS_RXR      BIT(19) /* RX FIFO needs reading (3/4 full) */
#define CS_RXF      BIT(20) /* RX FIFO full */

struct spi_bcm2835_config {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pcfg;
	uint32_t core_clock_hz;
};

struct spi_bcm2835_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
};

static inline uint32_t spi_bcm2835_read(const struct device *dev, uint32_t off)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + off);
}

static inline void spi_bcm2835_write(const struct device *dev, uint32_t off,
				     uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + off);
}

/* CDIV such that core_clock / CDIV <= spi_hz. CDIV must be even;
 * CDIV == 0 encodes the slowest divisor (65536).
 */
static uint32_t spi_bcm2835_cdiv(uint32_t core_hz, uint32_t spi_hz)
{
	uint32_t cdiv;

	if (spi_hz == 0U || spi_hz >= core_hz / 2U) {
		return 2U; /* fastest the divider supports */
	}

	cdiv = DIV_ROUND_UP(core_hz, spi_hz);
	cdiv += cdiv & 1U; /* round up to an even divisor */
	if (cdiv >= 65536U) {
		cdiv = 0U; /* 0 encodes 65536 */
	}

	return cdiv;
}

static int spi_bcm2835_configure(const struct device *dev,
				 const struct spi_config *config)
{
	const struct spi_bcm2835_config *cfg = dev->config;
	struct spi_bcm2835_data *data = dev->data;
	uint32_t cs = 0U;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("slave mode not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("only 8-bit words supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("only MSB-first supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_MODE_LOOP | SPI_LINES_DUAL |
				 SPI_LINES_QUAD | SPI_LINES_OCTAL)) {
		LOG_ERR("unsupported operation flags 0x%x", config->operation);
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_CPOL) {
		cs |= CS_CPOL;
	}
	if (config->operation & SPI_MODE_CPHA) {
		cs |= CS_CPHA;
	}

	/* Park idle: transfer inactive, FIFOs cleared, clock mode latched. */
	spi_bcm2835_write(dev, SPI_CS, cs | CS_CLEAR_TX | CS_CLEAR_RX);
	spi_bcm2835_write(dev, SPI_CLK,
			  spi_bcm2835_cdiv(cfg->core_clock_hz,
					   config->frequency));

	data->ctx.config = config;

	return 0;
}

/* Pump one max-continuous chunk through the FIFO. RX is drained before
 * TX is refilled on every pass, so the RX FIFO cannot overrun (the TX
 * and RX FIFOs are the same depth). A byte read back from SPI_FIFO has
 * by definition finished clocking, so rx_count reaching chunk_len means
 * the whole chunk has been transferred.
 */
static void spi_bcm2835_pump_chunk(const struct device *dev, size_t chunk_len)
{
	struct spi_bcm2835_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint8_t *tx_buf = ctx->tx_buf;
	uint8_t *rx_buf = ctx->rx_buf;
	size_t tx_count = 0U;
	size_t rx_count = 0U;

	while (tx_count < chunk_len || rx_count < chunk_len) {
		uint32_t cs = spi_bcm2835_read(dev, SPI_CS);

		while ((cs & CS_RXD) && rx_count < chunk_len) {
			uint8_t in = (uint8_t)spi_bcm2835_read(dev, SPI_FIFO);

			if (rx_buf != NULL) {
				rx_buf[rx_count] = in;
			}
			rx_count++;
			cs = spi_bcm2835_read(dev, SPI_CS);
		}

		while ((cs & CS_TXD) && tx_count < chunk_len) {
			uint8_t out = (tx_buf != NULL) ? tx_buf[tx_count] : 0U;

			spi_bcm2835_write(dev, SPI_FIFO, out);
			tx_count++;
			cs = spi_bcm2835_read(dev, SPI_CS);
		}
	}
}

static int spi_bcm2835_transceive(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	struct spi_bcm2835_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_bcm2835_configure(dev, config);
	if (ret < 0) {
		goto done;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	spi_context_cs_control(ctx, true);

	/* Start: clear stale FIFO contents, mark the transfer active. */
	spi_bcm2835_write(dev, SPI_CS,
			  spi_bcm2835_read(dev, SPI_CS) |
				  CS_CLEAR_TX | CS_CLEAR_RX | CS_TA);

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		size_t chunk = spi_context_max_continuous_chunk(ctx);

		spi_bcm2835_pump_chunk(dev, chunk);
		spi_context_update_tx(ctx, 1, chunk);
		spi_context_update_rx(ctx, 1, chunk);
	}

	/* Every clocked byte has been read back, so DONE is already set;
	 * just drop transfer-active.
	 */
	spi_bcm2835_write(dev, SPI_CS,
			  spi_bcm2835_read(dev, SPI_CS) & ~CS_TA);

	spi_context_cs_control(ctx, false);

done:
	spi_context_release(ctx, ret);
	return ret;
}

static int spi_bcm2835_release(const struct device *dev,
			       const struct spi_config *config)
{
	struct spi_bcm2835_data *data = dev->data;

	ARG_UNUSED(config);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_bcm2835_api) = {
	.transceive = spi_bcm2835_transceive,
	.release = spi_bcm2835_release,
};

static int spi_bcm2835_init(const struct device *dev)
{
	const struct spi_bcm2835_config *cfg = dev->config;
	struct spi_bcm2835_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* pinctrl-0 is optional. A board may mux the SPI pins here at
	 * init, or omit pinctrl-0 and mux imperatively at runtime (pin
	 * roles can be application-specific). When the property is
	 * absent, cfg->pcfg is NULL.
	 */
	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("pinctrl apply failed: %d", ret);
			return ret;
		}
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("CS GPIO config failed: %d", ret);
		return ret;
	}

	/* Park the controller idle: transfer inactive, FIFOs cleared. */
	spi_bcm2835_write(dev, SPI_CS, CS_CLEAR_TX | CS_CLEAR_RX);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SPI_BCM2835_INIT(n)                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),                       \
		    (PINCTRL_DT_INST_DEFINE(n);), ())                          \
                                                                               \
	static struct spi_bcm2835_data spi_bcm2835_data_##n = {                \
		SPI_CONTEXT_INIT_LOCK(spi_bcm2835_data_##n, ctx),              \
		SPI_CONTEXT_INIT_SYNC(spi_bcm2835_data_##n, ctx),              \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)           \
	};                                                                     \
                                                                               \
	static const struct spi_bcm2835_config spi_bcm2835_config_##n = {      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                          \
		.pcfg = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),       \
				    (PINCTRL_DT_INST_DEV_CONFIG_GET(n)),       \
				    (NULL)),                                   \
		.core_clock_hz = DT_INST_PROP(n, clock_frequency),             \
	};                                                                     \
                                                                               \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_bcm2835_init, NULL,                   \
				  &spi_bcm2835_data_##n,                       \
				  &spi_bcm2835_config_##n, POST_KERNEL,        \
				  CONFIG_SPI_INIT_PRIORITY, &spi_bcm2835_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_BCM2835_INIT)
