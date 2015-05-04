/* microkernel/task.h */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#ifndef TASK_H
#define TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/task_api_export.h>

extern struct k_proc *_k_current_task;

/*
 * The following task groups are reserved for system use.
 * SysGen automatically generates corresponding TASKGROUPs with reserved
 * GROUPIDs (in the SysGen files SgObjectCount.cpp - TaskGroupIDSetter
 * constructor; and SgSystem.cpp - AddDefaultObjects method).
 * These files must be updated if any changes are made to the reserved groups.
 */

#define EXE_GROUP 1 /* TASKGROUP EXE */
#define USR_GROUP 2 /* TASKGROUP SYS */
#define FPU_GROUP 4 /* TASKGROUP FPU */


extern void _task_ioctl(ktask_t, int);
extern void _task_group_ioctl(ktask_group_t, int);
extern void task_yield(void);

extern void task_priority_set(ktask_t task, kpriority_t prio);

extern void task_entry_set(ktask_t task, void (*func)(void));
extern void task_abort_handler_set(void (*func)(void));

/*
 * The following task/group operations may only be performed with
 * _task_ioctl() and _task_group_ioctl() in the kernel.
 */

#define TASK_BLOCK 4
#define TASK_UNBLOCK 5

#define TASK_GROUP_BLOCK 4
#define TASK_GROUP_UNBLOCK 5

#ifdef CONFIG_TASK_MONITOR
extern void KS_TaskSetSwitchCallBack(taskswitchcallbackfunc func);
#endif

#define task_id_get() (_k_current_task->Ident)
#define task_priority_get() (_k_current_task->Prio)
#define task_group_mask_get() (_k_current_task->Group)
#define task_group_join(g) (_k_current_task->Group |= g)
#define task_group_leave(g) (_k_current_task->Group &= ~g)

#define isr_task_id_get() task_id_get()
#define isr_task_priority_get() task_priority_get()
#define isr_task_group_mask_get() task_group_mask_get()

#ifdef __cplusplus
}
#endif

#endif /* TASK_H */
