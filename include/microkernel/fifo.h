/* microkernel/fifo.h - microkernel FIFO header file */

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

#ifndef FIFO_H
#define FIFO_H

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

extern int _task_fifo_put(kfifo_t queue, void *data, int32_t time);
extern int _task_fifo_get(kfifo_t queue, void *data, int32_t time);
extern int _task_fifo_ioctl(kfifo_t queue, int op);

#define task_fifo_put(q, p) _task_fifo_put(q, p, TICKS_NONE)
#define task_fifo_put_wait(q, p) _task_fifo_put(q, p, TICKS_UNLIMITED)

#ifndef CONFIG_TICKLESS_KERNEL
#define task_fifo_put_wait_timeout(q, p, t) _task_fifo_put(q, p, t)
#endif

#define task_fifo_get(q, p) _task_fifo_get(q, p, TICKS_NONE)
#define task_fifo_get_wait(q, p) _task_fifo_get(q, p, TICKS_UNLIMITED)

#ifndef CONFIG_TICKLESS_KERNEL
#define task_fifo_get_wait_timeout(q, p, t) _task_fifo_get(q, p, t)
#endif

#define task_fifo_size_get(q) _task_fifo_ioctl(q, 0)
#define task_fifo_purge(q) _task_fifo_ioctl(q, 1)

#ifdef __cplusplus
}
#endif

#endif /* FIFO_H */
