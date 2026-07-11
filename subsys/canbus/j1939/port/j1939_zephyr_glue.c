/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/canbus/j1939.h>
#include <J1939Timer.h>
#include "CriticalSectionManager.h"

LOG_MODULE_REGISTER(j1939, CONFIG_CAN_LOG_LEVEL);

// TODO: we could definitely reduce the scope of this lock
K_MUTEX_DEFINE(j1939_lock);

void CriticalSection_Lock(void)
{
    k_mutex_lock(&j1939_lock, K_FOREVER);
}

void CriticalSection_Unlock(void)
{
    k_mutex_unlock(&j1939_lock);
}

#if defined(CONFIG_J1939_LEGACY_AUTO_INIT)

static struct k_work_delayable j1939_legacy_work;

static void j1939_legacy_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	// J1939Timer_Update((J1939_Timer_T)CONFIG_J1939_LEGACY_TASK_PERIOD_MS);
	J1939_Task();

	k_work_schedule(&j1939_legacy_work,
			K_MSEC(CONFIG_J1939_LEGACY_TASK_PERIOD_MS));
}

static int j1939_legacy_init(void)
{
	J1939_Init();

	k_work_init_delayable(&j1939_legacy_work, j1939_legacy_work_handler);
	k_work_schedule(&j1939_legacy_work,
			K_MSEC(CONFIG_J1939_LEGACY_TASK_PERIOD_MS));
	LOG_INF("J1939 stack initialized");

	return 0;
}

SYS_INIT(j1939_legacy_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_J1939_LEGACY_AUTO_INIT */
