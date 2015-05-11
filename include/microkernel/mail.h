/* microkernel/mail.h - microkernel mailbox header file */

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

#ifndef MAIL_H
#define MAIL_H

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

extern int _task_mbox_put(kmbox_t mbox,
		                  kpriority_t prio,
		                  struct k_msg *M,
		                  int32_t time);

extern int _task_mbox_get(kmbox_t mbox, struct k_msg *M, int32_t time);

extern void _task_mbox_put_async(kmbox_t mbox,
		                         kpriority_t prio,
		                         struct k_msg *M,
		                         ksem_t sem);

extern void _task_mbox_data_get(struct k_msg *M);

extern int _task_mbox_data_get_async_block(struct k_msg *M,
				 struct k_block *rxblock,
				 kmemory_pool_t pid,
				 int32_t time);

#define task_mbox_put(b, p, m) _task_mbox_put(b, p, m, TICKS_NONE)
#define task_mbox_put_wait(b, p, m) _task_mbox_put(b, p, m, TICKS_UNLIMITED)

#ifndef CONFIG_TICKLESS_KERNEL
#define task_mbox_put_wait_timeout(b, p, m, t) _task_mbox_put(b, p, m, t)
#endif

#define task_mbox_get(b, m) _task_mbox_get(b, m, TICKS_NONE)
#define task_mbox_get_wait(b, m) _task_mbox_get(b, m, TICKS_UNLIMITED)

#ifndef CONFIG_TICKLESS_KERNEL
#define task_mbox_get_wait_timeout(b, m, t) _task_mbox_get(b, m, t)
#endif

#define task_mbox_put_async(b, p, m, s) _task_mbox_put_async(b, p, m, s)
#define task_mbox_data_get(m) _task_mbox_data_get(m)
#define task_mbox_data_get_async_block(m, b, p) \
		_task_mbox_data_get_async_block(m, b, p, TICKS_NONE)
#define task_mbox_data_get_async_block_wait(m, b, p) \
	_task_mbox_data_get_async_block(m, b, p, TICKS_UNLIMITED)

#ifndef CONFIG_TICKLESS_KERNEL
#define task_mbox_data_get_async_block_wait_timeout(m, b, p, t) \
		_task_mbox_data_get_async_block(m, b, p, t)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAIL_H */
