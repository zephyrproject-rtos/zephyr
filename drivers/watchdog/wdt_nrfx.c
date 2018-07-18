/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_wdt.h>
#include <watchdog.h>

#define SYS_LOG_DOMAIN "wdt_nrfx"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WDT_LEVEL
#include <logging/sys_log.h>

DEVICE_DECLARE(wdt_nrfx);

/* Each activated and not reloaded channel can generate watchdog interrupt.
 * Array m_callbacks provides storing callbacks for each channel.
 */
static wdt_callback_t m_callbacks[NRF_WDT_CHANNEL_NUMBER];

/* The m_allocated_channels variable stores number of currently allocated
 * and activated watchdog channels.
 */
static u8_t m_allocated_channels;

/* The m_timeout variable stores watchdog timeout value in millisecond units.
 * It is used to check whether installing watchdog provide the same timeout
 * value in the window configuration.
 */
static u32_t m_timeout;

static int wdt_nrf_setup(struct device *dev, u8_t options)
{
	nrf_wdt_behaviour_t behaviour;

	ARG_UNUSED(dev);

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

	nrf_wdt_behaviour_set(behaviour);
	/* The watchdog timer is driven by the LFCLK clock running at 32768 Hz.
	 * The timeout value given in milliseconds needs to be converted here
	 * to watchdog ticks.*/
	nrf_wdt_reload_value_set(
			(uint32_t)(((uint64_t)m_timeout * 32768) / 1000));

	nrfx_wdt_enable();

	return 0;
}

static int wdt_nrf_disable(struct device *dev)
{
	/* Started watchdog cannot be stopped on nRF devices. */
	ARG_UNUSED(dev);
	return -EPERM;
}

static int wdt_nrf_install_timeout(struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	nrfx_err_t err_code;
	nrfx_wdt_channel_id channel_id;

	ARG_UNUSED(dev);

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0) {
		return -EINVAL;
	}

	if (m_allocated_channels == 0) {
		/* According to relevant Product Specifications, watchdogs
		 * in all nRF chips can use reload values (determining
		 * the timeout) from range 0xF-0xFFFFFFFF given in 32768 Hz
		 * clock ticks. This makes the allowed range of 0x1-0x07CFFFFF
		 * in milliseconds. Check if the provided value is within
		 * this range. */
		if ((cfg->window.max == 0) || (cfg->window.max > 0x07CFFFFF)) {
			return -EINVAL;
		}

		/* Save timeout value from first registered watchdog channel. */
		m_timeout = cfg->window.max;
	} else if (cfg->window.max != m_timeout) {
		return -EINVAL;
	}

	err_code = nrfx_wdt_channel_alloc(&channel_id);

	if (err_code == NRFX_ERROR_NO_MEM) {
		return -ENOMEM;
	}

	if (cfg->callback != NULL) {
		m_callbacks[channel_id] = cfg->callback;
	}

	m_allocated_channels++;
	return channel_id;
}

static int wdt_nrf_feed(struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	if (channel_id > m_allocated_channels) {
		return -EINVAL;
	}

	nrfx_wdt_channel_feed((nrfx_wdt_channel_id)channel_id);

	return 0;
}

static void wdt_nrf_enable(struct device *dev)
{
	ARG_UNUSED(dev);
	/* Deprecated function. No implementation needed. */
	SYS_LOG_ERR("Function not implemented!");
}

static int wdt_nrf_set_config(struct device *dev, struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
	/* Deprecated function. No implementation needed. */
	SYS_LOG_ERR("Function not implemented!");
	return 0;
}

static void wdt_nrf_get_config(struct device *dev, struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
	/* Deprecated function. No implementation needed. */
	SYS_LOG_ERR("Function not implemented!");
}

static void wdt_nrf_reload(struct device *dev)
{
	ARG_UNUSED(dev);
	/* Deprecated function. No implementation needed. */
	SYS_LOG_ERR("Function not implemented!");
}

static const struct wdt_driver_api wdt_nrf_api = {
	.setup = wdt_nrf_setup,
	.disable = wdt_nrf_disable,
	.install_timeout = wdt_nrf_install_timeout,
	.feed = wdt_nrf_feed,
	.enable = wdt_nrf_enable,
	.get_config = wdt_nrf_get_config,
	.set_config = wdt_nrf_set_config,
	.reload = wdt_nrf_reload,
};

static void wdt_event_handler(void)
{
	int i;

	for (i = 0; i < m_allocated_channels ; ++i) {
		if (nrf_wdt_request_status((nrf_wdt_rr_register_t)i)) {
			if (m_callbacks[i]) {
				m_callbacks[i](DEVICE_GET(wdt_nrfx), i);
			}
		}
	}
}

static int init_wdt(struct device *dev)
{
	nrfx_err_t err_code;
	/* Set default values. They will be overwritten by setup function. */
	const nrfx_wdt_config_t config = {
		.behaviour          = NRF_WDT_BEHAVIOUR_RUN_SLEEP_HALT,
		.reload_value       = 2000
	};

	ARG_UNUSED(dev);

	err_code = nrfx_wdt_init(&config, wdt_event_handler);
	if (err_code != NRFX_SUCCESS) {
		return -EBUSY;
	}

	IRQ_CONNECT(CONFIG_WDT_NRF_IRQ, CONFIG_WDT_NRF_IRQ_PRI,
		    nrfx_isr, nrfx_wdt_irq_handler, 0);
	irq_enable(CONFIG_WDT_NRF_IRQ);

	return 0;
}

DEVICE_AND_API_INIT(wdt_nrf, CONFIG_WDT_0_NAME, init_wdt,
		    NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_nrf_api);
