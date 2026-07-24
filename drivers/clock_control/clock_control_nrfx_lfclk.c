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
#include "clock_control_nrfx_common.h"
#include <nrfx_clock_lfclk.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(clock_control_lfclk, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_lfclk

#define CLOCK_DEVICE_LFCLK DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk)
#define CLOCK_NODE_LFCLK DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_lfclk)

#if NRF_CLOCK_HAS_HFCLK
#define CLOCK_DEVICE_HF DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk)
#else /* NRF_CLOCK_HAS_XO */
#define CLOCK_DEVICE_HF DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)
#endif

#if (!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, rc) &&                                        \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, xtal) &&                                   \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth) &&                                  \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_low_swing) &&                          \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_full_swing))
#error "Unsupported LFCLK source configured in devicetree"
#endif

#if DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, rc)
#define K32SRC NRF_CLOCK_LFCLK_RC
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, xtal)
#define K32SRC NRF_CLOCK_LFCLK_XTAL
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth)
#define K32SRC NRF_CLOCK_LFCLK_SYNTH
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_low_swing)
#define K32SRC NRF_CLOCK_LFCLK_XTAL_LOW_SWING
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_full_swing)
#define K32SRC NRF_CLOCK_LFCLK_XTAL_FULL_SWING
#endif

#if (DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth))
/* Client to request HFXO to synthesize low frequency clock. */
static struct onoff_client lfsynth_cli;
#endif

static inline void anomaly_132_workaround(void)
{
#if (CONFIG_NRF52_ANOMALY_132_DELAY_US - 0)
	static bool once;

	if (!once) {
		k_busy_wait(CONFIG_NRF52_ANOMALY_132_DELAY_US);
		once = true;
	}
#endif
}

static void lfclk_start(void)
{
	if (IS_ENABLED(CONFIG_NRF52_ANOMALY_132_WORKAROUND)) {
		anomaly_132_workaround();
	}

#if (DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth))
	sys_notify_init_spinwait(&lfsynth_cli.notify);

	(void)nrf_clock_control_request(CLOCK_DEVICE_HF, NULL, &lfsynth_cli);
#endif

	nrfx_clock_lfclk_start();
}

static void lfclk_stop(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRFX_DRIVER_CALIBRATION)) {
		z_nrf_clock_calibration_lfclk_stopped();
	}

	nrfx_clock_lfclk_stop();

#if (DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth))

	(void)nrf_clock_control_cancel_or_release(CLOCK_DEVICE_HF, NULL, &lfsynth_cli);
#endif
}

/** @brief Wait for LF clock availability or stability.
 *
 * If LF clock source is SYNTH or RC then there is no distinction between
 * availability and stability. In case of XTAL source clock, system is initially
 * starting RC and then seamlessly switches to XTAL. Running RC means clock
 * availability and running target source means stability, That is because
 * significant difference in startup time (<1ms vs >200ms).
 *
 * In order to get event/interrupt when RC is ready (allowing CPU sleeping) two
 * stage startup sequence is used. Initially, LF source is set to RC and when
 * LFSTARTED event is handled it is reconfigured to the target source clock.
 * This approach is implemented in nrfx_clock_lfclk driver and utilized here.
 *
 * @param mode Start mode.
 */
static void lfclk_spinwait(enum nrf_lfclk_start_mode mode)
{
	static const nrf_clock_lfclk_t target_type =
		/* For sources XTAL, EXT_LOW_SWING, and EXT_FULL_SWING,
		 * NRF_CLOCK_LFCLK_XTAL is returned as the type of running clock.
		 */
		(DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, xtal) ||
		 DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_low_swing) ||
		 DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_full_swing))
			? NRF_CLOCK_LFCLK_XTAL
			: K32SRC;
	nrf_clock_lfclk_t type;

	if ((mode == CLOCK_CONTROL_NRF_LF_START_AVAILABLE) &&
	    (target_type == NRF_CLOCK_LFCLK_XTAL) &&
	    (nrf_clock_lf_srccopy_get(NRF_CLOCK) == K32SRC)) {
		/* If target clock source is using XTAL then due to two-stage
		 * clock startup sequence, RC might already be running.
		 * It can be determined by checking current LFCLK source. If it
		 * is set to the target clock source then it means that RC was
		 * started.
		 */
		return;
	}

	bool isr_mode = k_is_in_isr() || k_is_pre_kernel();
	int key = isr_mode ? irq_lock() : 0;

	if (!isr_mode) {
		nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_LF_STARTED_MASK);
	}

	while (!(nrfx_clock_lfclk_running_check((void *)&type) &&
		 ((type == target_type) || (mode == CLOCK_CONTROL_NRF_LF_START_AVAILABLE)))) {
		/* Synth source start is almost instant and LFCLKSTARTED may
		 * happen before calling idle. That would lead to deadlock.
		 */
		if (!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth)) {
			if (isr_mode || !IS_ENABLED(CONFIG_MULTITHREADING)) {
				k_cpu_atomic_idle(key);
			} else {
				k_msleep(1);
			}
		}

		/* Clock interrupt is locked, LFCLKSTARTED is handled here. */
		if ((target_type == NRF_CLOCK_LFCLK_XTAL) &&
		    (nrf_clock_lf_src_get(NRF_CLOCK) == NRF_CLOCK_LFCLK_RC) &&
		    nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED)) {
			nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED);
			nrf_clock_lf_src_set(NRF_CLOCK, K32SRC);

			/* Clear pending interrupt, otherwise new clock event
			 * would not wake up from idle.
			 */
			common_clear_pending_irq();
			nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
		}
	}

	if (isr_mode) {
		irq_unlock(key);
	} else {
		nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_LF_STARTED_MASK);
	}
}

