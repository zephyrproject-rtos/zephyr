/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "clock_control_nrfx_common.h"
#include <nrfx_clock_hfclkaudio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(clock_control_hfclkaudio, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_hfclkaudio

#define CLOCK_DEVICE_HFCLKAUDIO DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclkaudio)
#define CLOCK_NODE_HFCLKAUDIO DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclkaudio)

static void onoff_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int res;

	res = common_stop(CLOCK_DEVICE_HFCLKAUDIO, COMMON_CTX_ONOFF);
	notify(mgr, res);
}

static void onoff_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int err;

	err = common_async_start(CLOCK_DEVICE_HFCLKAUDIO, common_onoff_started_callback, notify,
				 COMMON_CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void clock_event_handler(void)
{
	common_clkstarted_handle(CLOCK_DEVICE_HFCLKAUDIO);
}

static void hfclkaudio_init(void)
{
#if DT_NODE_HAS_PROP(CLOCK_NODE_HFCLKAUDIO, hfclkaudio_frequency)
	const uint32_t frequency = DT_PROP(CLOCK_NODE_HFCLKAUDIO, hfclkaudio_frequency);
	/* As specified in the nRF5340 PS:
	 *
	 * FREQ_VALUE = 2^16 * ((12 * f_out / 32M) - 4)
	 */
	const uint32_t freq_value = (uint32_t)((384ULL * frequency) / 15625) - 262144;

#if NRF_CLOCK_HAS_HFCLKAUDIO
	nrf_clock_hfclkaudio_config_set(NRF_CLOCK, freq_value);
#else
#error "hfclkaudio-frequency specified but HFCLKAUDIO clock is not present."
#endif /* NRF_CLOCK_HAS_HFCLKAUDIO */
#endif
}

static int api_start(const struct device *dev, clock_control_subsys_t subsys, clock_control_cb_t cb,
		     void *user_data)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return common_async_start(CLOCK_DEVICE_HFCLKAUDIO, cb, user_data, COMMON_CTX_API);
}

static int api_blocking_start(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);
	int err;

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return -ENOTSUP;
	}

	err = api_start(NULL, NULL, common_blocking_start_callback, &sem);
	if (err < 0) {
		return err;
	}

	return k_sem_take(&sem, K_MSEC(500));
}

static int api_stop(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return common_stop(CLOCK_DEVICE_HFCLKAUDIO, COMMON_CTX_API);
}

static enum clock_control_status api_get_status(const struct device *dev,
						clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_HFCLKAUDIO->data)->flags);
}

static int api_request(const struct device *dev, const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_request(&((common_clock_data_t *)CLOCK_DEVICE_HFCLKAUDIO->data)->mgr, cli);
}

static int api_release(const struct device *dev, const struct nrf_clock_spec *spec)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_release(&((common_clock_data_t *)CLOCK_DEVICE_HFCLKAUDIO->data)->mgr);
}

static int api_cancel_or_release(const struct device *dev, const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_cancel_or_release(&((common_clock_data_t *)CLOCK_DEVICE_HFCLKAUDIO->data)->mgr,
				       cli);
}

static int clk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;
	static const struct onoff_transitions transitions = {.start = onoff_start,
							     .stop = onoff_stop};

	common_connect_irq();

	if (nrfx_clock_hfclkaudio_init(clock_event_handler) != 0) {
		return -EIO;
	}

	hfclkaudio_init();

	err = onoff_manager_init(&((common_clock_data_t *)CLOCK_DEVICE_HFCLKAUDIO->data)->mgr,
				 &transitions);
	if (err < 0) {
		return err;
	}

	((common_clock_data_t *)CLOCK_DEVICE_HFCLKAUDIO->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_hfclkaudio,
					&nrfx_clock_hfclkaudio_irq_handler);

static DEVICE_API(nrf_clock_control, clock_control_api) = {
	.std_api = {
		.on = api_blocking_start,
		.off = api_stop,
		.async_on = api_start,
		.get_status = api_get_status,
	},
	.request = api_request,
	.release = api_release,
	.cancel_or_release = api_cancel_or_release,
};

static common_clock_data_t data;

static const common_clock_config_t config = {
	.start = nrfx_clock_hfclkaudio_start,
	.stop = nrfx_clock_hfclkaudio_stop,
};

DEVICE_DT_DEFINE(CLOCK_NODE_HFCLKAUDIO, clk_init, NULL, &data, &config, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_api);
