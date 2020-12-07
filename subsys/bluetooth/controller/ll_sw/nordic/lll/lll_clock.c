/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <device.h>

#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_clock
#include "common/log.h"
#include "hal/debug.h"

/* Clock setup timeouts are unlikely, below values are experimental */
#define LFCLOCK_TIMEOUT_MS 500
#define HFCLOCK_TIMEOUT_MS 2

static uint16_t const sca_ppm_lut[] = {500, 250, 150, 100, 75, 50, 30, 20};

struct lll_clock_state {
	struct onoff_client cli;
	struct k_sem sem;
};

static struct onoff_client lf_cli;
static atomic_val_t hf_refcnt;

static void clock_ready(struct onoff_manager *mgr, struct onoff_client *cli,
			uint32_t state, int res)
{
	struct lll_clock_state *clk_state =
		CONTAINER_OF(cli, struct lll_clock_state, cli);

	k_sem_give(&clk_state->sem);
}

static int blocking_on(struct onoff_manager *mgr, uint32_t timeout)
{

	struct lll_clock_state state;
	int err;

	k_sem_init(&state.sem, 0, 1);
	sys_notify_init_callback(&state.cli.notify, clock_ready);
	err = onoff_request(mgr, &state.cli);
	if (err < 0) {
		return err;
	}

	return k_sem_take(&state.sem, K_MSEC(timeout));
}

int lll_clock_init(void)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);

	sys_notify_init_spinwait(&lf_cli.notify);

	return onoff_request(mgr, &lf_cli);
}

int lll_clock_wait(void)
{
	struct onoff_manager *mgr;
	static bool done;
	int err;

	if (done) {
		return 0;
	}
	done = true;

	mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	err = blocking_on(mgr, LFCLOCK_TIMEOUT_MS);
	if (err) {
		return err;
	}

	err = onoff_release(mgr);
	if (err != ONOFF_STATE_ON) {
		return -EIO;
	}

	return 0;
}

int lll_hfclock_on(void)
{
	if (atomic_inc(&hf_refcnt) > 0) {
		return 0;
	}

	z_nrf_clock_bt_ctlr_hf_request();
	DEBUG_RADIO_XTAL(1);

	return 0;
}

int lll_hfclock_on_wait(void)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	int err;

	atomic_inc(&hf_refcnt);

	err = blocking_on(mgr, HFCLOCK_TIMEOUT_MS);
	if (err >= 0) {
		DEBUG_RADIO_XTAL(1);
	}

	return err;
}

int lll_hfclock_off(void)
{
	if (hf_refcnt < 1) {
		return -EALREADY;
	}

	if (atomic_dec(&hf_refcnt) > 1) {
		return 0;
	}

	z_nrf_clock_bt_ctlr_hf_release();
	DEBUG_RADIO_XTAL(0);

	return 0;
}

uint8_t lll_clock_sca_local_get(void)
{
	return CLOCK_CONTROL_NRF_K32SRC_ACCURACY;
}

uint32_t lll_clock_ppm_local_get(void)
{
	return sca_ppm_lut[CLOCK_CONTROL_NRF_K32SRC_ACCURACY];
}

uint32_t lll_clock_ppm_get(uint8_t sca)
{
	return sca_ppm_lut[sca];
}
