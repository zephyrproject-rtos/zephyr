/* k_task_monitor.c - microkernel task monitoring subsystem */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
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

#include <micro_private.h>
#include <misc/kernel_event_logger.h>

#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
int _k_monitor_mask;
#else
const int _k_monitor_mask = CONFIG_TASK_MONITOR_MASK;
#endif

k_task_monitor_hook_t _k_task_switch_callback;

void task_monitor_hook_set(k_task_monitor_hook_t func)
{
	_k_task_switch_callback = func;
}

void _k_task_monitor(struct k_task *X, uint32_t D)
{
	uint32_t data[3];
#ifdef CONFIG_TASK_DEBUG
	if (!_k_debug_halt)
#endif
	{
		data[0] = sys_cycle_get_32();
		data[1] = X->id;
		data[2] = D;
		sys_k_event_logger_put(
		KERNEL_EVENT_LOGGER_TASK_MON_TASK_STATE_CHANGE_EVENT_ID,
		data, ARRAY_SIZE(data));
	}
	if ((_k_task_switch_callback != NULL) && (D == 0)) {
		(_k_task_switch_callback)(X->id, sys_cycle_get_32());
	}
}

void _k_task_monitor_args(struct k_args *A)
{
#ifdef CONFIG_TASK_DEBUG
	if (!_k_debug_halt)
#endif
	{
		int cmd_type;
		cmd_type = (int)A & KERNEL_CMD_TYPE_MASK;

		if (cmd_type == KERNEL_CMD_EVENT_TYPE) {
			uint32_t data[2];

			data[0] = sys_cycle_get_32();
			data[1] = MO_EVENT | (uint32_t)A;
			sys_k_event_logger_put(
			KERNEL_EVENT_LOGGER_TASK_MON_KEVENT_EVENT_ID,
			data, ARRAY_SIZE(data));
		} else {
			uint32_t data[3];

			data[0] = sys_cycle_get_32();
			data[1] = _k_current_task->id;
			data[2] = (uint32_t)A->Comm;
			sys_k_event_logger_put(
			KERNEL_EVENT_LOGGER_TASK_MON_CMD_PACKET_EVENT_ID,
			data, ARRAY_SIZE(data));
		}
	}
}
