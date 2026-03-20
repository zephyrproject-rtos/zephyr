/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include "app_conf.h"
#if defined(CONFIG_BT_STM32WBA)
#include "blestack.h"
#include "bpka.h"
#endif /* CONFIG_BT_STM32WBA */
#include "ll_intf.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host_if);

K_MUTEX_DEFINE(ble_ctrl_stack_mutex);
#if defined(CONFIG_BT_STM32WBA)
struct k_work_q ble_ctlr_work_q;
static struct k_work ble_ctlr_stack_work, bpka_work;
static bool ble_ctlr_work_q_initialized;
#endif /* CONFIG_BT_STM32WBA */
struct k_work_q ll_work_q;
static bool ll_work_q_initialized;
uint8_t ll_state_busy;

#if defined(CONFIG_BT_STM32WBA)
BUILD_ASSERT(CONFIG_STM32WBA_LL_THREAD_PRIO < CONFIG_STM32WBA_BLE_CTLR_THREAD_PRIO,
	"priority of the Link Layer thread must be higher than the priority of BLE Ctlr thread");

K_THREAD_STACK_DEFINE(ble_ctlr_work_area, CONFIG_STM32WBA_BLE_CTLR_THREAD_STACK_SIZE);
#endif /* CONFIG_BT_STM32WBA */
K_THREAD_STACK_DEFINE(ll_work_area, CONFIG_STM32WBA_LL_THREAD_STACK_SIZE);

#if defined(CONFIG_BT_STM32WBA)
void HostStack_Process(void);
#endif /* CONFIG_BT_STM32WBA */

#if defined(CONFIG_BT_STM32WBA)
static void ble_ctlr_stack_handler(struct k_work *work)
{
	uint8_t running = 0x00;
	change_state_options_t options;

	k_mutex_lock(&ble_ctrl_stack_mutex, K_FOREVER);
	running = BleStack_Process();
	k_mutex_unlock(&ble_ctrl_stack_mutex);

	if (ll_state_busy == 1) {
		options.combined_value = 0x0F;
		ll_intf_chng_evnt_hndlr_state(options);
		ll_state_busy = 0;
	}

	if (running == BLE_SLEEPMODE_RUNNING) {
		HostStack_Process();
	}
}

static void bpka_work_handler(struct k_work *work)
{
	BPKA_BG_Process();
}
#endif /* CONFIG_BT_STM32WBA */

int stm32wba_ll_ctlr_thread_init(void)
{
	if (!ll_work_q_initialized) {
		struct k_work_queue_config ll_cfg = {.name = "LL thread"};

		ll_work_q_initialized = true;
		k_work_queue_init(&ll_work_q);
		k_work_queue_start(&ll_work_q, ll_work_area,
				   K_THREAD_STACK_SIZEOF(ll_work_area),
				   K_PRIO_COOP(CONFIG_STM32WBA_LL_THREAD_PRIO),
				   &ll_cfg);
	}
	return 0;
}

int stm32wba_ble_ctlr_thread_init(void)
{
#if defined(CONFIG_BT_STM32WBA)
	if (!ble_ctlr_work_q_initialized) {
		struct k_work_queue_config ble_ctlr_cfg = {.name = "ble ctlr thread"};

		ble_ctlr_work_q_initialized = true;
		k_work_queue_init(&ble_ctlr_work_q);
		k_work_queue_start(&ble_ctlr_work_q, ble_ctlr_work_area,
				   K_THREAD_STACK_SIZEOF(ble_ctlr_work_area),
				   K_PRIO_COOP(CONFIG_STM32WBA_BLE_CTLR_THREAD_PRIO),
				   &ble_ctlr_cfg);

		k_work_init(&ble_ctlr_stack_work, &ble_ctlr_stack_handler);
		k_work_init(&bpka_work, &bpka_work_handler);
	}
#endif /* CONFIG_BT_STM32WBA */
	return 0;
}

#if defined(CONFIG_BT_STM32WBA)
void HostStack_Process(void)
{
	k_work_submit_to_queue(&ble_ctlr_work_q, &ble_ctlr_stack_work);
}

void BPKACB_Process(void)
{
	k_work_submit_to_queue(&ble_ctlr_work_q, &bpka_work);
}
#endif /* CONFIG_BT_STM32WBA */
