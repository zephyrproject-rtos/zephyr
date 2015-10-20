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

#ifdef CONFIG_TASK_MONITOR

#include <micro_private.h>
#include <microkernel/ticks.h>
#include <drivers/system_timer.h>
#include <toolchain.h>
#include <sections.h>

static struct k_mrec __noinit k_monitor_buff[CONFIG_TASK_MONITOR_CAPACITY];

static const int k_monitor_capacity = CONFIG_TASK_MONITOR_CAPACITY;
const int _k_monitor_mask = CONFIG_TASK_MONITOR_MASK;

static struct k_mrec *k_monitor_wptr = k_monitor_buff;
static int k_monitor_nrec;
static int K_monitor_wind;

k_task_monitor_hook_t _k_task_switch_callback;

extern const int _k_num_events;

void task_monitor_hook_set(k_task_monitor_hook_t func)
{
	_k_task_switch_callback = func;
}

void _k_task_monitor(struct k_task *X, uint32_t D)
{
#ifdef CONFIG_TASK_DEBUG
	if (!_k_debug_halt)
#endif
	{
		k_monitor_wptr->time = _sys_clock_cycle_get();
		k_monitor_wptr->data1 = X->id;
		k_monitor_wptr->data2 = D;
		if (++K_monitor_wind == k_monitor_capacity) {
			K_monitor_wind = 0;
			k_monitor_wptr = k_monitor_buff;
		} else {
			++k_monitor_wptr;
		}
		if (k_monitor_nrec < k_monitor_capacity) {
			k_monitor_nrec++;
		}
	}
	if ((_k_task_switch_callback != NULL) && (D == 0)) {
		(_k_task_switch_callback)(X->id, _sys_clock_cycle_get());
	}
}

void _k_task_monitor_args(struct k_args *A)
{
#ifdef CONFIG_TASK_DEBUG
	if (!_k_debug_halt)
#endif
	{
		k_monitor_wptr->time = _sys_clock_cycle_get();

		if ((uint32_t)A < _k_num_events) {
			k_monitor_wptr->data2 = MO_EVENT | (uint32_t)A;
		} else {
			k_monitor_wptr->data1 = _k_current_task->id;
			k_monitor_wptr->data2 = MO_LCOMM | A->Comm;
		}

		if (++K_monitor_wind == k_monitor_capacity) {
			K_monitor_wind = 0;
			k_monitor_wptr = k_monitor_buff;
		} else {
			++k_monitor_wptr;
		}

		if (k_monitor_nrec < k_monitor_capacity) {
			k_monitor_nrec++;
		}
	}
}

void _k_task_monitor_read(struct k_args *A)
{
	A->args.z4.nrec = k_monitor_nrec;
	if (A->args.z4.rind < k_monitor_nrec) {
		int i = K_monitor_wind - k_monitor_nrec + A->args.z4.rind;

		if (i < 0) {
			i += k_monitor_capacity;
		}
		A->args.z4.mrec = k_monitor_buff[i];
	}
}

#endif /* CONFIG_TASK_MONITOR */
