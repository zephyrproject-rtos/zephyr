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

#include <soc_lrcconf.h>
#include <hal/nrf_bicr.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");

struct dev_data_hfxo {
	struct onoff_manager mgr;
	onoff_notify_fn notify;
	struct k_timer timer;
	sys_snode_t hfxo_node;
#if defined(CONFIG_ZERO_LATENCY_IRQS)
	uint16_t request_count;
#endif /* CONFIG_ZERO_LATENCY_IRQS */
	k_timeout_t start_up_time;
};

struct dev_config_hfxo {
	uint32_t fixed_frequency;
	uint16_t fixed_accuracy;
};

#define BICR (NRF_BICR_Type *)DT_REG_ADDR(DT_NODELABEL(bicr))

#if defined(CONFIG_ZERO_LATENCY_IRQS)
static uint32_t full_irq_lock(void)
{
	uint32_t mcu_critical_state;

	mcu_critical_state = __get_PRIMASK();
	__disable_irq();

	return mcu_critical_state;
}

static void full_irq_unlock(uint32_t mcu_critical_state)
{
	__set_PRIMASK(mcu_critical_state);
}
#endif /* CONFIG_ZERO_LATENCY_IRQS */

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

static void start_hfxo(struct dev_data_hfxo *dev_data)
{
	nrf_lrcconf_event_clear(NRF_LRCCONF010, NRF_LRCCONF_EVENT_HFXOSTARTED);
	soc_lrcconf_poweron_request(&dev_data->hfxo_node, NRF_LRCCONF_POWER_MAIN);
	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_REQHFXO);
}

static void request_hfxo(struct dev_data_hfxo *dev_data)
{
#if defined(CONFIG_ZERO_LATENCY_IRQS)
	unsigned int key;

	key = full_irq_lock();
	if (dev_data->request_count == 0) {
		start_hfxo(dev_data);
	}

	dev_data->request_count++;
	full_irq_unlock(key);
#else
	start_hfxo(dev_data);
#endif /* CONFIG_ZERO_LATENCY_IRQS */
}

#if IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)
void nrf_clock_control_hfxo_request(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct dev_data_hfxo *dev_data = dev->data;

	request_hfxo(dev_data);
}
#endif /* CONFIG_ZERO_LATENCY_IRQS */

static void onoff_start_hfxo(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct dev_data_hfxo *dev_data =
		CONTAINER_OF(mgr, struct dev_data_hfxo, mgr);

	dev_data->notify = notify;
	request_hfxo(dev_data);

	/* Due to a hardware issue, the HFXOSTARTED event is currently
	 * unreliable. Hence the timer is used to simply wait the expected
	 * start-up time. To be removed once the hardware is fixed.
	 */
	k_timer_start(&dev_data->timer, dev_data->start_up_time, K_NO_WAIT);
}

static void stop_hfxo(struct dev_data_hfxo *dev_data)
{
	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_STOPREQHFXO);
	soc_lrcconf_poweron_release(&dev_data->hfxo_node, NRF_LRCCONF_POWER_MAIN);
}

static void release_hfxo(struct dev_data_hfxo *dev_data)
{
#if IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)
	unsigned int key;

	key = full_irq_lock();
	if (dev_data->request_count < 1) {
		full_irq_unlock(key);
		/* Misuse of the API, release without request? */
		__ASSERT_NO_MSG(false);
		/* In case asserts are disabled early return due to no requests pending */
		return;
	}

	dev_data->request_count--;
	if (dev_data->request_count < 1) {
		stop_hfxo(dev_data);
	}

	full_irq_unlock(key);
#else
	stop_hfxo(dev_data);
#endif /* CONFIG_ZERO_LATENCY_IRQS */
}

#if IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)
void nrf_clock_control_hfxo_release(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct dev_data_hfxo *dev_data = dev->data;

	release_hfxo(dev_data);
}
#endif /* IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) */

static void onoff_stop_hfxo(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct dev_data_hfxo *dev_data =
		CONTAINER_OF(mgr, struct dev_data_hfxo, mgr);

	release_hfxo(dev_data);
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
	uint32_t start_up_time;
	int rc;

	rc = onoff_manager_init(&dev_data->mgr, &transitions);
	if (rc < 0) {
		return rc;
	}

	start_up_time = nrf_bicr_hfxo_startup_time_us_get(BICR);
	if (start_up_time == NRF_BICR_HFXO_STARTUP_TIME_UNCONFIGURED) {
		return -EINVAL;
	}

	dev_data->start_up_time = K_USEC(start_up_time);

	k_timer_init(&dev_data->timer, hfxo_start_up_timer_handler, NULL);

	return 0;
}

static DEVICE_API(nrf_clock_control, drv_api_hfxo) = {
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
};

DEVICE_DT_INST_DEFINE(0, init_hfxo, NULL,
		      &data_hfxo, &config_hfxo,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &drv_api_hfxo);
