/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/math_extras.h>
#include <nrfx_wdt.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_nrfx);

struct wdt_nrfx_data {
	wdt_callback_t m_callbacks[NRF_WDT_CHANNEL_NUMBER];
	uint32_t m_timeout;
	uint8_t m_allocated_channels;
	bool enabled;
};

struct wdt_nrfx_config {
	nrfx_wdt_t wdt;
};

static int wdt_nrf_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;
	nrfx_err_t err_code;

	nrfx_wdt_config_t wdt_config = {
		.reload_value = data->m_timeout
	};

#if NRF_WDT_HAS_STOP
	wdt_config.behaviour |= NRF_WDT_BEHAVIOUR_STOP_ENABLE_MASK;
#endif

	if (!(options & WDT_OPT_PAUSE_IN_SLEEP)) {
		wdt_config.behaviour |= NRF_WDT_BEHAVIOUR_RUN_SLEEP_MASK;
	}

	if (!(options & WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		wdt_config.behaviour |= NRF_WDT_BEHAVIOUR_RUN_HALT_MASK;
	}

	err_code = nrfx_wdt_reconfigure(&config->wdt, &wdt_config);

	if (err_code != NRFX_SUCCESS) {
		return -EBUSY;
	}

	nrfx_wdt_enable(&config->wdt);

	data->enabled = true;
	return 0;
}

static int wdt_nrf_disable(const struct device *dev)
{
#if NRFX_WDT_HAS_STOP
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;
	nrfx_err_t err_code;
	int channel_id;

	err_code = nrfx_wdt_stop(&config->wdt);

	if (err_code != NRFX_SUCCESS) {
		/* This can only happen if wdt_nrf_setup() is not called first. */
		return -EFAULT;
	}

	nrfx_wdt_channels_free(&config->wdt);

	for (channel_id = 0; channel_id < data->m_allocated_channels; channel_id++) {
		data->m_callbacks[channel_id] = NULL;
	}
	data->m_allocated_channels = 0;
	data->enabled = false;

	return 0;
#else
	ARG_UNUSED(dev);
	return -EPERM;
#endif
}

static int wdt_nrf_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_nrfx_config *config = dev->config;
	struct wdt_nrfx_data *data = dev->data;
	nrfx_err_t err_code;
	nrfx_wdt_channel_id channel_id;

	if (data->enabled) {
		return -EBUSY;
	}

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

	if ((channel_id >= data->m_allocated_channels) || (channel_id < 0)) {
		return -EINVAL;
	}
	if (!data->enabled) {
		/* Watchdog is not running so does not need to be fed */
		return -EAGAIN;
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

static void wdt_event_handler(const struct device *dev, nrf_wdt_event_t event_type,
			      uint32_t requests, void *p_context)
{
	(void)event_type;
	(void)p_context;

	struct wdt_nrfx_data *data = dev->data;

	while (requests) {
		uint8_t i = u32_count_trailing_zeros(requests);

		if (data->m_callbacks[i]) {
			data->m_callbacks[i](dev, i);
		}
		requests &= ~BIT(i);
	}
}

#define WDT(idx) DT_NODELABEL(wdt##idx)

#define WDT_NRFX_WDT_DEVICE(idx)					       \
	static void wdt_##idx##_event_handler(nrf_wdt_event_t event_type,      \
					      uint32_t requests,	       \
					      void *p_context)		       \
	{								       \
		wdt_event_handler(DEVICE_DT_GET(WDT(idx)), event_type,         \
				  requests, p_context);			       \
	}								       \
	static int wdt_##idx##_init(const struct device *dev)		       \
	{								       \
		const struct wdt_nrfx_config *config = dev->config;	       \
		nrfx_err_t err_code;					       \
		IRQ_CONNECT(DT_IRQN(WDT(idx)), DT_IRQ(WDT(idx), priority),     \
			    nrfx_isr, nrfx_wdt_##idx##_irq_handler, 0);	       \
		err_code = nrfx_wdt_init(&config->wdt,			       \
					 NULL,				       \
					 wdt_##idx##_event_handler,	       \
					 NULL);				       \
		if (err_code != NRFX_SUCCESS) {				       \
			return -EBUSY;					       \
		}							       \
		return 0;						       \
	}								       \
	static struct wdt_nrfx_data wdt_##idx##_data;			       \
	static const struct wdt_nrfx_config wdt_##idx##z_config = {	       \
		.wdt = NRFX_WDT_INSTANCE(idx),				       \
	};								       \
	DEVICE_DT_DEFINE(WDT(idx),					       \
			    wdt_##idx##_init,				       \
			    NULL,					       \
			    &wdt_##idx##_data,				       \
			    &wdt_##idx##z_config,			       \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &wdt_nrfx_driver_api)

#ifdef CONFIG_HAS_HW_NRF_WDT0
WDT_NRFX_WDT_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT1
WDT_NRFX_WDT_DEVICE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT30
WDT_NRFX_WDT_DEVICE(30);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT31
WDT_NRFX_WDT_DEVICE(31);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT010
WDT_NRFX_WDT_DEVICE(010);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT011
WDT_NRFX_WDT_DEVICE(011);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT130
WDT_NRFX_WDT_DEVICE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT131
WDT_NRFX_WDT_DEVICE(131);
#endif

#ifdef CONFIG_HAS_HW_NRF_WDT132
WDT_NRFX_WDT_DEVICE(132);
#endif
