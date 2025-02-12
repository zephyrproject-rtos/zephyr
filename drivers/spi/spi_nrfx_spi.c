/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_spi.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_nrfx_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_nrfx_common.h"

struct spi_nrfx_data {
	struct spi_context ctx;
	const struct device *dev;
	size_t chunk_len;
	bool   busy;
	bool   initialized;
};

struct spi_nrfx_config {
	nrfx_spi_t	  spi;
	nrfx_spi_config_t def_config;
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pcfg;
	uint32_t wake_pin;
	nrfx_gpiote_t wake_gpiote;
};

static void event_handler(const nrfx_spi_evt_t *p_event, void *p_context);

static inline nrf_spi_frequency_t get_nrf_spi_frequency(uint32_t frequency)
{
	/* Get the highest supported frequency not exceeding the requested one.
	 */
	if (frequency < 250000) {
		return NRF_SPI_FREQ_125K;
	} else if (frequency < 500000) {
		return NRF_SPI_FREQ_250K;
	} else if (frequency < 1000000) {
		return NRF_SPI_FREQ_500K;
	} else if (frequency < 2000000) {
		return NRF_SPI_FREQ_1M;
	} else if (frequency < 4000000) {
		return NRF_SPI_FREQ_2M;
	} else if (frequency < 8000000) {
		return NRF_SPI_FREQ_4M;
	} else {
		return NRF_SPI_FREQ_8M;
	}
}

static inline nrf_spi_mode_t get_nrf_spi_mode(uint16_t operation)
{
	if (SPI_MODE_GET(operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPI_MODE_3;
		} else {
			return NRF_SPI_MODE_2;
		}
	} else {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPI_MODE_1;
		} else {
			return NRF_SPI_MODE_0;
		}
	}
}

static inline nrf_spi_bit_order_t get_nrf_spi_bit_order(uint16_t operation)
{
	if (operation & SPI_TRANSFER_LSB) {
		return NRF_SPI_BIT_ORDER_LSB_FIRST;
	} else {
		return NRF_SPI_BIT_ORDER_MSB_FIRST;
	}
}

static int configure(const struct device *dev,
		     const struct spi_config *spi_cfg)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_context *ctx = &dev_data->ctx;
	nrfx_spi_config_t config;
	nrfx_err_t result;

	if (dev_data->initialized && spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported on %s", dev->name);
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

	if (spi_cfg->frequency < 125000) {
		LOG_ERR("Frequencies lower than 125 kHz are not supported");
		return -EINVAL;
	}

	config = dev_config->def_config;

	config.frequency = get_nrf_spi_frequency(spi_cfg->frequency);
	config.mode      = get_nrf_spi_mode(spi_cfg->operation);
	config.bit_order = get_nrf_spi_bit_order(spi_cfg->operation);

	nrf_gpio_pin_write(nrf_spi_sck_pin_get(dev_config->spi.p_reg),
			   spi_cfg->operation & SPI_MODE_CPOL ? 1 : 0);

	if (dev_data->initialized) {
		nrfx_spi_uninit(&dev_config->spi);
		dev_data->initialized = false;
	}

	result = nrfx_spi_init(&dev_config->spi, &config,
			       event_handler, dev_data);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize nrfx driver: %08x", result);
		return -EIO;
	}

	dev_data->initialized = true;

	ctx->config = spi_cfg;

	return 0;
}

static void finish_transaction(const struct device *dev, int error)
{
	struct spi_nrfx_data *dev_data = dev->data;
	struct spi_context *ctx = &dev_data->ctx;

	LOG_DBG("Transaction finished with status %d", error);

	spi_context_complete(ctx, dev, error);
	dev_data->busy = false;
}

