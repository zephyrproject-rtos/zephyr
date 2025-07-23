/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hal/nrf_hsfll.h>
#include <zephyr/kernel.h>

#include <nrf_ironside/dvfs.h>
#include <nrf_ironside/call.h>

static enum ironside_dvfs_oppoint current_dvfs_oppoint = IRONSIDE_DVFS_OPP_HIGH;

#if defined(CONFIG_SOC_SERIES_NRF54HX)
#define ABB_STATUSANA_LOCKED_L_Pos (0UL)
#define ABB_STATUSANA_LOCKED_L_Msk (0x1UL << ABB_STATUSANA_LOCKED_L_Pos)
#define ABB_STATUSANA_REG_OFFSET   (0x102UL)
#else
#error "Unsupported SoC series for IronSide DVFS"
#endif

#define ABB_STATUSANA_CHECK_MAX_ATTEMPTS (CONFIG_NRF_IRONSIDE_ABB_STATUSANA_CHECK_MAX_ATTEMPTS)
#define ABB_STATUSANA_CHECK_INTERVAL_US  (10U)

struct dvfs_hsfll_data_t {
	uint32_t new_f_mult;
	uint32_t new_f_trim_entry;
	uint32_t max_hsfll_freq;
};

static const struct dvfs_hsfll_data_t dvfs_hsfll_data[] = {
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

BUILD_ASSERT(ARRAY_SIZE(dvfs_hsfll_data) == (IRONSIDE_DVFS_OPPOINT_COUNT),
	     "dvfs_hsfll_data size must match number of DVFS oppoints");

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
 */
static void ironside_dvfs_configure_hsfll(enum ironside_dvfs_oppoint oppoint)
{
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
#error "Only application core is supported for DVFS"
#endif

	nrf_hsfll_clkctrl_mult_set(NRF_HSFLL, dvfs_hsfll_data[oppoint].new_f_mult);
	nrf_hsfll_trim_set(NRF_HSFLL, &hsfll_trim);
	nrf_barrier_w();

	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);
	/* Trigger hsfll task one more time, SEE PAC-4078 */
	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);
}

/* Function handling steps for DVFS oppoint change. */
static void ironside_dvfs_prepare_to_scale(enum ironside_dvfs_oppoint dvfs_oppoint)
{
	if (ironside_dvfs_is_downscaling(dvfs_oppoint)) {
		ironside_dvfs_configure_hsfll(dvfs_oppoint);
	}
}

/* Update MDK variable which is used by nrfx_coredep_delay_us (k_busy_wait). */
static void ironside_dvfs_update_core_clock(enum ironside_dvfs_oppoint dvfs_oppoint)
{
	extern uint32_t SystemCoreClock;

	SystemCoreClock = dvfs_hsfll_data[dvfs_oppoint].max_hsfll_freq;
}

/* Perform scaling finnish procedure. */
static void ironside_dvfs_change_oppoint_complete(enum ironside_dvfs_oppoint dvfs_oppoint)
{
	if (!ironside_dvfs_is_downscaling(dvfs_oppoint)) {
		ironside_dvfs_configure_hsfll(dvfs_oppoint);
	}

	current_dvfs_oppoint = dvfs_oppoint;
	ironside_dvfs_update_core_clock(dvfs_oppoint);
}

/**
 * @brief Check if ABB analog part is locked.
 *
 * @param abb Pointer to ABB peripheral.
 *
 * @return true if ABB is locked, false otherwise.
 */
static inline bool ironside_dvfs_is_abb_locked(NRF_ABB_Type *abb)
{
	/* Check if ABB analog part is locked. */
	/* Temporary workaround until STATUSANA register is visible. */
	volatile const uint32_t *statusana = (uint32_t *)abb + ABB_STATUSANA_REG_OFFSET;

	uint8_t status_read_attempts = ABB_STATUSANA_CHECK_MAX_ATTEMPTS;

	while ((*statusana & ABB_STATUSANA_LOCKED_L_Msk) == 0 && status_read_attempts > 0) {
		k_busy_wait(ABB_STATUSANA_CHECK_INTERVAL_US);
		status_read_attempts--;
	}

	return ((*statusana & ABB_STATUSANA_LOCKED_L_Msk) != 0);
}

/**
 * @brief Request DVFS oppoint change from IronSide secure domain.
 * This function will send a request over IPC to the IronSide secure domain
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

	if (!ironside_dvfs_is_oppoint_valid(dvfs_oppoint)) {
		return -IRONSIDE_DVFS_ERROR_WRONG_OPPOINT;
	}

	if (!ironside_dvfs_is_abb_locked(NRF_ABB)) {
		return -IRONSIDE_DVFS_ERROR_BUSY;
	}

	if (dvfs_oppoint == current_dvfs_oppoint) {
		return status;
	}

	if (k_is_in_isr()) {
		return -IRONSIDE_DVFS_ERROR_ISR_NOT_ALLOWED;
	}

	ironside_dvfs_prepare_to_scale(dvfs_oppoint);

	status = ironside_dvfs_req_oppoint(dvfs_oppoint);

	if (status != 0) {
		return status;
	}
	ironside_dvfs_change_oppoint_complete(dvfs_oppoint);

	return status;
}
