/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/spi.h>
#include <nrfx_spim.h>
#include <string.h>

#define LOG_DOMAIN "spi_nrfx_spim"
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_nrfx_spim, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
struct spi_nrfx_data {
	struct spi_common_data common_data;
	spi_transfer_callback_t callback;
	void *user_data;
	u8_t *duplex_rxbuf;
	size_t duplex_rxlen;
	u32_t msg_flags;
	size_t chunk_len;
	bool   busy;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t pm_state;
#endif
#if (CONFIG_SPI_NRFX_RAM_BUFFER_SIZE > 0)
	u8_t   buffer[CONFIG_SPI_NRFX_RAM_BUFFER_SIZE];
#endif
};

struct spi_nrfx_config {
	nrfx_spim_t	   spim;
	size_t		   max_chunk_len;
	nrfx_spim_config_t config;
};

static inline struct spi_nrfx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct spi_nrfx_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static inline nrf_spim_frequency_t get_nrf_spim_frequency(u32_t frequency)
{
	/* Get the highest supported frequency not exceeding the requested one.
	 */
	if (frequency < 250000) {
		return NRF_SPIM_FREQ_125K;
	} else if (frequency < 500000) {
		return NRF_SPIM_FREQ_250K;
	} else if (frequency < 1000000) {
		return NRF_SPIM_FREQ_500K;
	} else if (frequency < 2000000) {
		return NRF_SPIM_FREQ_1M;
	} else if (frequency < 4000000) {
		return NRF_SPIM_FREQ_2M;
	} else if (frequency < 8000000) {
		return NRF_SPIM_FREQ_4M;
#if defined(CONFIG_SOC_NRF52833) || defined(CONFIG_SOC_NRF52840)
	} else if (frequency < 16000000) {
		return NRF_SPIM_FREQ_8M;
	} else if (frequency < 32000000) {
		return NRF_SPIM_FREQ_16M;
	} else {
		return NRF_SPIM_FREQ_32M;
#else
	} else {
		return NRF_SPIM_FREQ_8M;
#endif
	}
}

static inline nrf_spim_mode_t get_nrf_spim_mode(u16_t operation)
{
	if (SPI_MODE_GET(operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPIM_MODE_3;
		} else {
			return NRF_SPIM_MODE_2;
		}
	} else {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPIM_MODE_1;
		} else {
			return NRF_SPIM_MODE_0;
		}
	}
}

static inline nrf_spim_bit_order_t get_nrf_spim_bit_order(u16_t operation)
{
	if (operation & SPI_TRANSFER_LSB) {
		return NRF_SPIM_BIT_ORDER_LSB_FIRST;
	} else {
		return NRF_SPIM_BIT_ORDER_MSB_FIRST;
	}
}

