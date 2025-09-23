/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <dmm.h>
#include <soc.h>
#include <nrfx_spis.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_nrfx_spis, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#if NRF_DT_INST_ANY_IS_FAST
/* If fast instances are used then system managed device PM cannot be used because
 * it may call PM actions from locked context and fast SPIM PM actions can only be
 * called from a thread context.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED));
#endif

/*
 * Current factors requiring use of DT_NODELABEL:
 *
 * - HAL design (requirement of drv_inst_idx in nrfx_spis_t)
 * - Name-based HAL IRQ handlers, e.g. nrfx_spis_0_irq_handler
 */
#define SPIS_NODE(idx) \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(spis##idx)), (spis##idx), (spi##idx))
#define SPIS(idx) DT_NODELABEL(SPIS_NODE(idx))
#define SPIS_PROP(idx, prop) DT_PROP(SPIS(idx), prop)
#define SPIS_HAS_PROP(idx, prop) DT_NODE_HAS_PROP(SPIS(idx), prop)
#define SPIS_IS_FAST(idx) NRF_DT_IS_FAST(SPIS(idx))

#define SPIS_PINS_CROSS_DOMAIN(unused, prefix, idx, _)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(SPIS(prefix##idx)),		\
		   (SPIS_PROP(idx, cross_domain_pins_supported)),	\
		   (0))

#if NRFX_FOREACH_PRESENT(SPIS, SPIS_PINS_CROSS_DOMAIN, (||), (0))
#include <hal/nrf_gpio.h>
/* Certain SPIM instances support usage of cross domain pins in form of dedicated pins on
 * a port different from the default one.
 */
#define SPIS_CROSS_DOMAIN_SUPPORTED 1
#endif

#if SPIS_CROSS_DOMAIN_SUPPORTED && defined(CONFIG_NRF_SYS_EVENT)
#include <nrf_sys_event.h>
/* To use cross domain pins, constant latency mode needs to be applied, which is
 * handled via nrf_sys_event requests.
 */
#define SPIS_CROSS_DOMAIN_PINS_HANDLE 1
#endif

struct spi_nrfx_data {
	struct spi_context ctx;
	const struct device *dev;
#ifdef CONFIG_MULTITHREADING
	struct k_sem wake_sem;
#else
	atomic_t woken_up;
#endif
	struct gpio_callback wake_cb_data;
};

struct spi_nrfx_config {
	nrfx_spis_t spis;
	nrfx_spis_config_t config;
	void (*irq_connect)(void);
	uint16_t max_buf_len;
	const struct pinctrl_dev_config *pcfg;
	struct gpio_dt_spec wake_gpio;
	void *mem_reg;
#if SPIS_CROSS_DOMAIN_SUPPORTED
	bool cross_domain;
	int8_t default_port;
#endif
};

#if SPIS_CROSS_DOMAIN_SUPPORTED
static bool spis_has_cross_domain_connection(const struct spi_nrfx_config *config)
{
	const struct pinctrl_dev_config *pcfg = config->pcfg;
	const struct pinctrl_state *state;
	int ret;

	ret = pinctrl_lookup_state(pcfg, PINCTRL_STATE_DEFAULT, &state);
	if (ret < 0) {
		LOG_ERR("Unable to read pin state");
		return false;
	}

	for (uint8_t i = 0U; i < state->pin_cnt; i++) {
		uint32_t pin = NRF_GET_PIN(state->pins[i]);

		if ((pin != NRF_PIN_DISCONNECTED) &&
		    (nrf_gpio_pin_port_number_extract(&pin) != config->default_port)) {
			return true;
		}
	}

	return false;
}
#endif

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

	if (spi_cs_is_gpio(spi_cfg)) {
		LOG_ERR("CS control via GPIO is not supported");
		return -EINVAL;
	}

	ctx->config = spi_cfg;

	nrf_spis_configure(dev_config->spis.p_reg,
			   get_nrf_spis_mode(spi_cfg->operation),
			   get_nrf_spis_bit_order(spi_cfg->operation));

	return 0;
}

