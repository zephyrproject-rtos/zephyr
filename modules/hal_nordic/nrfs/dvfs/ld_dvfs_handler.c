/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ld_dvfs_handler.h"
#include "ld_dvfs.h"

#include <hal/nrf_hsfll.h>
#include <nrfs_dvfs.h>
#include <nrfs_backend_ipc_service.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LD_DVFS_LIB, CONFIG_LOCAL_DOMAIN_DVFS_LIB_LOG_LEVEL);

static K_SEM_DEFINE(dvfs_service_sync_sem, 0, 1);
static K_SEM_DEFINE(dvfs_service_idle_sem, 0, 1);

#define DVFS_SERV_HDL_INIT_DONE_BIT_POS (0)
#define DVFS_SERV_HDL_FREQ_CHANGE_REQ_PENDING_BIT_POS (1)

static atomic_t dvfs_service_handler_state_bits;
static volatile enum dvfs_frequency_setting current_freq_setting;
static volatile enum dvfs_frequency_setting requested_freq_setting;
static dvfs_service_handler_callback dvfs_frequency_change_applied_clb;

static void dvfs_service_handler_set_state_bit(uint32_t bit_pos)
{
	atomic_set_bit(&dvfs_service_handler_state_bits, bit_pos);
}

static void dvfs_service_handler_clear_state_bit(uint32_t bit_pos)
{
	atomic_clear_bit(&dvfs_service_handler_state_bits, bit_pos);
}

static bool dvfs_service_handler_get_state_bit(uint32_t bit_pos)
{
	return atomic_test_bit(&dvfs_service_handler_state_bits, bit_pos);
}

static bool dvfs_service_handler_init_done(void)
{
	return dvfs_service_handler_get_state_bit(DVFS_SERV_HDL_INIT_DONE_BIT_POS);
}

static bool dvfs_service_handler_freq_change_req_pending(void)
{
	return dvfs_service_handler_get_state_bit(DVFS_SERV_HDL_FREQ_CHANGE_REQ_PENDING_BIT_POS);
}

static void dvfs_service_handler_nrfs_error_check(nrfs_err_t err)
{
	if (err != NRFS_SUCCESS) {
		LOG_ERR("Failed with nrfs error: %d", err);
	}
}

static void dvfs_service_handler_error(int err)
{
	if (err != 0) {
		LOG_ERR("Failed with error: %d", err);
	}
}

static uint32_t *get_next_context(void)
{
	static uint32_t ctx;

	ctx++;
	return &ctx;
}

static bool dvfs_service_handler_freq_setting_allowed(enum dvfs_frequency_setting freq_setting)
{
	if (freq_setting == DVFS_FREQ_HIGH || freq_setting == DVFS_FREQ_MEDLOW ||
	    freq_setting == DVFS_FREQ_LOW) {
		return true;
	}

	return false;
}

static enum dvfs_frequency_setting dvfs_service_handler_get_current_oppoint(void)
{
	LOG_DBG("Current LD freq setting: %d", current_freq_setting);
	return current_freq_setting;
}

static enum dvfs_frequency_setting dvfs_service_handler_get_requested_oppoint(void)
{
	LOG_DBG("Requested LD freq setting: %d", requested_freq_setting);
	return requested_freq_setting;
}

/* Function to check if current operation is down-scaling */
static bool dvfs_service_handler_is_downscaling(enum dvfs_frequency_setting target_freq_setting)
{
	if (dvfs_service_handler_freq_setting_allowed(target_freq_setting)) {
		LOG_DBG("Checking if downscaling %s",
			(dvfs_service_handler_get_current_oppoint() < target_freq_setting) ? "YES" :
											     "NO");
		return dvfs_service_handler_get_current_oppoint() < target_freq_setting;
	}

	return false;
}

/* Function handling steps for scaling preparation. */
static void dvfs_service_handler_prepare_to_scale(enum dvfs_frequency_setting oppoint_freq)
{
	LOG_DBG("Prepare to scale, oppoint freq %d", oppoint_freq);
	enum dvfs_frequency_setting new_oppoint	    = oppoint_freq;
	enum dvfs_frequency_setting current_oppoint = dvfs_service_handler_get_current_oppoint();

	if (new_oppoint == current_oppoint) {
		LOG_DBG("New oppoint is same as previous, no change");
	} else {
		ld_dvfs_configure_abb_for_transition(current_oppoint, new_oppoint);

		if (dvfs_service_handler_is_downscaling(new_oppoint)) {
			int32_t err = ld_dvfs_configure_hsfll(new_oppoint);

			if (err != 0) {
				dvfs_service_handler_error(err);
			}
		}
	}
}

