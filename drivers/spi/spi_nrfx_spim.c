/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/cache.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <soc.h>
#ifdef CONFIG_SOC_NRF54H20_GPD
#include <nrf/gpd.h>
#endif
#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
#include <nrfx_ppi.h>
#endif
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

#if defined(CONFIG_SOC_NRF52832) && !defined(CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58)
#error  This driver is not available by default for nRF52832 because of Product Anomaly 58 \
	(SPIM: An additional byte is clocked out when RXD.MAXCNT == 1 and TXD.MAXCNT <= 1). \
	Use CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58=y to override this limitation.
#endif

#if (CONFIG_SPI_NRFX_RAM_BUFFER_SIZE > 0)
#define SPI_BUFFER_IN_RAM 1
#endif

struct spi_nrfx_data {
	struct spi_context ctx;
	const struct device *dev;
	size_t  chunk_len;
	bool    busy;
	bool    initialized;
#ifdef SPI_BUFFER_IN_RAM
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
#endif
#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
	bool    anomaly_58_workaround_active;
	uint8_t ppi_ch;
	uint8_t gpiote_ch;
#endif
};

struct spi_nrfx_config {
	nrfx_spim_t	   spim;
	uint32_t	   max_freq;
	nrfx_spim_config_t def_config;
	void (*irq_connect)(void);
	uint16_t max_chunk_len;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
	bool anomaly_58_workaround;
#endif
	uint32_t wake_pin;
	nrfx_gpiote_t wake_gpiote;
#ifdef CONFIG_DCACHE
	uint32_t mem_attr;
#endif
};

static void event_handler(const nrfx_spim_evt_t *p_event, void *p_context);

static inline void finalize_spi_transaction(const struct device *dev, bool deactivate_cs)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	void *reg = dev_config->spim.p_reg;

	if (deactivate_cs) {
		spi_context_cs_control(&dev_data->ctx, false);
	}

	if (NRF_SPIM_IS_320MHZ_SPIM(reg) && !(dev_data->ctx.config->operation & SPI_HOLD_ON_CS)) {
		nrfy_spim_disable(reg);
	}
}

static inline uint32_t get_nrf_spim_frequency(uint32_t frequency)
{
	/* Get the highest supported frequency not exceeding the requested one.
	 */
	if (frequency >= MHZ(32) && (NRF_SPIM_HAS_32_MHZ_FREQ || NRF_SPIM_HAS_PRESCALER)) {
		return MHZ(32);
	} else if (frequency >= MHZ(16) && (NRF_SPIM_HAS_16_MHZ_FREQ || NRF_SPIM_HAS_PRESCALER)) {
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

	nrfy_gpio_pin_write(nrfy_spim_sck_pin_get(dev_config->spim.p_reg),
			    spi_cfg->operation & SPI_MODE_CPOL ? 1 : 0);

	if (dev_data->initialized) {
		nrfx_spim_uninit(&dev_config->spim);
		dev_data->initialized = false;
	}

	result = nrfx_spim_init(&dev_config->spim, &config,
				event_handler, (void *)dev);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize nrfx driver: %08x", result);
		return -EIO;
	}

	dev_data->initialized = true;

	ctx->config = spi_cfg;

	return 0;
}

#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
static const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(0);

/*
 * Brief Workaround for transmitting 1 byte with SPIM.
 *
 * Derived from the setup_workaround_for_ftpan_58() function from
 * the nRF52832 Rev 1 Errata v1.6 document anomaly 58 workaround.
 *
 * Warning Must not be used when transmitting multiple bytes.
 *
 * Warning After this workaround is used, the user must reset the PPI
 * channel and the GPIOTE channel before attempting to transmit multiple
 * bytes.
 */
static void anomaly_58_workaround_setup(const struct device *dev)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	NRF_SPIM_Type *spim = dev_config->spim.p_reg;
	uint32_t ppi_ch = dev_data->ppi_ch;
	uint32_t gpiote_ch = dev_data->gpiote_ch;
	uint32_t eep = (uint32_t)&gpiote.p_reg->EVENTS_IN[gpiote_ch];
	uint32_t tep = (uint32_t)&spim->TASKS_STOP;

	dev_data->anomaly_58_workaround_active = true;

	/* Create an event when SCK toggles */
	nrf_gpiote_event_configure(gpiote.p_reg, gpiote_ch, spim->PSEL.SCK,
				   GPIOTE_CONFIG_POLARITY_Toggle);
	nrf_gpiote_event_enable(gpiote.p_reg, gpiote_ch);

	/* Stop the spim instance when SCK toggles */
	nrf_ppi_channel_endpoint_setup(NRF_PPI, ppi_ch, eep, tep);
	nrf_ppi_channel_enable(NRF_PPI, ppi_ch);

	/* The spim instance cannot be stopped mid-byte, so it will finish
	 * transmitting the first byte and then stop. Effectively ensuring
	 * that only 1 byte is transmitted.
	 */
}

