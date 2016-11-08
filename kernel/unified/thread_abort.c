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
 * @brief Primitive for aborting a thread when an arch-specific one is not
 * needed..
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <nano_internal.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <ksched.h>

extern void _k_thread_single_abort(struct k_thread *thread);

#if !defined(CONFIG_ARCH_HAS_NANO_FIBER_ABORT)
void k_thread_abort(k_tid_t thread)
{
	unsigned int key;

	key = irq_lock();

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	if (_current == thread) {
		_Swap(key);
		CODE_UNREACHABLE;
	}

	/* The abort handler might have altered the ready queue. */
	_reschedule_threads(key);
}
#endif

/* legacy API */

void task_abort_handler_set(void (*func)(void))
{
	_current->fn_abort = func;
}
