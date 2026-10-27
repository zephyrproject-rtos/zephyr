/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc84x_spi

#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <fsl_spi.h>
#include <zephyr/sys/clock.h>

LOG_MODULE_REGISTER(spi_lpc84x, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#define SPI_MAX_DATA_WIDTH 16
#define SPI_MIN_DATA_WIDTH 4

struct spi_lpc84x_config {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	/* Pointer to the instance-specific IRQ connect function */
	void (*irq_config_func)(const struct device *dev);
	uint32_t def_char;
	uint32_t pre_delay;
	uint32_t post_delay;
	uint32_t frame_delay;
	uint32_t transfer_delay;
};

struct spi_lpc84x_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	uint32_t last_freq; /* frequency the hardware is actually set to */
	uint16_t last_op;   /* operation flags the hardware is actually set to */
	uint8_t word_size_bytes;
#ifdef CONFIG_SPI_LPC84X_INTERRUPT
	spi_master_handle_t fsl_handle; /* FSL transactional handle            */
	size_t transfer_len;
#endif
};

#ifdef CONFIG_SPI_LPC84X_INTERRUPT
/*
 * FSL transactional callback – called from SPI_MasterTransferHandleIRQ()
 * inside the Zephyr ISR dispatcher (spi_lpc84x_irq_handler below).
 *
 * NOTE: The FSL HAL passes the handle as the second parameter.  We only need
 * the `status` to signal completion; the device pointer is recovered via
 * userData.
 */
static void spi_lpc84x_transfer_callback(SPI_Type *base, spi_master_handle_t *handle,
					 status_t status, void *userData)
{
	ARG_UNUSED(base);
	ARG_UNUSED(handle);
	ARG_UNUSED(status);
	const struct device *dev = (const struct device *)userData;
	struct spi_lpc84x_data *data = dev->data;

	spi_context_complete(&data->ctx, dev, 0);
}

/*
 * Zephyr ISR – connected to the hardware IRQ line.
 * Delegates to the FSL IRQ handler which drives the state machine and fires
 * spi_lpc84x_transfer_callback() when a transfer finishes.
 */
static void spi_lpc84x_irq_handler(const struct device *dev)
{
	struct spi_lpc84x_data *data = dev->data;
	SPI_Type *base = (SPI_Type *)DEVICE_MMIO_GET(dev);

	SPI_MasterTransferHandleIRQ(base, &data->fsl_handle);
}
#endif

static uint8_t spi_clock_cycles(uint32_t delay_ns, uint32_t sck_frequency_hz)
{
	/* Convert delay_ns to an integer number of clock cycles of frequency
	 * sck_frequency_hz. The maximum delay is 15 clock cycles.
	 */
	uint8_t delay_cycles = (uint64_t)delay_ns * sck_frequency_hz / NSEC_PER_SEC;

	delay_cycles = MIN(delay_cycles, 15);

	return delay_cycles;
}