static void anomaly_58_workaround_clear(struct spi_nrfx_data *dev_data)
{
	uint32_t ppi_ch = dev_data->ppi_ch;
	uint32_t gpiote_ch = dev_data->gpiote_ch;

	if (dev_data->anomaly_58_workaround_active) {
		nrf_ppi_channel_disable(NRF_PPI, ppi_ch);
		nrf_gpiote_task_disable(gpiote.p_reg, gpiote_ch);

		dev_data->anomaly_58_workaround_active = false;
	}
}

static int anomaly_58_workaround_init(const struct device *dev)
{
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;
	nrfx_err_t err_code;

	dev_data->anomaly_58_workaround_active = false;

	if (dev_config->anomaly_58_workaround) {
		err_code = nrfx_ppi_channel_alloc(&dev_data->ppi_ch);
		if (err_code != NRFX_SUCCESS) {
			LOG_ERR("Failed to allocate PPI channel");
			return -ENODEV;
		}

		err_code = nrfx_gpiote_channel_alloc(&gpiote, &dev_data->gpiote_ch);
		if (err_code != NRFX_SUCCESS) {
			LOG_ERR("Failed to allocate GPIOTE channel");
			return -ENODEV;
		}
		LOG_DBG("PAN 58 workaround enabled for %s: ppi %u, gpiote %u",
			dev->name, dev_data->ppi_ch, dev_data->gpiote_ch);
	}

	return 0;
}
#endif

static void finish_transaction(const struct device *dev, int error)
{
	struct spi_nrfx_data *dev_data = dev->data;
	struct spi_context *ctx = &dev_data->ctx;

	LOG_DBG("Transaction finished with status %d", error);

	spi_context_complete(ctx, dev, error);
	dev_data->busy = false;

	finalize_spi_transaction(dev, true);
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
		nrfx_err_t result;
		const uint8_t *tx_buf = ctx->tx_buf;
		uint8_t *rx_buf = ctx->rx_buf;

		if (chunk_len > dev_config->max_chunk_len) {
			chunk_len = dev_config->max_chunk_len;
		}

#ifdef SPI_BUFFER_IN_RAM
		if (spi_context_tx_buf_on(ctx) &&
		    !nrf_dma_accessible_check(&dev_config->spim.p_reg, tx_buf)) {

			if (chunk_len > CONFIG_SPI_NRFX_RAM_BUFFER_SIZE) {
				chunk_len = CONFIG_SPI_NRFX_RAM_BUFFER_SIZE;
			}

			memcpy(dev_data->tx_buffer, tx_buf, chunk_len);
#ifdef CONFIG_DCACHE
			if (dev_config->mem_attr & DT_MEM_CACHEABLE) {
				sys_cache_data_flush_range(dev_data->tx_buffer, chunk_len);
			}
#endif
			tx_buf = dev_data->tx_buffer;
		}

		if (spi_context_rx_buf_on(ctx) &&
		    !nrf_dma_accessible_check(&dev_config->spim.p_reg, rx_buf)) {

			if (chunk_len > CONFIG_SPI_NRFX_RAM_BUFFER_SIZE) {
				chunk_len = CONFIG_SPI_NRFX_RAM_BUFFER_SIZE;
			}

			rx_buf = dev_data->rx_buffer;
		}
#endif

		dev_data->chunk_len = chunk_len;

		xfer.p_tx_buffer = tx_buf;
		xfer.tx_length   = spi_context_tx_buf_on(ctx) ? chunk_len : 0;
		xfer.p_rx_buffer = rx_buf;
		xfer.rx_length   = spi_context_rx_buf_on(ctx) ? chunk_len : 0;

#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
		if (xfer.rx_length == 1 && xfer.tx_length <= 1) {
			if (dev_config->anomaly_58_workaround) {
				anomaly_58_workaround_setup(dev);
			} else {
				LOG_WRN("Transaction aborted since it would trigger "
					"nRF52832 PAN 58");
				error = -EIO;
			}
		}
#endif
		if (error == 0) {
			result = nrfx_spim_xfer(&dev_config->spim, &xfer, 0);
			if (result == NRFX_SUCCESS) {
				return;
			}
			error = -EIO;
#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
			anomaly_58_workaround_clear(dev_data);
#endif
		}
	}

	finish_transaction(dev, error);
}

