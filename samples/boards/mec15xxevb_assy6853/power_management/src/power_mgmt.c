/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <soc.h>
#include <power/power.h>
#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(pwrmgmt_test);


#define SLP_STATES_SUPPORTED 2

/* Thread properties */
#define TASK_STACK_SIZE 1024
#define PRIORITY K_PRIO_COOP(5)
/* Thread sleep should be lower than CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1 */
#define THREAD_A_SLEEP_TIME  100
#define THREAD_B_SLEEP_TIME  1000

K_THREAD_STACK_DEFINE(stackA, TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(stackB, TASK_STACK_SIZE);

static struct k_thread threadA_id;
static struct k_thread threadB_id;

struct pm_counter {
	u8_t entry_cnt;
	u8_t exit_cnt;
};

struct pm_counter pm_counters[SLP_STATES_SUPPORTED];

/* Hooks to count entry/exit */
void sys_pm_notify_power_state_entry(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_SLEEP_1:
		pm_counters[0].entry_cnt++;
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		pm_counters[1].entry_cnt++;
		break;
	default:
		break;
	}
}

void sys_pm_notify_power_state_exit(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_SLEEP_1:
		pm_counters[0].exit_cnt++;
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		pm_counters[1].exit_cnt++;
		break;
	default:
		break;
	}
}

static void pm_reset_counters(void)
{
	for (int i = 0; i < SLP_STATES_SUPPORTED; i++) {
		printk("[%d] PM entry %d\n", i, pm_counters[i].entry_cnt);
		printk("[%d] PM exit %d\n", i, pm_counters[i].exit_cnt);
		pm_counters[i].entry_cnt = 0;
		pm_counters[i].exit_cnt = 0;
	}
}

static void pm_entry_marker(void)
{
	/* Directly access a pin */
	GPIO_CTRL_REGS->CTRL_0014 = 0x00240UL;
	printk("PM >\n");
}

static void pm_exit_marker(void)
{
	/* Directly access a pin */
	GPIO_CTRL_REGS->CTRL_0014 = 0x10240UL;
	printk("PM <\n");
}

static int taskA_init(void)
{
	LOG_INF("Thread task A init");

	return 0;
}

static int taskB_init(void)
{
	printk("Thread task B init");

	return 0;
}

void taskA_thread(void *p1, void *p2, void *p3)
{
	while (true) {
		k_msleep(THREAD_A_SLEEP_TIME);
		printk("A");
	}
}

static void taskB_thread(void *p1, void *p2, void *p3)
{
	while (true) {
		k_msleep(THREAD_B_SLEEP_TIME);
		printk("B");
	}
}

static void create_tasks(void)
{
	taskA_init();
	taskB_init();

	k_thread_create(&threadA_id, stackA, TASK_STACK_SIZE, taskA_thread,
		NULL, NULL, NULL, PRIORITY,  K_INHERIT_PERMS, K_FOREVER);
	k_thread_create(&threadB_id, stackB, TASK_STACK_SIZE, taskB_thread,
		NULL, NULL, NULL, PRIORITY,  K_INHERIT_PERMS, K_FOREVER);

	k_thread_start(&threadA_id);
	k_thread_start(&threadB_id);

}

static void destroy_tasks(void)
{
	k_thread_abort(&threadA_id);
	k_thread_abort(&threadB_id);

	k_thread_join(&threadA_id, K_FOREVER);
	k_thread_join(&threadB_id, K_FOREVER);
}

static void suspend_all_tasks(void)
{
	k_thread_suspend(&threadA_id);
	k_thread_suspend(&threadB_id);
}

static void resume_all_tasks(void)
{
	k_thread_resume(&threadA_id);
	k_thread_resume(&threadB_id);
}

int test_pwr_mgmt_multithread(bool use_logging, u8_t cycles)
{

	/* Ensure we can enter deep sleep when stopping threads
	 * No UART output should occurr when threads are suspended
	 * Test to verify Zephyr RTOS issue #20033
	 * https://github.com/zephyrproject-rtos/zephyr/issues/20033
	 */

	create_tasks();

	pm_exit_marker();

	while (cycles-- > 0) {
		k_msleep(CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1 + 500);

		if (use_logging) {
			LOG_INF("Wake from Light Sleep\n");
		} else {
			printk("Wake from Light Sleep\n");
		}

		k_busy_wait(100);

		LOG_INF("Suspend...");
		suspend_all_tasks();

		/* GPIO toggle to measure latency */
		pm_entry_marker();

		k_msleep(CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1 + 500);

		k_busy_wait(100);

		if (use_logging) {
			LOG_INF("Wake from Deep Sleep\n");
		} else {
			printk("Wake from Deep Sleep\n");
		}

		pm_exit_marker();
		LOG_INF("Resume");
		resume_all_tasks();
	}

	destroy_tasks();

	pm_reset_counters();

	return 0;
}

int test_pwr_mgmt_singlethread(bool use_logging, u8_t cycles)
{
	pm_exit_marker();

	while (cycles-- > 0) {

		/* Trigger Light Sleep 1 state. 48MHz PLL stays on */
		k_msleep(CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1 + 500);

		if (use_logging) {
			LOG_INF("Wake from Light Sleep\n");
		} else {
			printk("Wake from Light Sleep\n");
		}

		k_busy_wait(100);

		/* Trigger Deep Sleep 1 state. 48MHz PLL off */

		LOG_INF("About to sleep for enough time for Deep Sleep\n");

		/* GPIO toggle to measure latency */
		pm_entry_marker();

		k_msleep(CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1 + 1000);

		k_busy_wait(100);

		if (use_logging) {
			LOG_INF("Wake from Deep Sleep\n");
		} else {
			printk("Wake from Deep Sleep\n");
		}

		pm_exit_marker();
	}

	pm_reset_counters();

	return 0;
}
