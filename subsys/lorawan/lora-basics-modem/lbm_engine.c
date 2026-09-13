/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <smtc_modem_api.h>
#include <smtc_modem_hal_ext.h>
#include <smtc_modem_utilities.h>

#include "lbm_lorawan.h"

LOG_MODULE_REGISTER(lorawan_lbm, CONFIG_LORAWAN_LOG_LEVEL);

#define LORA_NODE DT_ALIAS(lora0)

static K_SEM_DEFINE(engine_wake, 0, 1);

void lbm_engine_notify(void)
{
	k_sem_give(&engine_wake);
}

static void modem_event_cb(void)
{
	smtc_modem_event_t event;
	uint8_t pending;

	do {
		if (smtc_modem_get_event(&event, &pending) != SMTC_MODEM_RC_OK) {
			return;
		}

		lbm_lorawan_event(&event);
	} while (pending > 0);
}

static void engine_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *radio = DEVICE_DT_GET(LORA_NODE);

	if (!device_is_ready(radio)) {
		LOG_ERR("radio %s not ready", radio->name);
		return;
	}

	smtc_modem_hal_init(radio);
	smtc_modem_set_radio_context(radio);
	smtc_modem_init(modem_event_cb);

	for (;;) {
		uint32_t sleep_ms = smtc_modem_run_engine();

		sleep_ms = MIN(sleep_ms, CONFIG_LORAWAN_LBM_ENGINE_MAX_SLEEP_MS);

		LOG_DBG("sleep %u ms", sleep_ms);
		k_sem_take(&engine_wake, K_MSEC(sleep_ms));
	}
}

K_THREAD_DEFINE(lbm_engine_tid, CONFIG_LORAWAN_LBM_ENGINE_STACK_SIZE, engine_thread, NULL, NULL,
		NULL, CONFIG_LORAWAN_LBM_ENGINE_PRIORITY, 0,
		CONFIG_LORAWAN_LBM_ENGINE_START_DELAY_MS);