static void event_handler(const nrfx_spim_evt_t *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct spi_nrfx_data *dev_data = dev->data;
#ifdef CONFIG_DCACHE
	const struct spi_nrfx_config *dev_config = dev->config;
#endif

	if (p_event->type == NRFX_SPIM_EVENT_DONE) {
		/* Chunk length is set to 0 when a transaction is aborted
		 * due to a timeout.
		 */
		if (dev_data->chunk_len == 0) {
			finish_transaction(dev_data->dev, -ETIMEDOUT);
			return;
		}

#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
		anomaly_58_workaround_clear(dev_data);
#endif
#ifdef SPI_BUFFER_IN_RAM
		if (spi_context_rx_buf_on(&dev_data->ctx) &&
		    p_event->xfer_desc.p_rx_buffer != NULL &&
		    p_event->xfer_desc.p_rx_buffer != dev_data->ctx.rx_buf) {
#ifdef CONFIG_DCACHE
			if (dev_config->mem_attr & DT_MEM_CACHEABLE) {
				sys_cache_data_invd_range(dev_data->rx_buffer, dev_data->chunk_len);
			}
#endif
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
	void *reg = dev_config->spim.p_reg;
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
			nrfx_spim_uninit(&dev_config->spim);
			dev_data->initialized = false;

			/* Make sure the transaction is finished (it may be
			 * already finished if it actually did complete before
			 * the nrfx driver was deinitialized).
			 */
			finish_transaction(dev, -ETIMEDOUT);

			/* Clean up the driver state. */
			k_sem_reset(&dev_data->ctx.sync);
#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
			anomaly_58_workaround_clear(dev_data);
#endif
		} else if (error) {
			finalize_spi_transaction(dev, true);
		}
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
	finalize_spi_transaction(dev, false);

	return 0;
}

static const struct spi_driver_api spi_nrfx_driver_api = {
	.transceive = spi_nrfx_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nrfx_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_nrfx_release,
};

#ifdef CONFIG_PM_DEVICE
static int spim_nrfx_pm_action(const struct device *dev,
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
		/* nrfx_spim_init() will be called at configuration before
		 * the next transfer.
		 */

#ifdef CONFIG_SOC_NRF54H20_GPD
		nrf_gpd_retain_pins_set(dev_config->pcfg, false);
#endif

		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (dev_data->initialized) {
			nrfx_spim_uninit(&dev_config->spim);
			dev_data->initialized = false;
		}

#ifdef CONFIG_SOC_NRF54H20_GPD
		nrf_gpd_retain_pins_set(dev_config->pcfg, true);
#endif

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

#ifdef CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58
	return anomaly_58_workaround_init(dev);
#else
	return 0;
#endif
}
/*
 * We use NODELABEL here because the nrfx API requires us to call
 * functions which are named according to SoC peripheral instance
 * being operated on. Since DT_INST() makes no guarantees about that,
 * it won't work.
 */
#define SPIM(idx)			DT_NODELABEL(spi##idx)
#define SPIM_PROP(idx, prop)		DT_PROP(SPIM(idx), prop)
#define SPIM_HAS_PROP(idx, prop)	DT_NODE_HAS_PROP(SPIM(idx), prop)
#define SPIM_MEM_REGION(idx)		DT_PHANDLE(SPIM(idx), memory_regions)

#define SPI_NRFX_SPIM_EXTENDED_CONFIG(idx)				\
	IF_ENABLED(NRFX_SPIM_EXTENDED_ENABLED,				\
		(.dcx_pin = NRF_SPIM_PIN_NOT_CONNECTED,			\
		 COND_CODE_1(SPIM_PROP(idx, rx_delay_supported),	\
			     (.rx_delay = SPIM_PROP(idx, rx_delay),),	\
			     ())					\
		))

#define SPIM_GET_MEM_ATTR(idx)								 \
	COND_CODE_1(SPIM_HAS_PROP(idx, memory_regions),					 \
		(COND_CODE_1(DT_NODE_HAS_PROP(SPIM_MEM_REGION(idx), zephyr_memory_attr), \
			(DT_PROP(SPIM_MEM_REGION(idx), zephyr_memory_attr)),		 \
			(0))),								 \
		(0))

#define SPI_NRFX_SPIM_DEFINE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(SPIM(idx));			       \
	static void irq_connect##idx(void)				       \
	{								       \
		IRQ_CONNECT(DT_IRQN(SPIM(idx)), DT_IRQ(SPIM(idx), priority),   \
			    nrfx_isr, nrfx_spim_##idx##_irq_handler, 0);       \
	}								       \
	IF_ENABLED(SPI_BUFFER_IN_RAM,					       \
		(static uint8_t spim_##idx##_tx_buffer			       \
			[CONFIG_SPI_NRFX_RAM_BUFFER_SIZE]		       \
			SPIM_MEMORY_SECTION(idx);			       \
		 static uint8_t spim_##idx##_rx_buffer			       \
			[CONFIG_SPI_NRFX_RAM_BUFFER_SIZE]		       \
			SPIM_MEMORY_SECTION(idx);))			       \
	static struct spi_nrfx_data spi_##idx##_data = {		       \
		SPI_CONTEXT_INIT_LOCK(spi_##idx##_data, ctx),		       \
		SPI_CONTEXT_INIT_SYNC(spi_##idx##_data, ctx),		       \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(SPIM(idx), ctx)		       \
		IF_ENABLED(SPI_BUFFER_IN_RAM,				       \
			(.tx_buffer = spim_##idx##_tx_buffer,		       \
			 .rx_buffer = spim_##idx##_rx_buffer,))		       \
		.dev  = DEVICE_DT_GET(SPIM(idx)),			       \
		.busy = false,						       \
	};								       \
	PINCTRL_DT_DEFINE(SPIM(idx));					       \
	static const struct spi_nrfx_config spi_##idx##z_config = {	       \
		.spim = {						       \
			.p_reg = (NRF_SPIM_Type *)DT_REG_ADDR(SPIM(idx)),      \
			.drv_inst_idx = NRFX_SPIM##idx##_INST_IDX,	       \
		},							       \
		.max_freq = SPIM_PROP(idx, max_frequency),		       \
		.def_config = {						       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.ss_pin = NRF_SPIM_PIN_NOT_CONNECTED,		       \
			.orc    = SPIM_PROP(idx, overrun_character),	       \
			SPI_NRFX_SPIM_EXTENDED_CONFIG(idx)		       \
		},							       \
		.irq_connect = irq_connect##idx,			       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(SPIM(idx)),		       \
		.max_chunk_len = BIT_MASK(SPIM_PROP(idx, easydma_maxcnt_bits)),\
		COND_CODE_1(CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58,     \
			(.anomaly_58_workaround =			       \
				SPIM_PROP(idx, anomaly_58_workaround),),       \
			())						       \
		.wake_pin = NRF_DT_GPIOS_TO_PSEL_OR(SPIM(idx), wake_gpios,     \
						    WAKE_PIN_NOT_USED),	       \
		.wake_gpiote = WAKE_GPIOTE_INSTANCE(SPIM(idx)),		       \
		IF_ENABLED(CONFIG_DCACHE,				       \
			(.mem_attr = SPIM_GET_MEM_ATTR(idx),))		       \
	};								       \
	BUILD_ASSERT(!SPIM_HAS_PROP(idx, wake_gpios) ||			       \
		     !(DT_GPIO_FLAGS(SPIM(idx), wake_gpios) & GPIO_ACTIVE_LOW),\
		     "WAKE line must be configured as active high");	       \
	PM_DEVICE_DT_DEFINE(SPIM(idx), spim_nrfx_pm_action);		       \
	DEVICE_DT_DEFINE(SPIM(idx),					       \
		      spi_nrfx_init,					       \
		      PM_DEVICE_DT_GET(SPIM(idx)),			       \
		      &spi_##idx##_data,				       \
		      &spi_##idx##z_config,				       \
		      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,		       \
		      &spi_nrfx_driver_api)

#define SPIM_MEMORY_SECTION(idx)					       \
	COND_CODE_1(SPIM_HAS_PROP(idx, memory_regions),			       \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	       \
			SPIM_MEM_REGION(idx)))))),			       \
		())

