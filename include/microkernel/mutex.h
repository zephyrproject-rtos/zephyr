/* microkernel/res.h - microkernel mutex header file */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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

#ifndef MUTEX_H
#define MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include  <microkernel/k_types.h>

extern int _task_mutex_lock(kmutex_t mutex, int32_t time);
extern void _task_mutex_unlock(kmutex_t mutex);

#define task_mutex_lock(m) _task_mutex_lock(m, TICKS_NONE)
#define task_mutex_lock_wait(m) _task_mutex_lock(m, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS
#define task_mutex_lock_wait_timeout(m, t) _task_mutex_lock(m, t)
#endif

#define task_mutex_unlock(m) _task_mutex_unlock(m)

#ifdef __cplusplus
}
#endif

#endif /* MUTEX_H */
