/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_spim

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/cache.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <soc.h>
#include <dmm.h>
#ifdef CONFIG_SOC_NRF5340_CPUAPP
#include <hal/nrf_clock.h>
#endif
#include <nrfx_spim.h>
#include <string.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_nrfx_spim, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_nrfx_common.h"

#if (CONFIG_SPI_NRFX_RAM_BUFFER_SIZE > 0)
#define SPI_BUFFER_IN_RAM 1
#endif

struct spi_nrfx_data {
	nrfx_spim_t spim;
	struct spi_context ctx;
	const struct device *dev;
	size_t  chunk_len;
	bool    busy;
	bool    initialized;
#ifdef SPI_BUFFER_IN_RAM
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
#endif
};

struct spi_nrfx_config {
	uint32_t	   max_freq;
	nrfx_spim_config_t def_config;
	void (*irq_connect)(void);
	uint16_t max_chunk_len;
	const struct pinctrl_dev_config *pcfg;
	nrfx_gpiote_t *wake_gpiote;
	uint32_t wake_pin;
	void *mem_reg;
};

static void event_handler(const nrfx_spim_event_t *p_event, void *p_context);

static inline void finalize_spi_transaction(const struct device *dev, bool deactivate_cs)
{
	struct spi_nrfx_data *dev_data = dev->data;
	void *reg = dev_data->spim.p_reg;

	if (deactivate_cs) {
		spi_context_cs_control(&dev_data->ctx, false);
	}

	if (NRF_SPIM_IS_320MHZ_SPIM(reg) && !(dev_data->ctx.config->operation & SPI_HOLD_ON_CS)) {
		nrfy_spim_disable(reg);
	}
}

static inline uint32_t get_nrf_spim_frequency(uint32_t frequency)
{
	if (NRF_SPIM_HAS_PRESCALER) {
		return frequency;
	}
	/* Get the highest supported frequency not exceeding the requested one.
	 */
	if (frequency >= MHZ(32) && NRF_SPIM_HAS_32_MHZ_FREQ) {
		return MHZ(32);
	} else if (frequency >= MHZ(16) && NRF_SPIM_HAS_16_MHZ_FREQ) {
		return MHZ(16);
	} else if (frequency >= MHZ(8)) {
		return MHZ(8);
	} else if (frequency >= MHZ(4)) {
		return MHZ(4);
	} else if (frequency >= MHZ(2)) {
		return MHZ(2);
	} else if (frequency >= MHZ(1)) {
		return MHZ(1);
	} else if (frequency >= KHZ(500)) {
		return KHZ(500);
	} else if (frequency >= KHZ(250)) {
		return KHZ(250);
	} else {
		return KHZ(125);
	}
}

static inline nrf_spim_mode_t get_nrf_spim_mode(uint16_t operation)
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

static inline nrf_spim_bit_order_t get_nrf_spim_bit_order(uint16_t operation)
{
	if (operation & SPI_TRANSFER_LSB) {
		return NRF_SPIM_BIT_ORDER_LSB_FIRST;
	} else {
		return NRF_SPIM_BIT_ORDER_MSB_FIRST;
	}
}

static int configure(const struct device *dev,
		     const struct spi_config *spi_cfg)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_context *ctx = &dev_data->ctx;
	uint32_t max_freq = dev_config->max_freq;
	nrfx_spim_config_t config;
	int result;
	uint32_t sck_pin;

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

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
	/* On nRF5340, the 32 Mbps speed is supported by the application core
	 * when it is running at 128 MHz (see the Timing specifications section
	 * in the nRF5340 PS).
	 */
	if (max_freq > 16000000 &&
	    nrf_clock_hfclk_div_get(NRF_CLOCK) != NRF_CLOCK_HFCLK_DIV_1) {
		max_freq = 16000000;
	}
