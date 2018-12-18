/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <kernel.h>
#include <power.h>
#include <misc/printk.h>
#include <power.h>
#include <soc_power.h>
#include <counter.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

#ifdef TEST_CASE_SLEEP_SUCCESS
#define TASK_TIME_IN_SEC 15
#define IDLE_TIME_IN_SEC 5
#else
#define TASK_TIME_IN_SEC 5
#define IDLE_TIME_IN_SEC 15
#endif /* TEST_CASE_SLEEP_SUCCESS */

#define MAX_SUSPEND_DEVICE_COUNT 15

#define RTC_ALARM_SECOND (32768 / CONFIG_RTC_PRESCALER)


static struct device *suspended_devices[MAX_SUSPEND_DEVICE_COUNT];
static int suspend_device_count;
static struct k_fifo fifo;
static struct device *ipm;

QUARK_SE_IPM_DEFINE(alarm_notification, 0, QUARK_SE_IPM_OUTBOUND);

static void suspend_devices(void)
{
	for (int i = suspend_device_count - 1; i >= 0; i--) {
		device_set_power_state(suspended_devices[i],
				       DEVICE_PM_SUSPEND_STATE);
	}
}

static void resume_devices(void)
{
	for (int i = 0; i < suspend_device_count; i++) {
		device_set_power_state(suspended_devices[i],
				       DEVICE_PM_ACTIVE_STATE);
	}
}

int sys_suspend(s32_t ticks)
{
	printk("LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2"
	       " state\n");

	if (!sys_power_state_is_arc_ready()) {
		printk("LMT: Failed. ARC is busy.\n");
		return SYS_PM_NOT_HANDLED;
	}

	suspend_devices();

	sys_set_power_state(SYS_POWER_STATE_CPU_LPS_2);

	resume_devices();

	printk("LMT: Succeed.\n");

	sys_power_state_post_ops(SYS_POWER_STATE_CPU_LPS_2);

	return SYS_PM_DEEP_SLEEP;
}

static void build_suspend_device_list(void)
{
	int i, devcount;
	struct device *devices;

	device_list_get(&devices, &devcount);
	if (devcount > MAX_SUSPEND_DEVICE_COUNT) {
		printk("Error: List of devices exceeds what we can track "
		       "for suspend. Built: %d, Max: %d\n",
		       devcount, MAX_SUSPEND_DEVICE_COUNT);
		return;
	}

	suspend_device_count = 3;
	for (i = 0; i < devcount; i++) {
		if (!strcmp(devices[i].config->name, "loapic")) {
			suspended_devices[0] = &devices[i];
		} else if (!strcmp(devices[i].config->name, "ioapic")) {
			suspended_devices[1] = &devices[i];
		} else if (!strcmp(devices[i].config->name,
				 CONFIG_UART_CONSOLE_ON_DEV_NAME)) {
			suspended_devices[2] = &devices[i];
		} else {
			suspended_devices[suspend_device_count++] = &devices[i];
		}
	}
}

static void alarm_handler(struct device *dev,
			  const struct counter_alarm_cfg *cfg,
			  u32_t counter)
{
	/* Unblock LMT application thread. */
	k_fifo_put(&fifo, NULL);

	/* Send a dummy message to ARC so the ARC application
	 * thread can be unblocked.
	 */
	ipm_send(ipm, 0, 0, NULL, 0);
}

void main(void)
{
	struct device *rtc_dev;
	struct counter_alarm_cfg alarm_cfg;
	u32_t now;

	printk("LMT: Quark SE PM Multicore Demo\n");

	k_fifo_init(&fifo);

	build_suspend_device_list();

	ipm = device_get_binding("alarm_notification");
	if (!ipm) {
		printk("Error: Failed to get IPM device\n");
		return;
	}

	rtc_dev = device_get_binding(DT_RTC_0_NAME);
	if (!rtc_dev) {
		printk("Error: Failed to get RTC device\n");
		return;
	}

	counter_start(rtc_dev);

	/* In QMSI, in order to save the alarm callback we must set
	 * 'alarm_enable = 1' during configuration. However, this
	 * automatically triggers the alarm underneath. So, to avoid
	 * the alarm being fired any time soon, we set the 'init_val'
	 * to 1 and the 'alarm_val' to 0.
	 */
	alarm_cfg.channel_id = 0;
	alarm_cfg.absolute = true;
	alarm_cfg.handler = alarm_handler;


	while (1) {
		/* Add a busy loop to syncronize the
		 * console output b/w x86 and ARC.
		 */
		k_busy_wait(1000 * 1000);

		/* Simulate some task handling by busy waiting. */
		printk("LMT: busy\n");
		k_busy_wait(TASK_TIME_IN_SEC * 1000 * 1000);

		now = counter_read(rtc_dev);
		alarm_cfg.ticks = now + RTC_ALARM_SECOND;
		counter_set_ch_alarm(rtc_dev,
			      &alarm_cfg);

		printk("LMT: idle\n");
		k_fifo_get(&fifo, K_FOREVER);
	}
}
