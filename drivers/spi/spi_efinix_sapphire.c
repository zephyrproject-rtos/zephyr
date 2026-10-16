/*
 * Copyright (c) 2026 Arkadiusz Grzelka
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Efinix Sapphire SoC SPI controller (SpinalHDL SpiXdrMasterCtrl behind a BMB
 * bridge). It runs commands from a command FIFO and returns received bytes in
 * a response FIFO. This driver is polled, single-lane, 8-bit, MSB first.
 */

#define DT_DRV_COMPAT efinix_sapphire_spi

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(spi_efinix_sapphire, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

#define SAPPHIRE_SPI_DATA           0x00
#define SAPPHIRE_SPI_CONFIG         0x08
#define SAPPHIRE_SPI_INTERRUPT      0x0c
#define SAPPHIRE_SPI_CLK_DIVIDER    0x20
#define SAPPHIRE_SPI_SS_SETUP       0x24
#define SAPPHIRE_SPI_SS_HOLD        0x28
#define SAPPHIRE_SPI_SS_DISABLE     0x2c
#define SAPPHIRE_SPI_SS_ACTIVE_HIGH 0x30

#define SAPPHIRE_SPI_INT_CMD_EN BIT(0)
/* Command FIFO empty; reads as 1 only while SAPPHIRE_SPI_INT_CMD_EN is set */
#define SAPPHIRE_SPI_INT_CMD    BIT(8)
/* A command is presented to or executing in the SPI core */
#define SAPPHIRE_SPI_INT_BUSY   BIT(16)

/* DATA register, read side: pops the response FIFO */
#define SAPPHIRE_SPI_RSP_EMPTY BIT(31)
#define SAPPHIRE_SPI_RSP_DATA  GENMASK(7, 0)

#define SAPPHIRE_SPI_CMD_WRITE     BIT(8)
#define SAPPHIRE_SPI_CMD_READ      BIT(9)
#define SAPPHIRE_SPI_CMD_SS        BIT(11)
#define SAPPHIRE_SPI_CMD_SS_ENABLE BIT(7)

#define SAPPHIRE_SPI_CONFIG_CPOL BIT(0)
#define SAPPHIRE_SPI_CONFIG_CPHA BIT(1)

/* CLK_DIVIDER and SS_* timing registers are 12 bits wide */
#define SAPPHIRE_SPI_TIMING_MAX 0xfffU

/*
 * Commands in flight. The IP allows command and response FIFO depths of 64
 * to 2048 entries; staying well below 64 means neither FIFO can overflow,
 * including the SS command queued ahead of data.
 */
#define SAPPHIRE_SPI_MAX_INFLIGHT 32U

/* Minimum SS high time between transfers; covers tSHSL of common SPI NORs. */
#define SAPPHIRE_SPI_SS_DISABLE_NS 100U

#define SAPPHIRE_SPI_TIMEOUT_MIN_US 1000U

/*
 * Bound of the wait for an idle controller. 2048 entries, the deepest command
 * FIFO the IP generator offers, each a byte at the slowest divider. A byte takes
 * 16 * 0x1000 clocks, longer than any SS command.
 */
#define SAPPHIRE_SPI_IDLE_CLK_MAX (2048ULL * 16U * (SAPPHIRE_SPI_TIMING_MAX + 1U))

struct spi_efinix_sapphire_config {
	DEVICE_MMIO_ROM;
	uint32_t clock_frequency;
	uint8_t num_cs;
};

struct spi_efinix_sapphire_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	/* Shadow of the write-only SS_ACTIVE_HIGH register */
	uint32_t ss_active_high;
	uint32_t timeout_us;
	/* Hardware SS line currently asserted, or -1 */
	int ss_asserted;
	/* Controller state unknown (init, or a recovery that did not finish) */
	bool recover;
};

static inline void spi_efinix_sapphire_write(const struct device *dev, uint32_t offset,
					     uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_GET(dev) + offset);
}

static inline uint32_t spi_efinix_sapphire_read(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + offset);
}