static void transfer_next_chunk(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	struct spi_context *ctx = &dev_data->ctx;
	int error = 0;

	size_t chunk_len = spi_context_max_continuous_chunk(ctx);

	if (chunk_len > 0) {
		nrfx_spi_xfer_desc_t xfer;
		nrfx_err_t result;

		dev_data->chunk_len = chunk_len;

		xfer.p_tx_buffer = ctx->tx_buf;
		xfer.tx_length   = spi_context_tx_buf_on(ctx) ? chunk_len : 0;
		xfer.p_rx_buffer = ctx->rx_buf;
		xfer.rx_length   = spi_context_rx_buf_on(ctx) ? chunk_len : 0;
		result = nrfx_spi_xfer(&dev_config->spi, &xfer, 0);
		if (result == NRFX_SUCCESS) {
			return;
		}

		error = -EIO;
	}

	finish_transaction(dev, error);
}

static void event_handler(const nrfx_spi_evt_t *p_event, void *p_context)
{
	struct spi_nrfx_data *dev_data = p_context;

	if (p_event->type == NRFX_SPI_EVENT_DONE) {
		/* Chunk length is set to 0 when a transaction is aborted
		 * due to a timeout.
		 */
		if (dev_data->chunk_len == 0) {
			finish_transaction(dev_data->dev, -ETIMEDOUT);
			return;
		}

		spi_context_update_tx(&dev_data->ctx, 1, dev_data->chunk_len);
		spi_context_update_rx(&dev_data->ctx, 1, dev_data->chunk_len);

		transfer_next_chunk(dev_data->dev);
	}
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	int error;

	spi_context_lock(&dev_data->ctx, asynchronous, cb, userdata, spi_cfg);

	error = configure(dev, spi_cfg);
	if (error == 0) {
		dev_data->busy = true;

		if (dev_config->wake_pin != WAKE_PIN_NOT_USED) {
			error = spi_nrfx_wake_request(&dev_config->wake_gpiote,
						      dev_config->wake_pin);
			if (error == -ETIMEDOUT) {
				LOG_WRN("Waiting for WAKE acknowledgment timed out");
				/* If timeout occurs, try to perform the transfer
				 * anyway, just in case the slave device was unable
				 * to signal that it was already awaken and prepared
				 * for the transfer.
				 */
			}
		}

		spi_context_buffers_setup(&dev_data->ctx, tx_bufs, rx_bufs, 1);
		spi_context_cs_control(&dev_data->ctx, true);

		transfer_next_chunk(dev);

		error = spi_context_wait_for_completion(&dev_data->ctx);
		if (error == -ETIMEDOUT) {
			/* Set the chunk length to 0 so that event_handler()
			 * knows that the transaction timed out and is to be
			 * aborted.
			 */
			dev_data->chunk_len = 0;
			/* Abort the current transfer by deinitializing
			 * the nrfx driver.
			 */
			nrfx_spi_uninit(&dev_config->spi);
			dev_data->initialized = false;

			/* Make sure the transaction is finished (it may be
			 * already finished if it actually did complete before
			 * the nrfx driver was deinitialized).
			 */
			finish_transaction(dev, -ETIMEDOUT);

			/* Clean up the driver state. */
			k_sem_reset(&dev_data->ctx.sync);
		}

		spi_context_cs_control(&dev_data->ctx, false);
	}

	spi_context_release(&dev_data->ctx, error);

	return error;
}

