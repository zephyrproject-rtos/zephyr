/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_conf.h"
#include "main.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(mac_sys_if);

#define NUM_THREADS 1U
#define STACK_SIZE (2048U)
K_THREAD_STACK_ARRAY_DEFINE(MacSysStack, NUM_THREADS, STACK_SIZE);

K_SEM_DEFINE(sem_MAC, 0U, 1U);
K_SEM_DEFINE(sem_MAC_evnt, 0U, 1U);
K_MUTEX_DEFINE(mac_sys_if_mutex);

static struct k_thread t2[NUM_THREADS];

extern void mac_baremetal_run();
void MacSys_SemaphoreSet(void);
static void MacSys_process_handler(void *, void* , void*);

void MacSys_Init(void)
{
	k_tid_t tid;

	/* Register tasks */
	tid = k_thread_create(&t2[0], MacSysStack[0], STACK_SIZE, MacSys_process_handler, INT_TO_POINTER(1), NULL, NULL, K_PRIO_PREEMPT(15), 0, K_NO_WAIT);

	/* Release semaphore */
	MacSys_SemaphoreSet();

	LOG_DBG("MacSys_Init \n");
}

static void MacSys_process_handler(void *, void* , void*)
{
	while (1) {
		k_sem_take(&sem_MAC, K_FOREVER);
		LOG_DBG("MacSys_Process Entry\n");
		k_mutex_lock(&mac_sys_if_mutex, K_FOREVER);
		mac_baremetal_run();
		k_mutex_unlock(&mac_sys_if_mutex);
		LOG_DBG("MacSys_Process Out\n");
	}
}

void MacSys_SemaphoreSet(void)
{
	LOG_DBG("MacSys_SemaphoreSet\n");
	k_sem_give(&sem_MAC);
}

/**
  * @brief  MAC Layer Task wait. Not used with Sequencer.
  * @param  None
  * @retval None
  */
void MacSys_SemaphoreWait( void )
{
	LOG_DBG("MacSys_SemaphoreWait Entry\n");
	k_sem_take(&sem_MAC, K_FOREVER);
	LOG_DBG("MacSys_SemaphoreWait Out\n");
}

/**
  * @brief  MAC Layer set Event. 
  * @param  None
  * @retval None
  */
void MacSys_EventSet( void )
{
	LOG_DBG("MacSys_EventSet\n");
	k_sem_give(&sem_MAC_evnt);
}

/**
  * @brief  MAC Layer wait Event. 
  * @param  None
  * @retval None
  */
void MacSys_EventWait( void )
{
	LOG_DBG("MacSys_EventWait Entry\n");
	k_sem_take(&sem_MAC_evnt, K_FOREVER);
	LOG_DBG("MacSys_EventWait Out\n");
}