static int configure(struct device *dev,
		     const struct spi_config *spi_cfg)
{
	struct spi_context *ctx = &get_dev_data(dev)->common_data.ctx;
	const nrfx_spim_t *spim = &get_dev_config(dev)->spim;

	if (spi_context_configured(ctx, spi_cfg)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported on %s",
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

	if (spi_cfg->frequency < 125000) {
		LOG_ERR("Frequencies lower than 125 kHz are not supported");
		return -EINVAL;
	}

	ctx->config = spi_cfg;
	spi_context_cs_configure(ctx);

	nrf_spim_configure(spim->p_reg,
			   get_nrf_spim_mode(spi_cfg->operation),
			   get_nrf_spim_bit_order(spi_cfg->operation));
	nrf_spim_frequency_set(spim->p_reg,
			       get_nrf_spim_frequency(spi_cfg->frequency));

	return 0;
}

static int single_transfer(struct device *dev, const struct spi_msg *msg,
			   spi_transfer_callback_t callback, void *user_data)
{
	u8_t *txbuf;
	u8_t *rxbuf;
	size_t rxlen;
	size_t txlen;
	int err;
	struct spi_nrfx_data *dev_data = get_dev_data(dev);

	if (!atomic_cas((atomic_t *)&dev_data->callback,
			(atomic_val_t)NULL, (atomic_val_t)callback)) {
		return -EBUSY;
	}

	if ((msg->flags & SPI_MSG_DIR_MASK) == SPI_MSG_READ) {
		if (dev_data->duplex_rxbuf) {
			return -EBUSY;
		}
		if ((msg->flags & SPI_MSG_COMMIT) == SPI_MSG_COMMIT) {
			rxbuf = msg->buf;
			rxlen = msg->len;
			txbuf = NULL;
			txlen = 0;
		} else {
			dev_data->duplex_rxbuf = msg->buf;
			dev_data->duplex_rxlen = msg->len;
			dev_data->callback = NULL;
			callback(dev, 0, user_data);
			return 0;
		}
	} else if ((msg->flags & SPI_MSG_DIR_MASK) == SPI_MSG_WRITE) {
		if ((msg->flags & SPI_MSG_COMMIT) != SPI_MSG_COMMIT) {
			/* TX must always be the last one */
			return -EINVAL;
		}
#if (CONFIG_SPI_NRFX_RAM_BUFFER_SIZE > 0)
		if (!nrfx_is_in_ram(msg->buf)) {
			if (msg->len > sizeof(dev_data->buffer)) {
				err = -ENOMEM;
				goto on_error;
			}
			memcpy(dev_data->buffer, msg->buf, msg->len);
			txbuf = dev_data->buffer;
		} else
#endif
		{
			txbuf = msg->buf;
		}
		txlen = msg->len;
		rxbuf = dev_data->duplex_rxbuf;
		rxlen = dev_data->duplex_rxlen;
	}
	dev_data->user_data = user_data;
	dev_data->msg_flags = msg->flags;

	if (IS_ENABLED(CONFIG_SOC_NRF52832) && (rxlen == 1 && txlen <= 1)) {
		LOG_WRN("Transaction aborted since it would trigger nRF52832 PAN 58");
		err= -EIO;
		goto on_error;
	}

	nrfx_spim_xfer_desc_t xfer = {
		.p_tx_buffer = txbuf,
		.tx_length = txlen,
		.p_rx_buffer = rxbuf,
		.rx_length = rxlen
	};
	nrfx_err_t nrfx_err = nrfx_spim_xfer(&get_dev_config(dev)->spim,
						&xfer, 0);
	LOG_INF("spi xfer len(tx:%d, rx:%d) err:%d",
		xfer.tx_length, xfer.rx_length, nrfx_err);
	if (nrfx_err == NRFX_SUCCESS) {
		return 0;
	}
	err = -EIO;
on_error:
	get_dev_data(dev)->callback = NULL;
	return err;
}

static void complete_sequence(struct device *dev, int res)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);
	struct spi_context *ctx = &dev_data->common_data.ctx;

	spi_context_cs_control(ctx, false);

	LOG_DBG("Transaction finished with status %d", res);

	spi_context_complete(ctx, res);
	dev_data->busy = false;
}

static void transfer_next_chunk(struct device *dev);

static void completed_callback(struct device *dev, int res, void *user_data)
{
	struct spi_context *ctx = &get_dev_data(dev)->common_data.ctx;

	if (res < 0) {
		complete_sequence(dev, res);
		return;
	}

	spi_context_update_tx(ctx, 1, ctx->current_tx->len);
	spi_context_update_rx(ctx, 1, ctx->current_rx->len);

	transfer_next_chunk(dev);
}

static void partial_callback(struct device *dev, int res, void *user_data)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);
	struct spi_context *ctx = &dev_data->common_data.ctx;
	struct spi_msg msg;
	int err;

	if (res < 0) {
		completed_callback(dev, res, user_data);
		return;
	}

	msg.buf = ctx->current_tx->buf;
	msg.len = ctx->current_tx->len;
	msg.flags = SPI_MSG_WRITE | SPI_MSG_COMMIT;

	err = single_transfer(dev, &msg, completed_callback, user_data);
	LOG_DBG("Partial (RX) transfer, scheduled TX part (err: %d) ", err);
	if (err < 0) {
		completed_callback(dev, err, user_data);
	}
}

