/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_lfxo.h>

#include "lfxo_pixo.h"

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define LFXO_PIXO_POLL_MS             10
#define LFXO_PIERCE_SWITCH_TIMEOUT_US 2000

static void lfxo_pixo_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(lfxo_pixo_work, lfxo_pixo_work_handler);

void nrf71_lfxo_pixo_mirror_lock(void)
{
	/* Without this lock set before the LFXO starts, PIXO stops in low power */
	nrf_lfxo_mirror_lock_set(NRF_LFXO, true);
}

/* PIXO cannot start the crystal, so switch once Pierce start-up is done */
static void lfxo_pixo_work_handler(struct k_work *work)
{
	nrf_lfxo_status_t status;

	ARG_UNUSED(work);

	nrf_lfxo_status_get(NRF_LFXO, &status);
	if (!status.running) {
		k_work_schedule(&lfxo_pixo_work, K_MSEC(LFXO_PIXO_POLL_MS));
		return;
	}

	nrf_lfxo_mode_set(NRF_LFXO, NRF_LFXO_OSCILLATOR_MODE_PIXO, false);
	LOG_DBG("LFXO switch to PIXO mode requested");
}

void nrf71_lfxo_pixo_switch_start(void)
{
	k_work_schedule(&lfxo_pixo_work, K_NO_WAIT);
}

void nrf71_lfxo_prepare_system_off(void)
{
	nrf_lfxo_status_t status;

	nrf_lfxo_status_get(NRF_LFXO, &status);
	if ((status.oscmode != NRF_LFXO_OSCILLATOR_MODE_PIXO) &&
	    ((NRF_LFXO->MODE & LFXO_MODE_MODE_Msk) !=
	     (LFXO_MODE_MODE_Pixo << LFXO_MODE_MODE_Pos))) {
		return;
	}

	/* The boot ROM releases the mirrored registers on wake, so the boot hangs in PIXO mode */
	nrf_lfxo_event_clear(NRF_LFXO, NRF_LFXO_EVENT_MODECHANGED);
	nrf_lfxo_mode_set(NRF_LFXO, NRF_LFXO_OSCILLATOR_MODE_PIERCE, false);

	for (uint32_t us = 0; us < LFXO_PIERCE_SWITCH_TIMEOUT_US; us++) {
		nrf_lfxo_status_get(NRF_LFXO, &status);
		if (nrf_lfxo_event_check(NRF_LFXO, NRF_LFXO_EVENT_MODECHANGED) &&
		    (status.oscmode == NRF_LFXO_OSCILLATOR_MODE_PIERCE)) {
			break;
		}
		k_busy_wait(1);
	}
}
