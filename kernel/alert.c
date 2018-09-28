/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief kernel alerts.
*/

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <atomic.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <syscall_handler.h>
#include <stdbool.h>

extern struct k_alert _k_alert_list_start[];
extern struct k_alert _k_alert_list_end[];

struct k_alert *_trace_list_k_alert;

#ifdef CONFIG_OBJECT_TRACING

/*
 * Complete initialization of statically defined alerts.
 */
static int init_alert_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_alert *alert;

	for (alert = _k_alert_list_start; alert < _k_alert_list_end; alert++) {
		SYS_TRACING_OBJ_INIT(k_alert, alert);
	}
	return 0;
}

SYS_INIT(init_alert_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void _alert_deliver(struct k_work *work)
{
	struct k_alert *alert = CONTAINER_OF(work, struct k_alert, work_item);

	while (true) {
		if ((alert->handler)(alert) == 0) {
			/* do nothing -- handler has processed the alert */
		} else {
			/* pend the alert */
			k_sem_give(&alert->sem);
		}
		if (atomic_dec(&alert->send_count) == 1) {
			/* have finished delivering alerts */
			break;
		}
	}
}

void k_alert_init(struct k_alert *alert, k_alert_handler_t handler,
		  unsigned int max_num_pending_alerts)
{
	alert->handler = handler;
	alert->send_count = ATOMIC_INIT(0);
	alert->work_item = (struct k_work)_K_WORK_INITIALIZER(_alert_deliver);
	k_sem_init(&alert->sem, 0, max_num_pending_alerts);
	SYS_TRACING_OBJ_INIT(k_alert, alert);

	_k_object_init(alert);
}

void _impl_k_alert_send(struct k_alert *alert)
{
	if (alert->handler == K_ALERT_IGNORE) {
		/* ignore the alert */
	} else if (alert->handler == K_ALERT_DEFAULT) {
		/* pend the alert */
		k_sem_give(&alert->sem);
	} else {
		/* deliver the alert */
		if (atomic_inc(&alert->send_count) == 0) {
			/* add alert's work item to system work queue */
			k_work_submit_to_queue(&k_sys_work_q,
					       &alert->work_item);
		}
	}
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_alert_send, K_OBJ_ALERT, struct k_alert *);
#endif

int _impl_k_alert_recv(struct k_alert *alert, s32_t timeout)
{
	return k_sem_take(&alert->sem, timeout);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_alert_recv, alert, timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(alert, K_OBJ_ALERT));
	return _impl_k_alert_recv((struct k_alert *)alert, timeout);
}
#endif
