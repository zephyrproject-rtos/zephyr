/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#define MODULE mram_suspend_off
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#include <services/nrfs_mram.h>
#include <nrfs_backend_ipc_service.h>

#define MRAM_SUSPEND_OFF_INIT_PRIO 90

void mram_latency_handler(nrfs_mram_latency_evt_t const *p_evt, void *context)
{
	switch (p_evt->type) {
	case NRFS_MRAM_LATENCY_REQ_APPLIED:
		LOG_DBG("MRAM latency handler: response received");
		break;
	case NRFS_MRAM_LATENCY_REQ_REJECTED:
		LOG_ERR("MRAM latency handler - request rejected!");
		break;
	default:
		LOG_ERR("MRAM latency handler - unexpected event: 0x%x", p_evt->type);
		break;
	}
}

static int turn_off_suspend_mram(void)
{
	/* Turn off mram automatic suspend as it causes delays in time depended code sections. */

	nrfs_err_t err = NRFS_SUCCESS;

	/* Wait for ipc initialization */
	nrfs_backend_wait_for_connection(K_FOREVER);

	err = nrfs_mram_init(mram_latency_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("MRAM service init failed: %d", err);
	} else {
		LOG_DBG("MRAM service initialized");
	}

	LOG_DBG("MRAM: set latency: NOT ALLOWED");
	err = nrfs_mram_set_latency(MRAM_LATENCY_NOT_ALLOWED, NULL);
	if (err) {
		LOG_ERR("MRAM: set latency failed (%d)", err);
	}

	return err;
}

SYS_INIT(turn_off_suspend_mram, APPLICATION, MRAM_SUSPEND_OFF_INIT_PRIO);
