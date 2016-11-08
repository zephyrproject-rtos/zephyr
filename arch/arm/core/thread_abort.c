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
 * @brief ARM Cortex-M k_thread_abort() routine
 *
 * The ARM Cortex-M architecture provides its own k_thread_abort() to deal
 * with different CPU modes (handler vs thread) when a thread aborts. When its
 * entry point returns or when it aborts itself, the CPU is in thread mode and
 * must call _Swap() (which triggers a service call), but when in handler
 * mode, the CPU must exit handler mode to cause the context switch, and thus
 * must queue the PendSV exception.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <sections.h>
#include <ksched.h>
#include <wait_q.h>

extern void _k_thread_single_abort(struct tcs *thread);

void k_thread_abort(k_tid_t thread)
{
	unsigned int key;

	key = irq_lock();

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	if (_current == thread) {
		if (_ScbIsInThreadMode()) {
			_Swap(key);
			CODE_UNREACHABLE;
		} else {
			_ScbPendsvSet();
		}
	}

	/* The abort handler might have altered the ready queue. */
	_reschedule_threads(key);
}
