/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf54h_hfxo

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <hal/nrf_lrcconf.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");

struct dev_data_hfxo {
	struct onoff_manager mgr;
	onoff_notify_fn notify;
	struct k_timer timer;
	struct clock_lrcconf_sink lrcconf_sink;
};

struct dev_config_hfxo {
	uint32_t fixed_frequency;
	uint16_t fixed_accuracy;
	k_timeout_t start_up_time;
};

static void hfxo_start_up_timer_handler(struct k_timer *timer)
{
	struct dev_data_hfxo *dev_data =
		CONTAINER_OF(timer, struct dev_data_hfxo, timer);

	/* In specific cases, the HFXOSTARTED event might not be set even
	 * though the HFXO has started (this is a hardware issue that will
	 * be fixed). For now, the HFXO is simply assumed to be started
	 * after its configured start-up time expires.
	 */
	LOG_DBG("HFXOSTARTED: %u",
		nrf_lrcconf_event_check(NRF_LRCCONF010,
					NRF_LRCCONF_EVENT_HFXOSTARTED));

	if (dev_data->notify) {
		dev_data->notify(&dev_data->mgr, 0);
	}
}

static void onoff_start_hfxo(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct dev_data_hfxo *dev_data =
		CONTAINER_OF(mgr, struct dev_data_hfxo, mgr);
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct dev_config_hfxo *dev_config = dev->config;

	dev_data->notify = notify;

	nrf_lrcconf_event_clear(NRF_LRCCONF010, NRF_LRCCONF_EVENT_HFXOSTARTED);
	clock_request_lrcconf_poweron_main(&dev_data->lrcconf_sink);
	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_REQHFXO);

	/* Due to a hardware issue, the HFXOSTARTED event is currently
	 * unreliable. Hence the timer is used to simply wait the expected
	 * start-up time. To be removed once the hardware is fixed.
	 */
	k_timer_start(&dev_data->timer, dev_config->start_up_time, K_NO_WAIT);
}

static void onoff_stop_hfxo(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct dev_data_hfxo *dev_data =
		CONTAINER_OF(mgr, struct dev_data_hfxo, mgr);

	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_STOPREQHFXO);
	clock_release_lrcconf_poweron_main(&dev_data->lrcconf_sink);
	notify(mgr, 0);
}

static bool is_clock_spec_valid(const struct device *dev,
				const struct nrf_clock_spec *spec)
{
	const struct dev_config_hfxo *dev_config = dev->config;

	if (spec->frequency > dev_config->fixed_frequency) {
		LOG_ERR("invalid frequency");
		return false;
	}

	/* Signal an error if an accuracy better than available is requested. */
	if (spec->accuracy &&
	    spec->accuracy != NRF_CLOCK_CONTROL_ACCURACY_MAX &&
	    spec->accuracy < dev_config->fixed_accuracy) {
		LOG_ERR("invalid accuracy");
		return false;
	}

	/* Consider HFXO precision high, skip checking what is requested. */

	return true;
}

static int api_request_hfxo(const struct device *dev,
			    const struct nrf_clock_spec *spec,
			    struct onoff_client *cli)
{
	struct dev_data_hfxo *dev_data = dev->data;

	if (spec && !is_clock_spec_valid(dev, spec)) {
		return -EINVAL;
	}

	return onoff_request(&dev_data->mgr, cli);
}

static int api_release_hfxo(const struct device *dev,
			    const struct nrf_clock_spec *spec)
{
	struct dev_data_hfxo *dev_data = dev->data;

	if (spec && !is_clock_spec_valid(dev, spec)) {
		return -EINVAL;
	}

	return onoff_release(&dev_data->mgr);
}

static int api_cancel_or_release_hfxo(const struct device *dev,
				      const struct nrf_clock_spec *spec,
				      struct onoff_client *cli)
{
	struct dev_data_hfxo *dev_data = dev->data;

	if (spec && !is_clock_spec_valid(dev, spec)) {
		return -EINVAL;
	}

	return onoff_cancel_or_release(&dev_data->mgr, cli);
}

static int api_get_rate_hfxo(const struct device *dev,
			     clock_control_subsys_t sys,
			     uint32_t *rate)
{
	ARG_UNUSED(sys);

	const struct dev_config_hfxo *dev_config = dev->config;

	*rate = dev_config->fixed_frequency;

	return 0;
}

static int init_hfxo(const struct device *dev)
{
	struct dev_data_hfxo *dev_data = dev->data;
	static const struct onoff_transitions transitions = {
		.start = onoff_start_hfxo,
		.stop = onoff_stop_hfxo
	};
	int rc;

	rc = onoff_manager_init(&dev_data->mgr, &transitions);
	if (rc < 0) {
		return rc;
	}

	k_timer_init(&dev_data->timer, hfxo_start_up_timer_handler, NULL);

	return 0;
}

static struct nrf_clock_control_driver_api drv_api_hfxo = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
		.get_rate = api_get_rate_hfxo,
	},
	.request = api_request_hfxo,
	.release = api_release_hfxo,
	.cancel_or_release = api_cancel_or_release_hfxo,
};

static struct dev_data_hfxo data_hfxo;

static const struct dev_config_hfxo config_hfxo = {
	.fixed_frequency = DT_INST_PROP(0, clock_frequency),
	.fixed_accuracy = DT_INST_PROP(0, accuracy_ppm),
	.start_up_time = K_USEC(DT_INST_PROP(0, startup_time_us)),
};

DEVICE_DT_INST_DEFINE(0, init_hfxo, NULL,
		      &data_hfxo, &config_hfxo,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &drv_api_hfxo);
