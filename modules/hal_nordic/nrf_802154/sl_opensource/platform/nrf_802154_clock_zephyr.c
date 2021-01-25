/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <platform/clock/nrf_802154_clock.h>

#include <stddef.h>

#include <compiler_abstraction.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/clock_control.h>

static bool hfclk_is_running;
static bool lfclk_is_running;
static struct onoff_client hfclk_cli;
static struct onoff_client lfclk_cli;

void nrf_802154_clock_init(void)
{
	/* Intentionally empty. */
}

void nrf_802154_clock_deinit(void)
{
	/* Intentionally empty. */
}

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

bool nrf_802154_clock_hfclk_is_running(void)
{
	return hfclk_is_running;
}

static void lfclk_on_callback(struct onoff_manager *mgr,
			      struct onoff_client  *cli,
			      uint32_t state,
			      int res)
{
	lfclk_is_running = true;
	nrf_802154_clock_lfclk_ready();
}

void nrf_802154_clock_lfclk_start(void)
{
	int ret;
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);

	__ASSERT_NO_MSG(mgr != NULL);

	sys_notify_init_callback(&lfclk_cli.notify, lfclk_on_callback);

	ret = onoff_request(mgr, &lfclk_cli);
	__ASSERT_NO_MSG(ret >= 0);
}

void nrf_802154_clock_lfclk_stop(void)
{
	int ret;
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);

	__ASSERT_NO_MSG(mgr != NULL);

	ret = onoff_cancel_or_release(mgr, &lfclk_cli);
	__ASSERT_NO_MSG(ret >= 0);
	lfclk_is_running = false;
}

bool nrf_802154_clock_lfclk_is_running(void)
{
	return lfclk_is_running;
}

__WEAK void nrf_802154_clock_hfclk_ready(void)
{
	/* Intentionally empty. */
}

__WEAK void nrf_802154_clock_lfclk_ready(void)
{
	/* Intentionally empty. */
}
