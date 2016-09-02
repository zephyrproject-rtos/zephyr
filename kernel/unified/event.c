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
 * @brief kernel events.
*/

#include <kernel.h>
#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <atomic.h>
#include <toolchain.h>
#include <sections.h>

void _k_event_deliver(struct k_work *work)
{
	struct k_event *event = CONTAINER_OF(work, struct k_event, work_item);

	while (1) {
		if ((event->handler)(event) == 0) {
			/* do nothing -- handler has processed the event */
		} else {
			/* pend the event */
			k_sem_give(&event->sem);
		}
		if (atomic_dec(&event->send_count) == 1) {
			/* have finished delivering events */
			break;
		}
	}
}

void k_event_init(struct k_event *event, k_event_handler_t handler)
{
	const struct k_work my_work_item = { NULL, _k_event_deliver, { 1 } };

	event->handler = handler;
	event->send_count = ATOMIC_INIT(0);
	event->work_item = my_work_item;
	k_sem_init(&event->sem, 0, 1);
	SYS_TRACING_OBJ_INIT(event, event);
}

void k_event_send(struct k_event *event)
{
	if (event->handler == K_EVT_IGNORE) {
		/* ignore the event */
	} else if (event->handler == K_EVT_DEFAULT) {
		/* pend the event */
		k_sem_give(&event->sem);
	} else {
		/* deliver the event */
		if (atomic_inc(&event->send_count) == 0) {
			/* add event's work item to system work queue */
			k_work_submit_to_queue(&k_sys_work_q,
					       &event->work_item);
		}
	}
}

int k_event_recv(struct k_event *event, int32_t timeout)
{
	return k_sem_take(&event->sem, timeout);
}
