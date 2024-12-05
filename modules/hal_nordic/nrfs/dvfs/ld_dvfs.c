/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ld_dvfs.h"

#include <hal/nrf_hsfll.h>
#include <hal/nrf_ramc.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LD_DVFS_LIB, CONFIG_LOCAL_DOMAIN_DVFS_LIB_LOG_LEVEL);

#define TRANSIENT_ZBB_ABB_SLOT 0
#define CURR_TARG_ABB_SLOT 1
#define LD_ABB_CLR_ZBB 0
/* TODO: this values needs to be provided by HW team */
/* for now reset value will be used */
#define LD_ABB_CTRL4_NORMAL_OPERATION 0x10800UL
#define LD_ABB_CTRL4_TRANSITION_OPERATION 0x10800UL

/*
 * wait max 500ms with 10us intervals for hsfll freq change event
 */
#define HSFLL_FREQ_CHANGE_MAX_DELAY_MS 500UL
#define HSFLL_FREQ_CHANGE_CHECK_INTERVAL_US 10
#define HSFLL_FREQ_CHANGE_CHECK_MAX_ATTEMPTS                                                       \
	((HSFLL_FREQ_CHANGE_MAX_DELAY_MS) * (USEC_PER_MSEC) / (HSFLL_FREQ_CHANGE_CHECK_INTERVAL_US))

#define ABB_STATUS_CHANGE_MAX_DELAY_MS 5000UL
#define ABB_STATUS_CHANGE_CHECK_INTERVAL_US 10
#define ABB_STATUS_CHANGE_CHECK_MAX_ATTEMPTS                                                       \
	((ABB_STATUS_CHANGE_MAX_DELAY_MS) * (USEC_PER_MSEC) / (ABB_STATUS_CHANGE_CHECK_INTERVAL_US))

void ld_dvfs_init(void)
{
#if defined(NRF_SECURE)

	const struct dvfs_oppoint_data *opp_data = get_dvfs_oppoint_data(DVFS_FREQ_HIGH);

#if !defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_TEST)
	/* TODO: Change to NRFX Hal function when available. */
	NRF_ABB->TRIM.RINGO[CURR_TARG_ABB_SLOT]	       = opp_data->abb_ringo;
	NRF_ABB->TRIM.LOCKRANGE[CURR_TARG_ABB_SLOT]    = opp_data->abb_lockrange;
	NRF_ABB->TRIM.PVTMONCYCLES[CURR_TARG_ABB_SLOT] = opp_data->abb_pvtmoncycles;

	NRF_APPLICATION_ABB->TRIM.RINGO[CURR_TARG_ABB_SLOT]	   = opp_data->abb_ringo;
	NRF_APPLICATION_ABB->TRIM.LOCKRANGE[CURR_TARG_ABB_SLOT]	   = opp_data->abb_lockrange;
	NRF_APPLICATION_ABB->TRIM.PVTMONCYCLES[CURR_TARG_ABB_SLOT] = opp_data->abb_pvtmoncycles;
#endif
#endif
}

void ld_dvfs_clear_zbb(void)
{
#if defined(NRF_SECURE)
#if !defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_TEST)
	/* TODO: Change to NRFX Hal function when available. */
	NRF_ABB->CONFIG.CTRL1 &= ~(ABB_CONFIG_CTRL1_MODE_Msk);
	NRF_APPLICATION_ABB->CONFIG.CTRL1 &= ~(ABB_CONFIG_CTRL1_MODE_Msk);
#endif
#endif
}

#if defined(NRF_SECURE)

#define DOWNSCALE_SAFETY_TIMEOUT (K_USEC(CONFIG_NRFS_LOCAL_DOMAIN_DOWNSCALE_SAFETY_TIMEOUT_US))

atomic_t increased_power_consumption;

/**
 * @brief Secure domain needs to check if downscale is done in defined time
 *        window. This is needed to avoid battery drain if dvfs procedure
 *        takes to much time (some failure?).
 */
__weak void ld_dvfs_secure_downscale_timeout(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	LOG_ERR("Downscale timeout expired, reset board.");
	atomic_set(&increased_power_consumption, 0);
}

K_TIMER_DEFINE(dvfs_downscale_secure_timer, ld_dvfs_secure_downscale_timeout, NULL);

/**
 * @brief Secure domain starts increased power consumption, needed by dvfs sequence.
 *        This function can be reimplemented in other module if needed.
 */
__weak void ld_dvfs_secure_start_increased_power_consumption(void)
{
	LOG_DBG("Start increased power consumption for DVFS sequence and start safety timer.");
	k_timer_start(&dvfs_downscale_secure_timer, DOWNSCALE_SAFETY_TIMEOUT, K_NO_WAIT);
	atomic_set(&increased_power_consumption, 1);

	volatile uint8_t idle_counter = 0;

	while (atomic_get(&increased_power_consumption)) {
		if (idle_counter < 100) {
			k_yield();
			idle_counter++;
		} else {
			idle_counter = 0;
			k_usleep(1);
		}
	}
}

/**
 * @brief Secure domain stops increased power consumption at the end of downscale.
 *        This function can be reimplemented in other module if needed.
 */
__weak void ld_dvfs_secure_stop_increased_power_consumption(void)
{
	LOG_DBG("Stop increased power consumption for DVFS sequence.");
	k_timer_stop(&dvfs_downscale_secure_timer);
	atomic_set(&increased_power_consumption, 0);
}

#endif

void ld_dvfs_configure_abb_for_transition(enum dvfs_frequency_setting transient_opp,
					  enum dvfs_frequency_setting curr_targ_opp)
{
#if defined(NRF_SECURE)
	const struct dvfs_oppoint_data *opp_data = get_dvfs_oppoint_data(transient_opp);

#if defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_TEST)
	LOG_DBG("transient_opp: %d, curr_targ_opp: %d", transient_opp, curr_targ_opp);
#else

