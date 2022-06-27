/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_wdt.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_nrfx);

struct wdt_nrfx_data {
	wdt_callback_t m_callbacks[NRF_WDT_CHANNEL_NUMBER];
	uint32_t m_timeout;
	uint8_t m_allocated_channels;
};

struct wdt_nrfx_config {
	nrfx_wdt_t	  wdt;
	nrfx_wdt_config_t config;
};

static int wdt_nrf_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;
	nrf_wdt_behaviour_t behaviour;

	/* Activate all available options. Run in all cases. */
	behaviour = NRF_WDT_BEHAVIOUR_RUN_SLEEP_HALT;

	/* Deactivate running in sleep mode. */
	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		behaviour &= ~NRF_WDT_BEHAVIOUR_RUN_SLEEP;
	}

	/* Deactivate running when debugger is attached. */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		behaviour &= ~NRF_WDT_BEHAVIOUR_RUN_HALT;
	}

	nrf_wdt_behaviour_set(config->wdt.p_reg, behaviour);
	/* The watchdog timer is driven by the LFCLK clock running at 32768 Hz.
	 * The timeout value given in milliseconds needs to be converted here
	 * to watchdog ticks.*/
	nrf_wdt_reload_value_set(
		config->wdt.p_reg,
		(uint32_t)(((uint64_t)data->m_timeout * 32768U)
			   / 1000));

	nrfx_wdt_enable(&config->wdt);

	return 0;
}

static int wdt_nrf_disable(const struct device *dev)
{
	/* Started watchdog cannot be stopped on nRF devices. */
	ARG_UNUSED(dev);
	return -EPERM;
}

static int wdt_nrf_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;
	nrfx_err_t err_code;
	nrfx_wdt_channel_id channel_id;

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U) {
		return -EINVAL;
	}

	if (data->m_allocated_channels == 0U) {
		/* According to relevant Product Specifications, watchdogs
		 * in all nRF chips can use reload values (determining
		 * the timeout) from range 0xF-0xFFFFFFFF given in 32768 Hz
		 * clock ticks. This makes the allowed range of 0x1-0x07CFFFFF
		 * in milliseconds. Check if the provided value is within
		 * this range. */
		if ((cfg->window.max == 0U) || (cfg->window.max > 0x07CFFFFF)) {
			return -EINVAL;
		}

		/* Save timeout value from first registered watchdog channel. */
		data->m_timeout = cfg->window.max;
	} else if (cfg->window.max != data->m_timeout) {
		return -EINVAL;
	}

	err_code = nrfx_wdt_channel_alloc(&config->wdt,
					  &channel_id);

	if (err_code == NRFX_ERROR_NO_MEM) {
		return -ENOMEM;
	}

	if (cfg->callback != NULL) {
		data->m_callbacks[channel_id] = cfg->callback;
	}

	data->m_allocated_channels++;
	return channel_id;
}

static int wdt_nrf_feed(const struct device *dev, int channel_id)
{
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;

	if (channel_id > data->m_allocated_channels) {
		return -EINVAL;
	}

	nrfx_wdt_channel_feed(&config->wdt,
			      (nrfx_wdt_channel_id)channel_id);

	return 0;
}

static const struct wdt_driver_api wdt_nrfx_driver_api = {
	.setup = wdt_nrf_setup,
	.disable = wdt_nrf_disable,
	.install_timeout = wdt_nrf_install_timeout,
	.feed = wdt_nrf_feed,
};

static void wdt_event_handler(const struct device *dev)
{
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;
	int i;

	for (i = 0; i < data->m_allocated_channels; ++i) {
		if (nrf_wdt_request_status(config->wdt.p_reg,
					   (nrf_wdt_rr_register_t)i)) {
			if (data->m_callbacks[i]) {
				data->m_callbacks[i](dev, i);
			}
		}
	}
}

#define WDT(idx) DT_NODELABEL(wdt##idx)

#define WDT_NRFX_WDT_DEVICE(idx)					       \
	static void wdt_##idx##_event_handler(void)			       \
	{								       \
		wdt_event_handler(DEVICE_DT_GET(WDT(idx)));		       \
	}								       \
	static int wdt_##idx##_init(const struct device *dev)		       \
	{								       \
		const struct wdt_nrfx_config *config = dev->config;	       \
		nrfx_err_t err_code;					       \
		IRQ_CONNECT(DT_IRQN(WDT(idx)), DT_IRQ(WDT(idx), priority),     \
			    nrfx_isr, nrfx_wdt_##idx##_irq_handler, 0);	       \
		err_code = nrfx_wdt_init(&config->wdt,			       \
				 &config->config,			       \
				 wdt_##idx##_event_handler);		       \
		if (err_code != NRFX_SUCCESS) {				       \
			return -EBUSY;					       \
		}							       \
		return 0;						       \
	}								       \
	static struct wdt_nrfx_data wdt_##idx##_data = {		       \
		.m_timeout = 0,						       \
		.m_allocated_channels = 0,				       \
	};								       \
	static const struct wdt_nrfx_config wdt_##idx##z_config = {	       \
		.wdt = NRFX_WDT_INSTANCE(idx),				       \
		.config = {						       \
			.behaviour   = NRF_WDT_BEHAVIOUR_RUN_SLEEP_HALT,       \
			.reload_value  = 2000,				       \
		}							       \
	};								       \
	DEVICE_DT_DEFINE(WDT(idx),					       \
			    wdt_##idx##_init,				       \
			    NULL,					       \
			    &wdt_##idx##_data,				       \
			    &wdt_##idx##z_config,			       \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &wdt_nrfx_driver_api)

#ifdef CONFIG_NRFX_WDT0
WDT_NRFX_WDT_DEVICE(0);
#endif

#ifdef CONFIG_NRFX_WDT1
WDT_NRFX_WDT_DEVICE(1);
#endif
