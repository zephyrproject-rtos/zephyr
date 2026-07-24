/*
 * Copyright (c) 2016-2025 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include <nrfx_clock_xo.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>
#include "clock_control_nrfx_common.h"

LOG_MODULE_REGISTER(clock_control_xo, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_xo

#define CLOCK_DEVICE_XO DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)

/* Used only by HF clock */
#define XO_USER_BT      BIT(0)
#define XO_USER_GENERIC BIT(1)

static atomic_t xo_users;

#ifdef CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION

static void nrf54l_errata_30_workaround(void)
{
	while (FIELD_GET(CLOCK_XO_STAT_STATE_Msk, NRF_CLOCK->XO.STAT) !=
	       CLOCK_XO_STAT_STATE_Running) {
	}
	const uint32_t higher_bits = *((volatile uint32_t *)0x50120820UL) & 0xFFFFFFC0;
	*((volatile uint32_t *)0x50120864UL) = 1 | BIT(31);
	*((volatile uint32_t *)0x50120848UL) = 1;
	uint32_t off_abs = 24;

	while (off_abs >= 24) {
		*((volatile uint32_t *)0x50120844UL) = 1;
		while (((*((volatile uint32_t *)0x50120840UL)) & (1 << 16)) != 0) {
		}
		const uint32_t current_cal = *((volatile uint32_t *)0x50120820UL) & 0x3F;
		const uint32_t cal_result = *((volatile uint32_t *)0x50120840UL) & 0x7FF;
		int32_t off = 1024 - cal_result;

		off_abs = (off < 0) ? -off : off;

		if (off >= 24 && current_cal < 0x3F) {
			*((volatile uint32_t *)0x50120820UL) = higher_bits | (current_cal + 1);
		} else if (off <= -24 && current_cal > 0) {
			*((volatile uint32_t *)0x50120820UL) = higher_bits | (current_cal - 1);
		}
	}

	*((volatile uint32_t *)0x50120848UL) = 0;
	*((volatile uint32_t *)0x50120864UL) = 0;
}

#if CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD

static struct onoff_client hf_cal_cli;

static void calibration_finished_callback(struct onoff_manager *mgr, struct onoff_client *cli,
					  uint32_t state, int res)
{
	(void)onoff_cancel_or_release(mgr, cli);
}

static void calibration_handler(struct k_timer *timer)
{
	nrf_clock_hfclk_t clk_src;

	bool ret = nrfx_clock_xo_running_check(&clk_src);

	if (ret && (clk_src == NRF_CLOCK_HFCLK_HIGH_ACCURACY)) {
		return;
	}

	sys_notify_init_callback(&hf_cal_cli.notify, calibration_finished_callback);
	(void)onoff_request(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr, &hf_cal_cli);
}

static K_TIMER_DEFINE(calibration_timer, calibration_handler, NULL);

static int calibration_init(void)
{
	k_timer_start(&calibration_timer, K_NO_WAIT,
		      K_MSEC(CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD));

	return 0;
}

SYS_INIT(calibration_init, APPLICATION, 0);

#endif /* CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD */
#endif /* CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION */

static void clkstarted_event_handle(void)
{
	if (COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_XO->data)->flags) ==
	    CLOCK_CONTROL_STATUS_STARTING) {
		/* Handler is called only if state is set. BT specific API
		 * does not set this state and does not require handler to
		 * be called.
		 */
#if CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION
		if (NRF_ERRATA_DYNAMIC_CHECK(54L, 30)) {
			nrf54l_errata_30_workaround();
		}
#endif
		common_clkstarted_handle(CLOCK_DEVICE_XO);
	}
}

static void generic_xo_start(void)
{
	nrf_clock_hfclk_t type;
	bool already_started = false;
	unsigned int key = irq_lock();

	xo_users |= XO_USER_GENERIC;
	if (xo_users & XO_USER_BT) {
		(void)nrfx_clock_xo_running_check(&type);
		if (type == NRF_CLOCK_HFCLK_HIGH_ACCURACY) {
			already_started = true;
			/* Set on state in case clock interrupt comes and we
			 * want to avoid handling that.
			 */
			common_set_on_state(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->flags);
		}
	}

	irq_unlock(key);

	if (already_started) {
		/* Clock already started by z_nrf_clock_bt_ctlr_hf_request */
#if CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION
		if (NRF_ERRATA_DYNAMIC_CHECK(54L, 30)) {
			nrf54l_errata_30_workaround();
		}
#endif
		common_clkstarted_handle(CLOCK_DEVICE_XO);
		return;
	}

	nrfx_clock_xo_start();
}

