/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

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

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static int blocking_on(struct onoff_manager *mgr, uint32_t timeout)
#else
static int blocking_on(const struct device *dev, uint32_t timeout)
#endif
{

	struct lll_clock_state state;
	int err;

	k_sem_init(&state.sem, 0, 1);
	sys_notify_init_callback(&state.cli.notify, clock_ready);
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_request(mgr, &state.cli);
#else
	err = nrf_clock_control_request(dev, NULL, &state.cli);
#endif
	if (err < 0) {
		return err;
	}

	return k_sem_take(&state.sem, K_MSEC(timeout));
}

int lll_clock_init(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
#endif

	sys_notify_init_spinwait(&lf_cli.notify);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	return onoff_request(mgr, &lf_cli);
#else
	return nrf_clock_control_request(DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk), NULL, &lf_cli);
#endif
}

int lll_clock_deinit(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);

	/* Cancel any ongoing request */
	(void)onoff_cancel(mgr, &lf_cli);

	return onoff_release(mgr);
#else
	return nrf_clock_control_cancel_or_release(DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk), NULL,
						   &lf_cli);
#endif
}

int lll_clock_wait(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr;
#endif
	static bool done;
	int err;

	if (done) {
		return 0;
	}
	done = true;

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	err = blocking_on(mgr, LFCLOCK_TIMEOUT_MS);
#else
	err = blocking_on(DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk), LFCLOCK_TIMEOUT_MS);
#endif
	if (err) {
		return err;
	}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_release(mgr);
#else
	err = nrf_clock_control_release(DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk), NULL);
#endif
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
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
#endif
	int err;

	atomic_inc(&hf_refcnt);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = blocking_on(mgr, HFCLOCK_TIMEOUT_MS);
#else
	const struct device *clk_dev = DEVICE_DT_GET_ONE(COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
							       (nordic_nrf_clock_hfclk),
							       (nordic_nrf_clock_xo)));

	err = blocking_on(clk_dev, HFCLOCK_TIMEOUT_MS);
#endif
	if (err >= 0) {
		DEBUG_RADIO_XTAL(1);
	}

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

	z_nrf_clock_bt_ctlr_hf_release();
	DEBUG_RADIO_XTAL(0);

	return 0;
}

uint8_t lll_clock_sca_local_get(void)
{
#ifdef CONFIG_CLOCK_CONTROL_NRF
	return CLOCK_CONTROL_NRF_K32SRC_ACCURACY;
#else
	return DT_ENUM_IDX(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_lfclk),
			   k32src_accuracy_ppm);
#endif
}

uint32_t lll_clock_ppm_local_get(void)
{
#ifdef CONFIG_CLOCK_CONTROL_NRF
	return sca_ppm_lut[CLOCK_CONTROL_NRF_K32SRC_ACCURACY];
#else
	return sca_ppm_lut[DT_ENUM_IDX(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_lfclk),
				       k32src_accuracy_ppm)];
#endif
}

uint32_t lll_clock_ppm_get(uint8_t sca)
{
	return sca_ppm_lut[sca];
}