static uint32_t spi_efinix_sapphire_ns_to_clk(const struct device *dev, uint32_t ns)
{
	const struct spi_efinix_sapphire_config *cfg = dev->config;
	uint64_t clk = DIV_ROUND_UP((uint64_t)ns * cfg->clock_frequency, NSEC_PER_SEC);

	return (uint32_t)MIN(clk, SAPPHIRE_SPI_TIMING_MAX);
}

static bool spi_efinix_sapphire_pop(const struct device *dev, uint8_t *byte)
{
	uint32_t rsp = spi_efinix_sapphire_read(dev, SAPPHIRE_SPI_DATA);

	if ((rsp & SAPPHIRE_SPI_RSP_EMPTY) != 0U) {
		return false;
	}

	*byte = FIELD_GET(SAPPHIRE_SPI_RSP_DATA, rsp);

	return true;
}

static void spi_efinix_sapphire_drain(const struct device *dev)
{
	uint8_t rx;

	while (spi_efinix_sapphire_pop(dev, &rx)) {
	}
}

static bool spi_efinix_sapphire_idle(const struct device *dev)
{
	uint32_t irq = spi_efinix_sapphire_read(dev, SAPPHIRE_SPI_INTERRUPT);

	/* Responses are drained while waiting so no read command blocks on a full FIFO. */
	spi_efinix_sapphire_drain(dev);

	return (irq & (SAPPHIRE_SPI_INT_CMD | SAPPHIRE_SPI_INT_BUSY)) == SAPPHIRE_SPI_INT_CMD;
}

/* The IRQ line is not connected. */
static int spi_efinix_sapphire_wait_idle(const struct device *dev)
{
	const struct spi_efinix_sapphire_config *cfg = dev->config;
	uint32_t timeout_us =
		DIV_ROUND_UP(SAPPHIRE_SPI_IDLE_CLK_MAX * USEC_PER_SEC, cfg->clock_frequency);
	bool idle;

	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_INTERRUPT, SAPPHIRE_SPI_INT_CMD_EN);
	idle = WAIT_FOR(spi_efinix_sapphire_idle(dev), timeout_us, NULL);
	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_INTERRUPT, 0U);

	if (!idle) {
		LOG_ERR("Controller not idle after %u us", timeout_us);
		return -EIO;
	}

	/* The core pushes the last response a few clocks after it goes idle. */
	k_busy_wait(1);
	spi_efinix_sapphire_drain(dev);

	return 0;
}

