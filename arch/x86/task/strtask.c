/* strtask.c - Intel nanokernel APIs to start a task */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
 * See comments in context.c regarding the use of SSE_GROUP, and comments
 * in task.h regarding task groups reserved by SysGen.
 *
 * This identifier corresponds to the first user-defined task group.
 * It must be updated if any changes are made to the reserved groups.
 */

#define SSE_GROUP 0x10

/*******************************************************************************
*
* _StartTaskArch - Intel-specifc parts of task initialization
*
* RETURNS: N/A
*/

void _StartTaskArch(
	struct k_proc *X,	 /* ptr to task control block */
	unsigned int *pOpt /* context options container */
	)
{
	/*
	 * The IA-32 nanokernel implementation uses the USE_FP bit in the
	 * tCCS->flags structure as a "dirty bit".  The USE_FP flag bit will be
	 * set whenever a context uses any non-integer capability, whether it's
	 * just the x87 FPU capability, SSE instructions, or a combination of
	 * both. The USE_SSE flag bit will only be set if a context uses SSE
	 * instructions.
	 *
	 * However, callers of fiber_fiber_start(), task_fiber_start(), or even
	 * _NewContext() don't need to follow the protocol used by the IA-32
	 * nanokernel w.r.t. managing the tCCS->flags field.  If a context
	 * will be utilizing just the x87 FPU capability, then the USE_FP
	 * option bit is specified.  If a context will be utilizing SSE
	 * instructions (and possibly x87 FPU capability), then only the
	 * USE_SSE option bit needs to be specified.
	 *
	 * Likewise, the placement of tasks into "groups" doesn't need to follow
	 * the protocol used by the IA-32 nanokernel w.r.t. managing the
	 * tCCS->flags field.  If a task will utilize just the x87 FPU
	 *capability,
	 * then the task only needs to be placed in the FPU_GROUP group.
	 * If a task utilizes SSE instructions (and possibly x87 FPU
	 *capability),
	 * then the task only needs to be placed in the SSE_GROUP group.
	 */

	*pOpt |= (X->Group & SSE_GROUP) ? USE_SSE
					: (X->Group & FPU_GROUP) ? USE_FP : 0;
}

#endif /* CONFIG_MICROKERNEL */