#ifdef CONFIG_HAS_HW_NRF_SPIM0
SPI_NRFX_SPIM_DEFINE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM1
SPI_NRFX_SPIM_DEFINE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM2
SPI_NRFX_SPIM_DEFINE(2);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM3
SPI_NRFX_SPIM_DEFINE(3);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM4
SPI_NRFX_SPIM_DEFINE(4);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM00
SPI_NRFX_SPIM_DEFINE(00);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM20
SPI_NRFX_SPIM_DEFINE(20);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM21
SPI_NRFX_SPIM_DEFINE(21);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM22
SPI_NRFX_SPIM_DEFINE(22);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM30
SPI_NRFX_SPIM_DEFINE(30);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM120
SPI_NRFX_SPIM_DEFINE(120);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM121
SPI_NRFX_SPIM_DEFINE(121);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM130
SPI_NRFX_SPIM_DEFINE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM131
SPI_NRFX_SPIM_DEFINE(131);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM132
SPI_NRFX_SPIM_DEFINE(132);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM133
SPI_NRFX_SPIM_DEFINE(133);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM134
SPI_NRFX_SPIM_DEFINE(134);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM135
SPI_NRFX_SPIM_DEFINE(135);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM136
SPI_NRFX_SPIM_DEFINE(136);
#endif

#ifdef CONFIG_HAS_HW_NRF_SPIM137
SPI_NRFX_SPIM_DEFINE(137);
#endif
