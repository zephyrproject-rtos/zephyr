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
#define SPI_MCUX_ECSPI_MAX_RX_THRESHOLD                                                            \
	(ECSPI_DMAREG_RX_THRESHOLD_MASK >> ECSPI_DMAREG_RX_THRESHOLD_SHIFT)

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

	/* Bytes of the burst in flight still to be read from the Rx FIFO. */
	uint16_t rx_bytes;
};

/*
 * The MCUX driver sets the burst length and the Rx threshold from
 * ECSPI_MasterInit() only, which resets the peripheral, and it never uses the
 * exchange bit. Sizing a burst per transfer reaches those fields itself.
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

static inline void spi_mcux_set_rx_threshold(ECSPI_Type *base, uint8_t words)
{
	base->DMAREG = (base->DMAREG & ~ECSPI_DMAREG_RX_THRESHOLD_MASK) |
		       ECSPI_DMAREG_RX_THRESHOLD(words);
}

static inline void spi_mcux_start_burst(ECSPI_Type *base)
{
	base->CONREG |= ECSPI_CONREG_XCH_MASK;
}

static inline uint32_t frame_get(const uint8_t *buf, uint16_t dfs)
{
	switch (dfs) {
	case 1U:
		return UNALIGNED_GET((uint8_t *)buf);
	case 2U:
		return UNALIGNED_GET((uint16_t *)buf);
	default:
		return UNALIGNED_GET((uint32_t *)buf);
	}
}

static inline void frame_put(uint8_t *buf, uint16_t dfs, uint32_t frame)
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

/*
 * A burst goes out most significant frame first, and one that is not a multiple
 * of 32 bits long carries its leading frames right-aligned in its first FIFO
 * word. Both FIFOs follow that layout, so the bytes a FIFO word holds follow
 * from what is left of the burst.
 */
static inline uint8_t fifo_word_bytes(uint16_t bytes_left)
{
	uint8_t bytes = bytes_left % sizeof(uint32_t);

	return (bytes == 0U) ? (uint8_t)sizeof(uint32_t) : bytes;
}

static void spi_mcux_fill_burst(const struct device *dev, uint16_t bytes)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint16_t dfs = data->dfs;

	while (bytes != 0U) {
		uint8_t word_bytes = fifo_word_bytes(bytes);
		uint32_t word = 0U;

		for (int shift = (int)word_bytes - (int)dfs; shift >= 0; shift -= (int)dfs) {
			if (spi_context_tx_buf_on(ctx)) {
				word |= frame_get(ctx->tx_buf, dfs) << (8 * shift);
			}

			spi_context_update_tx(ctx, dfs, 1);
		}

		ECSPI_WriteData(config->base, word);
		bytes -= word_bytes;
	}
}

static void spi_mcux_drain_burst(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const uint16_t dfs = data->dfs;

	while ((data->rx_bytes != 0U) && (ECSPI_GetRxFifoCount(config->base) != 0U)) {
		uint8_t word_bytes = fifo_word_bytes(data->rx_bytes);
		uint32_t word = ECSPI_ReadData(config->base);

		for (int shift = (int)word_bytes - (int)dfs; shift >= 0; shift -= (int)dfs) {
			if (spi_context_rx_buf_on(ctx)) {
				frame_put(ctx->rx_buf, dfs, word >> (8 * shift));
			}

			spi_context_update_rx(ctx, dfs, 1);
		}

		data->rx_bytes -= word_bytes;
	}
}

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

