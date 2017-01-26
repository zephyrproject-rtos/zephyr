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
#include <rtc.h>
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

int _sys_soc_suspend(int32_t ticks)
{
	printk("LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2"
	       " state\n");

	if (!_sys_soc_power_state_is_arc_ready()) {
		printk("LMT: Failed. ARC is busy.\n");
		return SYS_PM_NOT_HANDLED;
	}

	suspend_devices();

	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP_2);

	resume_devices();

	printk("LMT: Succeed.\n");

	_sys_soc_power_state_post_ops(SYS_POWER_STATE_DEEP_SLEEP_2);

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

static void alarm_handler(struct device *dev)
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
	struct rtc_config config;
	uint32_t now;

	printk("LMT: Quark SE PM Multicore Demo\n");

	k_fifo_init(&fifo);

	build_suspend_device_list();

	ipm = device_get_binding("alarm_notification");
	if (!ipm) {
		printk("Error: Failed to get IPM device\n");
		return;
	}

	rtc_dev = device_get_binding("RTC_0");
	if (!rtc_dev) {
		printk("Error: Failed to get RTC device\n");
		return;
	}

	rtc_enable(rtc_dev);

	config.init_val = 0;
	config.alarm_enable = 0;
	config.alarm_val = 0;
	config.cb_fn = alarm_handler;
	rtc_set_config(rtc_dev, &config);

	while (1) {
		/* Simulate some task handling by busy waiting. */
		printk("LMT: busy\n");
		k_busy_wait(TASK_TIME_IN_SEC * 1000 * 1000);

		now = rtc_read(rtc_dev);
		rtc_set_alarm(rtc_dev,
			      now + (RTC_ALARM_SECOND * IDLE_TIME_IN_SEC));

		printk("LMT: idle\n");
		k_fifo_get(&fifo, K_FOREVER);
	}
}
