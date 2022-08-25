/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/debug/object_tracing.h>
#include <zephyr/kernel_structs.h>
#include <os_mgmt/os_mgmt.h>
#include <os_mgmt/os_mgmt_impl.h>

static void zephyr_os_mgmt_reset_cb(struct k_timer *timer);
static void zephyr_os_mgmt_reset_work_handler(struct k_work *work);

static K_TIMER_DEFINE(zephyr_os_mgmt_reset_timer, zephyr_os_mgmt_reset_cb, NULL);

K_WORK_DEFINE(zephyr_os_mgmt_reset_work, zephyr_os_mgmt_reset_work_handler);

static void
zephyr_os_mgmt_reset_work_handler(struct k_work *work)
{
	sys_reboot(SYS_REBOOT_WARM);
}

static void
zephyr_os_mgmt_reset_cb(struct k_timer *timer)
{
	/* Reboot the system from the system workqueue thread. */
	k_work_submit(&zephyr_os_mgmt_reset_work);
}

int
os_mgmt_impl_reset(unsigned int delay_ms)
{
	k_timer_start(&zephyr_os_mgmt_reset_timer, K_MSEC(delay_ms), K_NO_WAIT);
	return 0;
}
