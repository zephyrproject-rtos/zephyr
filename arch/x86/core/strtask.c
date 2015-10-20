/* strtask.c - Intel nanokernel APIs to start a task */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
Intel-specific parts of start_task(). Only FP functionality currently.
 */

#ifdef CONFIG_MICROKERNEL

#include <start_task_arch.h>

/*
 * The following IA-32-specific task group is used for tasks that use SSE
 * instructions. It is *not* formally reserved by SysGen for this purpose.
 * See comments in thread.c regarding the use of SSE_GROUP, and comments
 * in task.h regarding task groups reserved by SysGen.
 *
 * This identifier corresponds to the first user-defined task group.
 * It must be updated if any changes are made to the reserved groups.
 */

#define SSE_GROUP 0x10

/**
 * @brief Intel-specific parts of task initialization
 *
 * @param X pointer to task control block
 * @param pOpt thread options container
 *
 * @return N/A
 */

void _StartTaskArch( struct k_task *X, unsigned int *pOpt)
{
	/*
	 * The IA-32 nanokernel implementation uses the USE_FP bit in the
	 * struct tcs->flags structure as a "dirty bit".  The USE_FP flag bit
	 * will be set whenever a thread uses any non-integer capability,
	 * whether it's just the x87 FPU capability, SSE instructions, or a
	 * combination of both. The USE_SSE flag bit will only be set if a
	 * thread uses SSE instructions.
	 *
	 * However, callers of fiber_fiber_start(), task_fiber_start(), or even
	 * _new_thread() don't need to follow the protocol used by the IA-32
	 * nanokernel w.r.t. managing the struct tcs->flags field.  If a thread
	 * will be utilizing just the x87 FPU capability, then the USE_FP
	 * option bit is specified.  If a thread will be utilizing SSE
	 * instructions (and possibly x87 FPU capability), then only the
	 * USE_SSE option bit needs to be specified.
	 *
	 * Likewise, the placement of tasks into "groups" doesn't need to follow
	 * the protocol used by the IA-32 nanokernel w.r.t. managing the
	 * struct tcs->flags field.  If a task will utilize just the x87 FPU
	 * capability, then the task only needs to be placed in the FPU_GROUP
	 * group.  If a task utilizes SSE instructions (and possibly x87 FPU
	 * capability), then the task only needs to be placed in the SSE_GROUP
	 * group.
	 */

	*pOpt |= (X->group & SSE_GROUP) ? USE_SSE
					: (X->group & FPU_GROUP) ? USE_FP : 0;
}

#endif /* CONFIG_MICROKERNEL */