/* Do background job during scaling process (e.g. increased power consumption during down-scale). */
static void dvfs_service_handler_scaling_background_job(enum dvfs_frequency_setting oppoint_freq)
{
	LOG_DBG("Perform scaling background job if needed.");
	if (dvfs_service_handler_is_downscaling(oppoint_freq)) {
		k_sem_give(&dvfs_service_idle_sem);
	}
}

/* Perform scaling finnish procedure. */
static void dvfs_service_handler_scaling_finish(enum dvfs_frequency_setting oppoint_freq)
{
	LOG_DBG("Scaling finnish oppoint freq %d", oppoint_freq);
	ld_dvfs_scaling_finish(dvfs_service_handler_is_downscaling(oppoint_freq));
	if (!dvfs_service_handler_is_downscaling(oppoint_freq)) {
		int32_t err = ld_dvfs_configure_hsfll(oppoint_freq);

		if (err != 0) {
			dvfs_service_handler_error(err);
		}
	}
	dvfs_service_handler_clear_state_bit(DVFS_SERV_HDL_FREQ_CHANGE_REQ_PENDING_BIT_POS);
	current_freq_setting = oppoint_freq;
	LOG_DBG("Current LD freq setting: %d", current_freq_setting);
	if (dvfs_frequency_change_applied_clb) {
		dvfs_frequency_change_applied_clb(current_freq_setting);
	}
}

/* Function to set hsfll to highest frequency when switched to ABB. */
static void dvfs_service_handler_set_initial_hsfll_config(void)
{
	int32_t err = ld_dvfs_configure_hsfll(DVFS_FREQ_HIGH);

	current_freq_setting = DVFS_FREQ_HIGH;
	requested_freq_setting = DVFS_FREQ_HIGH;
	if (err != 0) {
		dvfs_service_handler_error(err);
	}
}

/* Timer to add additional delay to finish downscale procedure when domain other than secure */
#if !defined(NRF_SECURE)
#define SCALING_FINISH_DELAY_TIMEOUT_US                                                            \
	K_USEC(CONFIG_NRFS_LOCAL_DOMAIN_DOWNSCALE_FINISH_DELAY_TIMEOUT_US)

static void dvfs_service_handler_scaling_finish_delay_timeout(struct k_timer *timer)
{

	dvfs_service_handler_scaling_finish(
		*(enum dvfs_frequency_setting *)k_timer_user_data_get(timer));
}

K_TIMER_DEFINE(dvfs_service_scaling_finish_delay_timer,
	       dvfs_service_handler_scaling_finish_delay_timeout, NULL);
#endif

/* DVFS event handler callback function.*/
static void nrfs_dvfs_evt_handler(nrfs_dvfs_evt_t const *p_evt, void *context)
{
	LOG_DBG("%s", __func__);
	switch (p_evt->type) {
	case NRFS_DVFS_EVT_INIT_PREPARATION:
		LOG_DBG("DVFS handler EVT_INIT_PREPARATION");
#if defined(NRF_SECURE)
		ld_dvfs_clear_zbb();
		dvfs_service_handler_nrfs_error_check(
			nrfs_dvfs_init_complete_request(get_next_context()));
		LOG_DBG("DVFS handler EVT_INIT_PREPARATION handled");
#else
		LOG_ERR("DVFS handler - unexpected EVT_INIT_PREPARATION");
#endif
		break;
	case NRFS_DVFS_EVT_INIT_DONE:
		LOG_DBG("DVFS handler EVT_INIT_DONE");
		dvfs_service_handler_set_initial_hsfll_config();
		dvfs_service_handler_set_state_bit(DVFS_SERV_HDL_INIT_DONE_BIT_POS);
		k_sem_give(&dvfs_service_sync_sem);
		LOG_DBG("DVFS handler EVT_INIT_DONE handled");
		break;
	case NRFS_DVFS_EVT_OPPOINT_REQ_CONFIRMED:
		/* Optional confirmation from sysctrl, wait for oppoint.*/
		dvfs_service_handler_clear_state_bit(DVFS_SERV_HDL_FREQ_CHANGE_REQ_PENDING_BIT_POS);
		LOG_DBG("DVFS handler EVT_OPPOINT_REQ_CONFIRMED %d", (uint32_t)p_evt->freq);
		if (dvfs_service_handler_get_requested_oppoint() == p_evt->freq) {
			if (dvfs_frequency_change_applied_clb) {
				dvfs_frequency_change_applied_clb(p_evt->freq);
			}
		}
		break;
	case NRFS_DVFS_EVT_OPPOINT_SCALING_PREPARE:
		/*Target oppoint will be received here.*/
		LOG_DBG("DVFS handler EVT_OPPOINT_SCALING_PREPARE");
#if !defined(NRF_SECURE)
		if (dvfs_service_handler_is_downscaling(p_evt->freq)) {
#endif
			dvfs_service_handler_prepare_to_scale(p_evt->freq);
			dvfs_service_handler_nrfs_error_check(
						nrfs_dvfs_ready_to_scale(get_next_context()));
			dvfs_service_handler_scaling_background_job(p_evt->freq);
			LOG_DBG("DVFS handler EVT_OPPOINT_SCALING_PREPARE handled");
#if !defined(NRF_SECURE)
			/* Additional delay for downscale to finish on secdom side */
			static enum dvfs_frequency_setting freq;

			freq = p_evt->freq;
			k_timer_user_data_set(&dvfs_service_scaling_finish_delay_timer,
					      (void *)&freq);
			k_timer_start(&dvfs_service_scaling_finish_delay_timer,
				      SCALING_FINISH_DELAY_TIMEOUT_US, K_NO_WAIT);
		} else {
			LOG_ERR("DVFS handler - unexpected EVT_OPPOINT_SCALING_PREPARE");
		}
#endif
		break;
	case NRFS_DVFS_EVT_OPPOINT_SCALING_DONE:
		LOG_DBG("DVFS handler EVT_OPPOINT_SCALING_DONE");
		dvfs_service_handler_scaling_finish(p_evt->freq);
		LOG_DBG("DVFS handler EVT_OPPOINT_SCALING_DONE handled");
		break;
	case NRFS_DVFS_EVT_REJECT:
		LOG_ERR("DVFS handler - request rejected");
		break;
	default:
		LOG_ERR("DVFS handler - unexpected event: 0x%x", p_evt->type);
		break;
	}
}

