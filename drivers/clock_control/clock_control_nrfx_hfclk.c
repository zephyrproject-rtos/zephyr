/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include <nrfx_clock_hfclk.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>
#include "clock_control_nrfx_common.h"

LOG_MODULE_REGISTER(clock_control_hfclk, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_hfclk

#define CLOCK_DEVICE_HFCLK DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk)

/* Used only by HF clock */
#define HF_USER_BT      BIT(0)
#define HF_USER_GENERIC BIT(1)

static atomic_t hfclk_users;

static void onoff_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int err;

	err = common_async_start(CLOCK_DEVICE_HFCLK, common_onoff_started_callback, notify,
				 COMMON_CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void onoff_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int res;

	res = common_stop(CLOCK_DEVICE_HFCLK, COMMON_CTX_ONOFF);
	notify(mgr, res);
}

static void clock_event_handler(void)
{
	/* Check needed due to anomaly 201:
	 * HFCLKSTARTED may be generated twice.
	 */
	if (COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->flags) ==
	    CLOCK_CONTROL_STATUS_STARTING) {
		common_clkstarted_handle(CLOCK_DEVICE_HFCLK);
	}
}

static void generic_hfclk_start(void)
{
	nrf_clock_hfclk_t type;
	bool already_started = false;
	unsigned int key = irq_lock();

	hfclk_users |= HF_USER_GENERIC;
	if (hfclk_users & HF_USER_BT) {
		(void)nrfx_clock_hfclk_running_check(&type);
		if (type == NRF_CLOCK_HFCLK_HIGH_ACCURACY) {
			already_started = true;
			/* Set on state in case clock interrupt comes and we
			 * want to avoid handling that.
			 */

			common_set_on_state(
				&((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->flags);
		}
	}

	irq_unlock(key);

	if (already_started) {
		/* Clock already started by z_nrf_clock_bt_ctlr_hf_request */
		common_clkstarted_handle(CLOCK_DEVICE_HFCLK);
		return;
	}

	nrfx_clock_hfclk_start();
}

static void generic_hfclk_stop(void)
{
	/* It's not enough to use only atomic_and() here for synchronization,
	 * as the thread could be preempted right after that function but
	 * before nrfx_clock_hfclk_stop() is called and the preempting code could request
	 * the HFCLK again. Then, the HFCLK would be stopped inappropriately
	 * and hfclk_user would be left with an incorrect value.
	 */
	unsigned int key = irq_lock();

	hfclk_users &= ~HF_USER_GENERIC;
	/* Skip stopping if BT is still requesting the clock. */
	if (!(hfclk_users & HF_USER_BT)) {
		nrfx_clock_hfclk_stop();
	}

	irq_unlock(key);
}

void z_nrf_clock_bt_ctlr_hf_request(void)
{
	if (atomic_or(&hfclk_users, HF_USER_BT) & HF_USER_GENERIC) {
		/* generic request already activated clock. */
		return;
	}

	nrfx_clock_hfclk_start();
}

void z_nrf_clock_bt_ctlr_hf_release(void)
{
	/* It's not enough to use only atomic_and() here for synchronization,
	 * see the explanation in generic_hfclk_stop().
	 */
	unsigned int key = irq_lock();

	hfclk_users &= ~HF_USER_BT;
	/* Skip stopping if generic is still requesting the clock. */
	if (!(hfclk_users & HF_USER_GENERIC)) {
		nrfx_clock_hfclk_stop();
	}

	irq_unlock(key);
}

#if DT_NODE_EXISTS(DT_NODELABEL(hfxo))
uint32_t z_nrf_clock_bt_ctlr_hf_get_startup_time_us(void)
{
	return DT_PROP(DT_NODELABEL(hfxo), startup_time_us);
}
#endif

static int api_start(const struct device *dev, clock_control_subsys_t subsys, clock_control_cb_t cb,
		     void *user_data)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return common_async_start(CLOCK_DEVICE_HFCLK, cb, user_data, COMMON_CTX_API);
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

	return common_stop(CLOCK_DEVICE_HFCLK, COMMON_CTX_API);
}

static enum clock_control_status api_get_status(const struct device *dev,
						clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->flags);
}

static int api_request(const struct device *dev, const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_request(&((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->mgr, cli);
}

static int api_release(const struct device *dev, const struct nrf_clock_spec *spec)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_release(&((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->mgr);
}

static int api_cancel_or_release(const struct device *dev, const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_cancel_or_release(&((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->mgr,
				       cli);
}

static int clk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;
	static const struct onoff_transitions transitions = {.start = onoff_start,
							     .stop = onoff_stop};

	common_connect_irq();

	if (nrfx_clock_hfclk_init(clock_event_handler) != 0) {
		return -EIO;
	}

	err = onoff_manager_init(&((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->mgr,
				 &transitions);
	if (err < 0) {
		return err;
	}

	((common_clock_data_t *)CLOCK_DEVICE_HFCLK->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_hfclk, &nrfx_clock_hfclk_irq_handler);

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
	.start = generic_hfclk_start,
	.stop = generic_hfclk_stop,
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclk), clk_init, NULL, &data,
		 &config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_api);