static int spi_lpc84x_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_lpc84x_config *config = dev->config;
	struct spi_lpc84x_data *data = dev->data;
	spi_master_config_t master_config;
	uint32_t clock_freq;
	int ret;
	SPI_Type *base = (SPI_Type *)DEVICE_MMIO_GET(dev);
	uint8_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

	/*
	 * Skip reconfiguration if the peripheral is already initialized with
	 * the same frequency and operation flags. last_freq/last_op are copies
	 * of what was programmed into the hardware, so they stay valid even if
	 * the caller changes or frees its spi_config.Update only the context
	 * config pointer so the SPI subsystem sees the new reference.
	 */
	if (spi_cfg->frequency == 0U) {
		LOG_ERR("Invalid SPI frequency 0");
		return -EINVAL;
	}

	if (data->last_freq == spi_cfg->frequency && data->last_op == spi_cfg->operation) {
		data->ctx.config = spi_cfg;
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_OP_MODE_PERIPHERAL) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if ((word_size < SPI_MIN_DATA_WIDTH) || (word_size > SPI_MAX_DATA_WIDTH)) {
		LOG_ERR("Unsupported word size %u (supported: %u..%u)", word_size,
			SPI_MIN_DATA_WIDTH, SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	data->word_size_bytes = (word_size > 8) ? (sizeof(uint16_t)) : (sizeof(uint8_t));

	/*
	 * Check that SPI peripheral is idle before modifying CFG register
	 * Only check MSTIDLE when EOT is set; otherwise MSTIDLE stays 0 even when idle
	 */
	if (((base->CFG & SPI_CFG_ENABLE_MASK) != 0U) &&
	    ((base->TXCTL & SPI_TXCTL_EOT_MASK) != 0U) &&
	    ((base->STAT & SPI_STAT_MSTIDLE_MASK) == 0U)) {
		LOG_ERR("Cannot configure SPI while peripheral is busy");
		return -EBUSY;
	}

	SPI_MasterGetDefaultConfig(&master_config);

	master_config.dataWidth = (spi_data_width_t)(word_size - 1);

	spi_delay_config_t *delay_config = &master_config.delayConfig;

	delay_config->preDelay = spi_clock_cycles(config->pre_delay, spi_cfg->frequency);
	delay_config->postDelay = spi_clock_cycles(config->post_delay, spi_cfg->frequency);
	delay_config->frameDelay = spi_clock_cycles(config->frame_delay, spi_cfg->frequency);
	delay_config->transferDelay = spi_clock_cycles(config->transfer_delay, spi_cfg->frequency);

	master_config.clockPolarity = (spi_cfg->operation & SPI_MODE_CPOL)
					      ? kSPI_ClockPolarityActiveLow
					      : kSPI_ClockPolarityActiveHigh;

	master_config.clockPhase = (spi_cfg->operation & SPI_MODE_CPHA) ? kSPI_ClockPhaseSecondEdge
									: kSPI_ClockPhaseFirstEdge;

	master_config.direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB) ? kSPI_LsbFirst : kSPI_MsbFirst;

	master_config.baudRate_Bps = spi_cfg->frequency;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EINVAL;
	}

	if ((base->CFG & SPI_CFG_ENABLE_MASK) != 0U) {
		base->CFG &= ~SPI_CFG_ENABLE_MASK;
	}

	ret = SPI_MasterInit(base, &master_config, clock_freq);
	if (ret != kStatus_Success) {
		LOG_ERR("Master init failed");
		return -EINVAL;
	}

#ifdef CONFIG_SPI_LPC84X_INTERRUPT
	SPI_SetDummyData(base, (uint8_t)config->def_char);
	SPI_MasterTransferCreateHandle(base, &data->fsl_handle, spi_lpc84x_transfer_callback,
				       (void *)dev);
#endif

	data->ctx.config = spi_cfg;
	data->last_freq = spi_cfg->frequency;
	data->last_op = spi_cfg->operation;

	return 0;
}

static int spi_lpc84x_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs)
{
	struct spi_lpc84x_data *data = dev->data;
	int ret = 0;
	const struct spi_lpc84x_config *config = dev->config;
	SPI_Type *base = (SPI_Type *)DEVICE_MMIO_GET(dev);

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);

	ret = spi_lpc84x_configure(dev, spi_cfg);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->word_size_bytes);
	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_LPC84X_INTERRUPT
	const uint16_t dummy_tx = (uint16_t)config->def_char;
	status_t status;

	while (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx)) {
		spi_transfer_t xfer = {0};
		size_t tx_len = spi_context_tx_on(&data->ctx) ? data->ctx.tx_len : 0;
		size_t rx_len = spi_context_rx_on(&data->ctx) ? data->ctx.rx_len : 0;
		size_t chunk;

		if (tx_len > 0 && rx_len > 0) {
			chunk = MIN(tx_len, rx_len);
		} else if (tx_len > 0) {
			chunk = tx_len;
		} else {
			chunk = rx_len;
		}

		if (tx_len > 0 && data->ctx.tx_buf != NULL) {
			xfer.txData = (const uint8_t *)data->ctx.tx_buf;
		} else {
			xfer.txData = (const uint8_t *)&dummy_tx;
			chunk = 1;
		}

		/* If rx_buf is NULL, we can safely pass NULL to the HAL.
		 * The HAL will optimize the transfer by setting the RXIGNORE flag.
		 */
		xfer.rxData = (rx_len > 0 && data->ctx.rx_buf != NULL) ? (uint8_t *)data->ctx.rx_buf
								       : NULL;

		xfer.dataSize = chunk * data->word_size_bytes;

		bool last_chunk = ((tx_len == 0) || (chunk == tx_len)) &&
				  ((rx_len == 0) || (chunk == rx_len));
		xfer.configFlags = last_chunk ? kSPI_EndOfTransfer : 0U;

		status = SPI_MasterTransferNonBlocking(base, &data->fsl_handle, &xfer);
		if (status != kStatus_Success) {
			LOG_ERR("SPI_MasterTransferNonBlocking failed: %d", (int)status);
			ret = -EIO;
			break;
		}

		ret = spi_context_wait_for_completion(&data->ctx);
		if (ret != 0) {
			LOG_ERR("SPI transfer failed: %d", ret);
			SPI_MasterTransferAbort(base, &data->fsl_handle);
			break;
		}

		spi_context_update_tx(&data->ctx, data->word_size_bytes, tx_len ? chunk : 0);
		spi_context_update_rx(&data->ctx, data->word_size_bytes, rx_len ? chunk : 0);
	}