#endif

	config = dev_config->def_config;

	/* Limit the frequency to that supported by the SPIM instance. */
	config.frequency = get_nrf_spim_frequency(MIN(spi_cfg->frequency,
						      max_freq));
	config.mode      = get_nrf_spim_mode(spi_cfg->operation);
	config.bit_order = get_nrf_spim_bit_order(spi_cfg->operation);

	sck_pin = nrfy_spim_sck_pin_get(dev_data->spim.p_reg);

	if (sck_pin != NRF_SPIM_PIN_NOT_CONNECTED) {
		nrfy_gpio_pin_write(sck_pin, spi_cfg->operation & SPI_MODE_CPOL ? 1 : 0);
	}

	if (dev_data->initialized) {
		nrfx_spim_uninit(&dev_data->spim);
		dev_data->initialized = false;
	}

	result = nrfx_spim_init(&dev_data->spim, &config,
				event_handler, (void *)dev);
	if (result < 0) {
		LOG_ERR("Failed to initialize nrfx driver: %d", result);
		return result;
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

	finalize_spi_transaction(dev, true);

#ifdef CONFIG_SPI_ASYNC
	if (ctx->asynchronous) {
		pm_device_runtime_put_async(dev, K_NO_WAIT);
	}
#endif
}

static void transfer_next_chunk(const struct device *dev)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_context *ctx = &dev_data->ctx;
	int error = 0;

	size_t chunk_len = spi_context_max_continuous_chunk(ctx);

	if (chunk_len > 0) {
		nrfx_spim_xfer_desc_t xfer;
		const uint8_t *tx_buf = ctx->tx_buf;
		uint8_t *rx_buf = ctx->rx_buf;

		if (chunk_len > dev_config->max_chunk_len) {
			chunk_len = dev_config->max_chunk_len;
		}

#ifdef SPI_BUFFER_IN_RAM
		if (spi_context_tx_buf_on(ctx) &&
		    !nrf_dma_accessible_check(&dev_data->spim.p_reg, tx_buf)) {

			if (chunk_len > CONFIG_SPI_NRFX_RAM_BUFFER_SIZE) {
				chunk_len = CONFIG_SPI_NRFX_RAM_BUFFER_SIZE;
			}

			memcpy(dev_data->tx_buffer, tx_buf, chunk_len);
			tx_buf = dev_data->tx_buffer;
		}

		if (spi_context_rx_buf_on(ctx) &&
		    !nrf_dma_accessible_check(&dev_data->spim.p_reg, rx_buf)) {

			if (chunk_len > CONFIG_SPI_NRFX_RAM_BUFFER_SIZE) {
				chunk_len = CONFIG_SPI_NRFX_RAM_BUFFER_SIZE;
			}

			rx_buf = dev_data->rx_buffer;
		}
#endif

		dev_data->chunk_len = chunk_len;

		xfer.tx_length = spi_context_tx_buf_on(ctx) ? chunk_len : 0;
		xfer.rx_length = spi_context_rx_buf_on(ctx) ? chunk_len : 0;

		error = dmm_buffer_out_prepare(dev_config->mem_reg, tx_buf, xfer.tx_length,
					       (void **)&xfer.p_tx_buffer);
		if (error != 0) {
			goto out_alloc_failed;
		}

		error = dmm_buffer_in_prepare(dev_config->mem_reg, rx_buf, xfer.rx_length,
					      (void **)&xfer.p_rx_buffer);
		if (error != 0) {
			goto in_alloc_failed;
		}

		error = nrfx_spim_xfer(&dev_data->spim, &xfer, 0);
		if (error == 0) {
			return;
		}

		/* On nrfx_spim_xfer() error */
		dmm_buffer_in_release(dev_config->mem_reg, rx_buf, xfer.rx_length,
				      (void **)&xfer.p_rx_buffer);
in_alloc_failed:
		dmm_buffer_out_release(dev_config->mem_reg, (void **)&xfer.p_tx_buffer);
	}

out_alloc_failed:
	finish_transaction(dev, error);
}

