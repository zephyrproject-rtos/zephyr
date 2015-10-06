/* task_abort.c - ARM Cortex-M _TaskAbort() routine */

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

/*
DESCRIPTION
The ARM Cortex-M architecture provides its own _TaskAbort() to deal with
different CPU modes (handler vs thread) when a task aborts. When its entry
point returns or when it aborts itself, the CPU is in thread mode and must
call the equivalent of task_abort(<self>), but when in handler mode, the
CPU must queue a packet to _k_server(), then exit handler mode to queue the
PendSV exception and cause the immediate context switch to _k_server.
 */

#ifdef CONFIG_MICROKERNEL

#include <toolchain.h>
#include <sections.h>
#include <micro_private.h>
#include <nano_private.h>
#include <microkernel.h>
#include <nanokernel.h>
#include <misc/__assert.h>

static struct k_args cmd_packet;

/**
 *
 * @brief Abort the current task
 *
 * Possible reasons for a task aborting:
 *
 * - the task explicitly aborts itself by calling this routine
 * - the task implicitly aborts by returning from its entry point
 * - the task encounters a fatal exception
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void _TaskAbort(void)
{
	const int taskAbortCode = 1;

	if (_ScbIsInThreadMode()) {
		_task_ioctl(_k_current_task->id, taskAbortCode);
	} else {
		cmd_packet.Comm = _K_SVC_TASK_OP;
		cmd_packet.args.g1.task = _k_current_task->id;
		cmd_packet.args.g1.opt = taskAbortCode;
		cmd_packet.alloc = false;
		_k_current_task->args = &cmd_packet;
		nano_isr_stack_push(&_k_command_stack, (uint32_t) &cmd_packet);
		_ScbPendsvSet();
	}
}

#endif /* CONFIG_MICROKERNEL */