static void generic_xo_stop(void)
{
	/* It's not enough to use only atomic_and() here for synchronization,
	 * as the thread could be preempted right after that function but
	 * before nrfx_clock_xo_stop() is called and the preempting code could request
	 * the XO again. Then, the XO would be stopped inappropriately
	 * and xo_user would be left with an incorrect value.
	 */
	unsigned int key = irq_lock();

	xo_users &= ~XO_USER_GENERIC;
	/* Skip stopping if BT is still requesting the clock. */
	if (!(xo_users & XO_USER_BT)) {
		nrfx_clock_xo_stop();
	}

	irq_unlock(key);
}

static void onoff_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int err;

	err = common_async_start(CLOCK_DEVICE_XO, common_onoff_started_callback, notify,
				 COMMON_CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void onoff_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int res;

	res = common_stop(CLOCK_DEVICE_XO, COMMON_CTX_ONOFF);
	notify(mgr, res);
}

static void clock_event_handler(nrfx_clock_xo_event_type_t event)
{
	switch (event) {
#if NRF_CLOCK_HAS_XO_TUNE
	case NRFX_CLOCK_XO_EVT_XO_TUNED:
		clkstarted_event_handle();
		break;
	case NRFX_CLOCK_XO_EVT_XO_TUNE_ERROR:
	case NRFX_CLOCK_XO_EVT_XO_TUNE_FAILED:
		/* No processing needed. */
		break;
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED:
		/* HFCLK is stable after XOTUNED event.
		 * HFCLK_STARTED means only that clock has been started.
		 */
		break;
#else
	/* HFCLK started should be used only if tune operation is done implicitly. */
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED: {
		/* Check needed due to anomaly 201:
		 * HFCLKSTARTED may be generated twice.
		 */
		if (COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_XO->data)->flags) ==
		    CLOCK_CONTROL_STATUS_STARTING) {
			clkstarted_event_handle();
		}

		break;
	}
#endif

#if NRF_CLOCK_HAS_PLL
	case NRFX_CLOCK_XO_EVT_PLL_STARTED:
		/* No processing needed. */
		break;
#endif
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
}

void z_nrf_clock_bt_ctlr_hf_request(void)
{
	if (atomic_or(&xo_users, XO_USER_BT) & XO_USER_GENERIC) {
		/* generic request already activated clock. */
		return;
	}

	nrfx_clock_xo_start();
}

void z_nrf_clock_bt_ctlr_hf_release(void)
{
	/* It's not enough to use only atomic_and() here for synchronization,
	 * see the explanation in generic_hfclk_stop().
	 */
	unsigned int key = irq_lock();

	xo_users &= ~XO_USER_BT;
	/* Skip stopping if generic is still requesting the clock. */
	if (!(xo_users & XO_USER_GENERIC)) {
		nrfx_clock_xo_stop();
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
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);

	return common_async_start(CLOCK_DEVICE_XO, cb, user_data, COMMON_CTX_API);
}

static int api_blocking_start(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);

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
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);

	return common_stop(CLOCK_DEVICE_XO, COMMON_CTX_API);
}

static enum clock_control_status api_get_status(const struct device *dev,
						clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);

	return COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_XO->data)->flags);
}
static int api_request(const struct device *dev, const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(spec);

	return onoff_request(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr, cli);
}

static int api_release(const struct device *dev, const struct nrf_clock_spec *spec)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(spec);

	return onoff_release(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr);
}

static int api_cancel_or_release(const struct device *dev, const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(spec);

	return onoff_cancel_or_release(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr, cli);
}

static int clk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;
	static const struct onoff_transitions transitions = {.start = onoff_start,
							     .stop = onoff_stop};

	common_connect_irq();

	if (nrfx_clock_xo_init(clock_event_handler) != 0) {
		return -EIO;
	}

	err = onoff_manager_init(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr,
				 &transitions);
	if (err < 0) {
		return err;
	}

	((common_clock_data_t *)CLOCK_DEVICE_XO->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_xo, &nrfx_clock_xo_irq_handler);

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
	.start = generic_xo_start,
	.stop = generic_xo_stop,
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo), clk_init, NULL, &data,
		 &config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_api);
