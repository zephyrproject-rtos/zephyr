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
#if defined(CONFIG_CLOCK_CONTROL_NRF)
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#elif !defined(NRF54H_SERIES)
#error No implementation to start or stop HFCLK due to missing clock_control.
#endif

static bool hfclk_is_running;

void nrf_802154_clock_init(void)
{
	/* Intentionally empty. */
}

void nrf_802154_clock_deinit(void)
{
	/* Intentionally empty. */
}

bool nrf_802154_clock_hfclk_is_running(void)
{
	return hfclk_is_running;
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)

static struct onoff_client hfclk_cli;

static void hfclk_on_callback(struct onoff_manager *mgr,
			      struct onoff_client  *cli,
			      uint32_t state,
			      int res)
{
	hfclk_is_running = true;
	nrf_802154_clock_hfclk_ready();
}

void nrf_802154_clock_hfclk_start(void)
{
	int ret;
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	__ASSERT_NO_MSG(mgr != NULL);

	sys_notify_init_callback(&hfclk_cli.notify, hfclk_on_callback);

	ret = onoff_request(mgr, &hfclk_cli);
	__ASSERT_NO_MSG(ret >= 0);
}

void nrf_802154_clock_hfclk_stop(void)
{
	int ret;
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	__ASSERT_NO_MSG(mgr != NULL);

	ret = onoff_cancel_or_release(mgr, &hfclk_cli);
	__ASSERT_NO_MSG(ret >= 0);
	hfclk_is_running = false;
}

#elif defined(NRF54H_SERIES)

#define NRF_LRCCONF_RADIO_PD NRF_LRCCONF010
/* HF clock time to ramp-up. */
#define MAX_HFXO_RAMP_UP_TIME_US 550

static void hfclk_started_timer_handler(struct k_timer *dummy)
{
	hfclk_is_running = true;
	nrf_802154_clock_hfclk_ready();
}

K_TIMER_DEFINE(hfclk_started_timer, hfclk_started_timer_handler, NULL);

void nrf_802154_clock_hfclk_start(void)
{
	/* Use register directly, there is no support for that task in nrf_lrcconf_task_trigger.
	 * This code might cause troubles if there are other HFXO users in this CPU.
	 */
	NRF_LRCCONF_RADIO_PD->EVENTS_HFXOSTARTED = 0x0;
	NRF_LRCCONF_RADIO_PD->TASKS_REQHFXO = 0x1;

	k_timer_start(&hfclk_started_timer, K_USEC(MAX_HFXO_RAMP_UP_TIME_US), K_NO_WAIT);
}

void nrf_802154_clock_hfclk_stop(void)
{
	/* Use register directly, there is no support for that task in nrf_lrcconf_task_trigger.
	 * This code might cause troubles if there are other HFXO users in this CPU.
	 */
	NRF_LRCCONF_RADIO_PD->TASKS_STOPREQHFXO = 0x1;
	NRF_LRCCONF_RADIO_PD->EVENTS_HFXOSTARTED = 0x0;

	hfclk_is_running = false;
}

#endif