static void transfer_next_chunk(struct device *dev)
{
	struct spi_context *ctx = &get_dev_data(dev)->common_data.ctx;
	int err = 0;
	struct spi_msg msg;
	spi_transfer_callback_t callback;
	size_t chunk_len = spi_context_longest_current_buf(ctx);

	if (chunk_len) {
		if (ctx->current_rx) {
			msg.buf = ctx->current_rx->buf;
			msg.len = ctx->current_rx->len;
			msg.flags = SPI_MSG_READ |
				(ctx->current_tx->len ? 0 : SPI_MSG_COMMIT);
			callback = ctx->current_tx ?
				partial_callback : completed_callback;
		} else {
			/* tx only */
			msg.buf = ctx->current_tx->buf;
			msg.len = ctx->current_tx->len;
			msg.flags = SPI_MSG_WRITE | SPI_MSG_COMMIT;
			callback = completed_callback;
		}

		err = single_transfer(dev, &msg, callback, NULL);
		if (err >= 0) {
			return;
		}
	}

	complete_sequence(dev, err);
}

static int transceive(struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);
	struct spi_context *ctx = &get_dev_data(dev)->common_data.ctx;
	int error;

	error = configure(dev, spi_cfg);
	if (error == 0) {
		dev_data->busy = true;

		spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
		spi_context_cs_control(ctx, true);

		transfer_next_chunk(dev);

		error = spi_context_wait_for_completion(ctx);
	} else {
		LOG_ERR("Failed to configure (%d)", error);
	}

	spi_context_release(ctx, error);

	return error;
}

