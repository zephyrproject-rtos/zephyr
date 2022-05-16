/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_spis.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_nrfx_spis, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

struct spi_nrfx_data {
	struct spi_context ctx;
};

struct spi_nrfx_config {
	nrfx_spis_t spis;
	size_t      max_buf_len;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
};

static inline nrf_spis_mode_t get_nrf_spis_mode(uint16_t operation)
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

static inline nrf_spis_bit_order_t get_nrf_spis_bit_order(uint16_t operation)
{
	if (operation & SPI_TRANSFER_LSB) {
		return NRF_SPIS_BIT_ORDER_LSB_FIRST;
	} else {
		return NRF_SPIS_BIT_ORDER_MSB_FIRST;
	}
}

static int configure(const struct device *dev,
		     const struct spi_config *spi_cfg)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	struct spi_context *ctx = &dev_data->ctx;

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		LOG_ERR("Master mode is not supported on %s", dev->name);
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		LOG_ERR("Word sizes other than 8 bits are not supported");
		return -EINVAL;
	}

	if (spi_cfg->cs) {
		LOG_ERR("CS control via GPIO is not supported");
		return -EINVAL;
	}

	ctx->config = spi_cfg;

	nrf_spis_configure(dev_config->spis.p_reg,
			   get_nrf_spis_mode(spi_cfg->operation),
			   get_nrf_spis_bit_order(spi_cfg->operation));

	return 0;
}

static void prepare_for_transfer(const struct device *dev,
				 const uint8_t *tx_buf, size_t tx_buf_len,
				 uint8_t *rx_buf, size_t rx_buf_len)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	int status;

	if (tx_buf_len > dev_config->max_buf_len ||
	    rx_buf_len > dev_config->max_buf_len) {
		LOG_ERR("Invalid buffer sizes: Tx %d/Rx %d",
			tx_buf_len, rx_buf_len);
		status = -EINVAL;
	} else {
		nrfx_err_t result;

		result = nrfx_spis_buffers_set(&dev_config->spis,
					       tx_buf, tx_buf_len,
					       rx_buf, rx_buf_len);
		if (result == NRFX_SUCCESS) {
			return;
		}

		status = -EIO;
	}

	spi_context_complete(&dev_data->ctx, status);
}


static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	struct spi_nrfx_data *dev_data = dev->data;
	int error;

	spi_context_lock(&dev_data->ctx, asynchronous, signal, spi_cfg);

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
		prepare_for_transfer(dev,
				     tx_bufs ? tx_bufs->buffers[0].buf : NULL,
				     tx_bufs ? tx_bufs->buffers[0].len : 0,
				     rx_bufs ? rx_bufs->buffers[0].buf : NULL,
				     rx_bufs ? rx_bufs->buffers[0].len : 0);

		error = spi_context_wait_for_completion(&dev_data->ctx);
	}

	spi_context_release(&dev_data->ctx, error);

	return error;
}

static int spi_nrfx_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_nrfx_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_nrfx_release(const struct device *dev,
			    const struct spi_config *spi_cfg)
{
	struct spi_nrfx_data *dev_data = dev->data;

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
	struct spi_nrfx_data *dev_data = p_context;

	if (p_event->evt_type == NRFX_SPIS_XFER_DONE) {
		spi_context_complete(&dev_data->ctx, p_event->rx_amount);
	}
}

static int init_spis(const struct device *dev,
		     const nrfx_spis_config_t *config)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	/* This sets only default values of mode and bit order. The ones to be
	 * actually used are set in configure() when a transfer is prepared.
	 */
	nrfx_err_t result = nrfx_spis_init(&dev_config->spis,
					   config,
					   event_handler,
					   dev_data);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&dev_data->ctx);

	return 0;
}

/*
 * Current factors requiring use of DT_NODELABEL:
 *
 * - NRFX_SPIS_INSTANCE() requires an SoC instance number
 * - soc-instance-numbered kconfig enables
 * - ORC is a SoC-instance-numbered kconfig option instead of a DT property
 */

#define SPIS(idx) DT_NODELABEL(spi##idx)
#define SPIS_PROP(idx, prop) DT_PROP(SPIS(idx), prop)

#define SPI_NRFX_SPIS_PIN_CFG(idx)					\
	COND_CODE_1(CONFIG_PINCTRL,					\
		(.skip_gpio_cfg = true,					\
		 .skip_psel_cfg = true,),				\
		(.sck_pin    = SPIS_PROP(idx, sck_pin),			\
		 .mosi_pin   = DT_PROP_OR(SPIS(idx), mosi_pin,		\
					  NRFX_SPIS_PIN_NOT_USED),	\
		 .miso_pin   = DT_PROP_OR(SPIS(idx), miso_pin,		\
					  NRFX_SPIS_PIN_NOT_USED),	\
		 .csn_pin    = SPIS_PROP(idx, csn_pin),			\
		 .csn_pullup = NRF_GPIO_PIN_NOPULL,			\
		 .miso_drive = NRF_GPIO_PIN_S0S1,))

#define SPI_NRFX_SPIS_DEVICE(idx)					       \
	NRF_DT_CHECK_PIN_ASSIGNMENTS(SPIS(idx), 0,			       \
				     sck_pin, mosi_pin, miso_pin, csn_pin);    \
	static int spi_##idx##_init(const struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(SPIS(idx)), DT_IRQ(SPIS(idx), priority),   \
			    nrfx_isr, nrfx_spis_##idx##_irq_handler, 0);       \
		const nrfx_spis_config_t config = {			       \
			SPI_NRFX_SPIS_PIN_CFG(idx)			       \
			.mode      = NRF_SPIS_MODE_0,			       \
			.bit_order = NRF_SPIS_BIT_ORDER_MSB_FIRST,	       \
			.orc       = CONFIG_SPI_##idx##_NRF_ORC,	       \
			.def       = SPIS_PROP(idx, def_char),		       \
		};							       \
		IF_ENABLED(CONFIG_PINCTRL, (				       \
			const struct spi_nrfx_config *dev_config = dev->config;\
			int err = pinctrl_apply_state(dev_config->pcfg,	       \
						      PINCTRL_STATE_DEFAULT);  \
			if (err < 0) {					       \
				return err;				       \
			}						       \
		))							       \
		return init_spis(dev, &config);				       \
	}								       \
	static struct spi_nrfx_data spi_##idx##_data = {		       \
		SPI_CONTEXT_INIT_LOCK(spi_##idx##_data, ctx),		       \
		SPI_CONTEXT_INIT_SYNC(spi_##idx##_data, ctx),		       \
	};								       \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_DEFINE(SPIS(idx))));	       \
	static const struct spi_nrfx_config spi_##idx##z_config = {	       \
		.spis = NRFX_SPIS_INSTANCE(idx),			       \
		.max_buf_len = BIT_MASK(SPIS##idx##_EASYDMA_MAXCNT_SIZE),      \
		IF_ENABLED(CONFIG_PINCTRL,				       \
			(.pcfg = PINCTRL_DT_DEV_CONFIG_GET(SPIS(idx)),))       \
	};								       \
	DEVICE_DT_DEFINE(SPIS(idx),					       \
			    spi_##idx##_init,				       \
			    NULL,					       \
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
