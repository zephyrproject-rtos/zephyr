/*
 * Copyright (c) 2024, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ecspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_ecspi, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include "spi_rtio.h"
#include <fsl_ecspi.h>

#include "spi_context.h"

#define SPI_MCUX_ECSPI_MAX_BURST 4096
#define SPI_MCUX_ECSPI_WORD_BITS 32U

struct spi_mcux_config {
	ECSPI_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct spi_mcux_data {
	struct spi_context ctx;

	uint16_t dfs;
	uint16_t word_size;
	uint16_t max_frames;

	/* Bits of the burst in flight still to be read from the Rx FIFO. */
	uint16_t rx_bits;
	/* SCLK rate the dividers give, at or below the requested one */
	uint32_t sclk_hz;

	/* Set on completion timeout; makes a late interrupt a no-op */
	bool aborted;
};

static inline uint16_t bytes_per_word(uint16_t bits_per_word)
{
	if (bits_per_word <= 8U) {
		return 1U;
	}
	if (bits_per_word <= 16U) {
		return 2U;
	}

	return 4U;
}

/* The MCUX driver takes the divider pair closest to the requested rate, which
 * can lie above it. The requested rate is the device's maximum, so this takes
 * the fastest rate that does not exceed it, as CONREG divider fields, and
 * returns that rate, or 0 if even the slowest rate exceeds the request.
 */
static uint32_t spi_mcux_sclk_dividers(uint32_t clock_hz, uint32_t max_hz, uint32_t *dividers)
{
	uint32_t best_hz = 0U;
	uint32_t best_pre = 0U;
	uint32_t best_post = 0U;

	for (uint32_t post = 0U; post < 16U; post++) {
		for (uint32_t pre = 0U; pre < 16U; pre++) {
			uint32_t hz = (clock_hz / (pre + 1U)) >> post;

			if ((hz <= max_hz) && (hz > best_hz)) {
				best_hz = hz;
				best_pre = pre;
				best_post = post;
			}
		}
	}

	*dividers = ECSPI_CONREG_PRE_DIVIDER(best_pre) | ECSPI_CONREG_POST_DIVIDER(best_post);

	return best_hz;
}

/* The MCUX driver sets these fields only in ECSPI_MasterInit(), which resets
 * the peripheral, and never uses the exchange bit.
 */
static inline void spi_mcux_use_xch_start(ECSPI_Type *base)
{
	base->CONREG &= ~ECSPI_CONREG_SMC_MASK;
}

static inline void spi_mcux_set_burst_length(ECSPI_Type *base, uint16_t bits)
{
	base->CONREG = (base->CONREG & ~ECSPI_CONREG_BURST_LENGTH_MASK) |
		       ECSPI_CONREG_BURST_LENGTH(bits - 1U);
}

/* Transfer-completed is a sticky flag; clearing it right before the start makes
 * a set flag belong to this burst.
 */
static inline void spi_mcux_start_burst(ECSPI_Type *base)
{
	ECSPI_ClearStatusFlags(base, kECSPI_TransferCompleteFlag);
	base->CONREG |= ECSPI_CONREG_XCH_MASK;
}

static inline uint32_t spi_mcux_frame_get(const uint8_t *buf, uint16_t dfs)
{
	switch (dfs) {
	case 1U:
		return UNALIGNED_GET((const uint8_t *)buf);
	case 2U:
		return UNALIGNED_GET((const uint16_t *)buf);
	default:
		return UNALIGNED_GET((const uint32_t *)buf);
	}
}

static inline void spi_mcux_frame_put(uint8_t *buf, uint16_t dfs, uint32_t frame)
{
	switch (dfs) {
	case 1U:
		UNALIGNED_PUT(frame, (uint8_t *)buf);
		break;
	case 2U:
		UNALIGNED_PUT(frame, (uint16_t *)buf);
		break;
	default:
		UNALIGNED_PUT(frame, (uint32_t *)buf);
		break;
	}
}

/* A burst goes out most significant frame first. If it is not a multiple of
 * 32 bits long, its first FIFO word carries the leading frames right-aligned,
 * in both FIFOs.
 */
static inline uint8_t spi_mcux_fifo_word_bits(uint16_t bits_left)
{
	uint8_t bits = bits_left % SPI_MCUX_ECSPI_WORD_BITS;

	return (bits == 0U) ? (uint8_t)SPI_MCUX_ECSPI_WORD_BITS : bits;
}

static void spi_mcux_fill_burst(const struct device *dev, uint16_t bits)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint16_t dfs = data->dfs;
	const uint16_t word_size = data->word_size;
	const uint32_t mask = GENMASK(word_size - 1U, 0);

	while (bits != 0U) {
		uint8_t word_bits = spi_mcux_fifo_word_bits(bits);
		uint32_t word = 0U;

		if (spi_context_tx_buf_on(ctx)) {
			const uint8_t *buf = ctx->tx_buf;

			for (int shift = word_bits - word_size; shift >= 0; shift -= word_size) {
				word |= (spi_mcux_frame_get(buf, dfs) & mask) << shift;
				buf += dfs;
			}
		}

		ECSPI_WriteData(config->base, word);
		spi_context_update_tx(ctx, dfs, word_bits / word_size);
		bits -= word_bits;
	}
}

