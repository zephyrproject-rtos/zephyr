/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/toolchain.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/lorawan_lbm/lorawan_hal_init.h>
#include <smtc_modem_utilities.h>
#include <smtc_modem_api.h>

LOG_MODULE_DECLARE(lorawan, CONFIG_LORA_BASICS_MODEM_LOG_LEVEL);

static const struct device *transceiver = DEVICE_DT_GET(DT_CHOSEN(zephyr_lora_transceiver));

static struct k_thread lbm_main_thread_data;
static K_THREAD_STACK_DEFINE(lbm_main_thread_stack,
			     CONFIG_LORA_BASICS_MODEM_MAIN_THREAD_STACK_SIZE);

static void lora_basics_modem_main_thread(void *p1, void *p2, void *p3)
{
	uint32_t sleep_time_ms = 0;
	void (*event_callback)(void) = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	lorawan_smtc_modem_hal_init(transceiver);
	smtc_modem_init(event_callback);

	LOG_INF("Starting loop...");

	while (true) {
		sleep_time_ms = smtc_modem_run_engine();

		if (smtc_modem_is_irq_flag_pending()) {
			continue;
		}

#if CONFIG_LORA_BASICS_MODEM_MAIN_THREAD_MAX_SLEEP_MS
		sleep_time_ms =
			MIN(sleep_time_ms, CONFIG_LORA_BASICS_MODEM_MAIN_THREAD_MAX_SLEEP_MS);
#endif
		LOG_INF("Sleeping for %dms", sleep_time_ms);
		smtc_modem_hal_interruptible_msleep(K_MSEC(sleep_time_ms));
	}
}

void lora_basics_modem_start_work_thread(void (*event_callback)(void))
{
	k_thread_create(&lbm_main_thread_data, lbm_main_thread_stack,
			K_THREAD_STACK_SIZEOF(lbm_main_thread_stack), lora_basics_modem_main_thread,
			event_callback, NULL, NULL, CONFIG_LORA_BASICS_MODEM_MAIN_THREAD_PRIORITY,
			0, K_NO_WAIT);
}
