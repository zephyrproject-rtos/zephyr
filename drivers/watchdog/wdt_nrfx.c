/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_wdt

#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>
#include <nrfx_wdt.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_nrfx);

#if !CONFIG_WDT_NRFX_NO_IRQ && NRF_WDT_HAS_STOP
#define WDT_NRFX_SYNC_STOP 1
#endif

struct wdt_nrfx_data {
	nrfx_wdt_t wdt;
	wdt_callback_t m_callbacks[NRF_WDT_CHANNEL_NUMBER];
	uint32_t m_timeout;
	uint8_t m_allocated_channels;
	bool enabled;
#if defined(WDT_NRFX_SYNC_STOP)
	struct k_sem sync_stop;
#endif
};

static int wdt_nrf_setup(const struct device *dev, uint8_t options)
{
	struct wdt_nrfx_data *data = dev->data;
	int err_code;

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

	err_code = nrfx_wdt_reconfigure(&data->wdt, &wdt_config);

	if (err_code < 0) {
		return err_code;
	}

	nrfx_wdt_enable(&data->wdt);

	data->enabled = true;
	return 0;
}

static int wdt_nrf_disable(const struct device *dev)
{
#if NRFX_WDT_HAS_STOP
	struct wdt_nrfx_data *data = dev->data;
	int err_code;
	int channel_id;

	err_code = nrfx_wdt_stop(&data->wdt);

	if (err_code < 0) {
		/* This can only happen if wdt_nrf_setup() is not called first. */
		return -EFAULT;
	}

#if defined(WDT_NRFX_SYNC_STOP)
	k_sem_take(&data->sync_stop, K_FOREVER);
#endif

	nrfx_wdt_channels_free(&data->wdt);

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
	struct wdt_nrfx_data *data = dev->data;
	int err_code;
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
		 * in milliseconds, defined using NRF_WDT_RR_VALUE_MS symbol.
		 * Check if the provided value is within this range.
		 */
		if ((cfg->window.max == 0U) || (cfg->window.max > NRF_WDT_RR_VALUE_MS)) {
			return -EINVAL;
		}

		/* Save timeout value from first registered watchdog channel. */
		data->m_timeout = cfg->window.max;
	} else if (cfg->window.max != data->m_timeout) {
		return -EINVAL;
	}

	err_code = nrfx_wdt_channel_alloc(&data->wdt,
					  &channel_id);

	if (err_code == -ENOMEM) {
		return err_code;
	}

	if (cfg->callback != NULL) {
		data->m_callbacks[channel_id] = cfg->callback;
	}

	data->m_allocated_channels++;
	return channel_id;
}

static int wdt_nrf_feed(const struct device *dev, int channel_id)
{
	struct wdt_nrfx_data *data = dev->data;

	if ((channel_id >= data->m_allocated_channels) || (channel_id < 0)) {
		return -EINVAL;
	}
	if (!data->enabled) {
		/* Watchdog is not running so does not need to be fed */
		return -EAGAIN;
	}

	nrfx_wdt_channel_feed(&data->wdt,
			      (nrfx_wdt_channel_id)channel_id);

	return 0;
}

static DEVICE_API(wdt, wdt_nrfx_driver_api) = {
	.setup = wdt_nrf_setup,
	.disable = wdt_nrf_disable,
	.install_timeout = wdt_nrf_install_timeout,
	.feed = wdt_nrf_feed,
};

static void wdt_event_handler(const struct device *dev, nrf_wdt_event_t event_type,
			      uint32_t requests, void *p_context)
{
	struct wdt_nrfx_data *data = dev->data;

#if defined(WDT_NRFX_SYNC_STOP)
	if (event_type == NRF_WDT_EVENT_STOPPED) {
		k_sem_give(&data->sync_stop);
	}
#else
	(void)event_type;
#endif
	(void)p_context;

	while (requests) {
		uint8_t i = u32_count_trailing_zeros(requests);

		if (data->m_callbacks[i]) {
			data->m_callbacks[i](dev, i);
		}
		requests &= ~BIT(i);
	}
}

#define WDT(idx) DT_NODELABEL(wdt##idx)

#define WDT_NRFX_WDT_IRQ(inst)						       \
	COND_CODE_1(CONFIG_WDT_NRFX_NO_IRQ,				       \
		(),							       \
		(IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),  \
			nrfx_wdt_irq_handler, &wdt_##inst##_data.wdt, 0)))

#define WDT_NRFX_WDT_DEVICE(inst)					       \
	static void wdt_##inst##_event_handler(nrf_wdt_event_t event_type,     \
					       uint32_t requests,	       \
					       void *p_context)		       \
	{								       \
		wdt_event_handler(DEVICE_DT_INST_GET(inst), event_type,	       \
				  requests, p_context);			       \
	}								       \
	static struct wdt_nrfx_data wdt_##inst##_data =	{		       \
		.wdt = NRFX_WDT_INSTANCE(DT_INST_REG_ADDR(inst)),	       \
		IF_ENABLED(WDT_NRFX_SYNC_STOP,				       \
			(.sync_stop = Z_SEM_INITIALIZER(		       \
				wdt_##inst##_data.sync_stop, 0, 1),))	       \
	};								       \
	static int wdt_##inst##_init(const struct device *dev)		       \
	{								       \
		int err_code;						       \
		struct wdt_nrfx_data *data = dev->data;			       \
		WDT_NRFX_WDT_IRQ(inst);					       \
		err_code = nrfx_wdt_init(&data->wdt,			       \
					 NULL,				       \
					 IS_ENABLED(CONFIG_WDT_NRFX_NO_IRQ)    \
						 ? NULL			       \
						 : wdt_##inst##_event_handler, \
					 NULL);				       \
		return err_code;					       \
	}								       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			      wdt_##inst##_init,			       \
			      NULL,					       \
			      &wdt_##inst##_data,			       \
			      NULL,					       \
			      PRE_KERNEL_1,				       \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	       \
			      &wdt_nrfx_driver_api)

DT_INST_FOREACH_STATUS_OKAY(WDT_NRFX_WDT_DEVICE)
