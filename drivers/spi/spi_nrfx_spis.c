/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/spi.h>
#include <nrfx_spis.h>

#define LOG_DOMAIN "spi_nrfx_spis"
#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_nrfx_spis);

#include "spi_context.h"

struct spi_nrfx_data {
	struct spi_context ctx;
};

struct spi_nrfx_config {
	nrfx_spis_t spis;
	size_t      max_buf_len;
};

static inline struct spi_nrfx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct spi_nrfx_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static inline nrf_spis_mode_t get_nrf_spis_mode(u16_t operation)
{
	if (SPI_MODE_GET(operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPIS_MODE_3;
		} else {
			return NRF_SPIS_MODE_2;
		}
	} else {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPIS_MODE_1;
		} else {
			return NRF_SPIS_MODE_0;
		}
	}
}

static inline nrf_spis_bit_order_t get_nrf_spis_bit_order(u16_t operation)
{
	if (operation & SPI_TRANSFER_LSB) {
		return NRF_SPIS_BIT_ORDER_LSB_FIRST;
	} else {
		return NRF_SPIS_BIT_ORDER_MSB_FIRST;
	}
}

static int configure(struct device *dev,
		     const struct spi_config *spi_cfg)
{
	struct spi_context *ctx = &get_dev_data(dev)->ctx;

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		LOG_ERR("Master mode is not supported on %s",
			    dev->config->name);
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if ((spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		LOG_ERR("Word sizes other than 8 bits"
			    " are not supported");
		return -EINVAL;
	}

	if (spi_cfg->cs) {
		LOG_ERR("CS control via GPIO is not supported");
		return -EINVAL;
	}

	ctx->config = spi_cfg;

	nrf_spis_configure(get_dev_config(dev)->spis.p_reg,
			   get_nrf_spis_mode(spi_cfg->operation),
			   get_nrf_spis_bit_order(spi_cfg->operation));

	return 0;
}

static void prepare_for_transfer(struct device *dev)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);
	const struct spi_nrfx_config *dev_config = get_dev_config(dev);
	struct spi_context *ctx = &dev_data->ctx;
	int status;

	size_t buf_len = spi_context_longest_current_buf(ctx);

	if (buf_len > 0) {
		nrfx_err_t result;

		if (buf_len > dev_config->max_buf_len) {
			buf_len = dev_config->max_buf_len;
		}

		result = nrfx_spis_buffers_set(
			&dev_config->spis,
			ctx->tx_buf,
			spi_context_tx_buf_on(ctx) ? buf_len : 0,
			ctx->rx_buf,
			spi_context_rx_buf_on(ctx) ? buf_len : 0);
		if (result == NRFX_SUCCESS) {
			return;
		}

		/* Cannot prepare for transfer. */
		status = -EIO;
	} else {
		/* Zero-length buffer provided. */
		status = 0;
	}

	spi_context_complete(ctx, status);
}

static int transceive(struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);
	int error;

	error = configure(dev, spi_cfg);
	if (error != 0) {
		/* Invalid configuration. */
	} else if ((tx_bufs && tx_bufs->count > 1) ||
		   (rx_bufs && rx_bufs->count > 1)) {
		LOG_ERR("Scattered buffers are not supported");
		error = -ENOTSUP;
	} else if (tx_bufs && tx_bufs->buffers[0].len &&
		   !nrfx_is_in_ram(tx_bufs->buffers[0].buf)) {
		LOG_ERR("Only buffers located in RAM are supported");
		error = -ENOTSUP;
	} else {
		spi_context_buffers_setup(&dev_data->ctx, tx_bufs, rx_bufs, 1);

		prepare_for_transfer(dev);

		error = spi_context_wait_for_completion(&dev_data->ctx);
	}

	spi_context_release(&dev_data->ctx, error);

	return error;
}

static int spi_nrfx_transceive(struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	spi_context_lock(&get_dev_data(dev)->ctx, false, NULL);
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_nrfx_transceive_async(struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	spi_context_lock(&get_dev_data(dev)->ctx, true, async);
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_nrfx_release(struct device *dev,
			    const struct spi_config *spi_cfg)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);

	if (!spi_context_configured(&dev_data->ctx, spi_cfg)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&dev_data->ctx);

	return 0;
}