static int spi_nrfx_transceive(struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	spi_context_lock(&get_dev_data(dev)->common_data.ctx, false, NULL);
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_nrfx_transceive_async(struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	spi_context_lock(&get_dev_data(dev)->common_data->ctx, true, async);
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_nrfx_release(struct device *dev,
			    const struct spi_config *spi_cfg)
{
	struct spi_nrfx_data *dev_data = get_dev_data(dev);
	struct spi_context *ctx = &dev_data->common_data.ctx;

	if (!spi_context_configured(ctx, spi_cfg)) {
		return -EINVAL;
	}

	if (dev_data->busy) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static const struct spi_driver_api spi_nrfx_driver_api = {
	.transceive = spi_nrfx_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nrfx_transceive_async,
#endif
	.release = spi_nrfx_release,
	.single_transfer = single_transfer,
	.configure = configure
};


static void event_handler(const nrfx_spim_evt_t *p_event, void *p_context)
{
	struct device *dev = p_context;
	struct spi_nrfx_data *dev_data = get_dev_data(dev);

	if (p_event->type == NRFX_SPIM_EVENT_DONE) {
		spi_transfer_callback_t callback = dev_data->callback;
		void *user_data = dev_data->user_data;

		dev_data->callback = NULL;
		dev_data->user_data = NULL;
		dev_data->duplex_rxlen = 0;
		dev_data->duplex_rxbuf = NULL;
		callback(dev, 0, user_data);
	}
}

static int init_spim(struct device *dev)
{
	/* This sets only default values of frequency, mode and bit order.
	 * The proper ones are set in configure() when a transfer is started.
	 */
	nrfx_err_t result = nrfx_spim_init(&get_dev_config(dev)->spim,
					   &get_dev_config(dev)->config,
					   event_handler,
					   dev);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	get_dev_data(dev)->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif
	spi_context_unlock_unconditionally(&get_dev_data(dev)->common_data.ctx);

	return z_spi_mngr_init(dev);
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int spim_nrfx_pm_control(struct device *dev, u32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		u32_t new_state = *((const u32_t *)context);

		if (new_state != get_dev_data(dev)->pm_state) {
			switch (new_state) {
			case DEVICE_PM_ACTIVE_STATE:
				init_spim(dev);
				/* Force reconfiguration before next transfer */
				get_dev_data(dev)->common_data->ctx.config = NULL;
				break;

			case DEVICE_PM_LOW_POWER_STATE:
			case DEVICE_PM_SUSPEND_STATE:
			case DEVICE_PM_OFF_STATE:
				nrfx_spim_uninit(&get_dev_config(dev)->spim);
				break;

			default:
				ret = -ENOTSUP;
			}
			if (!ret) {
				get_dev_data(dev)->pm_state = new_state;
			}
		}
	} else {
		assert(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((u32_t *)context) = get_dev_data(dev)->pm_state;
	}

	if (cb) {
		cb(dev, ret, context, arg);
	}

	return ret;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#define SPIM_NRFX_MISO_PULL_DOWN(idx) \
	IS_ENABLED(DT_NORDIC_NRF_SPIM_SPI_##idx##_MISO_PULL_DOWN)

#define SPIM_NRFX_MISO_PULL_UP(idx) \
	IS_ENABLED(DT_NORDIC_NRF_SPIM_SPI_##idx##_MISO_PULL_UP)

#define SPIM_NRFX_MISO_PULL(idx)			\
	(SPIM_NRFX_MISO_PULL_UP(idx)			\
		? SPIM_NRFX_MISO_PULL_DOWN(idx)		\
			? -1 /* invalid configuration */\
			: NRF_GPIO_PIN_PULLUP		\
		: SPIM_NRFX_MISO_PULL_DOWN(idx)		\
			? NRF_GPIO_PIN_PULLDOWN		\
			: NRF_GPIO_PIN_NOPULL)

#define SPI_NRFX_SPIM_EXTENDED_CONFIG(idx)				\
	IF_ENABLED(NRFX_SPIM_EXTENDED_ENABLED,				\
		(.dcx_pin = NRFX_SPIM_PIN_NOT_USED,			\
		 IF_ENABLED(SPIM##idx##_FEATURE_RXDELAY_PRESENT,	\
			(.rx_delay = CONFIG_SPI_##idx##_NRF_RX_DELAY,))	\
		))

#define SPI_NRFX_SPIM_DEVICE(idx)					       \
	BUILD_ASSERT_MSG(						       \
		!SPIM_NRFX_MISO_PULL_UP(idx) || !SPIM_NRFX_MISO_PULL_DOWN(idx),\
		"SPIM"#idx						       \
		": cannot enable both pull-up and pull-down on MISO line");    \
	static int spi_##idx##_init(struct device *dev)			       \
	{								       \
		IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SPIM##idx),		       \
			    DT_NORDIC_NRF_SPIM_SPI_##idx##_IRQ_0_PRIORITY,     \
			    nrfx_isr, nrfx_spim_##idx##_irq_handler, 0);       \
		return init_spim(dev);					       \
	}								       \
	static struct spi_nrfx_data spi_##idx##_data = {		       \
		.common_data = {					       \
			SPI_CONTEXT_INIT_LOCK(spi_##idx##_data.common_data, ctx),	       \
			SPI_CONTEXT_INIT_SYNC(spi_##idx##_data.common_data, ctx),	       \
		},							       \
		.busy = false,						       \
	};								       \
	static const struct spi_nrfx_config spi_##idx##z_config = {	       \
		.spim = NRFX_SPIM_INSTANCE(idx),			       \
		.max_chunk_len = (1 << SPIM##idx##_EASYDMA_MAXCNT_SIZE) - 1,   \
		.config = {						       \
			.sck_pin   = DT_NORDIC_NRF_SPIM_SPI_##idx##_SCK_PIN,   \
			.mosi_pin  = DT_NORDIC_NRF_SPIM_SPI_##idx##_MOSI_PIN,  \
			.miso_pin  = DT_NORDIC_NRF_SPIM_SPI_##idx##_MISO_PIN,  \
			.ss_pin    = NRFX_SPIM_PIN_NOT_USED,		       \
			.orc       = CONFIG_SPI_##idx##_NRF_ORC,	       \
			.frequency = NRF_SPIM_FREQ_4M,			       \
			.mode      = NRF_SPIM_MODE_0,			       \
			.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST,	       \
			.miso_pull = SPIM_NRFX_MISO_PULL(idx),		       \
			SPI_NRFX_SPIM_EXTENDED_CONFIG(idx)		       \
		}							       \
	};								       \
	DEVICE_DEFINE(spi_##idx,					       \
		      DT_NORDIC_NRF_SPIM_SPI_##idx##_LABEL,		       \
		      spi_##idx##_init,					       \
		      spim_nrfx_pm_control,				       \
		      &spi_##idx##_data,				       \
		      &spi_##idx##z_config,				       \
		      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,		       \
		      &spi_nrfx_driver_api)

#ifdef CONFIG_SPI_0_NRF_SPIM
SPI_NRFX_SPIM_DEVICE(0);
#endif

#ifdef CONFIG_SPI_1_NRF_SPIM
SPI_NRFX_SPIM_DEVICE(1);
#endif

#ifdef CONFIG_SPI_2_NRF_SPIM
SPI_NRFX_SPIM_DEVICE(2);
#endif

#ifdef CONFIG_SPI_3_NRF_SPIM
SPI_NRFX_SPIM_DEVICE(3);
#endif