	NRF_ABB->TRIM.RINGO[TRANSIENT_ZBB_ABB_SLOT]	   = opp_data->abb_ringo;
	NRF_ABB->TRIM.LOCKRANGE[TRANSIENT_ZBB_ABB_SLOT]	   = opp_data->abb_lockrange;
	NRF_ABB->TRIM.PVTMONCYCLES[TRANSIENT_ZBB_ABB_SLOT] = opp_data->abb_pvtmoncycles;

	NRF_APPLICATION_ABB->TRIM.RINGO[TRANSIENT_ZBB_ABB_SLOT]	       = opp_data->abb_ringo;
	NRF_APPLICATION_ABB->TRIM.LOCKRANGE[TRANSIENT_ZBB_ABB_SLOT]    = opp_data->abb_lockrange;
	NRF_APPLICATION_ABB->TRIM.PVTMONCYCLES[TRANSIENT_ZBB_ABB_SLOT] = opp_data->abb_pvtmoncycles;
#endif
	opp_data = get_dvfs_oppoint_data(curr_targ_opp);

#if !defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_TEST)

	NRF_ABB->TRIM.RINGO[CURR_TARG_ABB_SLOT]			       = opp_data->abb_ringo;
	NRF_ABB->TRIM.LOCKRANGE[CURR_TARG_ABB_SLOT]		       = opp_data->abb_lockrange;
	NRF_ABB->TRIM.PVTMONCYCLES[CURR_TARG_ABB_SLOT]		       = opp_data->abb_pvtmoncycles;

	NRF_ABB->CONFIG.CTRL4 = LD_ABB_CTRL4_TRANSITION_OPERATION;

	NRF_APPLICATION_ABB->TRIM.RINGO[CURR_TARG_ABB_SLOT]	   = opp_data->abb_ringo;
	NRF_APPLICATION_ABB->TRIM.LOCKRANGE[CURR_TARG_ABB_SLOT]	   = opp_data->abb_lockrange;
	NRF_APPLICATION_ABB->TRIM.PVTMONCYCLES[CURR_TARG_ABB_SLOT] = opp_data->abb_pvtmoncycles;

	NRF_APPLICATION_ABB->CONFIG.CTRL4 = LD_ABB_CTRL4_TRANSITION_OPERATION;

#endif
#endif
}

int32_t ld_dvfs_configure_hsfll(enum dvfs_frequency_setting oppoint)
{
	nrf_hsfll_trim_t hsfll_trim = {};

	if (oppoint >= DVFS_FREQ_COUNT) {
		LOG_ERR("Not valid oppoint %d", oppoint);
		return -EINVAL;
	}

	uint8_t freq_trim = get_dvfs_oppoint_data(oppoint)->new_f_trim_entry;

	/* Temporary patch fixing medlow oppoint trim index */
	if (oppoint == DVFS_FREQ_MEDLOW) {
		freq_trim = 2;
	}

#if defined(NRF_APPLICATION)
	hsfll_trim.vsup	  = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.VSUP;
	hsfll_trim.coarse = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.COARSE[freq_trim];
	hsfll_trim.fine	  = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.FINE[freq_trim];
#else
	hsfll_trim.vsup	  = NRF_FICR->TRIM.SECURE.HSFLL.TRIM.VSUP;
	hsfll_trim.coarse = NRF_FICR->TRIM.SECURE.HSFLL.TRIM.COARSE[freq_trim];
	hsfll_trim.fine	  = NRF_FICR->TRIM.SECURE.HSFLL.TRIM.FINE[freq_trim];
#endif

#if defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_TEST)
	LOG_DBG("%s oppoint: %d", __func__, oppoint);
	return 0;
#else

	nrf_hsfll_trim_set(NRF_HSFLL, &hsfll_trim);
	nrf_barrier_w();

	nrf_hsfll_clkctrl_mult_set(NRF_HSFLL, get_dvfs_oppoint_data(oppoint)->new_f_mult);
	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);
	/* Trigger hsfll task one more time, SEE PAC-4078 */
	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);

	bool hsfll_freq_changed = false;

	NRFX_WAIT_FOR(nrf_hsfll_event_check(NRF_HSFLL, NRF_HSFLL_EVENT_FREQ_CHANGED),
		      HSFLL_FREQ_CHANGE_CHECK_MAX_ATTEMPTS,
		      HSFLL_FREQ_CHANGE_CHECK_INTERVAL_US,
		      hsfll_freq_changed);

	if (hsfll_freq_changed) {
		return 0;
	}

	return -ETIMEDOUT;
#endif
}

void ld_dvfs_scaling_background_process(bool downscaling)
{
#if defined(NRF_SECURE)
	if (NRF_DOMAIN == NRF_DOMAIN_SECURE) {
		if (downscaling) {
			ld_dvfs_secure_start_increased_power_consumption();
		}
	}
#endif
}

void ld_dvfs_scaling_finish(bool downscaling)
{
#if defined(NRF_SECURE)
#if !defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_TEST)
	NRF_ABB->CONFIG.CTRL4		  = LD_ABB_CTRL4_NORMAL_OPERATION;
	NRF_APPLICATION_ABB->CONFIG.CTRL4 = LD_ABB_CTRL4_NORMAL_OPERATION;
#endif

	if (NRF_DOMAIN == NRF_DOMAIN_SECURE) {
		if (downscaling) {
			ld_dvfs_secure_stop_increased_power_consumption();
		}
	}
#endif
}