static int prepare_for_transfer(const struct device *dev,
				const uint8_t *tx_buf, size_t tx_buf_len,
				uint8_t *rx_buf, size_t rx_buf_len)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	nrfx_err_t result;
	uint8_t *dmm_tx_buf;
	uint8_t *dmm_rx_buf;
	int err;

	if (tx_buf_len > dev_config->max_buf_len ||
	    rx_buf_len > dev_config->max_buf_len) {
		LOG_ERR("Invalid buffer sizes: Tx %d/Rx %d",
			tx_buf_len, rx_buf_len);
		return -EINVAL;
	}

	err = dmm_buffer_out_prepare(dev_config->mem_reg, tx_buf, tx_buf_len, (void **)&dmm_tx_buf);
	if (err != 0) {
		LOG_ERR("DMM TX allocation failed err=%d", err);
		goto out_alloc_failed;
	}

	/* Keep user RX buffer address to copy data from DMM RX buffer on transfer completion. */
	dev_data->ctx.rx_buf = rx_buf;
	err = dmm_buffer_in_prepare(dev_config->mem_reg, rx_buf, rx_buf_len, (void **)&dmm_rx_buf);
	if (err != 0) {
		LOG_ERR("DMM RX allocation failed err=%d", err);
		goto in_alloc_failed;
	}

	result = nrfx_spis_buffers_set(&dev_config->spis,
				       dmm_tx_buf, tx_buf_len,
				       dmm_rx_buf, rx_buf_len);
	if (result != NRFX_SUCCESS) {
		err = -EIO;
		goto buffers_set_failed;
	}

	return 0;

buffers_set_failed:
	dmm_buffer_in_release(dev_config->mem_reg, rx_buf, rx_buf_len, dmm_rx_buf);
in_alloc_failed:
	dmm_buffer_out_release(dev_config->mem_reg, (void *)dmm_tx_buf);
out_alloc_failed:
	return err;
}

static void wake_callback(const struct device *dev, struct gpio_callback *cb,
			  uint32_t pins)
{
	struct spi_nrfx_data *dev_data =
		CONTAINER_OF(cb, struct spi_nrfx_data, wake_cb_data);
	const struct spi_nrfx_config *dev_config = dev_data->dev->config;

	(void)gpio_pin_interrupt_configure_dt(&dev_config->wake_gpio,
					      GPIO_INT_DISABLE);
#ifdef CONFIG_MULTITHREADING
	k_sem_give(&dev_data->wake_sem);
#else
	atomic_set(&dev_data->woken_up, 1);
#endif /* CONFIG_MULTITHREADING */
}