static int spi_efinix_sapphire_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_efinix_sapphire_config *cfg = dev->config;
	struct spi_efinix_sapphire_data *data = dev->data;
	spi_operation_t op = spi_cfg->operation;
	uint32_t divider, half, setup, hold, disable, reg;
	uint64_t byte_clk;
	int ret;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(op) != SPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Peripheral mode not supported");
		return -ENOTSUP;
	}

	if ((op & (SPI_MODE_LOOP | SPI_TRANSFER_LSB | SPI_HALF_DUPLEX | SPI_FRAME_FORMAT_TI)) !=
	    0U) {
		LOG_ERR("Unsupported operation 0x%x", op);
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) && (op & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(op) != 8U) {
		LOG_ERR("Word size %u not supported", SPI_WORD_SIZE_GET(op));
		return -ENOTSUP;
	}

	if (!spi_cs_is_gpio(spi_cfg) && spi_cfg->peripheral >= cfg->num_cs) {
		LOG_ERR("Invalid peripheral %u (num-cs %u)", spi_cfg->peripheral, cfg->num_cs);
		return -EINVAL;
	}

	if (spi_cfg->frequency == 0U) {
		return -EINVAL;
	}

	/* f_sclk = f_clk / (2 * (divider + 1)); round divider up so f_sclk <= requested */
	divider = DIV_ROUND_UP((uint64_t)cfg->clock_frequency, 2ULL * spi_cfg->frequency);
	divider = (divider > 0U) ? divider - 1U : 0U;
	if (divider > SAPPHIRE_SPI_TIMING_MAX) {
		LOG_ERR("Frequency %u Hz too low", spi_cfg->frequency);
		return -EINVAL;
	}

	/* SS timings in system clocks, at least one SCLK half period; the fields are 12 bits */
	half = divider + 1U;
	setup = MIN(half, SAPPHIRE_SPI_TIMING_MAX);
	hold = setup;
	if (!spi_cs_is_gpio(spi_cfg)) {
		setup = MAX(setup, spi_efinix_sapphire_ns_to_clk(dev, spi_cfg->cs.setup_ns));
		hold = MAX(hold, spi_efinix_sapphire_ns_to_clk(dev, spi_cfg->cs.hold_ns));
	}
	disable = MAX(MIN(2U * half, SAPPHIRE_SPI_TIMING_MAX),
		      spi_efinix_sapphire_ns_to_clk(dev, SAPPHIRE_SPI_SS_DISABLE_NS));

	reg = 0U;
	if ((op & SPI_MODE_CPOL) != 0U) {
		reg |= SAPPHIRE_SPI_CONFIG_CPOL;
	}
	if ((op & SPI_MODE_CPHA) != 0U) {
		reg |= SAPPHIRE_SPI_CONFIG_CPHA;
	}

	/* Never reprogram the controller under a queued command. */
	ret = spi_efinix_sapphire_wait_idle(dev);
	if (ret < 0) {
		data->recover = true;
		return ret;
	}

	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_CONFIG, reg);
	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_CLK_DIVIDER, divider);
	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_SS_SETUP, setup);
	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_SS_HOLD, hold);
	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_SS_DISABLE, disable);

	if (!spi_cs_is_gpio(spi_cfg)) {
		WRITE_BIT(data->ss_active_high, spi_cfg->peripheral,
			  (op & SPI_CS_ACTIVE_HIGH) != 0U);
		spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_SS_ACTIVE_HIGH, data->ss_active_high);
	}

	/* Timeout: ten times the worst-case time of one byte, SS sequencing included */
	byte_clk = 16ULL * half + setup + hold + disable;
	data->timeout_us =
		MAX(SAPPHIRE_SPI_TIMEOUT_MIN_US,
		    (uint32_t)DIV_ROUND_UP(10ULL * byte_clk * USEC_PER_SEC, cfg->clock_frequency));

	data->ctx.config = spi_cfg;

	LOG_DBG("divider %u setup %u hold %u disable %u", divider, setup, hold, disable);

	return 0;
}

static void spi_efinix_sapphire_ss(const struct device *dev, uint16_t peripheral, bool assert)
{
	struct spi_efinix_sapphire_data *data = dev->data;
	uint32_t cmd = SAPPHIRE_SPI_CMD_SS | peripheral;

	if (assert) {
		cmd |= SAPPHIRE_SPI_CMD_SS_ENABLE;
	}

	/*
	 * The command FIFO never holds more than SAPPHIRE_SPI_MAX_INFLIGHT data
	 * commands, so there is always room for an SS command.
	 */
	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_DATA, cmd);
	data->ss_asserted = assert ? peripheral : -1;
}

/* Stays pending if the controller does not go idle. */
static int spi_efinix_sapphire_recover(const struct device *dev)
{
	const struct spi_efinix_sapphire_config *cfg = dev->config;
	struct spi_efinix_sapphire_data *data = dev->data;
	int ret;

	/* First idle: the command FIFO may be full of stale commands. */
	ret = spi_efinix_sapphire_wait_idle(dev);
	if (ret == 0) {
		for (uint16_t i = 0U; i < cfg->num_cs; i++) {
			spi_efinix_sapphire_ss(dev, i, false);
		}
		ret = spi_efinix_sapphire_wait_idle(dev);
	}
	data->recover = ret < 0;

	return ret;
}

