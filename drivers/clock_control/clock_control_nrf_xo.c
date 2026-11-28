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
#include "clock_control_nrf_common.h"

LOG_MODULE_REGISTER(clock_control_xo, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_xo

#define CLOCK_DEVICE_XO DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)

/* Used only by HF clock */
#define XO_USER_BT      BIT(0)
#define XO_USER_GENERIC BIT(1)

/* This is an extension to enum clock_control_status */
#define CLOCK_CONTROL_STATUS_STARTED (CLOCK_CONTROL_STATUS_UNKNOWN + 1)

typedef enum {
	XO_MGRS_READY,
	XO_MGRS_STARTED,
	XO_MGRS_COUNT
} xo_managers_t;

typedef struct {
	common_clock_data_t data;
#if CONFIG_CLOCK_CONTROL_NRF_ONOFF
	struct onoff_manager mgr_started;
	atomic_t active_clients[XO_MGRS_COUNT];
	struct onoff_client internal_cli[XO_MGRS_COUNT];
	clock_control_cb_t cb_started;
	atomic_t request_pending;
#endif
} xo_clock_data_t;

static atomic_t xo_users;
static atomic_t zli_clock_status;

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

void clk_started_event_handle(const struct device *dev)
{
	clock_control_cb_t callback = ((xo_clock_data_t *)dev->data)->cb_started;

	((xo_clock_data_t *)dev->data)->cb_started = NULL;

	if (callback) {
		callback(dev, NULL, ((common_clock_data_t *)dev->data)->user_data);
	}
}

static void clk_ready_event_handle(void)
{
#if CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION
	if (NRF_ERRATA_DYNAMIC_CHECK(54L, 30)) {
		nrf54l_errata_30_workaround();
	}
#endif
	common_clk_ready_handle(CLOCK_DEVICE_XO);
}

static void generic_xo_start(void)
{
	bool clk_started = false;
	bool clk_ready = false;
	uint32_t *flags = &((common_clock_data_t *)CLOCK_DEVICE_XO->data)->flags;

	unsigned int key = irq_lock();

	if (atomic_or(&xo_users, XO_USER_GENERIC) & XO_USER_BT) {

		*flags = atomic_get(&zli_clock_status) | COMMON_GET_CTX(*flags);

		switch (*flags & COMMON_STATUS_MASK) {
		case CLOCK_CONTROL_STATUS_STARTED:
			clk_started = true;
			break;
		case CLOCK_CONTROL_STATUS_ON:
			clk_ready = true;
			common_set_on_state(flags);
			break;
		}
	}

	irq_unlock(key);

	if (clk_started) {
		/* Clock already started by z_nrf_clock_bt_ctlr_hf_request */
		clk_started_event_handle(CLOCK_DEVICE_XO);
		return;
	}

	if (clk_ready) {
		/* Clock already started by z_nrf_clock_bt_ctlr_hf_request */
		clk_ready_event_handle();
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

/* XO STARTED state cannot be read from register like XO TUNED can,
 * so to keep track of clock state in ZLI separate status system has to be added.
 */
static void clock_event_zli_handler(nrfx_clock_xo_event_type_t event)
{
	unsigned int key = irq_lock();

	switch (event) {
#if NRF_CLOCK_HAS_XO_TUNE
	case NRFX_CLOCK_XO_EVT_XO_TUNED:
		atomic_set(&zli_clock_status, CLOCK_CONTROL_STATUS_ON);
		break;
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED:
		atomic_set(&zli_clock_status, CLOCK_CONTROL_STATUS_STARTED);
		break;
	case NRFX_CLOCK_XO_EVT_XO_TUNE_ERROR:
	case NRFX_CLOCK_XO_EVT_XO_TUNE_FAILED:
		/* No processing needed */
		break;
#else
	/* HFCLK started should be used only if tune operation is done implicitly. */
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED:
		atomic_set(&zli_clock_status, CLOCK_CONTROL_STATUS_ON);
		break;
#endif

#if NRF_CLOCK_HAS_PLL
	case NRFX_CLOCK_XO_EVT_PLL_STARTED:
		/* No processing needed. */
		break;
#endif
	}

	if (!(xo_users & XO_USER_BT)) {
		atomic_set(&zli_clock_status, CLOCK_CONTROL_STATUS_OFF);
	}

	irq_unlock(key);
}

static void clock_event_handler(nrfx_clock_xo_event_type_t event)
{
	if (xo_users & XO_USER_BT) {
		clock_event_zli_handler(event);
	}

	if (!(xo_users & XO_USER_GENERIC)) {
		return;
	}

	switch (event) {
#if NRF_CLOCK_HAS_XO_TUNE
	case NRFX_CLOCK_XO_EVT_XO_TUNED:
		clk_ready_event_handle();
		break;
	case NRFX_CLOCK_XO_EVT_XO_TUNE_ERROR:
	case NRFX_CLOCK_XO_EVT_XO_TUNE_FAILED:
		/* No processing needed. */
		break;
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED:
		/* HFCLK is stable after XOTUNED event.
		 * HFCLK_STARTED means only that clock has been started.
		 */
		clk_started_event_handle(CLOCK_DEVICE_XO);
		break;
#else
	/* HFCLK started should be used only if tune operation is done implicitly. */
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED: {
		clk_ready_event_handle();
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

#if CONFIG_CLOCK_CONTROL_NRF_ONOFF

static void onoff_started_callback(const struct device *dev, clock_control_subsys_t sys,
				   void *user_data)
{
	ARG_UNUSED(sys);

	onoff_notify_fn notify = user_data;

	notify(&((xo_clock_data_t *)dev->data)->mgr_started, 0);
}

/* There are two set of onoff_managers, internal clients and counter counting active clients for
 * each. First manager (lower quality) has start and stop implemented to start and stop xo and
 * completion callback when HFCLKSTARTED event is received. Second manager (higher quality) has
 * empty start and stop functions and on callback when XO_TUNED event is received. User always
 * requests one (primary set) and secondary set is handled only if it is the first request or last
 * release.
 */
static bool is_hi_precision(const struct nrf_clock_spec *spec)
{
	return spec == NULL || spec->precision == NRF_CLOCK_CONTROL_PRECISION_HIGH;
}

static struct onoff_manager *get_manager(xo_managers_t index)
{
	switch (index) {
	case XO_MGRS_READY:
		return &((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr;
	case XO_MGRS_STARTED:
		return &((xo_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr_started;
	default:
		return NULL;
	}
}

static int api_request(const struct device *dev, const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	xo_clock_data_t *xo_dev_data = ((xo_clock_data_t *)dev->data);
	int ret;
	uint32_t primary, secondary;

	atomic_set(&xo_dev_data->request_pending, 1);

	if (is_hi_precision(spec)) {
		primary = XO_MGRS_READY;
		secondary = XO_MGRS_STARTED;
	} else {
		primary = XO_MGRS_STARTED;
		secondary = XO_MGRS_READY;
	}

	if (atomic_inc(&xo_dev_data->active_clients[primary]) == 0) {
		/* if it is the first hi precision request that request low precision mgr. */
		sys_notify_init_spinwait(&xo_dev_data->internal_cli[secondary].notify);
		/* That will trigger the start of hclk*/
		ret = onoff_request(get_manager(secondary), &xo_dev_data->internal_cli[secondary]);

		if (ret < 0) {
			atomic_dec(&xo_dev_data->active_clients[primary]);
			atomic_set(&xo_dev_data->request_pending, 0);
			return ret;
		}
	}

	ret = onoff_request(get_manager(primary), cli);

	if (ret < 0) {
		atomic_dec(&xo_dev_data->active_clients[primary]);
	}

	atomic_set(&xo_dev_data->request_pending, 0);
	return ret;
}

static int api_release(const struct device *dev, const struct nrf_clock_spec *spec)
{
	int ret;
	xo_clock_data_t *xo_dev_data = ((xo_clock_data_t *)dev->data);
	uint32_t primary, secondary;

	if (atomic_get(&xo_dev_data->request_pending)) {
		return -ENOTSUP;
	}

	if (is_hi_precision(spec)) {
		primary = XO_MGRS_READY;
		secondary = XO_MGRS_STARTED;
	} else {
		primary = XO_MGRS_STARTED;
		secondary = XO_MGRS_READY;
	}

	/* When clock state is STARTED but not yet READY,
	 * releasing XO_MGRS_READY will fail and XO_MGRS_STARTED will succeed
	 * if the actions were done in different order it would cause some managers left
	 * not released after everything is supposedly released
	 */
	if (atomic_dec(&xo_dev_data->active_clients[primary]) == 1) {
		ret = onoff_release(get_manager(XO_MGRS_READY));
		if (ret < 0) {
			atomic_inc(&xo_dev_data->active_clients[primary]);
			return ret;
		}
		ret = onoff_release(get_manager(XO_MGRS_STARTED));
		goto out;
	}

	ret = onoff_release(get_manager(primary));
out:
	if (ret < 0) {
		atomic_inc(&xo_dev_data->active_clients[primary]);
		return ret;
	}

	return ret;
}

static int api_cancel_or_release(const struct device *dev, const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	int ret;
	xo_clock_data_t *xo_dev_data = ((xo_clock_data_t *)dev->data);
	uint32_t primary, secondary;

	if (atomic_get(&xo_dev_data->request_pending)) {
		return -ENOTSUP;
	}

	if (is_hi_precision(spec)) {
		primary = XO_MGRS_READY;
		secondary = XO_MGRS_STARTED;
	} else {
		primary = XO_MGRS_STARTED;
		secondary = XO_MGRS_READY;
	}

	if (atomic_dec(&xo_dev_data->active_clients[primary]) == 1) {
		ret = onoff_cancel_or_release(get_manager(secondary),
					      &xo_dev_data->internal_cli[secondary]);
		if (ret < 0) {
			atomic_inc(&xo_dev_data->active_clients[primary]);
			return ret;
		}
	}

	ret = onoff_cancel_or_release(get_manager(primary), cli);

	if (ret < 0) {
		atomic_inc(&xo_dev_data->active_clients[primary]);
		return ret;
	}

	return ret;
}

static void onoff_empty_start(struct onoff_manager *manager, onoff_notify_fn notify)
{
	ARG_UNUSED(manager);
	ARG_UNUSED(notify);
}

static void onoff_empty_stop(struct onoff_manager *manager, onoff_notify_fn notify)
{
	ARG_UNUSED(manager);
	ARG_UNUSED(notify);
}

static void onoff_start(struct onoff_manager *manager, onoff_notify_fn notify)
{
	int err;
	common_clock_data_t *common_dev_data = CONTAINER_OF(manager, common_clock_data_t, mgr);
	xo_clock_data_t *xo_dev_data = CONTAINER_OF(common_dev_data, xo_clock_data_t, data);
	const struct device *dev = common_dev_data->dev;

	err = common_set_starting_state(&((common_clock_data_t *)dev->data)->flags,
					COMMON_CTX_ONOFF);
	if (err < 0) {
		notify(manager, err);
		return;
	}

	xo_dev_data->cb_started = onoff_started_callback;
	common_dev_data->cb = common_onoff_ready_callback;
	common_dev_data->user_data = notify;

	((common_clock_config_t *)dev->config)->start();
}

#endif /* CONFIG_CLOCK_CONTROL_NRF_ONOFF */

static int clk_init(const struct device *dev)
{
#if CONFIG_CLOCK_CONTROL_NRF_ONOFF
	int err;
	static const struct onoff_transitions transitions = {.start = onoff_start,
							     .stop = common_onoff_stop};
	static const struct onoff_transitions empty_transitions = {.start = onoff_empty_start,
								   .stop = onoff_empty_stop};
#endif

	if (nrfx_clock_xo_init(clock_event_handler) != 0) {
		return -EIO;
	}

	((common_clock_data_t *)dev->data)->dev = dev;

	common_connect_irq();
#if CONFIG_CLOCK_CONTROL_NRF_ONOFF
	err = onoff_manager_init(get_manager(XO_MGRS_READY), &transitions);
	if (err < 0) {
		return err;
	}

	err = onoff_manager_init(get_manager(XO_MGRS_STARTED), &empty_transitions);
	if (err < 0) {
		return err;
	}
#endif

	((common_clock_data_t *)dev->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(clock_control_nrf_xo, &nrfx_clock_xo_irq_handler);

DEVICE_API(nrf_clock_control, xo_clock_control_api) = {
	.std_api = {
		.on = common_api_blocking_start,
		.off = common_api_stop,
		.async_on = common_api_start,
		.get_status = common_api_get_status,
	},
#if CONFIG_CLOCK_CONTROL_NRF_ONOFF
	.request = api_request,
	.release = api_release,
	.cancel_or_release = api_cancel_or_release,
#endif
};

static xo_clock_data_t data;

static const common_clock_config_t config = {
	.start = generic_xo_start,
	.stop = generic_xo_stop,
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo), clk_init, NULL, &data, &config,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &xo_clock_control_api);