static void wait_for_wake(struct spi_nrfx_data *dev_data,
			  const struct spi_nrfx_config *dev_config)
{
	/* If the WAKE line is low, wait until it goes high - this is a signal
	 * from the master that it wants to perform a transfer.
	 */
	if (gpio_pin_get_raw(dev_config->wake_gpio.port,
			     dev_config->wake_gpio.pin) == 0) {
		(void)gpio_pin_interrupt_configure_dt(&dev_config->wake_gpio,
						      GPIO_INT_LEVEL_HIGH);
#ifdef CONFIG_MULTITHREADING
		(void)k_sem_take(&dev_data->wake_sem, K_FOREVER);
#else
		unsigned int key = irq_lock();

		while (!atomic_get(&dev_data->woken_up)) {
			k_cpu_atomic_idle(key);
			key = irq_lock();
		}

		dev_data->woken_up = 0;
		irq_unlock(key);
#endif /* CONFIG_MULTITHREADING */
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
	const struct spi_buf *tx_buf = tx_bufs ? tx_bufs->buffers : NULL;
	const struct spi_buf *rx_buf = rx_bufs ? rx_bufs->buffers : NULL;
	int error;

	pm_device_runtime_get(dev);

	spi_context_lock(&dev_data->ctx, asynchronous, cb, userdata, spi_cfg);

	error = configure(dev, spi_cfg);
	if (error != 0) {
		/* Invalid configuration. */
	} else if ((tx_bufs && tx_bufs->count > 1) ||
		   (rx_bufs && rx_bufs->count > 1)) {
		LOG_ERR("Scattered buffers are not supported");
		error = -ENOTSUP;
	} else if (tx_buf && tx_buf->len && !nrfx_is_in_ram(tx_buf->buf)) {
		LOG_ERR("Only buffers located in RAM are supported");
		error = -ENOTSUP;
	} else {
		if (dev_config->wake_gpio.port) {
			wait_for_wake(dev_data, dev_config);

			nrf_spis_enable(dev_config->spis.p_reg);
		}

		error = prepare_for_transfer(dev,
					     tx_buf ? tx_buf->buf : NULL,
					     tx_buf ? tx_buf->len : 0,
					     rx_buf ? rx_buf->buf : NULL,
					     rx_buf ? rx_buf->len : 0);
		if (error == 0) {
			if (dev_config->wake_gpio.port) {
				/* Set the WAKE line low (tie it to ground)
				 * to signal readiness to handle the transfer.
				 */
				gpio_pin_set_raw(dev_config->wake_gpio.port,
						 dev_config->wake_gpio.pin,
						 0);
				/* Set the WAKE line back high (i.e. disconnect
				 * output for its pin since it's configured in
				 * open drain mode) so that it can be controlled
				 * by the other side again.
				 */
				gpio_pin_set_raw(dev_config->wake_gpio.port,
						 dev_config->wake_gpio.pin,
						 1);
			}

			error = spi_context_wait_for_completion(&dev_data->ctx);
		}

		if (dev_config->wake_gpio.port) {
			nrf_spis_disable(dev_config->spis.p_reg);
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

	spi_context_unlock_unconditionally(&dev_data->ctx);

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

static void event_handler(const nrfx_spis_evt_t *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct spi_nrfx_data *dev_data = dev->data;
	const struct spi_nrfx_config *dev_config = dev->config;

	if (p_event->evt_type == NRFX_SPIS_XFER_DONE) {
		int err;


		err = dmm_buffer_out_release(dev_config->mem_reg, p_event->p_tx_buf);
		(void)err;
		__ASSERT_NO_MSG(err == 0);

		err = dmm_buffer_in_release(dev_config->mem_reg, dev_data->ctx.rx_buf,
				      p_event->rx_amount, p_event->p_rx_buf);
		__ASSERT_NO_MSG(err == 0);

		spi_context_complete(&dev_data->ctx, dev_data->dev,
				     p_event->rx_amount);

		pm_device_runtime_put_async(dev_data->dev, K_NO_WAIT);
	}
}

static void spi_nrfx_suspend(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;

	if (dev_config->wake_gpio.port == NULL) {
		nrf_spis_disable(dev_config->spis.p_reg);
	}

#if SPIS_CROSS_DOMAIN_SUPPORTED
	if (dev_config->cross_domain && spis_has_cross_domain_connection(dev_config)) {
#if SPIS_CROSS_DOMAIN_PINS_HANDLE
		int err;

		err = nrf_sys_event_release_global_constlat();
		(void)err;
		__ASSERT_NO_MSG(err >= 0);
#else
		__ASSERT(false, "NRF_SYS_EVENT needs to be enabled to use cross domain pins.\n");
#endif
	}
#endif

	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
}

static void spi_nrfx_resume(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;

	(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);

#if SPIS_CROSS_DOMAIN_SUPPORTED
	if (dev_config->cross_domain && spis_has_cross_domain_connection(dev_config)) {
#if SPIS_CROSS_DOMAIN_PINS_HANDLE
		int err;

		err = nrf_sys_event_request_global_constlat();
		(void)err;
		__ASSERT_NO_MSG(err >= 0);
#else
		__ASSERT(false, "NRF_SYS_EVENT needs to be enabled to use cross domain pins.\n");
#endif
	}
#endif

	if (dev_config->wake_gpio.port == NULL) {
		nrf_spis_enable(dev_config->spis.p_reg);
	}
}

static int spi_nrfx_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		spi_nrfx_suspend(dev);
		return 0;

	case PM_DEVICE_ACTION_RESUME:
		spi_nrfx_resume(dev);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static int spi_nrfx_init(const struct device *dev)
{
	const struct spi_nrfx_config *dev_config = dev->config;
	struct spi_nrfx_data *dev_data = dev->data;
	nrfx_err_t result;
	int err;

	/* This sets only default values of mode and bit order. The ones to be
	 * actually used are set in configure() when a transfer is prepared.
	 */
	result = nrfx_spis_init(&dev_config->spis, &dev_config->config,
				event_handler, (void *)dev);

	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return -EBUSY;
	}

	/* When the WAKE line is used, the SPIS peripheral is enabled
	 * only after the master signals that it wants to perform a
	 * transfer and it is disabled right after the transfer is done.
	 * Waiting for the WAKE line to go high, what can be done using
	 * the GPIO PORT event, instead of just waiting for the transfer
	 * with the SPIS peripheral enabled, significantly reduces idle
	 * power consumption.
	 */
	nrf_spis_disable(dev_config->spis.p_reg);

	if (dev_config->wake_gpio.port) {
		if (!gpio_is_ready_dt(&dev_config->wake_gpio)) {
			return -ENODEV;
		}

		/* In open drain mode, the output is disconnected when set to
		 * the high state, so the following will effectively configure
		 * the pin as an input only.
		 */
		err = gpio_pin_configure_dt(&dev_config->wake_gpio,
					    GPIO_INPUT |
					    GPIO_OUTPUT_HIGH |
					    GPIO_OPEN_DRAIN);
		if (err < 0) {
			return err;
		}

		gpio_init_callback(&dev_data->wake_cb_data, wake_callback,
				   BIT(dev_config->wake_gpio.pin));
		err = gpio_add_callback(dev_config->wake_gpio.port,
					&dev_data->wake_cb_data);
		if (err < 0) {
			return err;
		}
	}

	spi_context_unlock_unconditionally(&dev_data->ctx);

	return pm_device_driver_init(dev, spi_nrfx_pm_action);
}

#define SPI_NRFX_SPIS_DEFINE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(SPIS(idx));	       \
	static void irq_connect##idx(void)				       \
	{								       \
		IRQ_CONNECT(DT_IRQN(SPIS(idx)), DT_IRQ(SPIS(idx), priority),   \
			    nrfx_isr, nrfx_spis_##idx##_irq_handler, 0);       \
	}								       \
	static struct spi_nrfx_data spi_##idx##_data = {		       \
		IF_ENABLED(CONFIG_MULTITHREADING,			       \
			(SPI_CONTEXT_INIT_LOCK(spi_##idx##_data, ctx),))       \
		IF_ENABLED(CONFIG_MULTITHREADING,			       \
			(SPI_CONTEXT_INIT_SYNC(spi_##idx##_data, ctx),))       \
		.dev  = DEVICE_DT_GET(SPIS(idx)),			       \
		IF_ENABLED(CONFIG_MULTITHREADING,			       \
			(.wake_sem = Z_SEM_INITIALIZER(			       \
				spi_##idx##_data.wake_sem, 0, 1),))	       \
	};								       \
	PINCTRL_DT_DEFINE(SPIS(idx));					       \
	static const struct spi_nrfx_config spi_##idx##z_config = {	       \
		.spis = {						       \
			.p_reg = (NRF_SPIS_Type *)DT_REG_ADDR(SPIS(idx)),      \
			.drv_inst_idx = NRFX_SPIS##idx##_INST_IDX,	       \
		},							       \
		.config = {						       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.mode      = NRF_SPIS_MODE_0,			       \
			.bit_order = NRF_SPIS_BIT_ORDER_MSB_FIRST,	       \
			.orc       = SPIS_PROP(idx, overrun_character),	       \
			.def       = SPIS_PROP(idx, def_char),		       \
		},							       \
		.irq_connect = irq_connect##idx,			       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(SPIS(idx)),		       \
		.max_buf_len = BIT_MASK(SPIS_PROP(idx, easydma_maxcnt_bits)),  \
		.wake_gpio = GPIO_DT_SPEC_GET_OR(SPIS(idx), wake_gpios, {0}),  \
		.mem_reg = DMM_DEV_TO_REG(SPIS(idx)),			       \
		IF_ENABLED(SPIS_PINS_CROSS_DOMAIN(_, /*empty*/, idx, _),       \
			(.cross_domain = true,				       \
			 .default_port =				       \
				DT_PROP_OR(DT_PHANDLE(SPIS(idx),	       \
					default_gpio_port), port, -1),))       \
	};								       \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(SPIS(idx), wake_gpios) ||	       \
		     !(DT_GPIO_FLAGS(SPIS(idx), wake_gpios) & GPIO_ACTIVE_LOW),\
		     "WAKE line must be configured as active high");	       \
	PM_DEVICE_DT_DEFINE(SPIS(idx), spi_nrfx_pm_action,		       \
		COND_CODE_1(SPIS_IS_FAST(idx), (0),			       \
			    (PM_DEVICE_ISR_SAFE)));			       \
	SPI_DEVICE_DT_DEFINE(SPIS(idx),					       \
			    spi_nrfx_init,				       \
			    PM_DEVICE_DT_GET(SPIS(idx)),		       \
			    &spi_##idx##_data,				       \
			    &spi_##idx##z_config,			       \
			    POST_KERNEL,				       \
			    CONFIG_SPI_INIT_PRIORITY,			       \
			    &spi_nrfx_driver_api)

/* Macro creates device instance if it is enabled in devicetree. */
#define SPIS_DEVICE(periph, prefix, id, _) \
	IF_ENABLED(CONFIG_HAS_HW_NRF_SPIS##prefix##id, (SPI_NRFX_SPIS_DEFINE(prefix##id);))

/* Macro iterates over nrfx_spis instances enabled in the nrfx_config.h. */
NRFX_FOREACH_ENABLED(SPIS, SPIS_DEVICE, (), (), _)
