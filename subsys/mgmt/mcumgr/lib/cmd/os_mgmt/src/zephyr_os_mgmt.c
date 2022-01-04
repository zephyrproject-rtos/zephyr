/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/reboot.h>
#include <debug/object_tracing.h>
#include <kernel_structs.h>
#include <mgmt/mgmt.h>
#include <util/mcumgr_util.h>
#include <os_mgmt/os_mgmt.h>
#include <os_mgmt/os_mgmt_impl.h>

static void zephyr_os_mgmt_reset_cb(struct k_timer *timer);
static void zephyr_os_mgmt_reset_work_handler(struct k_work *work);

static K_TIMER_DEFINE(zephyr_os_mgmt_reset_timer, zephyr_os_mgmt_reset_cb, NULL);

K_WORK_DEFINE(zephyr_os_mgmt_reset_work, zephyr_os_mgmt_reset_work_handler);

#ifdef CONFIG_THREAD_MONITOR
static const struct k_thread *
zephyr_os_mgmt_task_at(int idx)
{
	const struct k_thread *thread;
	int i;

	thread = SYS_THREAD_MONITOR_HEAD;
	for (i = 0; i < idx; i++) {
		if (thread == NULL) {
			break;
		}
		thread = SYS_THREAD_MONITOR_NEXT(thread);
	}

	return thread;
}

int
os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info)
{
	const struct k_thread *thread;
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	size_t unused;
#endif

	thread = zephyr_os_mgmt_task_at(idx);
	if (thread == NULL) {
		return MGMT_ERR_ENOENT;
	}

	*out_info = (struct os_mgmt_task_info){ 0 };

#ifdef CONFIG_THREAD_NAME
	strncpy(out_info->oti_name, thread->name, OS_MGMT_TASK_NAME_LEN-1);
	out_info->oti_name[OS_MGMT_TASK_NAME_LEN - 1] = '\0';
#else
	ll_to_s(thread->base.prio, sizeof(out_info->oti_name), out_info->oti_name);
#endif

	out_info->oti_prio = thread->base.prio;
	out_info->oti_taskid = idx;
	out_info->oti_state = thread->base.thread_state;
#ifdef CONFIG_THREAD_STACK_INFO
	out_info->oti_stksize = thread->stack_info.size / 4;
#ifdef CONFIG_INIT_STACKS
	if (k_thread_stack_space_get(thread, &unused) == 0) {
		out_info->oti_stkusage = (thread->stack_info.size - unused) / 4;
	} else {
		out_info->oti_stkusage = 0;
	}
#endif
#endif

	return 0;
}
#endif /* CONFIG_THREAD_MONITOR */

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