static const struct spi_driver_api spi_nrfx_driver_api = {
	.transceive = spi_nrfx_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nrfx_transceive_async,
#endif
	.release = spi_nrfx_release,
};


static void event_handler(const nrfx_spis_evt_t *p_event, void *p_context)
{
	struct device *dev = p_context;
	struct spi_nrfx_data *dev_data = get_dev_data(dev);

	if (p_event->evt_type == NRFX_SPIS_XFER_DONE) {
		spi_context_complete(&dev_data->ctx, p_event->rx_amount);
	}
}

static int init_spis(struct device *dev, const nrfx_spis_config_t *config)
{
	/* This sets only default values of frequency, mode and bit order.
	 * The proper ones are set in configure() when a transfer is started.
	 */
	nrfx_err_t result = nrfx_spis_init(&get_dev_config(dev)->spis,
					   config,
					   event_handler,
					   dev);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&get_dev_data(dev)->ctx);

	return 0;
}

#define SPI_NRFX_SPIS_DEVICE(idx)					       \
	static int spi_##idx##_init(struct device *dev)			       \
	{								       \
		IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SPIS##idx),		       \
			    DT_NORDIC_NRF_SPIS_SPI_##idx##_IRQ_0_PRIORITY,     \
			    nrfx_isr, nrfx_spis_##idx##_irq_handler, 0);       \
		const nrfx_spis_config_t config = {			       \
			.sck_pin    = DT_NORDIC_NRF_SPIS_SPI_##idx##_SCK_PIN,  \
			.mosi_pin   = DT_NORDIC_NRF_SPIS_SPI_##idx##_MOSI_PIN, \
			.miso_pin   = DT_NORDIC_NRF_SPIS_SPI_##idx##_MISO_PIN, \
			.csn_pin    = DT_NORDIC_NRF_SPIS_SPI_##idx##_CSN_PIN,  \
			.mode       = NRF_SPIS_MODE_0,			       \
			.bit_order  = NRF_SPIS_BIT_ORDER_MSB_FIRST,	       \
			.csn_pullup = NRFX_SPIS_DEFAULT_CSN_PULLUP,	       \
			.miso_drive = NRFX_SPIS_DEFAULT_MISO_DRIVE,	       \
			.orc        = CONFIG_SPI_##idx##_NRF_ORC,	       \
			.def        = DT_NORDIC_NRF_SPIS_SPI_##idx##_DEF_CHAR, \
		};							       \
		return init_spis(dev, &config);				       \
	}								       \
	static struct spi_nrfx_data spi_##idx##_data = {		       \
		SPI_CONTEXT_INIT_LOCK(spi_##idx##_data, ctx),		       \
		SPI_CONTEXT_INIT_SYNC(spi_##idx##_data, ctx),		       \
	};								       \
	static const struct spi_nrfx_config spi_##idx##z_config = {	       \
		.spis = NRFX_SPIS_INSTANCE(idx),			       \
		.max_buf_len = (1 << SPIS##idx##_EASYDMA_MAXCNT_SIZE) - 1,     \
	};								       \
	DEVICE_AND_API_INIT(spi_##idx,					       \
			    DT_NORDIC_NRF_SPIS_SPI_##idx##_LABEL,	       \
			    spi_##idx##_init,				       \
			    &spi_##idx##_data,				       \
			    &spi_##idx##z_config,			       \
			    POST_KERNEL,				       \
			    CONFIG_SPI_INIT_PRIORITY,			       \
			    &spi_nrfx_driver_api)

#ifdef CONFIG_SPI_0_NRF_SPIS
SPI_NRFX_SPIS_DEVICE(0);
#endif

#ifdef CONFIG_SPI_1_NRF_SPIS
SPI_NRFX_SPIS_DEVICE(1);
#endif

#ifdef CONFIG_SPI_2_NRF_SPIS
SPI_NRFX_SPIS_DEVICE(2);
#endif

#ifdef CONFIG_SPI_3_NRF_SPIS
SPI_NRFX_SPIS_DEVICE(3);
#endif