static void spi_mcux_drain_burst(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint16_t dfs = data->dfs;
	const uint16_t word_size = data->word_size;
	const uint32_t mask = GENMASK(word_size - 1U, 0);

	while ((data->rx_bits != 0U) && (ECSPI_GetRxFifoCount(config->base) != 0U)) {
		uint8_t word_bits = spi_mcux_fifo_word_bits(data->rx_bits);
		uint32_t word = ECSPI_ReadData(config->base);

		if (spi_context_rx_buf_on(ctx)) {
			uint8_t *buf = ctx->rx_buf;

			for (int shift = word_bits - word_size; shift >= 0; shift -= word_size) {
				spi_mcux_frame_put(buf, dfs, (word >> shift) & mask);
				buf += dfs;
			}
		}

		spi_context_update_rx(ctx, dfs, word_bits / word_size);
		data->rx_bits -= word_bits;
	}
}

static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	uint16_t frames;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		ECSPI_DisableInterrupts(base, kECSPI_TransferCompleteInterruptEnable);
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return;
	}

	frames = MIN(spi_context_max_continuous_chunk(ctx), data->max_frames);
	data->rx_bits = frames * data->word_size;

	spi_mcux_set_burst_length(base, data->rx_bits);
	spi_mcux_fill_burst(dev, data->rx_bits);
	spi_mcux_start_burst(base);
}

static void spi_mcux_fail_transfer(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;

	/* Makes the next transfer reinitialize the peripheral */
	data->aborted = true;
	ECSPI_DisableInterrupts(config->base, kECSPI_TransferCompleteInterruptEnable);
	spi_context_cs_control(&data->ctx, false);
	spi_context_complete(&data->ctx, dev, -EIO);
}

