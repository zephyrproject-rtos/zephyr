/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief kernel alerts.
*/

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/debug/object_tracing_common.h>
#include <atomic.h>
#include <init.h>
#include <toolchain.h>
#include <sections.h>

extern struct k_alert _k_alert_list_start[];
extern struct k_alert _k_alert_list_end[];

struct k_alert *_trace_list_k_alert;

#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS

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

#endif /* CONFIG_DEBUG_TRACING_KERNEL_OBJECTS */

void _alert_deliver(struct k_work *work)
{
	struct k_alert *alert = CONTAINER_OF(work, struct k_alert, work_item);

	while (1) {
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
	alert->work_item = (struct k_work)K_WORK_INITIALIZER(_alert_deliver);
	k_sem_init(&alert->sem, 0, max_num_pending_alerts);
	SYS_TRACING_OBJ_INIT(k_alert, alert);
}

void k_alert_send(struct k_alert *alert)
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

int k_alert_recv(struct k_alert *alert, int32_t timeout)
{
	return k_sem_take(&alert->sem, timeout);
}
