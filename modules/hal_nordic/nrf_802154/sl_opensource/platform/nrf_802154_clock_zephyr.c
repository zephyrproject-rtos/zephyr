/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_802154_config.h>
#include <platform/nrf_802154_clock.h>

#include <stddef.h>

#include <compiler_abstraction.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <nrf_sys_event.h>
#include <hal/nrf_egu.h>

/**
 * The implementation uses EGU to de-escalate execution context from ZLI to a regular interrupt
 * to ensure that Zephyr APIs can be used safely.
 *
 * Both the nrf_802154_clock_hfclk_start() and nrf_802154_clock_hfclk_stop() can potentially be
 * called from ZLI and non-ZLI contexts and consecutive calls are not guaranteed to be alternating.
 *
 * For example, it is possible that _stop() may be called multiple times in succession and the
 * same thing applies to _start(). What is known however is that the last call always takes
 * the precedence.
 */

#define SWI_INT NRFX_CONCAT_2(NRF_EGU_INT_TRIGGERED, NRF_802154_SL_EGU_CLOCK_CHANNEL_NO)
#define SWI_TASK NRFX_CONCAT_2(NRF_EGU_TASK_TRIGGER, NRF_802154_SL_EGU_CLOCK_CHANNEL_NO)
#define SWI_EVENT NRFX_CONCAT_2(NRF_EGU_EVENT_TRIGGERED, NRF_802154_SL_EGU_CLOCK_CHANNEL_NO)

#define CLOCK_NONE    0u
#define CLOCK_REQUEST 1u
#define CLOCK_RELEASE 2u

static bool hfclk_is_running;
static bool enabled;
static atomic_t request = CLOCK_NONE;

/* Forward declarations. */
static void hfclk_start(void);
static void hfclk_stop(void);

void nrf_802154_clock_init(void)
{
#ifdef NRF54L_SERIES
	uint32_t clock_latency_us = z_nrf_clock_bt_ctlr_hf_get_startup_time_us();

	nrf_802154_clock_hfclk_latency_set(clock_latency_us);
#endif

	nrf_egu_int_enable(NRF_802154_EGU_INSTANCE, SWI_INT);
}

void nrf_802154_clock_deinit(void)
{
	nrf_egu_int_disable(NRF_802154_EGU_INSTANCE, SWI_INT);
}

bool nrf_802154_clock_hfclk_is_running(void)
{
	return hfclk_is_running;
}


static struct onoff_client hfclk_cli;

static void hfclk_on_callback(struct onoff_manager *mgr,
			      struct onoff_client  *cli,
			      uint32_t state,
			      int res)
{
	hfclk_is_running = true;
	nrf_802154_clock_hfclk_ready();
}

void nrf_802154_sl_clock_swi_irq_handler(void)
{
	if (nrf_egu_event_check(NRF_802154_EGU_INSTANCE, SWI_EVENT)) {
		nrf_egu_event_clear(NRF_802154_EGU_INSTANCE, SWI_EVENT);

		atomic_val_t previous = atomic_set(&request, CLOCK_NONE);

		__ASSERT_NO_MSG(previous == CLOCK_REQUEST || previous == CLOCK_RELEASE);

		switch (previous) {
		case CLOCK_REQUEST:
			hfclk_start();
			break;

		case CLOCK_RELEASE:
			hfclk_stop();
			break;

		default:
			break;
		}
	}
}

void nrf_802154_clock_hfclk_start(void)
{
	atomic_set(&request, CLOCK_REQUEST);
	nrf_egu_task_trigger(NRF_802154_EGU_INSTANCE, SWI_TASK);
}

void nrf_802154_clock_hfclk_stop(void)
{
	atomic_set(&request, CLOCK_RELEASE);
	nrf_egu_task_trigger(NRF_802154_EGU_INSTANCE, SWI_TASK);
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static void hfclk_start(void)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	__ASSERT_NO_MSG(mgr != NULL);

	sys_notify_init_callback(&hfclk_cli.notify, hfclk_on_callback);

	if (!enabled) {
		unsigned int key = irq_lock();

		/*
		 * todo: replace constlat request with PM policy API when
		 * controlling the event latency becomes possible.
		 */
		if (IS_ENABLED(CONFIG_NRF_802154_CONSTLAT_CONTROL)) {
			nrf_sys_event_request_global_constlat();
		}

		int ret = onoff_request(mgr, &hfclk_cli);

		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;

		irq_unlock(key);
	}

	enabled = true;
}

static void hfclk_stop(void)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	__ASSERT_NO_MSG(mgr != NULL);

	if (enabled) {
		unsigned int key = irq_lock();

		int ret = onoff_cancel_or_release(mgr, &hfclk_cli);

		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;

		if (IS_ENABLED(CONFIG_NRF_802154_CONSTLAT_CONTROL)) {
			nrf_sys_event_release_global_constlat();
		}

		hfclk_is_running = false;

		irq_unlock(key);
	}

	enabled = false;
}

#elif DT_NODE_HAS_STATUS(DT_NODELABEL(hfxo), okay) && \
	DT_NODE_HAS_COMPAT(DT_NODELABEL(hfxo), nordic_nrf54h_hfxo)

static void hfclk_start(void)
{
	if (!enabled) {
		unsigned int key = irq_lock();

		sys_notify_init_callback(&hfclk_cli.notify, hfclk_on_callback);
		int ret = nrf_clock_control_request(DEVICE_DT_GET(DT_NODELABEL(hfxo)),
						    NULL, &hfclk_cli);

		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;

		irq_unlock(key);
	}

	enabled = true;
}

static void hfclk_stop(void)
{
	if (enabled) {
		unsigned int key = irq_lock();

		int ret = nrf_clock_control_cancel_or_release(DEVICE_DT_GET(DT_NODELABEL(hfxo)),
							      NULL, &hfclk_cli);

		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;

		irq_unlock(key);
	}

	enabled = false;
}

#endif