static void spi_mcux_isr(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;

	/* Not part of a burst: disable all interrupts */
	if (data->aborted || (data->rx_bits == 0U)) {
		ECSPI_DisableInterrupts(base, kECSPI_AllInterruptEnable);
		return;
	}

	/* The reference manual (i.MX 8M Plus, ECSPI STATREG[TC]) warns that the
	 * transfer-completed interrupt can occur before the transfer is complete
	 * and says to poll XCH to confirm it. Drain, release the chip select or
	 * reprogram only once XCH has cleared, so that the read-modify-writes of
	 * CONREG cannot carry XCH into the next burst.
	 */
	if (!WAIT_FOR((base->CONREG & ECSPI_CONREG_XCH_MASK) == 0U,
		      DIV_ROUND_UP(2U * USEC_PER_SEC, data->sclk_hz), NULL)) {
		LOG_ERR("Burst did not end");
		spi_mcux_fail_transfer(dev);
		return;
	}

	spi_mcux_drain_burst(dev);
	if (data->rx_bits != 0U) {
		LOG_ERR("Rx FIFO short of the burst");
		spi_mcux_fail_transfer(dev);
		return;
	}

	spi_mcux_transfer_next_packet(dev);
}

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;
	ecspi_master_config_t controller_config;
	uint32_t clock_freq;
	uint32_t burst_words;
	uint32_t dividers;
	uint32_t sclk_hz;
	uint16_t word_size;

	/* An aborted transfer can leave words in the FIFOs, only the init resets them */
	if (!data->aborted && spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Peripheral mode not supported");
		return -ENOTSUP;
	}

	if ((spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("HW byte re-ordering not supported");
		return -ENOTSUP;
	}

	if (!spi_cs_is_gpio(spi_cfg) && spi_cfg->peripheral > kECSPI_Channel3) {
		LOG_ERR("Peripheral %d is greater than %d", spi_cfg->peripheral, kECSPI_Channel3);
		return -EINVAL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		LOG_ERR("Failed to get clock rate");
		return -EINVAL;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (0 == word_size || word_size > 32) {
		LOG_ERR("Invalid word size (0 < %d <= 32)", word_size);
		return -EINVAL;
	}

	sclk_hz = spi_mcux_sclk_dividers(clock_freq, spi_cfg->frequency, &dividers);
	if (sclk_hz == 0U) {
		LOG_ERR("Frequency %u Hz is below the slowest SCLK", spi_cfg->frequency);
		return -EINVAL;
	}

	ECSPI_MasterGetDefaultConfig(&controller_config);

	controller_config.channel =
		spi_cs_is_gpio(spi_cfg) ? kECSPI_Channel0
					: (ecspi_channel_source_t)spi_cfg->peripheral;
	controller_config.channelConfig.clockInactiveState =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kECSPI_ClockInactiveStateHigh
		: kECSPI_ClockInactiveStateLow;
	controller_config.channelConfig.polarity =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kECSPI_PolarityActiveLow
		: kECSPI_PolarityActiveHigh;
	controller_config.channelConfig.phase =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kECSPI_ClockPhaseSecondEdge
		: kECSPI_ClockPhaseFirstEdge;
	controller_config.baudRate_Bps = spi_cfg->frequency;
	/* Non-zero for the register value init derives; replaced per burst. */
	controller_config.burstLength = word_size;

	controller_config.enableLoopback = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP);

	if (!spi_cs_is_gpio(spi_cfg)) {
		uint64_t clock_cycles =
			DIV_ROUND_UP((uint64_t)spi_cfg->cs.setup_ns * sclk_hz, NSEC_PER_SEC);

		if (clock_cycles > 63U) {
			LOG_ERR("CS setup delay %u ns exceeds 63 clock cycles",
				spi_cfg->cs.setup_ns);
			return -EINVAL;
		}

		/* CSD_CTL delays the first clock only; nothing delays the release.
		 * Several bindings default a hold delay, so it is not an error.
		 */
		if (spi_cfg->cs.hold_ns != 0U) {
			LOG_WRN("CS hold delay %u ns not supported, ignored", spi_cfg->cs.hold_ns);
		}
		controller_config.chipSelectDelay = (uint8_t)clock_cycles;
		controller_config.channelConfig.chipSlectActiveState =
			(spi_cfg->operation & SPI_CS_ACTIVE_HIGH) ? kECSPI_ChipSelectActiveStateHigh
								  : kECSPI_ChipSelectActiveStateLow;
	}

	ECSPI_MasterInit(base, &controller_config, clock_freq);
	base->CONREG =
		(base->CONREG & ~(ECSPI_CONREG_PRE_DIVIDER_MASK | ECSPI_CONREG_POST_DIVIDER_MASK)) |
		dividers;
	ECSPI_SetChannelSelect(base, controller_config.channel);

	/* Start a burst with the exchange bit once all of it is in the Tx FIFO,
	 * so it never waits for a refill with the chip select asserted.
	 */
	spi_mcux_use_xch_start(base);

	data->word_size = word_size;
	data->dfs = bytes_per_word(word_size);
	data->sclk_hz = sclk_hz;

	/* Only bursts of 8-, 16- or 32-bit frames span several FIFO words; a
	 * burst of any other frame size stays within one.
	 */
	burst_words = 1U;
	if (word_size == data->dfs * BITS_PER_BYTE) {
		burst_words = FSL_FEATURE_ECSPI_TX_FIFO_SIZEn(base);
	}
	data->max_frames =
		MIN(burst_words * SPI_MCUX_ECSPI_WORD_BITS, SPI_MCUX_ECSPI_MAX_BURST) / word_size;

	data->ctx.config = spi_cfg;

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(&data->ctx, true);

	data->aborted = false;
	/* A flag left by the previous transfer would otherwise fire at once */
	ECSPI_ClearStatusFlags(config->base, kECSPI_TransferCompleteFlag);
	ECSPI_EnableInterrupts(config->base, kECSPI_TransferCompleteInterruptEnable);

	spi_mcux_transfer_next_packet(dev);
	ret = spi_context_wait_for_completion(&data->ctx);
	if (ret != 0) {
		/*
		 * Sync path only (async never times out here). Disarm the
		 * interrupt, and drop a burst that completed in the race
		 * before it, so neither touches a released context.
		 */
		unsigned int key = irq_lock();

		data->aborted = true;
		ECSPI_DisableInterrupts(config->base, kECSPI_TransferCompleteInterruptEnable);
		irq_unlock(key);

		spi_context_cs_control(&data->ctx, false);
		spi_context_clear_completion(&data->ctx);
	}

out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_mcux_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mcux_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	ARG_UNUSED(spi_cfg);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_init(const struct device *dev)
{
	int ret;
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;

	config->irq_config_func(dev);

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_mcux_driver_api) = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mcux_release,
};

#define SPI_MCUX_ECSPI_INIT(n)									\
	PINCTRL_DT_INST_DEFINE(n);								\
	static void spi_mcux_config_func_##n(const struct device *dev);				\
												\
	static const struct spi_mcux_config spi_mcux_config_##n = {				\
		.base = (ECSPI_Type *) DT_INST_REG_ADDR(n),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),		\
		.irq_config_func = spi_mcux_config_func_##n,					\
	};											\
												\
	static struct spi_mcux_data spi_mcux_data_##n = {					\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),					\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),					\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)				\
	};											\
												\
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_init, NULL,					\
			      &spi_mcux_data_##n, &spi_mcux_config_##n,				\
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,				\
			      &spi_mcux_driver_api);						\
												\
	static void spi_mcux_config_func_##n(const struct device *dev)				\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),				\
			    spi_mcux_isr, DEVICE_DT_INST_GET(n), 0);				\
												\
		irq_enable(DT_INST_IRQN(n));							\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_ECSPI_INIT)