/* Task to handle dvfs init procedure. */
static void dvfs_service_handler_task(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	LOG_DBG("Trim ABB for default voltage.");
	ld_dvfs_init();

	LOG_DBG("Waiting for backend init");
	/* Wait for ipc initialization */
	nrfs_backend_wait_for_connection(K_FOREVER);

	nrfs_err_t status;

	LOG_DBG("nrfs_dvfs_init");
	status = nrfs_dvfs_init(nrfs_dvfs_evt_handler);
	dvfs_service_handler_nrfs_error_check(status);

	LOG_DBG("nrfs_dvfs_init_prepare_request");
	status = nrfs_dvfs_init_prepare_request(get_next_context());
	dvfs_service_handler_nrfs_error_check(status);

	/* Wait for init*/
	k_sem_take(&dvfs_service_sync_sem, K_FOREVER);

	LOG_DBG("DVFS init done.");

#if defined(CONFIG_NRFS_LOCAL_DOMAIN_DVFS_SCALE_DOWN_AFTER_INIT)
	LOG_DBG("Requesting lowest frequency oppoint.");
	dvfs_service_handler_change_freq_setting(DVFS_FREQ_LOW);
#endif

	while (1) {
		k_sem_take(&dvfs_service_idle_sem, K_FOREVER);
		/* perform background processing */
		ld_dvfs_scaling_background_process(true);
	}
}

K_THREAD_DEFINE(dvfs_service_handler_task_id,
		CONFIG_NRFS_LOCAL_DOMAIN_DVFS_HANDLER_TASK_STACK_SIZE,
		dvfs_service_handler_task,
		NULL,
		NULL,
		NULL,
		CONFIG_NRFS_LOCAL_DOMAIN_DVFS_HANDLER_TASK_PRIORITY,
		0,
		0);

int32_t dvfs_service_handler_change_freq_setting(enum dvfs_frequency_setting freq_setting)
{
	if (!dvfs_service_handler_init_done()) {
		LOG_WRN("Init not done!");
		return -EAGAIN;
	}

	if (!dvfs_service_handler_freq_setting_allowed(freq_setting)) {
		LOG_ERR("Requested frequency setting %d not supported.", freq_setting);
		return -ENXIO;
	}

	if (dvfs_service_handler_freq_change_req_pending()) {
		LOG_DBG("Frequency change request pending.");
		return -EBUSY;
	}

	dvfs_service_handler_set_state_bit(DVFS_SERV_HDL_FREQ_CHANGE_REQ_PENDING_BIT_POS);
	requested_freq_setting = freq_setting;

	nrfs_err_t status = nrfs_dvfs_oppoint_request(freq_setting, get_next_context());

	if (status != NRFS_SUCCESS) {
		dvfs_service_handler_clear_state_bit(DVFS_SERV_HDL_FREQ_CHANGE_REQ_PENDING_BIT_POS);
	}

	dvfs_service_handler_nrfs_error_check(status);

	return status;
}

void dvfs_service_handler_register_freq_setting_applied_callback(dvfs_service_handler_callback clb)
{
	if (clb) {
		LOG_DBG("Registered frequency applied callback");
		dvfs_frequency_change_applied_clb = clb;
	} else {
		LOG_ERR("Invalid callback function provided!");
	}
}