static void clock_event_handler(nrfx_clock_lfclk_evt_type_t event)
{
	switch (event) {
	case NRFX_CLOCK_LFCLK_EVT_LFCLK_STARTED:
		if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRFX_DRIVER_CALIBRATION)) {
			z_nrf_clock_calibration_lfclk_started();
		}
		common_clkstarted_handle(CLOCK_DEVICE_LFCLK);
		break;
#if NRF_CLOCK_HAS_CALIBRATION || NRF_LFRC_HAS_CALIBRATION
	case NRFX_CLOCK_LFCLK_EVT_CAL_DONE:
		if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRFX_DRIVER_CALIBRATION)) {
			z_nrf_clock_calibration_done_handler();
		} else {
			/* Should not happen when calibration is disabled. */
			__ASSERT_NO_MSG(false);
		}
		break;
#endif
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
}

static void onoff_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int err;

	err = common_async_start(CLOCK_DEVICE_LFCLK, common_onoff_started_callback, notify,
				 COMMON_CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void onoff_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	int res;

	res = common_stop(CLOCK_DEVICE_LFCLK, COMMON_CTX_ONOFF);
	notify(mgr, res);
}

void z_nrf_clock_control_lf_on(enum nrf_lfclk_start_mode start_mode)
{
	static atomic_t on;
	static struct onoff_client cli;

	if (atomic_set(&on, 1) == 0) {
		int err;
		struct onoff_manager *mgr = &((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->mgr;

		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(mgr, &cli);
		__ASSERT_NO_MSG(err >= 0);
	}

	/* In case of simulated board leave immediately. */
	if (IS_ENABLED(CONFIG_SOC_SERIES_BSIM_NRFXX)) {
		return;
	}

	switch (start_mode) {
	case CLOCK_CONTROL_NRF_LF_START_AVAILABLE:
	case CLOCK_CONTROL_NRF_LF_START_STABLE:
		lfclk_spinwait(start_mode);
		break;

	case CLOCK_CONTROL_NRF_LF_START_NOWAIT:
		break;

	default:
		__ASSERT_NO_MSG(false);
	}
}

static int api_start(const struct device *dev, clock_control_subsys_t subsys, clock_control_cb_t cb,
		     void *user_data)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return common_async_start(CLOCK_DEVICE_LFCLK, cb, user_data, COMMON_CTX_API);
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

	return common_stop(CLOCK_DEVICE_LFCLK, COMMON_CTX_API);
}

static enum clock_control_status api_get_status(const struct device *dev,
						clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	return COMMON_GET_STATUS(((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->flags);
}

static int api_request(const struct device *dev, const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_request(&((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->mgr, cli);
}

static int api_release(const struct device *dev, const struct nrf_clock_spec *spec)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_release(&((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->mgr);
}

static int api_cancel_or_release(const struct device *dev, const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(dev);

	return onoff_cancel_or_release(&((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->mgr,
				       cli);
}

static int clk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;
	static const struct onoff_transitions transitions = {.start = onoff_start,
							     .stop = onoff_stop};

	common_connect_irq();

	if (nrfx_clock_lfclk_init(clock_event_handler) != 0) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRFX_DRIVER_CALIBRATION)) {
		z_nrf_clock_calibration_init();
	}

	err = onoff_manager_init(&((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->mgr,
				 &transitions);
	if (err < 0) {
		return err;
	}

	((common_clock_data_t *)CLOCK_DEVICE_LFCLK->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_lfclk, &nrfx_clock_lfclk_irq_handler);

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

	.start = lfclk_start,
	.stop = lfclk_stop,
};

DEVICE_DT_DEFINE(CLOCK_NODE_LFCLK, clk_init, NULL, &data, &config, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_api);