static int spi_efinix_sapphire_xfer(const struct device *dev)
{
	struct spi_efinix_sapphire_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t len = MAX(spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));
	size_t sent = 0;
	size_t recv = 0;
	uint8_t rx;

	/* Every byte is a write+read command, so each yields one response. */
	while (recv < len) {
		while (sent < len && (sent - recv) < SAPPHIRE_SPI_MAX_INFLIGHT) {
			uint8_t tx = spi_context_tx_buf_on(ctx) ? *ctx->tx_buf : 0U;

			spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_DATA,
						  SAPPHIRE_SPI_CMD_WRITE | SAPPHIRE_SPI_CMD_READ |
							  tx);
			spi_context_update_tx(ctx, 1, 1);
			sent++;
		}

		if (!WAIT_FOR(spi_efinix_sapphire_pop(dev, &rx), data->timeout_us, NULL)) {
			LOG_ERR("Timeout after %zu of %zu bytes", recv, len);
			return -ETIMEDOUT;
		}

		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = rx;
		}
		spi_context_update_rx(ctx, 1, 1);
		recv++;
	}

	return 0;
}

static int spi_efinix_sapphire_transceive(const struct device *dev,
					  const struct spi_config *spi_cfg,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs)
{
	struct spi_efinix_sapphire_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool hw_cs = !spi_cs_is_gpio(spi_cfg);
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);

	if (data->recover) {
		ret = spi_efinix_sapphire_recover(dev);
		if (ret < 0) {
			goto out;
		}
	}

	/* Release an SS line still held for another device, also before a cs-gpios transfer. */
	if (data->ss_asserted >= 0 && (!hw_cs || data->ss_asserted != (int)spi_cfg->peripheral)) {
		spi_efinix_sapphire_ss(dev, data->ss_asserted, false);
	}

	ret = spi_efinix_sapphire_configure(dev, spi_cfg);
	if (ret < 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	if (hw_cs) {
		if (data->ss_asserted < 0) {
			spi_efinix_sapphire_ss(dev, spi_cfg->peripheral, true);
		}
	} else {
		spi_context_cs_control(ctx, true);
	}

	ret = spi_efinix_sapphire_xfer(dev);

	if (ret < 0) {
		/* Discard the rest of the failed transfer so later ones stay aligned. */
		(void)spi_efinix_sapphire_recover(dev);
	} else if (hw_cs && (spi_cfg->operation & SPI_HOLD_ON_CS) == 0U) {
		spi_efinix_sapphire_ss(dev, spi_cfg->peripheral, false);
	}

	if (!hw_cs) {
		spi_context_cs_control(ctx, false);
	}

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_efinix_sapphire_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_efinix_sapphire_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, spi_cfg)) {
		return -EINVAL;
	}

	if (data->ss_asserted >= 0) {
		spi_efinix_sapphire_ss(dev, data->ss_asserted, false);
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_efinix_sapphire_init(const struct device *dev)
{
	struct spi_efinix_sapphire_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_INTERRUPT, 0U);
	data->ss_asserted = -1;

	/* A previous boot stage may have left commands, responses or SS asserted. */
	(void)spi_efinix_sapphire_recover(dev);

	spi_efinix_sapphire_write(dev, SAPPHIRE_SPI_SS_ACTIVE_HIGH, 0U);
	data->ss_active_high = 0U;

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_efinix_sapphire_api) = {
	.transceive = spi_efinix_sapphire_transceive,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_efinix_sapphire_release,
};

#define SPI_EFINIX_SAPPHIRE_INIT(n)                                                                \
	static struct spi_efinix_sapphire_data spi_efinix_sapphire_data_##n = {                    \
		SPI_CONTEXT_INIT_LOCK(spi_efinix_sapphire_data_##n, ctx),                          \
		SPI_CONTEXT_INIT_SYNC(spi_efinix_sapphire_data_##n, ctx),                          \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	static const struct spi_efinix_sapphire_config spi_efinix_sapphire_config_##n = {          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.num_cs = DT_INST_PROP(n, num_cs),                                                 \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_efinix_sapphire_init, NULL,                               \
				  &spi_efinix_sapphire_data_##n, &spi_efinix_sapphire_config_##n,  \
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,                           \
				  &spi_efinix_sapphire_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_EFINIX_SAPPHIRE_INIT)
