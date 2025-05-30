/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hal/nrf_hsfll.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/firmware/nrf_ironside/ironside_dvfs.h>
#include <zephyr/drivers/firmware/nrf_ironside/call.h>

static volatile enum ironside_dvfs_oppoint current_dvfs_oppoint = IRONSIDE_DVFS_OPP_HIGH;
static volatile enum ironside_dvfs_oppoint requested_dvfs_oppoint = IRONSIDE_DVFS_OPP_HIGH;
K_MUTEX_DEFINE(ironside_dvfs_mutex);

struct dvfs_hsfll_data {
	uint32_t new_f_mult;
	uint32_t new_f_trim_entry;
	uint32_t max_hsfll_freq;
};

static const struct dvfs_hsfll_data dvfs_hsfll_data[IRONSIDE_DVFS_OPP_COUNT] = {
	/* ABB oppoint 0.8V */
	{
		.new_f_mult = 20,
		.new_f_trim_entry = 0,
		.max_hsfll_freq = 320000000,
	},
	/* ABB oppoint 0.6V */
	{
		.new_f_mult = 8,
		.new_f_trim_entry = 2,
		.max_hsfll_freq = 128000000,
	},
	/* ABB oppoint 0.5V */
	{
		.new_f_mult = 4,
		.new_f_trim_entry = 3,
		.max_hsfll_freq = 64000000,
	},
};

/**
 * @brief Check if the requested oppoint is allowed.
 *
 * @param freq_setting The requested oppoint to check.
 * @return true if the oppoint is allowed, false otherwise.
 */
static bool ironside_dvfs_opp_allowed(enum ironside_dvfs_oppoint freq_setting)
{
	if (freq_setting == IRONSIDE_DVFS_OPP_HIGH || freq_setting == IRONSIDE_DVFS_OPP_MEDLOW ||
	    freq_setting == IRONSIDE_DVFS_OPP_LOW) {
		return true;
	}

	return false;
}

/**
 * @brief Check if the requested oppoint change operation is downscaling.
 *
 * @param target_freq_setting The target oppoint to check.
 * @return true if the current oppoint is higher than the target, false otherwise.
 */
static bool ironside_dvfs_is_downscaling(enum ironside_dvfs_oppoint target_freq_setting)
{
	return current_dvfs_oppoint < target_freq_setting;
}

/**
 * @brief Configure hsfll depending on selected oppoint
 *
 * @param enum oppoint target operation point
 * @return 0 value indicates no error.
 * @return -EINVAL invalid oppoint or domain.
 */
static int32_t ironside_dvfs_configure_hsfll(enum ironside_dvfs_oppoint oppoint)
{
	if (oppoint >= IRONSIDE_DVFS_OPP_COUNT) {
		return -EINVAL;
	}

	nrf_hsfll_trim_t hsfll_trim = {};
	uint8_t freq_trim_idx = dvfs_hsfll_data[oppoint].new_f_trim_entry;

#if defined(NRF_APPLICATION)
	hsfll_trim.vsup = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.VSUP;
	hsfll_trim.coarse = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.COARSE[freq_trim_idx];
	hsfll_trim.fine = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.FINE[freq_trim_idx];
#if NRF_HSFLL_HAS_TCOEF_TRIM
	hsfll_trim.tcoef = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.TCOEF;
#endif
#else
	/* Currently only application core is supported */
	return -EINVAL;
#endif

	nrf_hsfll_clkctrl_mult_set(NRF_HSFLL, dvfs_hsfll_data[oppoint].new_f_mult);
	nrf_hsfll_trim_set(NRF_HSFLL, &hsfll_trim);
	nrf_barrier_w();

	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);
	/* Trigger hsfll task one more time, SEE PAC-4078 */
	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);

	return 0;
}

/* Function handling steps for DVFS oppoint change. */
static int ironside_dvfs_prepare_to_scale(enum ironside_dvfs_oppoint freq_setting)
{
	int err = 0;

	if (ironside_dvfs_is_downscaling(freq_setting)) {
		err = ironside_dvfs_configure_hsfll(freq_setting);
	}

	return err;
}

/* Update MDK variable which is used by nrfx_coredep_delay_us (k_busy_wait). */
static void ironside_dvfs_update_core_clock(enum ironside_dvfs_oppoint oppoint_freq)
{
	extern uint32_t SystemCoreClock;

	SystemCoreClock = dvfs_hsfll_data[oppoint_freq].max_hsfll_freq;
}

/* Perform scaling finnish procedure. */
static int ironside_dvfs_change_oppoint_complete(enum ironside_dvfs_oppoint dvfs_oppoint)
{
	int err = 0;

	if (!ironside_dvfs_is_downscaling(dvfs_oppoint)) {
		err = ironside_dvfs_configure_hsfll(dvfs_oppoint);
	}

	current_dvfs_oppoint = dvfs_oppoint;
	ironside_dvfs_update_core_clock(dvfs_oppoint);

	return err;
}

/**
 * @brief Request DVFS oppoint change from IRONside secure domain.
 * This function will send a request over IPC to the IRONside secure domain
 * This function is synchronous and will return when the request is completed.
 *
 * @param oppoint @ref enum ironside_dvfs_oppoint
 * @return int
 */
static int ironside_dvfs_req_oppoint(enum ironside_dvfs_oppoint oppoint)
{
	int err;

	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_DVFS_SERVICE_V0;
	buf->args[IRONSIDE_DVFS_SERVICE_OPPOINT_IDX] = oppoint;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_DVFS_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}

int ironside_dvfs_change_oppoint(enum ironside_dvfs_oppoint dvfs_oppoint)
{
	int status = 0;

	if (!ironside_dvfs_opp_allowed(dvfs_oppoint)) {
		return -EINVAL;
	}

	if (dvfs_oppoint == current_dvfs_oppoint) {
		return 0;
	}

	if (!k_mutex_lock(&ironside_dvfs_mutex, K_MSEC(1000))) {
		requested_dvfs_oppoint = dvfs_oppoint;

		ironside_dvfs_prepare_to_scale(requested_dvfs_oppoint);

		status = ironside_dvfs_req_oppoint(requested_dvfs_oppoint);

		if (status != 0) {
			k_mutex_unlock(&ironside_dvfs_mutex);
			return status;
		}
		status = ironside_dvfs_change_oppoint_complete(requested_dvfs_oppoint);
		k_mutex_unlock(&ironside_dvfs_mutex);
	} else {
		return -EBUSY;
	}

	return status;
}
