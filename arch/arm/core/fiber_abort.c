/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * @brief ARM Cortex-M fiber_abort() routine
 *
 * The ARM Cortex-M architecture provides its own fiber_abort() to deal with
 * different CPU modes (handler vs thread) when a fiber aborts. When its entry
 * point returns or when it aborts itself, the CPU is in thread mode and must
 * call _Swap() (which triggers a service call), but when in handler mode, the
 * CPU must exit handler mode to cause the context switch, and thus must queue
 * the PendSV exception.
 */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <micro_private_types.h>
#endif

#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <nanokernel.h>
#include <arch/cpu.h>

/**
 *
 * @brief Abort the currently executing fiber
 *
 * Possible reasons for a fiber aborting:
 *
 * - the fiber explicitly aborts itself by calling this routine
 * - the fiber implicitly aborts by returning from its entry point
 * - the fiber encounters a fatal exception
 *
 * @return N/A
 */

void fiber_abort(void)
{
	_thread_exit(_nanokernel.current);
	if (_ScbIsInThreadMode()) {
		_nano_fiber_swap();
	} else {
		_ScbPendsvSet();
	}
}
