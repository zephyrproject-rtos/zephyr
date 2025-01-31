/*
 * Copyright (c) 2024 Daikin Comfort Technologies North America, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_eusart_spi

#include <stdbool.h>

#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <em_cmu.h>
#include <em_eusart.h>

LOG_MODULE_REGISTER(spi_silabs_eusart, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_WORD_SIZE 8

/* Structure Declarations */

struct spi_silabs_eusart_data {
	struct spi_context ctx;
};

struct spi_silabs_eusart_config {
	EUSART_TypeDef *base;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
};

/* Helper Functions */
static int spi_silabs_eusart_configure(const struct device *dev, const struct spi_config *config,
			     uint16_t *control)
{
	struct spi_silabs_eusart_data *data = dev->data;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	uint32_t spi_frequency;

	EUSART_SpiAdvancedInit_TypeDef eusartAdvancedSpiInit = EUSART_SPI_ADVANCED_INIT_DEFAULT;
	EUSART_SpiInit_TypeDef eusartInit = EUSART_SPI_MASTER_INIT_DEFAULT_HF;

	int err;

	err = clock_control_get_rate(eusart_config->clock_dev,
				     (clock_control_subsys_t)&eusart_config->clock_cfg,
				     &spi_frequency);
	if (err) {
		return err;
	}
	/* Max supported SPI frequency is half the source clock */
	spi_frequency /= 2;

	if (spi_context_configured(&data->ctx, config)) {
		/* Already configured. No need to do it again, but must re-enable in case
		 * TXEN/RXEN were cleared due to deep sleep.
		 */
		EUSART_Enable(eusart_config->base, eusartEnable);

		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	/* Set frequency to the minimum of what the device supports, what the
	 * user has configured the controller to, and the max frequency for the
	 * transaction.
	 */
	if (eusart_config->clock_frequency > spi_frequency) {
		LOG_ERR("SPI clock-frequency too high");
		return -EINVAL;
	}
	spi_frequency = MIN(eusart_config->clock_frequency, spi_frequency);
	if (config->frequency) {
		spi_frequency = MIN(config->frequency, spi_frequency);
	}
	eusartInit.bitRate = spi_frequency;

	if (config->operation & SPI_MODE_LOOP) {
		eusartInit.loopbackEnable = eusartLoopbackEnable;
	} else {
		eusartInit.loopbackEnable = eusartLoopbackDisable;
	}

	/* Set Clock Mode */
	if (config->operation & SPI_MODE_CPOL) {
		if (config->operation & SPI_MODE_CPHA) {
			eusartInit.clockMode = eusartClockMode3;
		} else {
			eusartInit.clockMode = eusartClockMode2;
		}
	} else {
		if (config->operation & SPI_MODE_CPHA) {
			eusartInit.clockMode = eusartClockMode1;
		} else {
			eusartInit.clockMode = eusartClockMode0;
		}
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		eusartAdvancedSpiInit.csPolarity = eusartCsActiveHigh;
	} else {
		eusartAdvancedSpiInit.csPolarity = eusartCsActiveLow;
	}

	eusartAdvancedSpiInit.msbFirst = !(config->operation & SPI_TRANSFER_LSB);
	eusartAdvancedSpiInit.autoCsEnable = !spi_cs_is_gpio(config);
	eusartInit.databits = eusartDataBits8;
	eusartInit.advancedSettings = &eusartAdvancedSpiInit;

	/* Enable EUSART clock */
	err = clock_control_on(eusart_config->clock_dev,
			       (clock_control_subsys_t)&eusart_config->clock_cfg);
	if (err < 0) {
		return err;
	}

	/* Initialize the EUSART */
	EUSART_SpiInit(eusart_config->base, &eusartInit);

	data->ctx.config = config;

	return 0;
}

static void spi_silabs_eusart_send(EUSART_TypeDef *eusart, uint8_t frame)
{
	/* Write frame to register */
	EUSART_Tx(eusart, frame);

	/* Wait until the transfer ends */
	while (!(eusart->STATUS & EUSART_STATUS_TXC)) {
	}
}

static uint8_t spi_silabs_eusart_recv(EUSART_TypeDef *eusart)
{
	/* Return data inside rx register */
	return EUSART_Rx(eusart);
}

static bool spi_silabs_eusart_transfer_ongoing(struct spi_silabs_eusart_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static inline uint8_t spi_silabs_eusart_next_tx(struct spi_silabs_eusart_data *data)
{
	uint8_t tx_frame = 0;

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
	}

	return tx_frame;
}

static int spi_silabs_eusart_shift_frames(EUSART_TypeDef *eusart,
					  struct spi_silabs_eusart_data *data)
{
	uint8_t tx_frame;
	uint8_t rx_frame;

	tx_frame = spi_silabs_eusart_next_tx(data);
	spi_silabs_eusart_send(eusart, tx_frame);
	spi_context_update_tx(&data->ctx, 1, 1);

	rx_frame = spi_silabs_eusart_recv(eusart);

	if (spi_context_rx_buf_on(&data->ctx)) {
		UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
	}
	spi_context_update_rx(&data->ctx, 1, 1);
	return 0;
}

static void spi_silabs_eusart_xfer(const struct device *dev, const struct spi_config *config)
{
	int ret;
	struct spi_silabs_eusart_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;

	spi_context_cs_control(ctx, true);

	do {
		ret = spi_silabs_eusart_shift_frames(eusart_config->base, data);
	} while (!ret && spi_silabs_eusart_transfer_ongoing(data));

	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, 0);
}

/* API Functions */
static int spi_silabs_eusart_init(const struct device *dev)
{
	int err;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;

	err = pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_silabs_eusart_transceive(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	struct spi_silabs_eusart_data *data = dev->data;
	uint16_t control = 0;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_silabs_eusart_configure(dev, config, &control);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);
	spi_silabs_eusart_xfer(dev, config);

	spi_context_release(&data->ctx, ret);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_silabs_eusart_transceive_async(const struct device *dev,
					     const struct spi_config *config,
					     const struct spi_buf_set *tx_bufs,
					     const struct spi_buf_set *rx_bufs,
					     struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_silabs_eusart_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	if (!(eusart_config->base->STATUS & EUSART_STATUS_TXIDLE)) {
		return -EBUSY;
	}
	return 0;
}

/* Device Instantiation */
static DEVICE_API(spi, spi_silabs_eusart_api) = {
	.transceive = spi_silabs_eusart_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_silabs_eusart_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_silabs_eusart_release,
};

#define SPI_INIT(n)										\
	PINCTRL_DT_INST_DEFINE(n);								\
	static struct spi_silabs_eusart_data spi_silabs_eusart_data_##n = {			\
		SPI_CONTEXT_INIT_LOCK(spi_silabs_eusart_data_##n, ctx),				\
		SPI_CONTEXT_INIT_SYNC(spi_silabs_eusart_data_##n, ctx),				\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};				\
	static struct spi_silabs_eusart_config spi_silabs_eusart_cfg_##n = {			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.base = (EUSART_TypeDef *)DT_INST_REG_ADDR(n),					\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n),					\
		.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 1000000)			\
	};											\
	SPI_DEVICE_DT_INST_DEFINE(n, spi_silabs_eusart_init, NULL, &spi_silabs_eusart_data_##n,	\
			      &spi_silabs_eusart_cfg_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,\
			      &spi_silabs_eusart_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