#else

	while (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx)) {
		spi_transfer_t transfer = {0};
		uint16_t tx_data = (uint16_t)config->def_char;
		uint16_t rx_data = 0;

		if (spi_context_tx_buf_on(&data->ctx)) {
			if (data->word_size_bytes > 1) {
				tx_data = *(const uint16_t *)data->ctx.tx_buf;
			} else {
				tx_data = *data->ctx.tx_buf;
			}
		}

		transfer.txData = (uint8_t *)&tx_data;
		transfer.rxData = (uint8_t *)&rx_data;
		transfer.dataSize = data->word_size_bytes;

		bool last_byte = (data->ctx.tx_len <= 1) && (data->ctx.rx_len <= 1) &&
				 (data->ctx.rx_count <= 1 && data->ctx.rx_len <= 1);

		transfer.configFlags = last_byte ? kSPI_EndOfTransfer : 0;

		if (SPI_MasterTransferBlocking(base, &transfer) != kStatus_Success) {
			ret = -EIO;
			break;
		}

		LOG_DBG("poll: tx=0x%02x rx=0x%02x", tx_data, rx_data);

		if (spi_context_rx_buf_on(&data->ctx)) {
			if (data->word_size_bytes > 1) {
				*(uint16_t *)data->ctx.rx_buf = rx_data;
			} else {
				*data->ctx.rx_buf = (uint8_t)rx_data;
			}
		}

		spi_context_update_tx(&data->ctx, data->word_size_bytes, 1);
		spi_context_update_rx(&data->ctx, data->word_size_bytes, 1);
	}
#endif

	spi_context_cs_control(&data->ctx, false);

out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_lpc84x_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				       void *userdata)
{
	/* Async mode is not implemented. */
	return -ENOTSUP;
}
#endif

static int spi_lpc84x_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_lpc84x_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_lpc84x_init(const struct device *dev)
{
	const struct spi_lpc84x_config *config = dev->config;
	struct spi_lpc84x_data *data = dev->data;

	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock device is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_SPI_LPC84X_INTERRUPT

	/*
	 * Wire up the hardware IRQ.config->irq_config_func is generated by
	 * the SPI_LPC84X_IRQ_CONNECT() for each DT instance.
	 */
	config->irq_config_func(dev);

#endif

	spi_context_unlock_unconditionally(&data->ctx);
	LOG_INF("spi initialized");

	return 0;
}

static DEVICE_API(spi, spi_lpc84x_driver_api) = {
	.transceive = spi_lpc84x_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_lpc84x_transceive_async,
#endif
	.release = spi_lpc84x_release,
};

#define SPI_LPC84X_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	COND_CODE_1(CONFIG_SPI_LPC84X_INTERRUPT, (						   \
	static void irq_config_func_##n(const struct device *dev)				   \
				{                                                                  \
				IRQ_CONNECT(DT_INST_IRQN(n),					   \
					     DT_INST_IRQ(n, priority),				   \
					     spi_lpc84x_irq_handler,				   \
					     DEVICE_DT_INST_GET(n), 0);				   \
				 irq_enable(DT_INST_IRQN(n));					   \
				 }								   \
			), ())                                       \
                                                                                                   \
	static const struct spi_lpc84x_config spi_lpc84x_config_##n = {                            \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.pre_delay = DT_INST_PROP_OR(n, pre_delay, 0),                                     \
		.post_delay = DT_INST_PROP_OR(n, post_delay, 0),                                   \
		.frame_delay = DT_INST_PROP_OR(n, frame_delay, 0),                                 \
		.transfer_delay = DT_INST_PROP_OR(n, transfer_delay, 0),                           \
		.def_char = DT_INST_PROP_OR(id, def_char, 0),                                      \
		.irq_config_func =                                                                 \
			COND_CODE_1(CONFIG_SPI_LPC84X_INTERRUPT,			   \
				(irq_config_func_##n),						   \
				(NULL)),     \
                                                                                                   \
			};                                                                         \
                                                                                                   \
	static struct spi_lpc84x_data spi_lpc84x_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_lpc84x_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(spi_lpc84x_data_##n, ctx),                                   \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, spi_lpc84x_init, NULL, &spi_lpc84x_data_##n,                      \
			      &spi_lpc84x_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_lpc84x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_LPC84X_INIT)