static int spi_nrfx_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_nrfx_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_nrfx_release(const struct device *dev,
			    const struct spi_config *spi_cfg)
{
	struct spi_nrfx_data *dev_data = dev->data;

	if (!spi_context_configured(&dev_data->ctx, spi_cfg)) {
		return -EINVAL;
	}

	if (dev_data->busy) {
		return -EBUSY;
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

#ifdef CONFIG_PM_DEVICE
static int spi_nrfx_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	int ret = 0;
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(dev_config->pcfg,
					  PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
		/* nrfx_spi_init() will be called at configuration before
		 * the next transfer.
		 */
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (dev_data->initialized) {
			nrfx_spi_uninit(&dev_config->spi);
			dev_data->initialized = false;
		}

		ret = pinctrl_apply_state(dev_config->pcfg,
					  PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int spi_nrfx_init(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	int err;

	err = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	if (dev_config->wake_pin != WAKE_PIN_NOT_USED) {
		err = spi_nrfx_wake_init(&dev_config->wake_gpiote, dev_config->wake_pin);
		if (err == -ENODEV) {
			LOG_ERR("Failed to allocate GPIOTE channel for WAKE");
			return err;
		}
		if (err == -EIO) {
			LOG_ERR("Failed to configure WAKE pin");
			return err;
		}
	}

	dev_config->irq_connect();

	err = spi_context_cs_configure_all(&dev_data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&dev_data->ctx);

	return 0;
}

/*
 * Current factors requiring use of DT_NODELABEL:
 *
 * - HAL design (requirement of drv_inst_idx in nrfx_spi_t)
 * - Name-based HAL IRQ handlers, e.g. nrfx_spi_0_irq_handler
 */

#define SPI(idx)			DT_NODELABEL(spi##idx)
#define SPI_PROP(idx, prop)		DT_PROP(SPI(idx), prop)

#define SPI_NRFX_SPI_DEFINE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(SPI(idx));			       \
	static void irq_connect##idx(void)				       \
	{								       \
		IRQ_CONNECT(DT_IRQN(SPI(idx)), DT_IRQ(SPI(idx), priority),     \
			    nrfx_isr, nrfx_spi_##idx##_irq_handler, 0);	       \
	}								       \
	static struct spi_nrfx_data spi_##idx##_data = {		       \
		SPI_CONTEXT_INIT_LOCK(spi_##idx##_data, ctx),		       \
		SPI_CONTEXT_INIT_SYNC(spi_##idx##_data, ctx),		       \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(SPI(idx), ctx)		       \
		.dev  = DEVICE_DT_GET(SPI(idx)),			       \
		.busy = false,						       \
	};								       \
	PINCTRL_DT_DEFINE(SPI(idx));					       \
	static const struct spi_nrfx_config spi_##idx##z_config = {	       \
		.spi = {						       \
			.p_reg = (NRF_SPI_Type *)DT_REG_ADDR(SPI(idx)),	       \
			.drv_inst_idx = NRFX_SPI##idx##_INST_IDX,	       \
		},							       \
		.def_config = {						       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.ss_pin = NRFX_SPI_PIN_NOT_USED,		       \
			.orc    = SPI_PROP(idx, overrun_character),	       \
		},							       \
		.irq_connect = irq_connect##idx,			       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(SPI(idx)),		       \
		.wake_pin = NRF_DT_GPIOS_TO_PSEL_OR(SPI(idx), wake_gpios,      \
						    WAKE_PIN_NOT_USED),	       \
		.wake_gpiote = WAKE_GPIOTE_INSTANCE(SPI(idx)),		       \
	};								       \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(SPI(idx), wake_gpios) ||		       \
		     !(DT_GPIO_FLAGS(SPI(idx), wake_gpios) & GPIO_ACTIVE_LOW), \
		     "WAKE line must be configured as active high");	       \
	PM_DEVICE_DT_DEFINE(SPI(idx), spi_nrfx_pm_action);		       \
	DEVICE_DT_DEFINE(SPI(idx),					       \
		      spi_nrfx_init,					       \
		      PM_DEVICE_DT_GET(SPI(idx)),			       \
		      &spi_##idx##_data,				       \
		      &spi_##idx##z_config,				       \
		      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,		       \
		      &spi_nrfx_driver_api)

#ifdef CONFIG_HAS_HW_NRF_SPI0
SPI_NRFX_SPI_DEFINE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPI1
SPI_NRFX_SPI_DEFINE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPI2
SPI_NRFX_SPI_DEFINE(2);
#endif
