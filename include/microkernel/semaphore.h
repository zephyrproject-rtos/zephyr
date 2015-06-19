/* microkernel/semaphore.h - microkernel semaphore header file */

/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
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

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/cmdPkt.h>

extern void isr_sem_give(ksem_t sema, struct cmd_pkt_set *pSet);
extern void fiber_sem_give(ksem_t sema, struct cmd_pkt_set *pSet);

extern void task_sem_give(ksem_t sema);
extern void task_sem_group_give(ksemg_t semagroup);
extern int task_sem_count_get(ksem_t sema);
extern void task_sem_reset(ksem_t sema);
extern void task_sem_group_reset(ksemg_t semagroup);
extern int _task_sem_take(ksem_t sema, int32_t time);
extern ksem_t _task_sem_group_take(ksemg_t semagroup, int32_t time);

#define task_sem_take(s) _task_sem_take(s, TICKS_NONE)
#define task_sem_take_wait(s) _task_sem_take(s, TICKS_UNLIMITED)
#define task_sem_group_take_wait(g) _task_sem_group_take(g, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS
#define task_sem_take_wait_timeout(s, t) _task_sem_take(s, t)
#define task_sem_group_take_wait_timeout(g, t) _task_sem_group_take(g, t)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SEMAPHORE_H */