static void event_handler(const nrfx_spim_event_t *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;

	if (p_event->type == NRFX_SPIM_EVENT_DONE) {
		/* Chunk length is set to 0 when a transaction is aborted
		 * due to a timeout.
		 */
		if (dev_data->chunk_len == 0) {
			finish_transaction(dev_data->dev, -ETIMEDOUT);
			return;
		}

		if (spi_context_tx_buf_on(&dev_data->ctx)) {
			dmm_buffer_out_release(dev_config->mem_reg,
					       (void **)p_event->xfer_desc.p_tx_buffer);
		}

		if (spi_context_rx_buf_on(&dev_data->ctx)) {
			dmm_buffer_in_release(dev_config->mem_reg, dev_data->ctx.rx_buf,
				dev_data->chunk_len, p_event->xfer_desc.p_rx_buffer);
		}

#ifdef SPI_BUFFER_IN_RAM
		if (spi_context_rx_buf_on(&dev_data->ctx) &&
		    p_event->xfer_desc.p_rx_buffer != NULL &&
		    p_event->xfer_desc.p_rx_buffer != dev_data->ctx.rx_buf) {
			(void)memcpy(dev_data->ctx.rx_buf,
				     dev_data->rx_buffer,
				     dev_data->chunk_len);
		}
#endif
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
	void *reg = dev_data->spim.p_reg;
	int error;

	pm_device_runtime_get(dev);
	spi_context_lock(&dev_data->ctx, asynchronous, cb, userdata, spi_cfg);

	error = configure(dev, spi_cfg);

	if (error == 0) {
		dev_data->busy = true;

		if (dev_config->wake_pin != WAKE_PIN_NOT_USED) {
			error = spi_nrfx_wake_request(dev_config->wake_gpiote,
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
		if (NRF_SPIM_IS_320MHZ_SPIM(reg)) {
			nrfy_spim_enable(reg);
		}
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
			nrfx_spim_uninit(&dev_data->spim);
			dev_data->initialized = false;

			/* Make sure the transaction is finished (it may be
			 * already finished if it actually did complete before
			 * the nrfx driver was deinitialized).
			 */
			finish_transaction(dev, -ETIMEDOUT);

			/* Clean up the driver state. */
#ifdef CONFIG_MULTITHREADING
			k_sem_reset(&dev_data->ctx.sync);
#else
			dev_data->ctx.ready = 0;
#endif /* CONFIG_MULTITHREADING */
		} else if (error) {
			finalize_spi_transaction(dev, true);
		}
	}

	spi_context_release(&dev_data->ctx, error);
	if (error || !asynchronous) {
		pm_device_runtime_put(dev);
	}
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

#ifdef CONFIG_MULTITHREADING
	if (dev_data->ctx.owner != spi_cfg) {
		return -EALREADY;
	}
#endif

	if (!spi_context_configured(&dev_data->ctx, spi_cfg)) {
		return -EINVAL;
	}

	if (dev_data->busy) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&dev_data->ctx);
	finalize_spi_transaction(dev, false);

	return 0;
}

static DEVICE_API(spi, spi_nrfx_driver_api) = {
	.transceive = spi_nrfx_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nrfx_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_nrfx_release,
};

static int spim_resume(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	(void)dev_data;

	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	/* nrfx_spim_init() will be called at configuration before
	 * the next transfer.
	 */

	if (spi_context_cs_get_all(&dev_data->ctx)) {
		return -EAGAIN;
	}

	return 0;
}

static void spim_suspend(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	int err;

	if (dev_data->initialized) {
		nrfx_spim_uninit(&dev_data->spim);
		dev_data->initialized = false;
	}

	err = spi_context_cs_put_all(&dev_data->ctx);
	(void)err;

	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
}

static int spim_nrfx_pm_action(const struct device *dev, enum pm_device_action action)
{
	if (action == PM_DEVICE_ACTION_RESUME) {
		return spim_resume(dev);
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		spim_suspend(dev);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int spi_nrfx_init(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	int err;

	/* Apply sleep state by default.
	 * If PM is disabled, the default state will be applied in pm_device_driver_init.
	 */
	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);

	if (dev_config->wake_pin != WAKE_PIN_NOT_USED) {
		err = spi_nrfx_wake_init(dev_config->wake_gpiote, dev_config->wake_pin);
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

	return pm_device_driver_init(dev, spim_nrfx_pm_action);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int spi_nrfx_deinit(const struct device *dev)
{
#if defined(CONFIG_PM_DEVICE)
	enum pm_device_state state;

	/*
	 * PM must have suspended the device before driver can
	 * be deinitialized
	 */
	(void)pm_device_state_get(dev, &state);
	return state == PM_DEVICE_STATE_SUSPENDED ||
	       state == PM_DEVICE_STATE_OFF ?
	       0 : -EBUSY;
#else
	/* PM suspend implementation does everything we need */
	spim_suspend(dev);
#endif

	return 0;
}
#endif

#define SPI_NRFX_SPIM_EXTENDED_CONFIG(inst)				\
	IF_ENABLED(NRFX_SPIM_EXTENDED_ENABLED,				\
		(.dcx_pin = NRF_SPIM_PIN_NOT_CONNECTED,			\
		 COND_CODE_1(DT_INST_PROP(inst, rx_delay_supported),	\
			     (.rx_delay = DT_INST_PROP(inst, rx_delay),),	\
			     ())					\
		))

#define SPI_NRFX_SPIM_DEFINE(inst)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(DT_DRV_INST(inst));		       \
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(inst));      \
	IF_ENABLED(SPI_BUFFER_IN_RAM,					       \
		(static uint8_t spim_##inst##_tx_buffer			       \
			[CONFIG_SPI_NRFX_RAM_BUFFER_SIZE]		       \
			DMM_MEMORY_SECTION(DT_DRV_INST(inst));		       \
		 static uint8_t spim_##inst##_rx_buffer			       \
			[CONFIG_SPI_NRFX_RAM_BUFFER_SIZE]		       \
			DMM_MEMORY_SECTION(DT_DRV_INST(inst));))	       \
	static struct spi_nrfx_data spi_##inst##_data = {		       \
		.spim = NRFX_SPIM_INSTANCE(DT_INST_REG_ADDR(inst)),	       \
		IF_ENABLED(CONFIG_MULTITHREADING,			       \
			(SPI_CONTEXT_INIT_LOCK(spi_##inst##_data, ctx),))      \
		IF_ENABLED(CONFIG_MULTITHREADING,			       \
			(SPI_CONTEXT_INIT_SYNC(spi_##inst##_data, ctx),))      \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)	       \
		IF_ENABLED(SPI_BUFFER_IN_RAM,				       \
			(.tx_buffer = spim_##inst##_tx_buffer,		       \
			 .rx_buffer = spim_##inst##_rx_buffer,))	       \
		.dev  = DEVICE_DT_GET(DT_DRV_INST(inst)),		       \
		.busy = false,						       \
	};								       \
	NRF_DT_INST_IRQ_DIRECT_DEFINE(					       \
		inst,							       \
		nrfx_spim_irq_handler,					       \
		&CONCAT(spi_, inst, _data.spim)				       \
	)								       \
	static void irq_connect##inst(void)				       \
	{								       \
		NRF_DT_INST_IRQ_CONNECT(				       \
			inst,						       \
			nrfx_spim_irq_handler,				       \
			&CONCAT(spi_, inst, _data.spim)			       \
		);							       \
	}								       \
	PINCTRL_DT_INST_DEFINE(inst);					       \
	static const struct spi_nrfx_config spi_##inst##z_config = {	       \
		.max_freq = DT_INST_PROP(inst, max_frequency),		       \
		.def_config = {						       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.ss_pin = NRF_SPIM_PIN_NOT_CONNECTED,		       \
			.orc    = DT_INST_PROP(inst, overrun_character),       \
			SPI_NRFX_SPIM_EXTENDED_CONFIG(inst)		       \
		},							       \
		.irq_connect = irq_connect##inst,			       \
		.max_chunk_len = BIT_MASK(				       \
			DT_INST_PROP(inst, easydma_maxcnt_bits)),	       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		       \
		.wake_gpiote = WAKE_GPIOTE_NODE(DT_DRV_INST(inst)),	       \
		.wake_pin = NRF_DT_GPIOS_TO_PSEL_OR(DT_DRV_INST(inst),	       \
						    wake_gpios,		       \
						    WAKE_PIN_NOT_USED),	       \
		.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),		       \
	};								       \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(inst, wake_gpios) ||	       \
		     !(DT_GPIO_FLAGS(DT_DRV_INST(inst), wake_gpios) &	       \
		     GPIO_ACTIVE_LOW),					       \
		     "WAKE line must be configured as active high");	       \
	PM_DEVICE_DT_INST_DEFINE(inst, spim_nrfx_pm_action);		       \
	SPI_DEVICE_DT_INST_DEINIT_DEFINE(inst,				       \
		      spi_nrfx_init,					       \
		      spi_nrfx_deinit,					       \
		      PM_DEVICE_DT_INST_GET(inst),			       \
		      &spi_##inst##_data,				       \
		      &spi_##inst##z_config,				       \
		      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,		       \
		      &spi_nrfx_driver_api)

DT_INST_FOREACH_STATUS_OKAY(SPI_NRFX_SPIM_DEFINE)
