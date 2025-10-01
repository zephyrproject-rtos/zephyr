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

K_MUTEX_DEFINE(ble_ctrl_stack_mutex);
#if defined(CONFIG_BT_STM32WBA)
struct k_work_q ble_ctrl_work_q;
struct k_work ble_ctrl_stack_work, bpka_work;
#endif /* CONFIG_BT_STM32WBA */
struct k_work_q ll_work_q;
uint8_t ll_state_busy;

/* TODO: More tests to be done to optimize thread stacks' sizes */
#if defined(CONFIG_BT_STM32WBA)
#define BLE_CTRL_THREAD_STACK_SIZE (256 * 7)
#define BLE_CTRL_THREAD_PRIO (14)
#endif /* CONFIG_BT_STM32WBA */
#define LL_THREAD_STACK_SIZE (256 * 7)

/* The LL thread has higher priority than the BLE CTRL thread and the Zephyr BLE stack threads */
#define LL_THREAD_PRIO (4)

#if defined(CONFIG_BT_STM32WBA)
K_THREAD_STACK_DEFINE(ble_ctrl_work_area, BLE_CTRL_THREAD_STACK_SIZE);
#endif /* CONFIG_BT_STM32WBA */
K_THREAD_STACK_DEFINE(ll_work_area, LL_THREAD_STACK_SIZE);

#if defined(CONFIG_BT_STM32WBA)
void HostStack_Process(void);
#endif /* CONFIG_BT_STM32WBA */

#if defined(CONFIG_BT_STM32WBA)
static void ble_ctrl_stack_handler(struct k_work *work)
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

static int stm32wba_ctrl_init(void)
{
	struct k_work_queue_config ll_cfg = {.name = "LL thread"};

#if defined(CONFIG_BT_STM32WBA)
	struct k_work_queue_config ble_ctrl_cfg = {.name = "ble ctrl thread"};

	k_work_queue_init(&ble_ctrl_work_q);
	k_work_queue_start(&ble_ctrl_work_q, ble_ctrl_work_area,
			   K_THREAD_STACK_SIZEOF(ble_ctrl_work_area),
			   BLE_CTRL_THREAD_PRIO, &ble_ctrl_cfg);

	k_work_init(&ble_ctrl_stack_work, &ble_ctrl_stack_handler);
	k_work_init(&bpka_work, &bpka_work_handler);
#endif /* CONFIG_BT_STM32WBA */

	k_work_queue_init(&ll_work_q);
	k_work_queue_start(&ll_work_q, ll_work_area,
			   K_THREAD_STACK_SIZEOF(ll_work_area),
			   LL_THREAD_PRIO, &ll_cfg);

	return 0;
}

#if defined(CONFIG_BT_STM32WBA)
void HostStack_Process(void)
{
	k_work_submit_to_queue(&ble_ctrl_work_q, &ble_ctrl_stack_work);
}

void BPKACB_Process(void)
{
	k_work_submit_to_queue(&ble_ctrl_work_q, &bpka_work);
}
#endif /* CONFIG_BT_STM32WBA */

SYS_INIT(stm32wba_ctrl_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
