/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <kernel.h>
#include <power.h>
#include <device.h>
#include <misc/printk.h>
#include <power.h>
#include <soc_power.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

#define TASK_TIME_IN_SEC 10
#define MAX_SUSPEND_DEVICE_COUNT 15

static struct device *suspended_devices[MAX_SUSPEND_DEVICE_COUNT];
static int suspend_device_count;
static struct k_fifo fifo;
static int post_ops_done;

QUARK_SE_IPM_DEFINE(alarm_notification, 0, QUARK_SE_IPM_INBOUND);

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
	post_ops_done = 0;

	suspend_devices();

	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP_2);

	if (!post_ops_done) {
		post_ops_done = 1;
		resume_devices();
		_sys_soc_power_state_post_ops(SYS_POWER_STATE_DEEP_SLEEP_2);
	}

	return SYS_PM_DEEP_SLEEP;
}

void _sys_soc_resume(void)
{
	if (!post_ops_done) {
		post_ops_done = 1;
		_sys_soc_power_state_post_ops(SYS_POWER_STATE_DEEP_SLEEP_2);
		resume_devices();
	}
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

	suspend_device_count = 2;
	for (i = 0; i < devcount; i++) {
		if (!strcmp(devices[i].config->name, "arc_v2_irq_unit")) {
			suspended_devices[0] = &devices[i];
		} else if (!strcmp(devices[i].config->name, "sys_clock")) {
			suspended_devices[1] = &devices[i];
		} else {
			suspended_devices[suspend_device_count++] = &devices[i];
		}
	}
}

void alarm_notification_handler(void *context, uint32_t id,
				volatile void *data)
{
	/* Unblock ARC application thread. */
	k_fifo_put(&fifo, NULL);
}

void main(void)
{
	struct device *ipm;

	printk("ARC: Quark SE PM Multicore Demo\n");

	build_suspend_device_list();

	k_fifo_init(&fifo);

	ipm = device_get_binding("alarm_notification");
	ipm_register_callback(ipm, alarm_notification_handler, NULL);
	ipm_set_enabled(ipm, 1);

	while (1) {
		/* Simulate some task handling by busy waiting. */
		printk("ARC: busy\n");
		k_busy_wait(TASK_TIME_IN_SEC * 1000 * 1000);

		printk("ARC: idle\n");
		k_fifo_get(&fifo, K_FOREVER);
	}
}
