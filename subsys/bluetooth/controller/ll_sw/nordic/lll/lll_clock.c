/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>

#if defined(CONFIG_CLOCK_CONTROL_NRF)
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif /* CONFIG_CLOCK_CONTROL_NRF */

#include "hal/debug.h"

/* LF (XO, RC) clock setup timeout.
 * LF settling time can vary on the type of the source.
 */
#define LFCLOCK_TIMEOUT_MS 1000U

/* HFXO clock setup timeout */
#if DT_NODE_HAS_PROP(DT_NODELABEL(hfxo), startup_time_us)
/* Use value from devicetree plus an additional margin */
#define HFCLOCK_TIMEOUT_MS (DIV_ROUND_UP(DT_PROP(DT_NODELABEL(hfxo), startup_time_us), 1000U) + 3U)
#else
/* Use manually tested (peripheral role) value, includes added additional margin */
#define HFCLOCK_TIMEOUT_MS 5U
#endif

static uint16_t const sca_ppm_lut[] = {500, 250, 150, 100, 75, 50, 30, 20};

static atomic_val_t hf_refcnt;

#if defined(CONFIG_CLOCK_CONTROL_NRF)
struct lll_clock_state {
	struct onoff_client cli;
	struct k_sem sem;
};

static struct onoff_client lf_cli;

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

int lll_clock_deinit(void)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);

	/* Cancel any ongoing request */
	(void)onoff_cancel(mgr, &lf_cli);

	return onoff_release(mgr);
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

#else /* !CONFIG_CLOCK_CONTROL_NRF */
int lll_clock_init(void)
{
	/* FIXME: Add implementation alternative for clock control */
	return 0;
}

int lll_clock_deinit(void)
{
	/* FIXME: Add implementation alternative for clock control */
	return 0;
}

int lll_clock_wait(void)
{
	/* FIXME: Add implementation alternative for clock control */
	return 0;
}
#endif /* !CONFIG_CLOCK_CONTROL_NRF */

int lll_hfclock_on(void)
{
	if (atomic_inc(&hf_refcnt) > 0) {
		return 0;
	}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	z_nrf_clock_bt_ctlr_hf_request();

#else /* !CONFIG_CLOCK_CONTROL_NRF */
	/* FIXME: Add implementation alternative for clock control */
	NRF_CLOCK->TASKS_XOSTART = 1U;
#endif /* !CONFIG_CLOCK_CONTROL_NRF */

	DEBUG_RADIO_XTAL(1);

	return 0;
}

int lll_hfclock_on_wait(void)
{
	int err;

	atomic_inc(&hf_refcnt);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	err = blocking_on(mgr, HFCLOCK_TIMEOUT_MS);
	if (err >= 0) {
		DEBUG_RADIO_XTAL(1);
	}

#else /* !CONFIG_CLOCK_CONTROL_NRF */
	/* FIXME: Add implementation alternative for clock control */
	err = 0U;
#endif /* !CONFIG_CLOCK_CONTROL_NRF */

	return err;
}

int lll_hfclock_off(void)
{
	if (atomic_get(&hf_refcnt) < 1) {
		return -EALREADY;
	}

	if (atomic_dec(&hf_refcnt) > 1) {
		return 0;
	}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	z_nrf_clock_bt_ctlr_hf_release();

#else /* !CONFIG_CLOCK_CONTROL_NRF */
	/* FIXME: Add implementation alternative for clock control */
	NRF_CLOCK->TASKS_XOSTOP = 1U;
#endif /* !CONFIG_CLOCK_CONTROL_NRF */

	DEBUG_RADIO_XTAL(0);

	return 0;
}

uint8_t lll_clock_sca_local_get(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	return CLOCK_CONTROL_NRF_K32SRC_ACCURACY;

#else /* !CONFIG_CLOCK_CONTROL_NRF */
	/* FIXME: Add implementation alternative for clock control */
	/* For time being, default to 50 ppm accuracy when clock control is not available */
	#define LLL_CLOCK_SCA_DEFAULT 5

	return LLL_CLOCK_SCA_DEFAULT;
#endif /* !CONFIG_CLOCK_CONTROL_NRF */

}

uint32_t lll_clock_ppm_local_get(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	return sca_ppm_lut[CLOCK_CONTROL_NRF_K32SRC_ACCURACY];

#else /* !CONFIG_CLOCK_CONTROL_NRF */
	/* For time being, default to 50 ppm accuracy when clock control is not available */
	return sca_ppm_lut[LLL_CLOCK_SCA_DEFAULT];
#endif /* !CONFIG_CLOCK_CONTROL_NRF */
}

uint32_t lll_clock_ppm_get(uint8_t sca)
{
	return sca_ppm_lut[sca];
}