static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	uint16_t frames;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		ECSPI_DisableInterrupts(base, kECSPI_RxFifoDataRequstInterruptEnable);
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return;
	}

	ECSPI_SetChannelSelect(base, spi_cs_is_gpio(ctx->config)
					     ? kECSPI_Channel0
					     : (ecspi_channel_source_t)ctx->config->slave);

	/* One burst per chunk: a burst is one chip-select window and, preloaded
	 * into the Tx FIFO, one interrupt.
	 */
	frames = MIN(spi_context_max_continuous_chunk(ctx), data->max_frames);
	data->rx_bytes = frames * data->dfs;

	spi_mcux_set_burst_length(base, frames * data->word_size);
	spi_mcux_set_rx_threshold(base, DIV_ROUND_UP(data->rx_bytes, sizeof(uint32_t)) - 1U);

	spi_mcux_fill_burst(dev, data->rx_bytes);

	/* The Tx FIFO holds all of the burst now, so start it. */
	spi_mcux_start_burst(base);
}

static void spi_mcux_isr(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;

	spi_mcux_drain_burst(dev);

	if (data->rx_bytes == 0U) {
		spi_mcux_transfer_next_packet(dev);
	}
}

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	ECSPI_Type *base = config->base;
	ecspi_master_config_t master_config;
	uint32_t clock_freq;
	uint16_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("HW byte re-ordering not supported");
		return -ENOTSUP;
	}

	if (!spi_cs_is_gpio(spi_cfg) && spi_cfg->slave > kECSPI_Channel3) {
		LOG_ERR("Slave %d is greater than %d", spi_cfg->slave, kECSPI_Channel3);
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

	ECSPI_MasterGetDefaultConfig(&master_config);

	master_config.channel =
		spi_cs_is_gpio(spi_cfg) ? kECSPI_Channel0 : (ecspi_channel_source_t)spi_cfg->slave;
	master_config.channelConfig.clockInactiveState =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kECSPI_ClockInactiveStateHigh
		: kECSPI_ClockInactiveStateLow;
	master_config.channelConfig.polarity =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kECSPI_PolarityActiveLow
		: kECSPI_PolarityActiveHigh;
	master_config.channelConfig.phase =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kECSPI_ClockPhaseSecondEdge
		: kECSPI_ClockPhaseFirstEdge;
	master_config.baudRate_Bps = spi_cfg->frequency;
	master_config.burstLength = word_size;

	master_config.enableLoopback = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP);

	if (!spi_cs_is_gpio(spi_cfg)) {
		uint32_t clock_cycles =
			DIV_ROUND_UP(spi_cfg->cs.delay * USEC_PER_SEC, spi_cfg->frequency);

		if (clock_cycles > 63U) {
			LOG_ERR("CS delay is greater than 63 clock cycles (%u)", clock_cycles);
			return -EINVAL;
		}
		master_config.chipSelectDelay = (uint8_t)clock_cycles;
	}

	ECSPI_MasterInit(base, &master_config, clock_freq);

	/* Start a burst from the exchange bit instead of from the first Tx FIFO
	 * write, so that it starts only once all of it is in the FIFO. A burst
	 * under way then never waits for software to refill it, whatever preempts
	 * the driver, and cannot stall with the chip select asserted.
	 */
	spi_mcux_use_xch_start(base);

	data->word_size = word_size;
	data->dfs = bytes_per_word(word_size);

	/* Only frames that fill their bytes pack into FIFO words; the rest keep
	 * a burst each. A preloaded burst is bounded by the FIFO depth, by the
	 * Rx threshold field and by the burst length field.
	 */
	if (word_size == data->dfs * BITS_PER_BYTE) {
		uint32_t words = MIN(FSL_FEATURE_ECSPI_TX_FIFO_SIZEn(base),
				     SPI_MCUX_ECSPI_MAX_RX_THRESHOLD + 1U);

		data->max_frames = MIN(SPI_MCUX_ECSPI_MAX_BURST / word_size,
				       words * sizeof(uint32_t) / data->dfs);
	} else {
		data->max_frames = 1U;
	}

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

	/* The Rx threshold is set per burst, so this fires once a burst is in. */
	ECSPI_EnableInterrupts(config->base, kECSPI_RxFifoDataRequstInterruptEnable);

	spi_mcux_transfer_next_packet(dev);
	ret = spi_context_wait_for_completion(&data->ctx);

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
